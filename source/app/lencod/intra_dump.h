#ifndef __INTRADUMP_H__
#define __INTRADUMP_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "typedefs.h"

#ifndef USE_SATD_FOR_DISTORTION
#define USE_SATD_FOR_DISTORTION 1
#endif

enum IntraDumpTag {
  DUMP_TAG_SEQ_HEADER   = 0x0001,
  DUMP_TAG_MB_BEGIN     = 0x0010,
  DUMP_TAG_MB_END       = 0x0011,
  DUMP_TAG_BLOCK_BEGIN  = 0x0020,
  DUMP_TAG_BLOCK_END    = 0x0021,
  DUMP_TAG_REF_SAMPLES  = 0x0030,
  DUMP_TAG_RECON_PELS   = 0x0031,
  DUMP_TAG_PRED_PELS    = 0x0032,
  DUMP_TAG_MODE_METRIC  = 0x0040,
  DUMP_TAG_FINAL_MODE   = 0x0041,
};

typedef struct IntraDumpSeqInfo {
  uint32_t picWidthLuma;
  uint32_t picHeightLuma;
  uint32_t mbSize;
  uint32_t chromaFormat;
  uint32_t bitDepthLuma;
  uint32_t bitDepthChroma;
  uint32_t baseQp;
  uint8_t  useDqp;
  uint8_t  padding[3];
} IntraDumpSeqInfo;

typedef struct IntraDumpMbKey {
  uint32_t picIdx;
  uint32_t mbAddrX;
  uint32_t mbPelX;
  uint32_t mbPelY;
  uint32_t sliceType;
  int8_t   sliceQp;
  uint8_t  padding[3];
} IntraDumpMbKey;

typedef struct IntraDumpBlockKey {
  uint32_t blockUid;
  uint32_t parentUid;
  uint32_t mbAddrX;
  uint32_t mbPelX;
  uint32_t mbPelY;
  uint32_t blkPelXInMb;
  uint32_t blkPelYInMb;
  uint32_t width;
  uint32_t height;
  uint8_t  compID;
  uint8_t  mbMode;
  uint8_t  intraMode;
  uint8_t  partIdx;
  uint8_t  padding[3];
} IntraDumpBlockKey;

enum IntraDumpModeKind {
  MODE_KIND_DC     = 0,
  MODE_KIND_HOR    = 1,
  MODE_KIND_VER    = 2,
  MODE_KIND_PLANE  = 3,
  MODE_KIND_D4     = 4,
  MODE_KIND_D4R    = 5,
  MODE_KIND_VR     = 6,
  MODE_KIND_HD     = 7,
  MODE_KIND_VL     = 8,
  MODE_KIND_HU     = 9,
};

typedef struct IntraDumpModeMetric {
  uint32_t modeId;
  uint8_t  modeKind;
  uint8_t  mbMode;
  uint8_t  padding[2];
  uint64_t distortionSatd;
} IntraDumpModeMetric;

typedef struct IntraDumpFinalMode {
  uint32_t modeId;
  uint8_t  modeKind;
  uint8_t  mbMode;
  uint8_t  padding[2];
  uint64_t distortionSatd;
} IntraDumpFinalMode;

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
  int dead;
} IntraDumpBlockData;

typedef struct IntraDumper {
  FILE*       fp;
  FILE*       fpPost;
  int         enabled;
  char        dumpDir[512];
  char        pathPre[600];
  char        pathPost[600];
  uint32_t    blockUidCounter;
  uint32_t    picIdx;
  uint32_t    picWidth;
  uint32_t    picHeight;
  IntraDumpBlockData  mbBlocks[INTRA_DUMP_MAX_BLOCKS_PER_MB];
  int                 numMbBlocks;
  IntraDumpBlockData* curBlock;
  IntraDumpTlvRecord  mbPreRecords[16];
  int                 numMbPreRecords;
} IntraDumper;

IntraDumper* intra_dumper_get_instance(void);
void intra_dumper_write_seq_header(IntraDumper* dumper, const IntraDumpSeqInfo* info);
void intra_dumper_begin_picture(IntraDumper* dumper, uint32_t picIdx);
void intra_dumper_end_picture(IntraDumper* dumper);
void intra_dumper_begin_mb(IntraDumper* dumper, const IntraDumpMbKey* key);
void intra_dumper_end_mb(IntraDumper* dumper);
void intra_dumper_begin_block(IntraDumper* dumper, const IntraDumpBlockKey* key);
void intra_dumper_end_block(IntraDumper* dumper);
void intra_dumper_dump_ref_samples(IntraDumper* dumper, const imgpel* buf, uint32_t totalLen, int filtered);
void intra_dumper_dump_recon_pels(IntraDumper* dumper, imgpel** buf, uint32_t x, uint32_t w, uint32_t h);
void intra_dumper_dump_pred_pels(IntraDumper* dumper, uint32_t modeId, uint8_t modeKind,
                                 imgpel** buf, uint32_t x, uint32_t w, uint32_t h, uint64_t satd);
void intra_dumper_dump_mode_metric(IntraDumper* dumper, const IntraDumpModeMetric* metric);
void intra_dumper_dump_final_mode(IntraDumper* dumper, const IntraDumpFinalMode* final);
uint64_t intra_dump_compute_satd_4x4(imgpel** org, int orgX, imgpel** pred, int predX);
uint64_t intra_dump_compute_satd_8x8(imgpel** org, int orgX, imgpel** pred, int predX);
uint64_t intra_dump_compute_satd_16x16(imgpel** org, int orgX, imgpel** pred, int predX);
void intra_dumper_flush_post_order(IntraDumper* dumper);
uint32_t intra_dumper_alloc_block_uid(IntraDumper* dumper);

#endif
