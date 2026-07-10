#include "global.h"
#include "intra_dump.h"

extern int HadamardSAD4x4(short* diff);
extern int HadamardSAD8x8(short* diff);

static IntraDumper g_intra_dumper;
static int g_intra_dumper_initialized = 0;

IntraDumper* intra_dumper_get_instance(void)
{
  if (!g_intra_dumper_initialized) {
    memset(&g_intra_dumper, 0, sizeof(IntraDumper));
    g_intra_dumper.enabled = 0;
    g_intra_dumper.fp = NULL;
    g_intra_dumper.fpPost = NULL;
    g_intra_dumper.blockUidCounter = 0;
    g_intra_dumper.numMbBlocks = 0;
    g_intra_dumper.curBlock = NULL;

    const char* dumpDir = getenv("JM_INTRA_DUMP_DIR");
    if (dumpDir != NULL && dumpDir[0] != '\0') {
      g_intra_dumper.enabled = 1;
      strncpy(g_intra_dumper.dumpDir, dumpDir, sizeof(g_intra_dumper.dumpDir) - 1);
      g_intra_dumper.dumpDir[sizeof(g_intra_dumper.dumpDir) - 1] = '\0';

      snprintf(g_intra_dumper.pathPre, sizeof(g_intra_dumper.pathPre), "%s/intra_dump.bin", dumpDir);
      snprintf(g_intra_dumper.pathPost, sizeof(g_intra_dumper.pathPost), "%s/intra_dump_postorder.bin", dumpDir);

      g_intra_dumper.fp = fopen(g_intra_dumper.pathPre, "wb");
      g_intra_dumper.fpPost = fopen(g_intra_dumper.pathPost, "wb");
    }
    g_intra_dumper_initialized = 1;
  }
  return &g_intra_dumper;
}

static void write_tlv(FILE* fp, uint32_t tag, const void* payload, uint32_t len)
{
  if (!fp) return;
  fwrite(&tag, 4, 1, fp);
  fwrite(&len, 4, 1, fp);
  if (len > 0 && payload)
    fwrite(payload, len, 1, fp);
}

static void write_tlv_record(FILE* fp, const IntraDumpTlvRecord* rec)
{
  write_tlv(fp, rec->tag, rec->payload, rec->len);
}

static int block_area(const IntraDumpBlockKey* key)
{
  return (int)(key->width * key->height);
}

static int block_order_key(const IntraDumpBlockKey* key)
{
  return key->compID == 0 ? block_area(key) : 256 + (int)key->compID;
}

static void buffer_tlv(IntraDumper* dumper, uint32_t tag, const void* payload, uint32_t len)
{
  if (!dumper->curBlock) return;
  IntraDumpBlockData* bd = dumper->curBlock;
  if (bd->numRecords >= bd->capacity) {
    bd->capacity = bd->capacity ? bd->capacity * 2 : 16;
    bd->records = (IntraDumpTlvRecord*)realloc(bd->records, bd->capacity * sizeof(IntraDumpTlvRecord));
  }
  IntraDumpTlvRecord* rec = &bd->records[bd->numRecords];
  rec->tag = tag;
  rec->len = len;
  rec->payload = (uint8_t*)malloc(len);
  if (len > 0 && payload)
    memcpy(rec->payload, payload, len);
  bd->numRecords++;

  write_tlv(dumper->fp, tag, payload, len);
}

void intra_dumper_write_seq_header(IntraDumper* dumper, const IntraDumpSeqInfo* info)
{
  if (!dumper->enabled) return;
  dumper->picWidth = info->picWidthLuma;
  dumper->picHeight = info->picHeightLuma;
  write_tlv(dumper->fp, DUMP_TAG_SEQ_HEADER, info, sizeof(IntraDumpSeqInfo));
  write_tlv(dumper->fpPost, DUMP_TAG_SEQ_HEADER, info, sizeof(IntraDumpSeqInfo));
}

void intra_dumper_begin_picture(IntraDumper* dumper, uint32_t picIdx)
{
  if (!dumper->enabled) return;
  dumper->picIdx = picIdx;
  dumper->blockUidCounter = 0;
}

void intra_dumper_end_picture(IntraDumper* dumper)
{
  if (!dumper->enabled) return;
  if (dumper->fp) fflush(dumper->fp);
  if (dumper->fpPost) fflush(dumper->fpPost);
}

uint32_t intra_dumper_alloc_block_uid(IntraDumper* dumper)
{
  return dumper->blockUidCounter++;
}

void intra_dumper_begin_mb(IntraDumper* dumper, const IntraDumpMbKey* key)
{
  if (!dumper->enabled) return;
  dumper->numMbBlocks = 0;
  dumper->curBlock = NULL;

  dumper->mbPreRecords[0].tag = DUMP_TAG_MB_BEGIN;
  dumper->mbPreRecords[0].len = sizeof(IntraDumpMbKey);
  dumper->mbPreRecords[0].payload = (uint8_t*)malloc(sizeof(IntraDumpMbKey));
  memcpy(dumper->mbPreRecords[0].payload, key, sizeof(IntraDumpMbKey));
  dumper->numMbPreRecords = 1;

  write_tlv(dumper->fp, DUMP_TAG_MB_BEGIN, key, sizeof(IntraDumpMbKey));
}

void intra_dumper_end_mb(IntraDumper* dumper, const IntraDumpMbEnd* end)
{
  if (!dumper->enabled) return;
  intra_dumper_flush_post_order(dumper, end);
  write_tlv(dumper->fp, DUMP_TAG_MB_END, end, end ? sizeof(IntraDumpMbEnd) : 0);

  for (int i = 0; i < dumper->numMbPreRecords; i++) {
    free(dumper->mbPreRecords[i].payload);
    dumper->mbPreRecords[i].payload = NULL;
  }
  dumper->numMbPreRecords = 0;
}

void intra_dumper_begin_block(IntraDumper* dumper, const IntraDumpBlockKey* key)
{
  if (!dumper->enabled) return;

  IntraDumpBlockData* bd = &dumper->mbBlocks[dumper->numMbBlocks];
  bd->key = *key;
  bd->numRecords = 0;
  bd->capacity = 0;
  bd->records = NULL;
  bd->dead = 0;
  dumper->curBlock = bd;

  buffer_tlv(dumper, DUMP_TAG_BLOCK_BEGIN, key, sizeof(IntraDumpBlockKey));
}

void intra_dumper_end_block(IntraDumper* dumper)
{
  if (!dumper->enabled || !dumper->curBlock) return;
  buffer_tlv(dumper, DUMP_TAG_BLOCK_END, NULL, 0);
  dumper->numMbBlocks++;
  dumper->curBlock = NULL;
}

void intra_dumper_dump_ref_samples(IntraDumper* dumper, const imgpel* buf, uint32_t totalLen, int filtered)
{
  if (!dumper->enabled || !dumper->curBlock) return;
  uint32_t payloadLen = 1 + totalLen * sizeof(int16_t);
  uint8_t* payload = (uint8_t*)malloc(payloadLen);
  payload[0] = (uint8_t)(filtered ? 1 : 0);
  for (uint32_t i = 0; i < totalLen; i++) {
    int16_t pel = (int16_t)buf[i];
    memcpy(payload + 1 + i * sizeof(int16_t), &pel, sizeof(int16_t));
  }
  buffer_tlv(dumper, DUMP_TAG_REF_SAMPLES, payload, payloadLen);
  free(payload);
}

void intra_dumper_dump_recon_pels(IntraDumper* dumper, imgpel** buf, uint32_t x, uint32_t w, uint32_t h)
{
  if (!dumper->enabled || !dumper->curBlock) return;
  uint32_t payloadLen = 2 * sizeof(uint32_t) + w * h * sizeof(int16_t);
  uint8_t* payload = (uint8_t*)malloc(payloadLen);
  uint8_t* p = payload;
  memcpy(p, &w, sizeof(uint32_t)); p += sizeof(uint32_t);
  memcpy(p, &h, sizeof(uint32_t)); p += sizeof(uint32_t);
  for (uint32_t j = 0; j < h; j++) {
    for (uint32_t i = 0; i < w; i++) {
      int16_t pel = (int16_t)buf[j][x + i];
      memcpy(p + (j * w + i) * sizeof(int16_t), &pel, sizeof(int16_t));
    }
  }
  buffer_tlv(dumper, DUMP_TAG_RECON_PELS, payload, payloadLen);
  free(payload);
}

void intra_dumper_dump_pred_pels(IntraDumper* dumper, uint32_t modeId, uint8_t modeKind,
                                 imgpel** buf, uint32_t x, uint32_t w, uint32_t h, uint64_t satd)
{
  if (!dumper->enabled || !dumper->curBlock) return;
  uint32_t hdrLen = 4 + 1 + 4 + 4 + 8;
  uint32_t payloadLen = hdrLen + w * h * sizeof(int16_t);
  uint8_t* payload = (uint8_t*)malloc(payloadLen);
  uint8_t* p = payload;
  memcpy(p, &modeId, 4); p += 4;
  *p = modeKind; p += 1;
  memcpy(p, &w, 4); p += 4;
  memcpy(p, &h, 4); p += 4;
  memcpy(p, &satd, 8); p += 8;
  for (uint32_t j = 0; j < h; j++) {
    for (uint32_t i = 0; i < w; i++) {
      int16_t pel = (int16_t)buf[j][x + i];
      memcpy(p + (j * w + i) * sizeof(int16_t), &pel, sizeof(int16_t));
    }
  }
  buffer_tlv(dumper, DUMP_TAG_PRED_PELS, payload, payloadLen);
  free(payload);
}

void intra_dumper_dump_mode_metric(IntraDumper* dumper, const IntraDumpModeMetric* metric)
{
  if (!dumper->enabled || !dumper->curBlock) return;
  buffer_tlv(dumper, DUMP_TAG_MODE_METRIC, metric, sizeof(IntraDumpModeMetric));
}

void intra_dumper_dump_final_mode(IntraDumper* dumper, const IntraDumpFinalMode* final)
{
  if (!dumper->enabled || !dumper->curBlock) return;
  buffer_tlv(dumper, DUMP_TAG_FINAL_MODE, final, sizeof(IntraDumpFinalMode));
}

uint64_t intra_dump_compute_satd_4x4(imgpel** org, int orgX, imgpel** pred, int predX)
{
  short diff[16];
  int idx = 0;
  for (int j = 0; j < 4; j++) {
    for (int i = 0; i < 4; i++) {
      diff[idx++] = (short)((int)org[j][orgX + i] - (int)pred[j][predX + i]);
    }
  }
  return (uint64_t)HadamardSAD4x4(diff);
}

uint64_t intra_dump_compute_satd_8x8(imgpel** org, int orgX, imgpel** pred, int predX)
{
  short diff[64];
  int idx = 0;
  for (int j = 0; j < 8; j++) {
    for (int i = 0; i < 8; i++) {
      diff[idx++] = (short)((int)org[j][orgX + i] - (int)pred[j][predX + i]);
    }
  }
  return (uint64_t)HadamardSAD8x8(diff);
}

uint64_t intra_dump_compute_satd_16x16(imgpel** org, int orgX, imgpel** pred, int predX)
{
  uint64_t totalSatd = 0;
  for (int by = 0; by < 2; by++) {
    for (int bx = 0; bx < 2; bx++) {
      totalSatd += intra_dump_compute_satd_8x8(org + by * 8, orgX + bx * 8, pred + by * 8, predX + bx * 8);
    }
  }
  return totalSatd;
}

void intra_dumper_flush_post_order(IntraDumper* dumper, const IntraDumpMbEnd* end)
{
  if (!dumper->fpPost) return;

  for (int i = 0; i < dumper->numMbPreRecords; i++) {
    write_tlv_record(dumper->fpPost, &dumper->mbPreRecords[i]);
  }

  int N = dumper->numMbBlocks;
  if (N == 0) {
    write_tlv(dumper->fpPost, DUMP_TAG_MB_END, end, end ? sizeof(IntraDumpMbEnd) : 0);
    fflush(dumper->fpPost);
    return;
  }

  int parent[INTRA_DUMP_MAX_BLOCKS_PER_MB];
  int children[INTRA_DUMP_MAX_BLOCKS_PER_MB][8];
  int numChildren[INTRA_DUMP_MAX_BLOCKS_PER_MB];

  for (int i = 0; i < N; i++) {
    parent[i] = -1;
    numChildren[i] = 0;
  }

  for (int i = 0; i < N; i++) {
    IntraDumpBlockKey* ki = &dumper->mbBlocks[i].key;
    if (ki->width == 4 && ki->height == 4) {
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
    }
  }

  int postOrder[INTRA_DUMP_MAX_BLOCKS_PER_MB];
  int postOrderCount = 0;
  int roots[INTRA_DUMP_MAX_BLOCKS_PER_MB];
  int numRoots = 0;

  for (int i = 0; i < N; i++) {
    if (parent[i] == -1)
      roots[numRoots++] = i;
  }

  for (int i = 0; i < numRoots - 1; i++) {
    for (int j = i + 1; j < numRoots; j++) {
      IntraDumpBlockKey* ki = &dumper->mbBlocks[roots[i]].key;
      IntraDumpBlockKey* kj = &dumper->mbBlocks[roots[j]].key;
      int orderI = block_order_key(ki);
      int orderJ = block_order_key(kj);
      int swapRoots = orderI > orderJ ||
        (orderI == orderJ && ki->blockUid > kj->blockUid);
      if (swapRoots) {
        int tmp = roots[i];
        roots[i] = roots[j];
        roots[j] = tmp;
      }
    }
  }

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

  for (int i = 0; i < postOrderCount; i++) {
    IntraDumpBlockData* bd = &dumper->mbBlocks[postOrder[i]];
    if (bd->dead) continue;
    for (int j = 0; j < bd->numRecords; j++) {
      write_tlv_record(dumper->fpPost, &bd->records[j]);
    }
  }

  write_tlv(dumper->fpPost, DUMP_TAG_MB_END, end, end ? sizeof(IntraDumpMbEnd) : 0);
  fflush(dumper->fpPost);

  for (int i = 0; i < N; i++) {
    IntraDumpBlockData* bd = &dumper->mbBlocks[i];
    if (bd->records) {
      for (int j = 0; j < bd->numRecords; j++) {
        free(bd->records[j].payload);
      }
      free(bd->records);
      bd->records = NULL;
    }
    bd->numRecords = 0;
    bd->capacity = 0;
  }
  dumper->numMbBlocks = 0;
}
