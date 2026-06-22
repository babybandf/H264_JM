# Plan: H264 JM Intra Block Dump — Post-Order Focus

## Overview

在 H264 JM 编码器中实现 intra block dump，重点是 **post-order dump**（自底向上输出），用于外部 intra 子系统验证。

## 当前实现状态（2026-06-18）

本文件最初是设计计划；当前代码已经完成了一部分，并有几处实现策略与原计划不同。下面各章节保留原始设计背景，同时标注当前实现差异。

### 已实现

- JM encoder dump 基础设施：`source/app/lencod/intra_dump.h` / `source/app/lencod/intra_dump.c`。
- TLV 输出：`SEQ_HEADER`、`MB_BEGIN/END`、`BLOCK_BEGIN/END`、`REF_SAMPLES`、`RECON_PELS`、`PRED_PELS`、`MODE_METRIC`、`FINAL_MODE`。
- Luma 全候选 dump：每个 MB 输出 16 个 4x4 block、4 个 8x8 block、1 个 16x16 block；4x4/8x8 dump 9 种模式，16x16 dump 4 种模式。
- Chroma dump：跟在 luma 后面，Cb 和 Cr 各一个 chroma block，分别 dump 4 种模式和 recon。
- Post-order 文件：`intra_dump_postorder.bin` 会按 luma 4x4、luma 8x8、luma 16x16、Cb、Cr 的顺序输出。
- 验证工具：`tools/verify_dump.py` 已适配 JM/H.264 dump，支持 pred/recon/ref 像素打印和 `--max-blocks`。
- `run_enc.sh` 已可用于 `akiyo` 一帧 smoke 测试，已验证 `BLOCK_BEGIN=9108`、`PRED_PELS=76032`、`MODE_METRIC=76032`。
- H.264 `ut_wd_intra` reader 和 directed driver 初版已实现：`ut_wd_intra/H264IntraDumpReader.*`、`ut_wd_intra/ut_intra_directed_h264.cc`。
- HM `JVET_HM/ut_wd_intra/main.cc` 已按 `--codec h264` 分发到 `h264_directed_*`，HEVC 保留原 `directed_*` 路径。

### 与原计划不同

- Dump pixel API 已改为 JM 的 `imgpel`/`imgpel**` 行指针，不再使用 `int16_t* + stride` 的扁平 API。
- `dead` 字段仍在结构里，但当前要求是 dump 所有候选模式和败选结果，因此没有用它过滤 block。
- 当前没有实现 per-size `pred_4x4.yuv` / `recon_4x4.yuv` / `resi_4x4.yuv` 这类 YUV 辅助输出。
- H.264 chroma dump 是独立 dump-only 路径，用实际最终 `currMB->c_ipred_mode` 标记 `FINAL_MODE`；luma 的 SATD-only 选择受 `USE_SATD_FOR_DISTORTION` 控制。
- `ut_wd_intra` 没有复制出一整套新的统一 `main.cc/ut_intra.cc`；当前做法是在现有 `JVET_HM/ut_wd_intra/main.cc` 中 include H.264 reader/driver 并分发。

### 仍未完成 / 需要外部环境

- 完整 HM `ut_wd_intra` executable 尚未在本 repo 编译成功，因为当前环境缺少外部 VCE SDK 头/库，例如 `vce/emul/vceemul.h`。
- H.264 directed driver 目前能解析 JM dump 并填 ESRC/above-edge/PMF/TQITQ/SEL/UT，但还没有在真实 VCE emulator 上做端到端结果比对。
- `h264_directed_lambda_for_mb()` 当前返回 0，因为 JM dump 不记录 lambda/bits/cost。
- `h264_directed_verify_mb()` 目前只做 `ctu_end` 和 block layout 粗检查，还没有比对硬件/model 输出的最终 mode/cost。
- HEVC directed 回归没有在当前环境跑通，同样受 VCE SDK 依赖限制。

## Post-Order Dump 核心概念

### JVET_HM 的 Post-Order 机制

JVET_HM 在每个 CTU 内：
1. `beginCtu()` 时清空 CTU buffer
2. 每个 block 的 dump 数据被 buffer 到 `m_curBlock->records`
3. `endCtu()` 时调用 `flushPostOrder()`：
   - 构建 parent-child 树（基于 depth + 空间包含关系）
   - DFS 后序遍历输出（children before parents）
    - 原 HM 逻辑会跳过 `dead` blocks（败方 partition 试验的产物）
    - 当前 JM/H.264 实现为满足“dump 所有候选，即使败选”的要求，不使用 `dead` 过滤

### H264 的层次结构

H264 使用固定 16x16 macroblock，层次结构更简单：

```
MB (16x16)
├── I4MB mode:
│   ├── 8x8 sub-block 0 (b8=0)
│   │   ├── 4x4 block 0 (b4=0)
│   │   ├── 4x4 block 1 (b4=1)
│   │   ├── 4x4 block 2 (b4=2)
│   │   └── 4x4 block 3 (b4=3)
│   ├── 8x8 sub-block 1 (b8=1)
│   ├── 8x8 sub-block 2 (b8=2)
│   └── 8x8 sub-block 3 (b8=3)
├── I8MB mode:
│   ├── 8x8 block 0
│   ├── 8x8 block 1
│   ├── 8x8 block 2
│   └── 8x8 block 3
└── I16MB mode:
    └── 16x16 block (no sub-partition)
```

### Post-Order 输出顺序

当前实现是“MB 内全部候选 block 一起输出”，不是只按最终 MB type 输出。一个 4:2:0 MB 的 post-order 逻辑顺序为：

1. 16 个 luma 4x4 block，每个 9 个 pred/mode metric。
2. 4 个 luma 8x8 block，每个 9 个 pred/mode metric。
3. 1 个 luma 16x16 block，4 个 pred/mode metric。
4. 1 个 Cb chroma block，4 个 pred/mode metric。
5. 1 个 Cr chroma block，4 个 pred/mode metric。

早期设计中 I4MB 的树形例子如下，作为结构背景保留：
```
[MB_BEGIN]
  [BLOCK_BEGIN 4x4 b8=0 b4=0] ... [BLOCK_END]
  [BLOCK_BEGIN 4x4 b8=0 b4=1] ... [BLOCK_END]
  [BLOCK_BEGIN 4x4 b8=0 b4=2] ... [BLOCK_END]
  [BLOCK_BEGIN 4x4 b8=0 b4=3] ... [BLOCK_END]
  [BLOCK_BEGIN 8x8 b8=0] ... [BLOCK_END]  // 可选：8x8 级别聚合
  [BLOCK_BEGIN 4x4 b8=1 b4=0] ... [BLOCK_END]
  ...
  [BLOCK_BEGIN 8x8 b8=3] ... [BLOCK_END]
  [BLOCK_BEGIN 16x16] ... [BLOCK_END]  // MB 级别
[MB_END]
```

早期设计中 I16MB 的最小例子如下；当前实现仍会同时输出 4x4/8x8/16x16/chroma 的全部候选：
```
[MB_BEGIN]
  [BLOCK_BEGIN 16x16] ... [BLOCK_END]
[MB_END]
```

## 数据结构设计 (intra_dump.h)

**关键设计决策**: dump 的 distortion 使用 **SATD (Hadamard SAD)** 替代 SSE，与 JVET_HM 一致。SATD 替换通过宏开关控制，默认启用。

### SATD vs SSE 对比

| 指标 | SSE | SATD |
|:---|:---|:---|
| 计算方式 | 像素差值平方和 | Hadamard 变换域 SAD |
| 与编码性能相关性 | 高 (变换后量化) | 中高 (近似变换域) |
| JM 实现 | `compute_SSE4x4()` | `HadamardSAD4x4()` |
| HM 实现 | `distortionSse` | `calcHAD()` |

### 宏开关设计

```c
// intra_dump.h
#ifndef USE_SATD_FOR_DISTORTION
#define USE_SATD_FOR_DISTORTION 1  // 默认启用: 只用 SATD，忽略 bits
#endif
```

### 模式决策行为

**关键设计**: 当 `USE_SATD_FOR_DISTORTION=1` 时，模式决策**只使用 SATD**，**完全忽略 bits**。

当前实现范围：该宏控制 luma 4x4/8x8/16x16 的选择；chroma 当前不改 JM 原决策，只在最终 chroma mode 已确定后 dump 所有 chroma modes。

```c
// 正常编码模式 (USE_SATD_FOR_DISTORTION=0):
rdcost = distortion_SSE + weighted_cost(lambda, rate);

// SATD-only 模式 (USE_SATD_FOR_DISTORTION=1):
rdcost = distortion_SATD;  // 无 bits 组件
```

### 对编码的影响

| 维度 | 正常编码 | SATD-only 模式 |
|:---|:---|:---|
| 模式选择依据 | distortion + bits | 只有 distortion |
| 编码效率 | 高 (RDO 平衡) | 低 (可能选 bits 多的模式) |
| PSNR | 正常 | 可能略有不同 |
| 比特率 | 正常 | 可能增加 |
| Dump 完整性 | 需 bits 信息 | 只需 distortion |

### 选择此设计的原因

| 原因 | 说明 |
|:---|:---|
| Dump 完整性 | dump 只记录 distortion，不记录 bits |
| 验证一致性 | 外部验证只需 distortion 即可重现决策 |
| 简化实现 | 无需在 dump 中存储 bits/lambda/cost |

### 数据结构

**关键设计**: 当 `USE_SATD_FOR_DISTORTION=1` 时，dump 只记录 SATD distortion，不记录 bits/lambda/cost。

```c
#ifndef __INTRADUMP_H__
#define __INTRADUMP_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ====================================================================================================================
// 宏开关
// ====================================================================================================================

#ifndef USE_SATD_FOR_DISTORTION
#define USE_SATD_FOR_DISTORTION 1  // 默认启用: 只用 SATD，忽略 bits
#endif

// ====================================================================================================================
// TLV Tags (与 JVET_HM 兼容)
// ====================================================================================================================

enum IntraDumpTag {
  DUMP_TAG_SEQ_HEADER   = 0x0001,
  DUMP_TAG_MB_BEGIN     = 0x0010,  // 替代 CTU_BEGIN
  DUMP_TAG_MB_END       = 0x0011,  // 替代 CTU_END
  DUMP_TAG_BLOCK_BEGIN  = 0x0020,
  DUMP_TAG_BLOCK_END    = 0x0021,
  DUMP_TAG_REF_SAMPLES  = 0x0030,
  DUMP_TAG_RECON_PELS   = 0x0031,
  DUMP_TAG_PRED_PELS    = 0x0032,
  DUMP_TAG_MODE_METRIC  = 0x0040,
  DUMP_TAG_FINAL_MODE   = 0x0041,
};

// ====================================================================================================================
// Data Structures
// ====================================================================================================================

struct IntraDumpSeqInfo {
  uint32_t picWidthLuma;
  uint32_t picHeightLuma;
  uint32_t mbSize;          // 固定 16
  uint32_t chromaFormat;    // 0=400, 1=420, 2=422, 3=444
  uint32_t bitDepthLuma;
  uint32_t bitDepthChroma;
  uint32_t baseQp;
  uint8_t  useDqp;
  uint8_t  padding[3];
};

struct IntraDumpMbKey {
  uint32_t picIdx;
  uint32_t mbAddrX;         // MB 地址 (光栅扫描)
  uint32_t mbPelX;          // MB 左上角像素坐标
  uint32_t mbPelY;
  uint32_t sliceType;       // 0=B, 1=P, 2=I
  int8_t   sliceQp;
  uint8_t  padding[3];
};

struct IntraDumpBlockKey {
  uint32_t blockUid;
  uint32_t parentUid;
  uint32_t mbAddrX;
  uint32_t mbPelX;
  uint32_t mbPelY;
  uint32_t blkPelXInMb;     // block 在 MB 内的偏移
  uint32_t blkPelYInMb;
  uint32_t width;           // 4, 8, 16
  uint32_t height;
  uint8_t  compID;          // 0=Y, 1=Cb, 2=Cr
  uint8_t  mbMode;          // I4MB=0, I8MB=1, I16MB=2, IPCM=3
  uint8_t  intraMode;       // 具体的 intra 方向模式
  uint8_t  partIdx;         // sub-partition index (b8*4 + b4 for I4x4)
  uint8_t  padding[3];
};

// H.264 intra mode kinds
enum IntraDumpModeKind {
  MODE_KIND_DC     = 0,
  MODE_KIND_HOR    = 1,
  MODE_KIND_VER    = 2,
  MODE_KIND_PLANE  = 3,
  MODE_KIND_D4     = 4,   // I4x4 diagonal down-left
  MODE_KIND_D4R    = 5,   // I4x4 diagonal down-right
  MODE_KIND_VR     = 6,   // I4x4 vertical-right
  MODE_KIND_HD     = 7,   // I4x4 horizontal-down
  MODE_KIND_VL     = 8,   // I4x4 vertical-left
  MODE_KIND_HU     = 9,   // I4x4 horizontal-up
};

// ====================================================================================================================
// Mode Metric - 只记录 SATD distortion，不记录 bits
// ====================================================================================================================

struct IntraDumpModeMetric {
  uint32_t modeId;
  uint8_t  modeKind;
  uint8_t  mbMode;
  uint8_t  padding[2];
  uint64_t distortionSatd;   // SATD distortion (Hadamard SAD)
  // 不记录 bits, lambda, cost - 模式决策只用 SATD
};

struct IntraDumpFinalMode {
  uint32_t modeId;
  uint8_t  modeKind;
  uint8_t  mbMode;
  uint8_t  padding[2];
  uint64_t distortionSatd;   // winner 的 SATD distortion
};

// ====================================================================================================================
// IntraDumper Singleton
// ====================================================================================================================

// Maximum number of blocks per MB (I4MB: 16 4x4 + 4 8x8 + 1 16x16 = 21, padded)
#define INTRA_DUMP_MAX_BLOCKS_PER_MB 32

typedef struct IntraDumpTlvRecord {
  uint32_t tag;
  uint32_t len;
  uint8_t* payload;
} IntraDumpTlvRecord;

typedef struct IntraDumpBlockData {
  IntraDumpBlockKey key;
  IntraDumpTlvRecord* records;
  int numRecords;
  int capacity;
  int dead;  // 当前保留字段；实际实现不标记 dead，因为需要保留败选候选
} IntraDumpBlockData;

typedef struct IntraDumper {
  FILE*       fp;           // pre-order dump
  FILE*       fpPost;       // post-order dump
  int         enabled;
  char        dumpDir[512];
  char        pathPre[600];
  char        pathPost[600];
  uint32_t    blockUidCounter;
  uint32_t    picIdx;

  // Sequence info
  uint32_t    picWidth;
  uint32_t    picHeight;

  // 当前实现未包含 per-size YUV frame buffers；只输出 TLV bin/txt 可解析数据

  // MB-level buffer
  IntraDumpBlockData  mbBlocks[INTRA_DUMP_MAX_BLOCKS_PER_MB];
  int                 numMbBlocks;
  IntraDumpBlockData* curBlock;  // currently accumulating block (NULL if outside)
  IntraDumpTlvRecord  mbPreRecords[16];  // MB_BEGIN record
  int                 numMbPreRecords;
} IntraDumper;

// ====================================================================================================================
// API
// ====================================================================================================================

IntraDumper* intra_dumper_get_instance(void);

// Phase 0: Sequence header
void intra_dumper_write_seq_header(IntraDumper* dumper, const IntraDumpSeqInfo* info);

// Phase 1: Picture begin/end
void intra_dumper_begin_picture(IntraDumper* dumper, uint32_t picIdx);
void intra_dumper_end_picture(IntraDumper* dumper);

// Phase 2: MB hooks
void intra_dumper_begin_mb(IntraDumper* dumper, const IntraDumpMbKey* key);
void intra_dumper_end_mb(IntraDumper* dumper);

// Phase 3: Block hooks
void intra_dumper_begin_block(IntraDumper* dumper, const IntraDumpBlockKey* key);
void intra_dumper_end_block(IntraDumper* dumper);

// Data dump
void intra_dumper_dump_ref_samples(IntraDumper* dumper, const imgpel* buf, uint32_t totalLen, int filtered);
void intra_dumper_dump_recon_pels(IntraDumper* dumper, imgpel** buf, uint32_t x, uint32_t w, uint32_t h);
void intra_dumper_dump_pred_pels(IntraDumper* dumper, uint32_t modeId, uint8_t modeKind,
                                 imgpel** buf, uint32_t x, uint32_t w, uint32_t h, uint64_t satd);
void intra_dumper_dump_mode_metric(IntraDumper* dumper, const IntraDumpModeMetric* metric);
void intra_dumper_dump_final_mode(IntraDumper* dumper, const IntraDumpFinalMode* final);

// SATD 计算辅助函数
// 注意: JM 已有 HadamardSAD4x4() 和 HadamardSAD8x8()
// 16x16 SATD 需要新增: 分解为 4 个 8x8 SATD 求和
uint64_t intra_dump_compute_satd_4x4(imgpel** org, int orgX, imgpel** pred, int predX);
uint64_t intra_dump_compute_satd_8x8(imgpel** org, int orgX, imgpel** pred, int predX);
uint64_t intra_dump_compute_satd_16x16(imgpel** org, int orgX, imgpel** pred, int predX);

// Per-size YUV output: not implemented in current code

// Post-order flush
void intra_dumper_flush_post_order(IntraDumper* dumper);

// Helpers
uint32_t intra_dumper_alloc_block_uid(IntraDumper* dumper);

#endif // __INTRADUMP_H__
```

## Post-Order 实现核心 (intra_dump.c)

### 1. Tree Building Algorithm

```c
void intra_dumper_flush_post_order(IntraDumper* dumper) {
  if (!dumper->fpPost) return;

  // Write MB_BEGIN
  // ...

  int N = dumper->numMbBlocks;
  int parent[INTRA_DUMP_MAX_BLOCKS_PER_MB];
  int children[INTRA_DUMP_MAX_BLOCKS_PER_MB][8];  // max 8 children per block
  int numChildren[INTRA_DUMP_MAX_BLOCKS_PER_MB];

  // Initialize
  for (int i = 0; i < N; i++) {
    parent[i] = -1;  // -1 = root
    numChildren[i] = 0;
  }

  // Build parent-child relationships
  for (int i = 0; i < N; i++) {
    IntraDumpBlockKey* ki = &dumper->mbBlocks[i].key;

    if (ki->width == 4 && ki->height == 4) {
      // 4x4 block: parent is the 8x8 sub-block containing it
      for (int j = i - 1; j >= 0; j--) {
        IntraDumpBlockKey* kj = &dumper->mbBlocks[j].key;
        if (kj->width == 8 && kj->height == 8 &&
            ki->blkPelXInMb >= kj->blkPelXInMb &&
            ki->blkPelXInMb < kj->blkPelXInMb + 8 &&
            ki->blkPelYInMb >= kj->blkPelYInMb &&
            ki->blkPelYInMb < kj->blkPelYInMb + 8) {
          parent[i] = j;
          children[j][numChildren[j]++] = i;
          break;
        }
      }
    } else if (ki->width == 8 && ki->height == 8) {
      // 8x8 block: parent is the MB (16x16) containing it
      // In H264, all 8x8 blocks are direct children of MB
      // We don't buffer MB as a block, so they become roots
      parent[i] = -1;  // root
    } else if (ki->width == 16 && ki->height == 16) {
      // 16x16 block: root
      parent[i] = -1;
    }
  }

  // Post-order DFS traversal
  int postOrder[INTRA_DUMP_MAX_BLOCKS_PER_MB];
  int postOrderCount = 0;

  // Find roots
  int roots[INTRA_DUMP_MAX_BLOCKS_PER_MB];
  int numRoots = 0;
  for (int i = 0; i < N; i++) {
    if (parent[i] == -1) {
      roots[numRoots++] = i;
    }
  }

  // DFS with explicit stack
  struct StackEntry { int idx; int expanded; };
  struct StackEntry stack[INTRA_DUMP_MAX_BLOCKS_PER_MB];
  int stackTop = 0;

  for (int r = numRoots - 1; r >= 0; r--) {
    stack[stackTop].idx = roots[r];
    stack[stackTop].expanded = 0;
    stackTop++;
  }

  while (stackTop > 0) {
    struct StackEntry* top = &stack[stackTop - 1];
    if (!top->expanded) {
      top->expanded = 1;
      // Push children in reverse order
      for (int c = numChildren[top->idx] - 1; c >= 0; c--) {
        stack[stackTop].idx = children[top->idx][c];
        stack[stackTop].expanded = 0;
        stackTop++;
      }
    } else {
      postOrder[postOrderCount++] = top->idx;
      stackTop--;
    }
  }

  // Write blocks in post-order
  for (int i = 0; i < postOrderCount; i++) {
    IntraDumpBlockData* bd = &dumper->mbBlocks[postOrder[i]];
    if (bd->dead) continue;  // 当前不会设置 dead；保留兼容字段
    for (int j = 0; j < bd->numRecords; j++) {
      write_tlv_to_file(dumper->fpPost, bd->records[j].tag,
                        bd->records[j].payload, bd->records[j].len);
    }
  }

  // Write MB_END
  write_tlv_to_file(dumper->fpPost, DUMP_TAG_MB_END, NULL, 0);
  fflush(dumper->fpPost);

  // Clear buffers
  // ...
}
```

### 2. Hook Points in H264 Encoder

**关键**: 当 `USE_SATD_FOR_DISTORTION=1` 时，编码器模式决策只使用 SATD，忽略 bits。dump 记录实际使用的 distortion。

#### 2.1 Sequence Header

**位置**: `lencod.c` 的 `encode_sequence()` 开头

```c
// 在编码器初始化完成后
IntraDumper* dumper = intra_dumper_get_instance();
if (dumper->enabled) {
  IntraDumpSeqInfo info;
  info.picWidthLuma = p_Vid->width;
  info.picHeightLuma = p_Vid->height;
  info.mbSize = 16;
  info.chromaFormat = p_Vid->yuv_format;
  info.bitDepthLuma = p_Vid->bitdepth_luma;
  info.bitDepthChroma = p_Vid->bitdepth_chroma;
  info.baseQp = p_Vid->qp;
  info.useDqp = p_Inp->UseDQP;
  intra_dumper_write_seq_header(dumper, &info);
}
```

#### 2.2 Picture Begin/End

**位置**: `lencod.c` 的 `encode_one_frame()`

```c
// 每帧开始
intra_dumper_begin_picture(dumper, frameNum);

// 每帧结束
intra_dumper_end_picture(dumper);
```

#### 2.3 MB Begin/End

**位置**: `macroblock.c` 的 `encode_one_macroblock()`

```c
// 函数开头
IntraDumpMbKey mbKey;
mbKey.picIdx = frameNum;
mbKey.mbAddrX = currMB->mbAddrX;
mbKey.mbPelX = currMB->pix_x;
mbKey.mbPelY = currMB->pix_y;
mbKey.sliceType = currSlice->slice_type;
mbKey.sliceQp = (int8_t)currMB->qp;
intra_dumper_begin_mb(dumper, &mbKey);

// 函数结尾
intra_dumper_end_mb(dumper);
```

#### 2.4 I4x4 Block Hooks

**位置**: `rd_intra_jm.c` 的 `mode_decision_for_I4x4_blocks_JM_High()`

**关键修改**: 当 `USE_SATD_FOR_DISTORTION=1` 时，模式决策只使用 SATD，忽略 bits。

```c
// 在模式循环前
IntraDumpBlockKey blockKey;
blockKey.blockUid = intra_dumper_alloc_block_uid(dumper);
blockKey.mbAddrX = currMB->mbAddrX;
blockKey.blkPelXInMb = block_x;
blockKey.blkPelYInMb = block_y;
blockKey.width = 4;
blockKey.height = 4;
blockKey.compID = 0;
blockKey.mbMode = 0;  // I4MB
intra_dumper_begin_block(dumper, &blockKey);

// 在模式循环内 (每个 ipmode)
{
  // 1. Dump pred pels (预测像素)
  intra_dumper_dump_pred_pels(dumper, ipmode, modeKind,
                              currSlice->mpr_4x4[0][ipmode], 4, 4, 4, 0);

#if USE_SATD_FOR_DISTORTION
  // SATD-only 模式: 只用 distortion，忽略 bits
  // 计算 SATD
  short diff[16];
  int idx = 0;
  for (int j = 0; j < 4; j++) {
    for (int i = 0; i < 4; i++) {
      diff[idx++] = (short)(&p_Vid->pCurImg[pic_opix_y + j][pic_opix_x + i] 
                     - currSlice->mpr_4x4[0][ipmode][j * 4 + i]);
    }
  }
  uint64_t satd = (uint64_t)HadamardSAD4x4(diff);
  
  // 模式决策: 只用 SATD，无 bits
  rdcost = (distblk)satd;
  
  // 2. Dump mode metric (只记录 SATD)
  IntraDumpModeMetric metric;
  metric.modeId = ipmode;
  metric.modeKind = modeKind;
  metric.mbMode = 0;
  metric.distortionSatd = satd;
  intra_dumper_dump_mode_metric(dumper, &metric);
#else
  // 正常模式: 使用原始 rdcost (SSE + bits)
  rdcost = currSlice->rdcost_for_4x4_intra_blocks(
      currMB, &c_nz, b8, b4, ipmode, lambda, mostProbableMode, min_rdcost);
  
  // 提取 distortion 用于 dump
  uint64_t distortion = get_last_distortion();
  
  // 3. Dump mode metric
  IntraDumpModeMetric metric;
  metric.modeId = ipmode;
  metric.modeKind = modeKind;
  metric.mbMode = 0;
  metric.distortionSatd = distortion;
  intra_dumper_dump_mode_metric(dumper, &metric);
#endif
}

// 在 winner 选出后
{
  IntraDumpFinalMode final;
  final.modeId = best_ipmode;
  final.modeKind = bestModeKind;
  final.mbMode = 0;
  final.distortionSatd = bestDistortion;  // 从循环中保存
  intra_dumper_dump_final_mode(dumper, &final);

  // Dump recon pels (重建像素)
  intra_dumper_dump_recon_pels(dumper, &p_Vid->enc_picture->imgY[pic_pix_y], p_Vid->width, 4, 4);
}

intra_dumper_end_block(dumper);
```

#### 2.5 I16x16 Block Hooks

**位置**: `rd_intra_jm.c` 的 `mode_decision_for_I16x16_macroblocks_JM_High()`

类似 I4x4，但使用 `HadamardSAD8x8` 或 `intra_dump_compute_satd_16x16()`。

#### 2.6 I8x8 Block Hooks

**位置**: `transform8x8.c` 的 `rdcost_for_8x8_intra_blocks()`

类似 I4x4，但使用 `HadamardSAD8x8`。

### 3. SATD 计算实现

```c
// intra_dump.c

#include "me_distortion.h"  // HadamardSAD4x4, HadamardSAD8x8

uint64_t intra_dump_compute_satd_4x4(const int16_t* org, int orgStride,
                                      const int16_t* pred, int predStride) {
  short diff[16];
  int idx = 0;
  for (int j = 0; j < 4; j++) {
    for (int i = 0; i < 4; i++) {
      diff[idx++] = (short)(org[j * orgStride + i] - pred[j * predStride + i]);
    }
  }
  return (uint64_t)HadamardSAD4x4(diff);
}

uint64_t intra_dump_compute_satd_8x8(const int16_t* org, int orgStride,
                                      const int16_t* pred, int predStride) {
  short diff[64];
  int idx = 0;
  for (int j = 0; j < 8; j++) {
    for (int i = 0; i < 8; i++) {
      diff[idx++] = (short)(org[j * orgStride + i] - pred[j * predStride + i]);
    }
  }
  return (uint64_t)HadamardSAD8x8(diff);
}

uint64_t intra_dump_compute_satd_16x16(const int16_t* org, int orgStride,
                                         const int16_t* pred, int predStride) {
  // 16x16 SATD = 4 * (8x8 SATD) 求和
  uint64_t totalSatd = 0;
  for (int by = 0; by < 2; by++) {
    for (int bx = 0; bx < 2; bx++) {
      const int16_t* org8x8 = org + by * 8 * orgStride + bx * 8;
      const int16_t* pred8x8 = pred + by * 8 * predStride + bx * 8;
      totalSatd += intra_dump_compute_satd_8x8(org8x8, orgStride, pred8x8, predStride);
    }
  }
  return totalSatd;
}
```

### 3. Ref Samples 提取

H264 的 ref samples 与 HEVC 类似，都是 L 形排列：

```c
// 从 currSlice->mpred_4x4 或 p_Vid->pCurImg 中提取
void extract_ref_samples_h264(Macroblock* currMB, int block_x, int block_y,
                               int16_t* refBuf, int* refLen) {
  // Top row: A, B, C, D (above-left, above, above-right)
  // Left column: I, J, K, L (left, below-left)

  int pic_pix_x = currMB->pix_x + block_x;
  int pic_pix_y = currMB->pix_y + block_y;

  // Above samples
  for (int i = 0; i < 4; i++) {
    refBuf[i] = p_Vid->pCurImg[pic_pix_y - 1][pic_pix_x + i];
  }
  // Left samples
  for (int i = 0; i < 4; i++) {
    refBuf[4 + i] = p_Vid->pCurImg[pic_pix_y + i][pic_pix_x - 1];
  }
  *refLen = 8;
}
```

### 4. Environment Variable Control

```c
// 启用 dump
const char* dumpDir = getenv("JM_INTRA_DUMP_DIR");
if (dumpDir != NULL && dumpDir[0] != '\0') {
  dumper->enabled = 1;
  strncpy(dumper->dumpDir, dumpDir, sizeof(dumper->dumpDir) - 1);
  // Open files...
}
```

## 实现步骤

### Part A: H264 JM 编码器 Intra Dump

#### Step 1: 创建 intra_dump.h — 已完成

定义所有数据结构和 API，包括 SATD 计算辅助函数声明。

当前实现使用 `imgpel`/`imgpel**` 行指针 API，而不是最初计划里的 `int16_t* + stride`。

#### Step 2: 实现 intra_dump.c 基础框架 — 已完成

- 单例实现
- TLV 写入函数
- 环境变量检查
- File open/flush。当前没有显式 close API，进程退出时由运行时回收文件句柄。

#### Step 3: 实现 SATD 计算函数 — 已完成

```c
// intra_dump.c
#include "me_distortion.h"  // HadamardSAD4x4, HadamardSAD8x8

uint64_t intra_dump_compute_satd_4x4(imgpel** org, int orgX, imgpel** pred, int predX);
uint64_t intra_dump_compute_satd_8x8(imgpel** org, int orgX, imgpel** pred, int predX);
uint64_t intra_dump_compute_satd_16x16(imgpel** org, int orgX, imgpel** pred, int predX);
```

#### Step 4: 实现 Post-Order Buffer 管理 — 已完成

- `begin_mb()`: 清空 buffer
- `begin_block()`: 添加 block 到 buffer
- `dump_*()`: 记录 TLV 到当前 block
- `end_block()`: 结束当前 block
- `flush_post_order()`: 构建 4x4 -> 8x8 的简单父子关系 + DFS + root 排序输出
- 当前按 block 面积和 component 排序，保证 luma 4x4、8x8、16x16 后再输出 Cb/Cr。

#### Step 5: 实现 Per-size YUV Output — 未实现

- `write_pred_yuv()`, `write_recon_yuv()`, `write_resi_yuv()`
- `begin_picture()`, `end_picture()`

当前实现只输出 TLV dump bin，不输出 per-size YUV 文件。若后续需要该能力，应新增独立 writer，避免影响当前已验证的 TLV 路径。

#### Step 6: 集成到编码器 — 已完成但 hook 点与原计划不同

- 已在 `lencod.c` 写 sequence/picture hooks。
- 已在 `md_high.c` 写 MB begin/end 和 chroma dump hooks。
- 已在 `rd_intra_jm.c` 写 luma 4x4/8x8/16x16 hooks。
- 当前没有单独修改 `transform8x8.c`，8x8 dump 逻辑集中在 `rd_intra_jm.c`。
- Hook 中调用独立的 SATD 计算函数

#### Step 7: 编译测试 — 已完成 smoke 验证

- 编码 I-only 序列
- 验证 dump 文件完整性
- 已用 `./run_enc.sh -c akiyo -n 1` 生成并验证 dump。
- 已验证 post-order dump 的 counts：`MB=396`、`BLOCK_BEGIN=9108`、`PRED_PELS=76032`、`MODE_METRIC=76032`。
- 尚未做 JM SATD 与 HM `calcHAD()` 的数值逐项对比。

### Part B: ut_wd_intra 集成（当前实现：在 HM UT 中接入 H.264 driver）

当前实现没有复制一整套新的统一测试框架，而是在 `JVET_HM/ut_wd_intra/main.cc` 中保留原 HM HEVC 路径，并 include JM/H.264 reader 和 H.264 directed driver。这样可以最大限度复用 HM 的 port callback、VCTRL/NBI 基础设施，同时避免 `directed_*` 符号冲突。

#### 目录结构

```
H264_JM/
├── ut_wd_intra/                        # 新增：JM/H.264 扩展文件
│   ├── H264IntraDumpReader.h           # JM dump reader
│   ├── H264IntraDumpReader.cc
│   ├── ut_intra_directed_h264.cc       # H.264 directed driver
│   ├── Makefile                        # reader-only checks
│   └── README.md
├── JVET_HM/
│   ├── ut_wd_intra/                    # 已有：HM UT，main.cc 已加入 codec 分发
│   └── tools/dump_reader/              # 已有：IntraDumpReader
└── source/app/lencod/                  # H.264 编码器
```

#### 文件职责分离

| 文件 | 职责 |
|:---|:---|
| `JVET_HM/ut_wd_intra/main.cc` | CLI 解析、codec 选择、主循环调度、port callback 分发 |
| `JVET_HM/ut_wd_intra/ut_intra.cc` | 公共函数: NBI buffer, port factories, print helpers |
| `JVET_HM/ut_wd_intra/ut_intra_directed.cc` | 原 H.265 定向: ESRC(64x64), above-edge, best-modes, PMF/TQITQ/SEL/UT 响应 |
| `ut_wd_intra/H264IntraDumpReader.*` | H.264/JM TLV reader，独立于 HM dump reader |
| `ut_wd_intra/ut_intra_directed_h264.cc` | H.264 定向: MB reader, ESRC(16x16), above-edge, PMF/TQITQ/SEL/UT 响应 |
| `JVET_HM/ut_wd_intra/vce_port_data_types.h` | Port 数据结构定义 (264/265 共用) |

#### Step 8: 创建 ut_wd_intra 目录 — 已完成，但只放 H.264 增量文件

没有复制 `main.cc` / `ut_intra.cc` / `vce_port_data_types.h`。当前复用 `JVET_HM/ut_wd_intra` 的现有文件。

#### Step 9: 创建 ut_intra_directed_h264.cc — 已完成初版

```c
// H.264 定向测试的核心函数。所有符号加 h264_ 前缀，避免撞 HEVC directed_*。
static H264IntraDump::SeqInfo h264_directed_init(...);
static bool h264_directed_next_mb(H264DirectedMbRecord& out);
static void h264_directed_load_yuv_frame(int picIdx);
static void h264_directed_prepare_mb(struct t_intra_test* tester, const H264DirectedMbRecord& mb);
static void h264_directed_pmf_response(...);
static void h264_directed_tqitq_response(...);
static void h264_directed_sel_response(...);
static void h264_directed_update_above_recon(const H264DirectedMbRecord& mb);
```

实际实现还包括：

- `h264_directed_next_mb()`：从 JM post-order dump 中按 MB 聚合 block、pred、metric、recon。
- `h264_directed_update_above_recon()`：用上一行 MB 底边 recon 更新 above-edge 滚动缓存。
- `h264_directed_ut_response()`：从 dump 的 `PRED_PELS` 回填 UT auto-check 端口。

#### Step 10: 复制 ut_intra_directed_hevc.cc — 不执行

当前保留并复用 `JVET_HM/ut_wd_intra/ut_intra_directed.cc`，没有复制为 `ut_intra_directed_hevc.cc`。

#### Step 11: 修改 main.cc — 已完成初版

```c
// CLI 选项
cmd.add<std::string>("codec", 0, "Codec: hevc(default), h264", false, "hevc");
cmd.add<std::string>("directed", 0, "Directed mode with dump file path", false, "");
cmd.add<std::string>("yuv", 0, "Source YUV file for directed mode", false, "");

// 全局变量
static int g_codec = VCE_PROTOCOL_CODEC_HEVC;
static bool g_directed_test = false;

// 主循环调度：H.264 按 MB 读取，HEVC 按 CTU 读取
if (g_codec == VCE_PROTOCOL_CODEC_H264) {
  H264DirectedMbRecord curMb;
  while (h264_directed_next_mb(curMb)) {
    h264_directed_prepare_mb(&s_intra_test, curMb);
    // make_vctrl2intra(mbX, mbY, ...)
  }
} else {
  CtuRecord curCtu;
  while (directed_next_ctu(curCtu)) {
    directed_prepare_ctu(&s_intra_test, curCtu);
  }
}

// Port 回调
void port_cb_intra2pmf(void* ctx) {
  if (g_directed_test) {
    if (g_codec == VCE_PROTOCOL_CODEC_H264) {
      h264_directed_pmf_response(tester, &intra2pmf, &tester->pmf2intra);
    } else {
      directed_pmf_response(tester, &intra2pmf, &tester->pmf2intra);
    }
  } else {
    // 随机模式
  }
}
```

当前 main.cc 的 H.264 directed loop 使用 `H264DirectedMbRecord`，按 `mbPelX/mbPelY/mbSize` 生成 VCTRL 的 `ctu_x/ctu_y`（对 H.264 表示 MB 坐标）。

#### Step 12: 集成 IntraDumpReader — 已完成 H.264 reader，完整 HM 构建仍依赖外部 VCE 环境

H.264 使用 `ut_wd_intra/H264IntraDumpReader.*`。它不是 HM 的 `tools/dump_reader/IntraDumpReader`，因为 JM/H.264 dump 的 TLV payload 与 HM/HEVC 不同。

#### Step 13: 编译测试 — reader 已验证，完整 executable 待 VCE SDK 环境

```bash
make -C ut_wd_intra h264-reader-check
make -C ut_wd_intra h264-reader-smoke DUMP=../dump_output_cases/akiyo/intra_dump_postorder.bin

# 完整 executable 需要 VCE SDK include/lib 后再构建
# 当前本 repo 环境缺少 vce/emul/vceemul.h

# 测试 H.264
./ut_wd_intra --codec h264 --directed h264_dump.bin --yuv h264.yuv
```

## 两个 Directed 文件的 API 对比

### 公共接口 (两个文件都实现)

```c
// HEVC path keeps existing names: directed_*
SeqInfo directed_init(...);
bool directed_next_ctu(CtuRecord& out);

// H.264 path uses prefixed names to avoid conflicts: h264_directed_*
H264IntraDump::SeqInfo h264_directed_init(...);
bool h264_directed_next_mb(H264DirectedMbRecord& out);
```

### H.265 独有

```c
void directed_fill_best_modes(struct t_intra_test* tester, const CtuRecord& ctu);
int raster_to_zscan(int rx, int ry, int gridSize);
```

### H.264 独有

```c
H264IntraDump::Reader
H264DirectedMbRecord
H264DirectedBlockRecord
h264_directed_ut_response(...)
```

## 关键差异处理

| 维度 | H.265 | H.264 |
|:---|:---|:---|
| 编码单元 | CTU (64x64) | MB (16x16) |
| Intra 模式 | 35 modes (0-34) | 9 modes (I4x4: 8 dir + DC) |
| Edge RAM slots | 4 slots | 1 slot |
| Above-edge addr/slot | 4 words | 1 word |
| PINTRA best-mode | 支持 | 不支持 |
| Directed 文件 | `JVET_HM/ut_wd_intra/ut_intra_directed.cc` | `ut_wd_intra/ut_intra_directed_h264.cc` |

## 验证方法

### Part A: 编码器 Dump 验证

1. 编码 `akiyo_cif.yuv` (I-only)
2. 检查 `intra_dump.bin` 和 `intra_dump_postorder.bin` 生成
3. 验证 TLV 记录完整性
4. **验证 SATD 值**:
   - 对比 JM `HadamardSAD4x4()` 输出与 HM `calcHAD()` 输出
   - 确保 4x4/8x8/16x16 SATD 计算正确
5. 对比 pred/recon YUV 与编码器输出
6. 关闭 dump 后确认编码结果不变

当前状态：TLV 结构和数量已验证；per-size YUV 输出未实现，因此第 5 项只能通过 TLV 中的 `PRED_PELS` / `RECON_PELS` 对比，而不是独立 YUV 文件。

### Part B: ut_wd_intra 验证

1. **H.265 回归测试**: 确保现有 H.265 定向测试不受影响
2. **H.264 功能测试**: 使用新生成的 H.264 dump 文件测试
3. **Codec 切换测试**: 同一程序无缝切换 264/265
4. **ESRC 一致性**: 验证填充的像素与 YUV 输入匹配
5. **Above-edge 一致性**: 验证 edge 数据与 dump 中的 recon 匹配

## 工具: run_enc.sh

### JVET_HM 版本

JVET_HM 已有 `run_enc.sh`，功能：
- 设置测试用例 (输入文件, 分辨率)
- 运行编码器并生成 dump
- 验证 dump 文件完整性
- 移动输出到按用例分类的目录

### H264_JM 版本（已实现，当前脚本状态）

当前仓库已有 `run_enc.sh`。它支持 `-c`、`-n`、`-h`，默认 case 为 `akiyo`，会设置 `JM_INTRA_DUMP_DIR=dump_output`，构建工程，然后运行 `./bin/lencod_static -d cfg/encoder.cfg`。

与早期草案不同：

- 当前脚本使用 `./bin/lencod_static`，不是 `./build/bin/lencod`。
- 当前脚本使用 `cfg/encoder.cfg`，不是 `cfg/encoder_baseline.cfg`。
- 当前脚本会用 `tools/verify_dump.py --pixels` 同时生成 `dump_log.txt` 和 `dump_postorder_log.txt`。
- `akiyo` 已有输入序列；`football/city256/city352` case 名保留在脚本里，但对应 yuv 文件是否存在取决于本地 `sequences/` 内容。

当前脚本核心逻辑如下（节选）：

```bash
#!/bin/bash

# Default values
CASE=akiyo
FRAMES=1

ALL_CASES="akiyo football city256 city352"

usage() {
    echo "Usage: $0 [-c case] [-n frames] [-h]"
    echo "  -c case    Test case: akiyo (default), football, city256, city352, all"
    echo "  -n frames  Number of frames to encode (default: 1)"
    echo "  -h         Show this help"
    exit 0
}

while getopts "c:n:h" opt; do
    case "$opt" in
        c) CASE=$OPTARG ;;
        n) FRAMES=$OPTARG ;;
        h) usage ;;
        *) usage ;;
    esac
done

setup_case() {
    local c=$1
    case "$c" in
        akiyo)
            INPUT_FILE=./sequences/akiyo_cif.yuv
            SOURCE_WIDTH=352
            SOURCE_HEIGHT=288
            BITSTREAM_FILE=./stream_akiyo_cif.264
            ;;
        football)
            INPUT_FILE=./sequences/football_cif.yuv
            SOURCE_WIDTH=352
            SOURCE_HEIGHT=288
            BITSTREAM_FILE=./stream_football_cif.264
            ;;
        city256)
            INPUT_FILE=./sequences/city_256x256.yuv
            SOURCE_WIDTH=256
            SOURCE_HEIGHT=256
            BITSTREAM_FILE=./stream_city_256x256.264
            ;;
        city352)
            INPUT_FILE=./sequences/city_cif.yuv
            SOURCE_WIDTH=352
            SOURCE_HEIGHT=288
            BITSTREAM_FILE=./stream_city_cif.264
            ;;
        *)
            echo "Unknown case: $c"
            echo "Available cases: ${ALL_CASES} all"
            return 1
            ;;
    esac
}

export JM_INTRA_DUMP_DIR=dump_output

run_encode() {
    local c=$1
    setup_case "$c" || return 1

    echo "Running test case: $c (${SOURCE_WIDTH}x${SOURCE_HEIGHT})"    

    rm -fr "${JM_INTRA_DUMP_DIR}"
    mkdir -p "${JM_INTRA_DUMP_DIR}"

    ./bin/lencod_static -d cfg/encoder.cfg \
        -p InputFile=${INPUT_FILE} \
        -p SourceWidth=${SOURCE_WIDTH} \
        -p SourceHeight=${SOURCE_HEIGHT} \
        -p FrameToBeEncoded=${FRAMES} \
        -p IntraPeriod=1 \
        -p OutputFile=${BITSTREAM_FILE}

    md5sum ${BITSTREAM_FILE}

    # move dump files to case-specific directory
    local CASE_DIR=dump_output_cases/${c}
    mkdir -p dump_output_cases
    rm -fr "${CASE_DIR}"
    mv "${JM_INTRA_DUMP_DIR}" "${CASE_DIR}"

    python3 tools/verify_dump.py --pixels ${CASE_DIR}/intra_dump.bin > ${CASE_DIR}/dump_log.txt
    python3 tools/verify_dump.py --pixels ${CASE_DIR}/intra_dump_postorder.bin > ${CASE_DIR}/dump_postorder_log.txt
}

# Current script always rebuilds before running
echo "Building lencod..."
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j
cd ..

if [ "$CASE" = "all" ]; then
    for c in ${ALL_CASES}; do
        run_encode "$c"
    done
else
    run_encode "$CASE"
fi
```

### 使用方法

```bash
# 单个测试用例
./run_enc.sh -c akiyo

# 指定帧数
./run_enc.sh -c akiyo -n 10

# 所有测试用例
./run_enc.sh -c all

# 查看帮助
./run_enc.sh -h
```

### 输出结构

```
dump_output_cases/
├── akiyo/
│   ├── intra_dump.bin
│   ├── intra_dump_postorder.bin
│   ├── dump_log.txt
│   └── dump_postorder_log.txt
├── football/
│   └── ...
└── ...
```

### JVET_HM 版本

JVET_HM 已有 `tools/verify_dump.py`，功能：
- 解析 TLV 格式 dump 文件
- 验证结构完整性 (tag 配对, nesting)
- 打印统计摘要

### H264_JM 版本（已实现）

当前仓库已有 `tools/verify_dump.py`，已适配 H.264/JM dump 格式。它比早期草案多了以下能力：

- `--max-blocks N`：限制打印的 block 数，避免日志过大。
- `--pixels` / `-p`：打印 `REF_SAMPLES`、`RECON_PELS`、`PRED_PELS` 的像素矩阵。
- `PRED_PELS` payload 已按当前 compact wire format 解析：`modeId + modeKind + w + h + satd + pels`。
- block 输出同时打印图像内绝对坐标 `pos=(...)` 和 MB 内坐标 `inMb=(...)`。

当前使用方式如下：

```bash
python3 tools/verify_dump.py dump_output_cases/akiyo/intra_dump.bin
python3 tools/verify_dump.py --pixels --max-blocks 2 dump_output_cases/akiyo/intra_dump_postorder.bin
```

早期草案代码如下，仅作为格式背景保留；实现以 `tools/verify_dump.py` 为准：

```python
#!/usr/bin/env python3
"""
verify_dump.py - Parse and verify the JM intra dump binary file.
Checks structural integrity of TLV records and prints a summary.
"""

import struct
import sys
from collections import defaultdict

# TLV Tags (H.264 version)
TAGS = {
    0x0001: 'SEQ_HEADER',
    0x0010: 'MB_BEGIN',      # H.264 使用 MB 而非 CTU
    0x0011: 'MB_END',
    0x0020: 'BLOCK_BEGIN',
    0x0021: 'BLOCK_END',
    0x0030: 'REF_SAMPLES',
    0x0031: 'RECON_PELS',
    0x0032: 'PRED_PELS',
    0x0040: 'MODE_METRIC',
    0x0041: 'FINAL_MODE',
    # H.264 无 TU_PK (0x0050) 和 DEPTH_PK (0x0060)
}

# H.264 intra mode kinds
MODE_KINDS = {
    0: 'dc', 1: 'hor', 2: 'ver', 3: 'plane',
    4: 'd4', 5: 'd4r', 6: 'vr', 7: 'hd', 8: 'vl', 9: 'hu'
}

def parse_seq_header(data):
    """Parse H.264 sequence header."""
    fields = struct.unpack_from('<IIIIIIIIIBBBBBB', data, 0)
    return {
        'picW': fields[0], 'picH': fields[1], 'mbSize': fields[2],
        'chromaFmt': fields[3], 'bitDepthL': fields[4], 'bitDepthC': fields[5],
        'baseQp': fields[6], 'useDqp': fields[7]
    }

def parse_mb_key(data):
    """Parse H.264 MB key."""
    fields = struct.unpack_from('<IIIIIbB', data, 0)
    return {
        'picIdx': fields[0], 'mbAddrX': fields[1],
        'mbPelX': fields[2], 'mbPelY': fields[3],
        'sliceType': fields[4], 'sliceQp': fields[5]
    }

def parse_block_key(data):
    """Parse H.264 block key."""
    fields = struct.unpack_from('<IIIIIIIIIBBBBBB', data, 0)
    return {
        'blockUid': fields[0], 'parentUid': fields[1],
        'mbAddrX': fields[2], 'mbPelX': fields[3], 'mbPelY': fields[4],
        'blkPelXInMb': fields[5], 'blkPelYInMb': fields[6],
        'width': fields[7], 'height': fields[8],
        'compID': fields[9], 'mbMode': fields[10],
        'intraMode': fields[11], 'partIdx': fields[12]
    }

def parse_mode_metric(data):
    """Parse H.264 mode metric (SATD only)."""
    fields = struct.unpack_from('<IBBxxQ', data, 0)
    return {
        'modeId': fields[0], 'modeKind': fields[1],
        'mbMode': fields[2], 'distortionSatd': fields[3]
    }

def parse_final_mode(data):
    """Parse H.264 final mode."""
    fields = struct.unpack_from('<IBBxxQ', data, 0)
    return {
        'modeId': fields[0], 'modeKind': fields[1],
        'mbMode': fields[2], 'distortionSatd': fields[3]
    }

def main():
    import argparse
    parser = argparse.ArgumentParser(description='Verify JM intra dump file')
    parser.add_argument('dump_file', help='Path to intra_dump.bin')
    parser.add_argument('--quiet', '-q', action='store_true')
    args = parser.parse_args()

    with open(args.dump_file, 'rb') as f:
        data = f.read()

    pos = 0
    tag_counts = defaultdict(int)
    errors = []
    mb_count = 0
    block_nesting = 0

    while pos < len(data):
        if pos + 8 > len(data):
            errors.append(f"Truncated TLV header at offset {pos}")
            break

        tag, payload_len = struct.unpack_from('<II', data, pos)
        pos += 8

        if pos + payload_len > len(data):
            errors.append(f"Truncated payload at offset {pos-8}")
            break

        payload = data[pos:pos+payload_len]
        pos += payload_len

        tag_name = TAGS.get(tag, f'UNKNOWN(0x{tag:04x})')
        tag_counts[tag_name] += 1

        if tag == 0x0010:  # MB_BEGIN
            mb = parse_mb_key(payload)
            mb_count += 1
            if not args.quiet:
                print(f"[MB_BEGIN] pic={mb['picIdx']}, addr={mb['mbAddrX']}, "
                      f"pos=({mb['mbPelX']},{mb['mbPelY']}), QP={mb['sliceQp']}")

        elif tag == 0x0020:  # BLOCK_BEGIN
            blk = parse_block_key(payload)
            block_nesting += 1
            if not args.quiet:
                print(f"  [BLOCK uid={blk['blockUid']}] {blk['width']}x{blk['height']} "
                      f"@({blk['blkPelXInMb']},{blk['blkPelYInMb']}) "
                      f"comp={blk['compID']} mbMode={blk['mbMode']}")

        elif tag == 0x0021:  # BLOCK_END
            block_nesting -= 1
            if block_nesting < 0:
                errors.append("BLOCK_END without matching BLOCK_BEGIN")

        elif tag == 0x0040:  # MODE_METRIC
            m = parse_mode_metric(payload)
            if not args.quiet:
                kind = MODE_KINDS.get(m['modeKind'], f'?{m["modeKind"]}')
                print(f"    [MODE_METRIC] mode={m['modeId']} kind={kind} "
                      f"satd={m['distortionSatd']}")

        elif tag == 0x0041:  # FINAL_MODE
            fm = parse_final_mode(payload)
            if not args.quiet:
                kind = MODE_KINDS.get(fm['modeKind'], f'?{fm["modeKind"]}')
                print(f"    [FINAL_MODE] mode={fm['modeId']} kind={kind} "
                      f"satd={fm['distortionSatd']}")

    # Summary
    print("\n" + "=" * 50)
    print("SUMMARY")
    print("=" * 50)
    print(f"Total bytes: {pos}/{len(data)}")
    print(f"MBs: {mb_count}")
    print(f"\nTag counts:")
    for name, count in sorted(tag_counts.items()):
        print(f"  {name}: {count}")

    if block_nesting != 0:
        errors.append(f"Unmatched block nesting: {block_nesting}")

    if errors:
        print(f"\nERRORS ({len(errors)}):")
        for e in errors:
            print(f"  !! {e}")
        return 1
    else:
        print("\nAll checks PASSED.")
        return 0

if __name__ == '__main__':
    sys.exit(main())
```

### 使用方法

```bash
# 验证 pre-order dump
python3 tools/verify_dump.py $JM_INTRA_DUMP_DIR/intra_dump.bin

# 验证 post-order dump
python3 tools/verify_dump.py $JM_INTRA_DUMP_DIR/intra_dump_postorder.bin

# 安静模式
python3 tools/verify_dump.py -q $JM_INTRA_DUMP_DIR/intra_dump.bin
```

### 验证内容

| 检查项 | 说明 |
|:---|:---|
| TLV 结构 | tag + len + payload 格式正确 |
| Tag 配对 | MB_BEGIN/END, BLOCK_BEGIN/END 数量相等 |
| Nesting | BLOCK 嵌套深度正确 |
| 数据完整性 | payload 长度匹配 |
| 统计摘要 | MB/Block 数量, tag 分布 |

## 测试命令

### 使用 run_enc.sh (推荐)

```bash
# 编码单个测试用例并生成 dump
./run_enc.sh -c akiyo

# 编码所有测试用例
./run_enc.sh -c all
```

### 手动测试

```bash
# 编译
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j

# 编码 I-only 序列并生成 dump
export JM_INTRA_DUMP_DIR=/tmp/jm_dump
mkdir -p $JM_INTRA_DUMP_DIR

./bin/lencod_static -d cfg/encoder.cfg \
  -p InputFile="./sequences/akiyo_cif.yuv" \
  -p SourceWidth=352 -p SourceHeight=288 \
  -p FrameToBeEncoded=1 -p IntraPeriod=1

# 验证 dump 文件
ls -la $JM_INTRA_DUMP_DIR/
python3 tools/verify_dump.py $JM_INTRA_DUMP_DIR/intra_dump.bin
python3 tools/verify_dump.py $JM_INTRA_DUMP_DIR/intra_dump_postorder.bin
```

### ut_wd_intra 测试

```bash
# 当前可在本 repo 直接验证的部分：H.264 dump reader
make -C ut_wd_intra h264-reader-check
make -C ut_wd_intra h264-reader-smoke DUMP=../dump_output_cases/akiyo/intra_dump_postorder.bin

# 完整 HM/VCE executable 需要外部 VCE SDK include/lib 后再构建运行
# 当前环境缺少 vce/emul/vceemul.h

# 测试 H.264 定向
./ut_wd_intra --codec h264 \
  --directed /path/to/h264_intra_dump_postorder.bin \
  --yuv /path/to/akiyo_cif.yuv
```

JM 和 HM 的 SATD 实现基于相同的 Hadamard 变换算法:
- 4x4: 直接调用 `HadamardSAD4x4()`
- 8x8: 直接调用 `HadamardSAD8x8()`
- 16x16: 分解为 4 个 8x8 SATD 求和 (HM 的 `calcHAD()` 也是同样实现)

**注意**: JM 的 `HadamardSAD4x4()` 返回值需要 `dist_scale()` 处理，但 dump 时应使用原始值 (未经 scale) 以保持与 HM 一致。

当前实现说明：dump 记录的是当前 `HadamardSAD4x4()` / `HadamardSAD8x8()` helper 的返回值；尚未做与 HM `calcHAD()` 的逐项数值对照。

## 关键设计决策总结

| 决策 | 选择 | 原因 |
|:---|:---|:---|
| Luma 模式决策 | `USE_SATD_FOR_DISTORTION=1` 时只用 SATD | dump 只记录 distortion，便于外部复现 luma 决策 |
| Chroma 模式决策 | 当前仍使用 JM 原决策，dump-only 输出所有 chroma modes | H.264 chroma intra 为 MB-level，当前只补全 dump 数据 |
| Bits 处理 | dump 不记录 bits/lambda/cost | 简化 dump 格式；`ut_wd_intra` H.264 lambda 当前为 0 |
| 宏开关 | `USE_SATD_FOR_DISTORTION` | 默认=1，可切换回正常编码 |
| Dump 格式 | 只记录 `distortionSatd` | 无 bits/lambda/cost |
| 验证工具 | `verify_dump.py` | 验证 TLV 结构完整性 |
| Per-size YUV | 未实现 | 当前通过 TLV 中的 pred/recon/ref 像素验证 |
| Full `ut_wd_intra` | H.264 reader/driver 已接入，完整运行待 VCE SDK | 本 repo 不包含 `vce/emul/vceemul.h` 等外部依赖 |
