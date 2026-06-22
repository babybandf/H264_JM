# Plan: ut_wd_intra 集成到 H264_JM — 支持 264/265 定向测试切换

## Overview

将 JVET_HM 的 `ut_wd_intra` 定向测试框架扩展到 H264_JM，实现：
1. 统一的测试入口，支持 H.264 和 H.265 定向测试无缝切换
2. 复用 `IntraDumpReader` 读取两种格式的 dump 文件
3. H.264 和 H.265 各自独立的 directed test 文件，共享公共基础设施

## 架构设计

### 目录结构

```
H264_JM/
├── ut_wd_intra/                    # 新增：统一测试框架
│   ├── main.cc                     # 统一入口，支持 --codec h264/hevc
│   ├── ut_intra.cc                 # 公共测试逻辑 (NBI, port factories)
│   ├── ut_intra_directed_hevc.cc   # H.265 定向测试驱动 (从 JVET_HM 复制)
│   ├── ut_intra_directed_h264.cc   # H.264 定向测试驱动 (新增)
│   ├── vce_port_data_types.h       # 公共 port 数据结构 (已支持 264/265)
│   └── plan.md
├── JVET_HM/
│   ├── ut_wd_intra/                # 已有：HM 专用测试
│   └── tools/dump_reader/          # 已有：IntraDumpReader
└── source/app/lencod/              # H.264 编码器
```

### 文件职责分离

| 文件 | 职责 |
|:---|:---|
| `main.cc` | CLI 解析、codec 选择、主循环调度 |
| `ut_intra.cc` | 公共函数: NBI buffer, port factories, print helpers |
| `ut_intra_directed_hevc.cc` | H.265 定向: ESRC(64x64), above-edge(4 slots), best-modes, PMF/TQITQ/SEL 响应 |
| `ut_intra_directed_h264.cc` | H.264 定向: ESRC(16x16), above-edge(1 slot), PMF/TQITQ/SEL 响应 |
| `vce_port_data_types.h` | Port 数据结构定义 (264/265 共用) |

### 关键设计决策

| 维度 | H.265 (HEVC) | H.264 (AVC) |
|:---|:---|:---|
| 编码单元 | CTU (64x64) | MB (16x16) |
| Intra 模式 | 35 modes (0-34) | 9 modes (I4x4: 8 dir + DC) |
| Edge RAM slots | 4 slots | 1 slot |
| Above-edge addr/slot | 4 words | 1 word |
| PINTRA best-mode | 支持 | 不支持 |
| Directed 文件 | `ut_intra_directed_hevc.cc` | `ut_intra_directed_h264.cc` |

### vce_port_data_types.h 已有支持

从 `vce_port_data_types.h` 可见，数据结构已预留 H.264 支持：

```c
// H.264 specific defines
#define INTRA_RAM_CTU_ABOVE_EDGE_H264_ADDR_PER_SLOT    (1)
#define INTRA_RAM_CTU_ABOVE_EDGE_H264_BASE_ADDR_Y      (0)
#define INTRA_RAM_CTU_ABOVE_EDGE_H264_BASE_ADDR_UV     (4)

// H.265 specific defines  
#define INTRA_RAM_CTU_ABOVE_EDGE_HEVC_ADDR_PER_SLOT    (4)
#define INTRA_RAM_CTU_ABOVE_EDGE_HEVC_BASE_ADDR_Y      (0)
#define INTRA_RAM_CTU_ABOVE_EDGE_HEVC_BASE_ADDR_UV     (16)
```

## 实现计划

### Phase 1: 统一 main.cc 入口

在 `ut_wd_intra/main.cc` 中添加 codec 选择和文件 include：

```c
// CLI options
cmd.add<std::string>("codec", 0, "Codec: hevc(default), h264", false, "hevc");
cmd.add<std::string>("directed", 0, "Directed mode with dump file path", false, "");
cmd.add<std::string>("yuv", 0, "Source YUV file for directed mode", false, "");

// 全局变量
static int g_codec = VCE_PROTOCOL_CODEC_HEVC;  // 默认 H.265
static bool g_directed_test = false;

// parse_opt 中
if (cmd.get<std::string>("codec").compare("h264") == 0) {
  g_codec = VCE_PROTOCOL_CODEC_H264;
}

// 根据 codec include 对应的 directed 文件
#if defined(ENABLE_H264_DIRECTED)
#include "ut_intra_directed_h264.cc"
#else
#include "ut_intra_directed_hevc.cc"
#endif
```

### Phase 2: ut_intra_directed_h264.cc (新增)

H.264 定向测试的核心差异：

```c
// ============================================================
// H.264 Directed Driver
// ============================================================

// SeqInfo 适配
static int detect_codec_from_seq(const SeqInfo& seq) {
  return (seq.ctuSize == 16) ? VCE_PROTOCOL_CODEC_H264 : VCE_PROTOCOL_CODEC_HEVC;
}

// PictureInfo 适配
static PictureInfo directed_pic_info() {
  PictureInfo pic;
  pic.width  = (int)g_directed.seq.picWidthLuma;
  pic.height = (int)g_directed.seq.picHeightLuma;
  pic.codec  = g_codec;
  return pic;
}

// ESRC 填充 (16x16 MB)
static void directed_fill_esrc(struct t_intra_test* tester, const CtuRecord& ctu) {
  const SeqInfo& s = g_directed.seq;
  int mbSize = 16;  // H.264 固定 16x16
  
  // Luma: 16x16
  for (int y = 0; y < mbSize; y++) {
    for (int x = 0; x < mbSize; x++) {
      int pel = f.getY(ctuPelX + x, ctuPelY + y);
      int addr = vce_rams_me_src_addr_calc(esrc_slot, x, y, YUV_FMT, PLANER_Y);
      vceemul_ram_write_elem(VCE_RAM_ID_ESRC, addr, x % 8, pel);
    }
  }
  
  // Chroma: 8x8 (4:2:0)
  if (s.chromaFormat > 0) {
    int chromaMbW = 8;
    int chromaMbH = 8;
    // ... 填充 U/V
  }
}

// Above-edge 填充 (1 slot)
static void directed_fill_above_edge(struct t_intra_test* tester, const CtuRecord& ctu) {
  const int codec = VCE_PROTOCOL_CODEC_H264;
  int addr_num_per_slot = INTRA_RAM_CTU_ABOVE_EDGE_H264_ADDR_PER_SLOT;  // = 1
  
  // Luma: 1 word = 16 pixels
  int addr = vce_rams_intra_ram_ctu_above_edge_addr_calc(codec, INTRA_RAM_CTU_ABOVE_EDGE_Y, slot);
  for (int i = 0; i < INTRA_RAM_CTU_ABOVE_EDGE_LUMA_PIXS_PER_ADDR; i++) {
    int px = ctuPelX + i;
    int16_t val = (ctuY > 0 && px < picWidth) ? aboveReconY[px] : 0;
    vceemul_ram_write_elem(VCE_RAM_ID_INTRA_RAM, addr, i, val);
  }
  
  // Chroma: 1 word = 8 pixels
  // ...
}

// Best-mode: H.264 不支持，直接跳过
static void directed_fill_best_modes(struct t_intra_test* tester, const CtuRecord& ctu) {
  // H.264 无 PINTRA best-mode RAM
  return;
}

// PMF 响应 (H.264 坐标)
static void directed_pmf_response(struct t_intra_test* tester,
                                   const t_intra2pmf* req,
                                   t_pmf2intra* resp) {
  int mbSize = 16;
  int ctuX = (int)ctu.info.ctuPelX;
  int ctuY = (int)ctu.info.ctuPelY;
  
  // H.264: pos_x/pos_y 是 MB 内偏移 (0-15)
  // H.265: pos_x/pos_y 是 CTU 内偏移 (0-63)
  
  // ... 从 YUV 帧读取像素
}

// TQITQ 响应
static void directed_tqitq_response(struct t_intra_test* tester,
                                     const t_tqitq_in* intra2tq,
                                     t_tqitq_out* tq2intra) {
  // H.264: 块匹配使用 MB 坐标
  // inf_pu_size: 1=4x4, 2=8x8, 4=16x16
}

// SEL 响应
static void directed_sel_response(struct t_intra_test* tester,
                                   const t_intra2sel* intra2sel,
                                   t_sel2intra* sel2intra) {
  // H.264: recon 边缘提取
  // 4x4: chunk 0 = 右列 + 底行
  // 8x8: chunk 0 = 右列, chunk 1 = 底行
  // 16x16: chunk 0-1 = 右列, chunk 2-3 = 底行
}
```

### Phase 3: ut_intra_directed_hevc.cc (从 JVET_HM 复制)

直接从 `JVET_HM/ut_wd_intra/ut_intra_directed.cc` 复制，保持原有逻辑不变。

### Phase 4: main.cc 主循环集成

```c
// main.cc

// 根据 codec 选择 directed 函数
static void directed_prepare_ctu(struct t_intra_test* tester, const CtuRecord& ctu) {
  if (g_codec == VCE_PROTOCOL_CODEC_H264) {
    // H.264 路径
    directed_fill_above_edge_h264(tester, ctu);
    directed_fill_esrc_h264(tester, ctu);
    // 无 best-modes
  } else {
    // H.265 路径
    directed_fill_above_edge_hevc(tester, ctu);
    directed_fill_best_modes_hevc(tester, ctu);
    directed_fill_esrc_hevc(tester, ctu);
  }
}

// Port 回调根据 codec 分发
void port_cb_intra2pmf(void* ctx) {
  if (g_directed_test) {
    if (g_codec == VCE_PROTOCOL_CODEC_H264) {
      directed_pmf_response_h264(tester, &intra2pmf, &tester->pmf2intra);
    } else {
      directed_pmf_response_hevc(tester, &intra2pmf, &tester->pmf2intra);
    }
  } else {
    // 随机模式
    ut_intra_pmf_get(...);
  }
}
```

## 数据流对比

### H.265 数据流

```
dump_postorder.bin → IntraDumpReader → CtuRecord
                                        ↓
                            directed_fill_esrc_hevc (64x64 CTU)
                            directed_fill_above_edge_hevc (4 slots)
                            directed_fill_best_modes_hevc (PINTRA RAM)
                                        ↓
                            vctrl2intra → intra model → verify
```

### H.264 数据流

```
dump_postorder.bin → IntraDumpReader → CtuRecord (MB-level)
                                        ↓
                            directed_fill_esrc_h264 (16x16 MB)
                            directed_fill_above_edge_h264 (1 slot)
                            (无 best-modes)
                                        ↓
                            vctrl2intra → intra model → verify
```

## 关键差异处理

### 1. Block 尺寸

| 维度 | H.265 | H.264 |
|:---|:---|:---|
| 最小 PU | 4x4 | 4x4 |
| 最大 PU | 64x64 | 16x16 |
| 分区方式 | 递归四叉树 | 固定 MB |

**处理**: 两个 directed 文件各自实现，不共享。

### 2. Intra 模式

| 维度 | H.265 | H.264 |
|:---|:---|:---|
| 模式数 | 35 (0-34) | 9 (I4x4: 8 dir + DC) |
| Planar | mode 0 | N/A |
| DC | mode 1 | DC_PRED (1) |
| Angular | mode 2-34 | D4Pred..HUPred (2-9) |

**处理**: 各自 directed 文件中的 mode 映射。

### 3. Edge RAM 布局

| 维度 | H.265 | H.264 |
|:---|:---|:---|
| Addr/slot | 4 words | 1 word |
| Luma pixels/addr | 16 | 16 |
| Slot 数 | 4 | 1 |

**处理**: 使用 `vce_port_data_types.h` 中的宏，各 directed 文件独立实现。

## 实现步骤

### Step 1: 创建 ut_wd_intra 目录

在 H264_JM 根目录创建 `ut_wd_intra/`，从 JVET_HM 复制基础文件。

### Step 2: 创建 ut_intra_directed_h264.cc

实现 H.264 定向测试的所有函数。

### Step 3: 修改 main.cc

- 添加 `--codec` 选项
- 根据 codec include 对应的 directed 文件
- 修改主循环调度逻辑

### Step 4: 集成 IntraDumpReader

- 添加 `tools/dump_reader/` 到构建路径
- 确保能读取 H.264 dump 文件

### Step 5: 编译测试

```bash
cd ut_wd_intra
make

# 测试 H.265
./ut_wd_intra --codec hevc --directed hevc_dump.bin --yuv hevc.yuv

# 测试 H.264
./ut_wd_intra --codec h264 --directed h264_dump.bin --yuv h264.yuv
```

## 验证方法

1. **H.265 回归测试**: 确保现有 H.265 定向测试不受影响
2. **H.264 功能测试**: 使用新生成的 H.264 dump 文件测试
3. **Codec 切换测试**: 同一程序无缝切换 264/265
4. **ESRC 一致性**: 验证填充的像素与 YUV 输入匹配
5. **Above-edge 一致性**: 验证 edge 数据与 dump 中的 recon 匹配

## 两个 Directed 文件的 API 对比

### 公共接口 (两个文件都实现)

```c
// 初始化
SeqInfo directed_init(const std::string& dumpPath, const std::string& yuvPath);
PictureInfo directed_pic_info();

// 帧操作
void directed_load_yuv_frame(int picIdx);

// CTU 操作
bool directed_next_ctu(CtuRecord& out);
void directed_prepare_ctu(struct t_intra_test* tester, const CtuRecord& ctu);
uint16_t directed_lambda_for_ctu(const CtuRecord& ctu);
void directed_verify_ctu(struct t_intra_test* tester, const CtuRecord& ctu);
void directed_update_above_recon(const CtuRecord& ctu);

// Port 回调
void directed_pmf_response(struct t_intra_test* tester,
                            const t_intra2pmf* req,
                            t_pmf2intra* resp);
void directed_tqitq_response(struct t_intra_test* tester,
                              const t_tqitq_in* intra2tq,
                              t_tqitq_out* tq2intra);
void directed_sel_response(struct t_intra_test* tester,
                            const t_intra2sel* intra2sel,
                            t_sel2intra* sel2intra);
```

### H.265 独有

```c
// Best-mode RAM 填充
void directed_fill_best_modes(struct t_intra_test* tester, const CtuRecord& ctu);

// Z-scan order 转换
int raster_to_zscan(int rx, int ry, int gridSize);
```

### H.264 独有

```c
// 无独有函数，所有 H.264 逻辑在公共接口内根据 codec 类型分支
```

### main.cc 中的调度

```c
// 主循环
while (directed_next_ctu(curCtu)) {
  // 新帧检测
  if (picIdx != lastPicIdx) {
    directed_load_yuv_frame(picIdx);
    // ...
  }
  
  // CTU 准备
  directed_prepare_ctu(tester, curCtu);
  
  // 发送 job
  ut_intra_port_vctrl2intra_write(vctrl);
  ut_intra_wait_ctu_job_done(tester);
  
  // 验证
  directed_verify_ctu(tester, curCtu);
  
  // 更新 above recon
  directed_update_above_recon(curCtu);
}

// Port 回调
void port_cb_intra2pmf(void* ctx) {
  if (g_directed_test) {
    directed_pmf_response(tester, &intra2pmf, &tester->pmf2intra);
  } else {
    // 随机模式
  }
}
```
