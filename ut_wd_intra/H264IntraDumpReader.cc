#include "H264IntraDumpReader.h"

namespace H264IntraDump {

Reader::Reader(const std::string& path)
  : m_fp(NULL), m_seqValid(false), m_posAfterSeq(0)
{
  memset(&m_seqInfo, 0, sizeof(m_seqInfo));
  m_fp = fopen(path.c_str(), "rb");
  if (m_fp) parseSeqHeader();
}

Reader::~Reader()
{
  if (m_fp) {
    fclose(m_fp);
    m_fp = NULL;
  }
}

bool Reader::readRecord(uint32_t& tag, std::vector<uint8_t>& payload)
{
  if (!m_fp) return false;

  uint32_t header[2];
  if (fread(header, sizeof(uint32_t), 2, m_fp) != 2) return false;

  tag = header[0];
  uint32_t len = header[1];
  payload.resize(len);
  if (len > 0 && fread(&payload[0], 1, len, m_fp) != len) return false;
  return true;
}

bool Reader::parseSeqHeader()
{
  uint32_t tag = 0;
  std::vector<uint8_t> payload;
  if (!readRecord(tag, payload)) return false;
  if (tag != TAG_SEQ_HEADER || payload.size() < sizeof(SeqInfo)) return false;

  memcpy(&m_seqInfo, &payload[0], sizeof(SeqInfo));
  m_seqValid = true;
  m_posAfterSeq = ftell(m_fp);
  return true;
}

void Reader::rewind()
{
  if (m_fp && m_posAfterSeq > 0) fseek(m_fp, m_posAfterSeq, SEEK_SET);
}

bool Reader::next(Event& out)
{
  out = Event();

  uint32_t tag = 0;
  if (!readRecord(tag, m_payload)) return false;

  switch (tag) {
  case TAG_SEQ_HEADER:
    if (m_payload.size() >= sizeof(SeqInfo)) {
      memcpy(&m_seqInfo, &m_payload[0], sizeof(SeqInfo));
      m_seqValid = true;
    }
    out.kind = EventKind::SeqHeader;
    out.seqInfo = &m_seqInfo;
    break;

  case TAG_MB_BEGIN:
    if (m_payload.size() >= sizeof(MbInfo)) memcpy(&m_curMb, &m_payload[0], sizeof(MbInfo));
    out.kind = EventKind::MbBegin;
    out.mbInfo = &m_curMb;
    break;

  case TAG_MB_END:
    if (m_payload.size() >= sizeof(MbEndInfo)) memcpy(&m_curMbEnd, &m_payload[0], sizeof(MbEndInfo));
    out.kind = EventKind::MbEnd;
    out.mbEndInfo = &m_curMbEnd;
    break;

  case TAG_BLOCK_BEGIN:
    if (m_payload.size() >= sizeof(BlockInfo)) memcpy(&m_curBlock, &m_payload[0], sizeof(BlockInfo));
    out.kind = EventKind::BlockBegin;
    out.blockInfo = &m_curBlock;
    break;

  case TAG_BLOCK_END:
    out.kind = EventKind::BlockEnd;
    break;

  case TAG_REF_SAMPLES:
    if (m_payload.size() >= 1) {
      m_curRef.filtered = (m_payload[0] != 0);
      m_curRef.numPels = (uint32_t)(m_payload.size() - 1) / sizeof(int16_t);
      m_curRef.pels = (m_curRef.numPels > 0) ? reinterpret_cast<const int16_t*>(&m_payload[0] + 1) : NULL;
    }
    out.kind = EventKind::RefSamples;
    out.refSamples = &m_curRef;
    break;

  case TAG_RECON_PELS:
    if (m_payload.size() >= 2 * sizeof(uint32_t)) {
      uint32_t w = 0, h = 0;
      memcpy(&w, &m_payload[0], sizeof(uint32_t));
      memcpy(&h, &m_payload[0] + sizeof(uint32_t), sizeof(uint32_t));
      m_curRecon.w = w;
      m_curRecon.h = h;
      m_curRecon.data = (m_payload.size() >= 2 * sizeof(uint32_t) + w * h * sizeof(int16_t))
        ? reinterpret_cast<const int16_t*>(&m_payload[0] + 2 * sizeof(uint32_t)) : NULL;
    }
    out.kind = EventKind::ReconPels;
    out.reconPels = &m_curRecon;
    break;

  case TAG_PRED_PELS:
    if (m_payload.size() >= sizeof(uint32_t) + 1 + 2 * sizeof(uint32_t) + sizeof(uint64_t)) {
      const uint8_t* p = &m_payload[0];
      memcpy(&m_curPred.modeId, p, sizeof(uint32_t)); p += sizeof(uint32_t);
      m_curPred.modeKind = *p; p += 1;
      memcpy(&m_curPred.pels.w, p, sizeof(uint32_t)); p += sizeof(uint32_t);
      memcpy(&m_curPred.pels.h, p, sizeof(uint32_t)); p += sizeof(uint32_t);
      memcpy(&m_curPred.satd, p, sizeof(uint64_t)); p += sizeof(uint64_t);
      size_t headerSize = sizeof(uint32_t) + 1 + 2 * sizeof(uint32_t) + sizeof(uint64_t);
      uint32_t numPels = m_curPred.pels.w * m_curPred.pels.h;
      m_curPred.pels.data = (m_payload.size() >= headerSize + numPels * sizeof(int16_t))
        ? reinterpret_cast<const int16_t*>(&m_payload[0] + headerSize) : NULL;
    }
    out.kind = EventKind::PredPels;
    out.predPels = &m_curPred;
    break;

  case TAG_MODE_METRIC:
    if (m_payload.size() >= sizeof(ModeMetric)) memcpy(&m_curMetric, &m_payload[0], sizeof(ModeMetric));
    out.kind = EventKind::ModeMetric_;
    out.modeMetric = &m_curMetric;
    break;

  case TAG_FINAL_MODE:
    if (m_payload.size() >= sizeof(FinalMode)) memcpy(&m_curFinal, &m_payload[0], sizeof(FinalMode));
    out.kind = EventKind::FinalMode_;
    out.finalMode = &m_curFinal;
    break;

  default:
    return next(out);
  }

  return true;
}

} // namespace H264IntraDump
