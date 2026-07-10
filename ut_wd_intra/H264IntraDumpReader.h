#ifndef H264_INTRA_DUMP_READER_H
#define H264_INTRA_DUMP_READER_H

#include <cstdio>
#include <cstring>
#include <stdint.h>
#include <string>
#include <vector>

namespace H264IntraDump {

enum Tag
{
  TAG_SEQ_HEADER  = 0x0001,
  TAG_MB_BEGIN    = 0x0010,
  TAG_MB_END      = 0x0011,
  TAG_BLOCK_BEGIN = 0x0020,
  TAG_BLOCK_END   = 0x0021,
  TAG_REF_SAMPLES = 0x0030,
  TAG_RECON_PELS  = 0x0031,
  TAG_PRED_PELS   = 0x0032,
  TAG_MODE_METRIC = 0x0040,
  TAG_FINAL_MODE  = 0x0041
};

struct SeqInfo
{
  uint32_t picWidthLuma;
  uint32_t picHeightLuma;
  uint32_t mbSize;
  uint32_t chromaFormat;
  uint32_t bitDepthLuma;
  uint32_t bitDepthChroma;
  uint32_t baseQp;
  uint8_t  useDqp;
  uint8_t  padding[3];
};

struct MbInfo
{
  uint32_t picIdx;
  uint32_t mbAddrX;
  uint32_t mbPelX;
  uint32_t mbPelY;
  uint32_t sliceType;
  int8_t   sliceQp;
  uint8_t  padding[3];
};

struct BlockInfo
{
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
};

struct ModeMetric
{
  uint32_t modeId;
  uint8_t  modeKind;
  uint8_t  mbMode;
  uint8_t  padding[2];
  uint64_t distortionSatd;
};

struct FinalMode
{
  uint32_t modeId;
  uint8_t  modeKind;
  uint8_t  mbMode;
  uint8_t  padding[2];
  uint64_t distortionSatd;
};

struct MbEndInfo
{
  uint32_t mbType;
  uint32_t finalPartWidth;
  uint32_t finalPartHeight;
  uint32_t lumaTransformSize8x8Flag;
};

struct PelArrayView
{
  const int16_t* data;
  uint32_t w;
  uint32_t h;
};

struct RefSamplesData
{
  bool filtered;
  const int16_t* pels;
  uint32_t numPels;
};

struct PredPelsData
{
  uint32_t modeId;
  uint8_t modeKind;
  uint64_t satd;
  PelArrayView pels;
};

struct EventKind
{
  enum Value
  {
    SeqHeader,
    MbBegin,
    MbEnd,
    BlockBegin,
    BlockEnd,
    RefSamples,
    ReconPels,
    PredPels,
    ModeMetric_,
    FinalMode_,
    Eof
  };
};

struct Event
{
  EventKind::Value kind;
  const SeqInfo* seqInfo;
  const MbInfo* mbInfo;
  const MbEndInfo* mbEndInfo;
  const BlockInfo* blockInfo;
  const RefSamplesData* refSamples;
  const PelArrayView* reconPels;
  const PredPelsData* predPels;
  const ModeMetric* modeMetric;
  const FinalMode* finalMode;

  Event()
    : kind(EventKind::Eof), seqInfo(NULL), mbInfo(NULL), mbEndInfo(NULL),
      blockInfo(NULL),
      refSamples(NULL), reconPels(NULL), predPels(NULL), modeMetric(NULL),
      finalMode(NULL) {}
};

class Reader
{
public:
  explicit Reader(const std::string& path);
  ~Reader();

  bool isOpen() const { return m_fp != NULL; }
  const SeqInfo& seqInfo() const { return m_seqInfo; }
  bool next(Event& out);
  void rewind();

private:
  FILE* m_fp;
  SeqInfo m_seqInfo;
  bool m_seqValid;
  long m_posAfterSeq;

  std::vector<uint8_t> m_payload;
  MbInfo m_curMb;
  MbEndInfo m_curMbEnd;
  BlockInfo m_curBlock;
  RefSamplesData m_curRef;
  PelArrayView m_curRecon;
  PredPelsData m_curPred;
  ModeMetric m_curMetric;
  FinalMode m_curFinal;

  bool readRecord(uint32_t& tag, std::vector<uint8_t>& payload);
  bool parseSeqHeader();
};

} // namespace H264IntraDump

#endif
