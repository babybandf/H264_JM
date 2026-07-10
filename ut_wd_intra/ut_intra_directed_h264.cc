// H.264 directed intra test support.
// Included by main.cc after ut_intra.cc and the HEVC directed implementation.

#include "H264IntraDumpReader.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

struct H264DirectedBlockRecord
{
    struct PredEntry
    {
        uint32_t modeId;
        uint8_t modeKind;
        uint64_t satd;
        uint32_t w;
        uint32_t h;
        std::vector<int16_t> pels;
    };

    H264IntraDump::BlockInfo info;
    std::vector<PredEntry> preds;
    std::vector<H264IntraDump::ModeMetric> metrics;
    bool hasRecon;
    uint32_t reconW;
    uint32_t reconH;
    std::vector<int16_t> reconPels;

    H264DirectedBlockRecord() : hasRecon(false), reconW(0), reconH(0) {}
};

struct H264DirectedMbRecord
{
    H264IntraDump::MbInfo info;
    H264IntraDump::MbEndInfo endInfo;
    std::vector<H264DirectedBlockRecord> blocks;

    void clear()
    {
        memset(&info, 0, sizeof(info));
        memset(&endInfo, 0, sizeof(endInfo));
        blocks.clear();
    }
};

struct H264DirectedYuvFrame
{
    std::vector<int16_t> y;
    std::vector<int16_t> u;
    std::vector<int16_t> v;
    uint32_t width;
    uint32_t height;
    uint32_t chromaW;
    uint32_t chromaH;

    H264DirectedYuvFrame() : width(0), height(0), chromaW(0), chromaH(0) {}

    int16_t getY(int x, int yy) const
    {
        if (x < 0 || yy < 0 || (uint32_t)x >= width || (uint32_t)yy >= height) return 0;
        return y[(uint32_t)yy * width + (uint32_t)x];
    }

    int16_t getU(int x, int yy) const
    {
        if (x < 0 || yy < 0 || (uint32_t)x >= chromaW || (uint32_t)yy >= chromaH) return 0;
        return u[(uint32_t)yy * chromaW + (uint32_t)x];
    }

    int16_t getV(int x, int yy) const
    {
        if (x < 0 || yy < 0 || (uint32_t)x >= chromaW || (uint32_t)yy >= chromaH) return 0;
        return v[(uint32_t)yy * chromaW + (uint32_t)x];
    }
};

struct H264DirectedDriver
{
    H264IntraDump::Reader* reader;
    FILE* yuvFp;
    H264IntraDump::SeqInfo seq;
    H264DirectedMbRecord curMb;
    H264DirectedYuvFrame frame;
    uint64_t frameBytes;
    int lastPicIdx;
    bool hasPending;
    H264IntraDump::MbInfo pendingMb;
    std::vector<int16_t> aboveReconY;
    std::vector<int16_t> aboveReconU;
    std::vector<int16_t> aboveReconV;

    H264DirectedDriver() : reader(NULL), yuvFp(NULL), frameBytes(0), lastPicIdx(-1), hasPending(false)
    {
        memset(&seq, 0, sizeof(seq));
        memset(&pendingMb, 0, sizeof(pendingMb));
    }

    ~H264DirectedDriver()
    {
        delete reader;
        if (yuvFp) fclose(yuvFp);
    }
};

static H264DirectedDriver g_h264_directed;

static uint32_t h264_directed_chroma_width(const H264IntraDump::SeqInfo& seq)
{
    if (seq.chromaFormat == 0) return 0;
    return (seq.chromaFormat == 1 || seq.chromaFormat == 2) ? seq.picWidthLuma / 2 : seq.picWidthLuma;
}

static uint32_t h264_directed_chroma_height(const H264IntraDump::SeqInfo& seq)
{
    if (seq.chromaFormat == 0) return 0;
    return (seq.chromaFormat == 1) ? seq.picHeightLuma / 2 : seq.picHeightLuma;
}

static int h264_directed_yuv_format(const H264IntraDump::SeqInfo& seq)
{
    return (seq.chromaFormat == 2) ? YUV422 : YUV420;
}

static int h264_directed_chroma_x(const H264IntraDump::SeqInfo& seq, int lumaX)
{
    return (seq.chromaFormat == 3) ? lumaX : lumaX / 2;
}

static int h264_directed_chroma_y(const H264IntraDump::SeqInfo& seq, int lumaY)
{
    return (seq.chromaFormat == 1) ? lumaY / 2 : lumaY;
}

static H264IntraDump::SeqInfo h264_directed_init(const std::string& dumpPath, const std::string& yuvPath)
{
    delete g_h264_directed.reader;
    g_h264_directed.reader = new H264IntraDump::Reader(dumpPath);
    if (!g_h264_directed.reader->isOpen()) {
        fprintf(stderr, "ERROR: Cannot open H.264 dump file: %s\n", dumpPath.c_str());
        exit(1);
    }

    if (g_h264_directed.yuvFp) fclose(g_h264_directed.yuvFp);
    g_h264_directed.yuvFp = fopen(yuvPath.c_str(), "rb");
    if (!g_h264_directed.yuvFp) {
        fprintf(stderr, "ERROR: Cannot open H.264 YUV file: %s\n", yuvPath.c_str());
        exit(1);
    }

    g_h264_directed.seq = g_h264_directed.reader->seqInfo();
    const H264IntraDump::SeqInfo& seq = g_h264_directed.seq;
    uint32_t chromaW = h264_directed_chroma_width(seq);
    uint32_t chromaH = h264_directed_chroma_height(seq);
    uint32_t bytesL = (seq.bitDepthLuma > 8) ? 2U : 1U;
    uint32_t bytesC = (seq.bitDepthChroma > 8) ? 2U : 1U;

    g_h264_directed.frameBytes = (uint64_t)seq.picWidthLuma * seq.picHeightLuma * bytesL +
        2ULL * chromaW * chromaH * bytesC;
    g_h264_directed.aboveReconY.assign(seq.picWidthLuma, 0);
    g_h264_directed.aboveReconU.assign(chromaW, 0);
    g_h264_directed.aboveReconV.assign(chromaW, 0);
    g_h264_directed.lastPicIdx = -1;
    g_h264_directed.hasPending = false;

    printf("=== H.264 Directed Mode ===\n");
    printf("  Dump: %s\n", dumpPath.c_str());
    printf("  YUV:  %s\n", yuvPath.c_str());
    printf("  Resolution: %ux%u, MB: %u, BitDepth: %u/%u, ChromaFmt: %u\n",
           seq.picWidthLuma, seq.picHeightLuma, seq.mbSize,
           seq.bitDepthLuma, seq.bitDepthChroma, seq.chromaFormat);

    return seq;
}

static PictureInfo h264_directed_pic_info()
{
    PictureInfo pic;
    pic.width = (int)g_h264_directed.seq.picWidthLuma;
    pic.height = (int)g_h264_directed.seq.picHeightLuma;
    pic.codec = VCE_PROTOCOL_CODEC_H264;
    return pic;
}

static void h264_directed_load_yuv_frame(int picIdx)
{
    const H264IntraDump::SeqInfo& seq = g_h264_directed.seq;
    H264DirectedYuvFrame& frame = g_h264_directed.frame;
    uint32_t yPels = seq.picWidthLuma * seq.picHeightLuma;
    uint32_t chromaW = h264_directed_chroma_width(seq);
    uint32_t chromaH = h264_directed_chroma_height(seq);
    uint32_t cPels = chromaW * chromaH;

    frame.width = seq.picWidthLuma;
    frame.height = seq.picHeightLuma;
    frame.chromaW = chromaW;
    frame.chromaH = chromaH;
    frame.y.assign(yPels, 0);
    frame.u.assign(cPels, 0);
    frame.v.assign(cPels, 0);

    fseek(g_h264_directed.yuvFp, (long)((uint64_t)picIdx * g_h264_directed.frameBytes), SEEK_SET);
    if (seq.bitDepthLuma <= 8) {
        std::vector<uint8_t> tmp(yPels);
        if (yPels) fread(&tmp[0], 1, yPels, g_h264_directed.yuvFp);
        for (uint32_t i = 0; i < yPels; i++) frame.y[i] = tmp[i];
    } else {
        std::vector<uint16_t> tmp(yPels);
        if (yPels) fread(&tmp[0], 2, yPels, g_h264_directed.yuvFp);
        for (uint32_t i = 0; i < yPels; i++) frame.y[i] = (int16_t)tmp[i];
    }

    if (cPels && seq.bitDepthChroma <= 8) {
        std::vector<uint8_t> tmp(cPels);
        fread(&tmp[0], 1, cPels, g_h264_directed.yuvFp);
        for (uint32_t i = 0; i < cPels; i++) frame.u[i] = tmp[i];
        fread(&tmp[0], 1, cPels, g_h264_directed.yuvFp);
        for (uint32_t i = 0; i < cPels; i++) frame.v[i] = tmp[i];
    } else if (cPels) {
        std::vector<uint16_t> tmp(cPels);
        fread(&tmp[0], 2, cPels, g_h264_directed.yuvFp);
        for (uint32_t i = 0; i < cPels; i++) frame.u[i] = (int16_t)tmp[i];
        fread(&tmp[0], 2, cPels, g_h264_directed.yuvFp);
        for (uint32_t i = 0; i < cPels; i++) frame.v[i] = (int16_t)tmp[i];
    }
    g_h264_directed.lastPicIdx = picIdx;
}

static bool h264_directed_next_mb(H264DirectedMbRecord& out)
{
    out.clear();
    H264IntraDump::Event ev;
    if (g_h264_directed.hasPending) {
        out.info = g_h264_directed.pendingMb;
        g_h264_directed.hasPending = false;
    } else {
        while (g_h264_directed.reader->next(ev)) {
            if (ev.kind == H264IntraDump::EventKind::MbBegin) {
                out.info = *ev.mbInfo;
                break;
            }
        }
        if (ev.kind != H264IntraDump::EventKind::MbBegin) return false;
    }

    H264DirectedBlockRecord* curBlock = NULL;
    while (g_h264_directed.reader->next(ev)) {
        switch (ev.kind) {
        case H264IntraDump::EventKind::MbBegin:
            g_h264_directed.hasPending = true;
            g_h264_directed.pendingMb = *ev.mbInfo;
            return true;
        case H264IntraDump::EventKind::MbEnd:
            if (ev.mbEndInfo) out.endInfo = *ev.mbEndInfo;
            return true;
        case H264IntraDump::EventKind::BlockBegin:
            out.blocks.push_back(H264DirectedBlockRecord());
            curBlock = &out.blocks.back();
            curBlock->info = *ev.blockInfo;
            break;
        case H264IntraDump::EventKind::BlockEnd:
            curBlock = NULL;
            break;
        case H264IntraDump::EventKind::ReconPels:
            if (curBlock && ev.reconPels && ev.reconPels->data) {
                curBlock->hasRecon = true;
                curBlock->reconW = ev.reconPels->w;
                curBlock->reconH = ev.reconPels->h;
                uint32_t n = ev.reconPels->w * ev.reconPels->h;
                curBlock->reconPels.assign(ev.reconPels->data, ev.reconPels->data + n);
            }
            break;
        case H264IntraDump::EventKind::PredPels:
            if (curBlock && ev.predPels && ev.predPels->pels.data) {
                H264DirectedBlockRecord::PredEntry pred;
                pred.modeId = ev.predPels->modeId;
                pred.modeKind = ev.predPels->modeKind;
                pred.satd = ev.predPels->satd;
                pred.w = ev.predPels->pels.w;
                pred.h = ev.predPels->pels.h;
                uint32_t n = pred.w * pred.h;
                pred.pels.assign(ev.predPels->pels.data, ev.predPels->pels.data + n);
                curBlock->preds.push_back(pred);
            }
            break;
        case H264IntraDump::EventKind::ModeMetric_:
            if (curBlock && ev.modeMetric) curBlock->metrics.push_back(*ev.modeMetric);
            break;
        case H264IntraDump::EventKind::Eof:
            return !out.blocks.empty();
        default:
            break;
        }
    }
    return !out.blocks.empty();
}

static uint16_t h264_directed_lambda_for_mb(const H264DirectedMbRecord& mb)
{
    (void)mb;
    return 0;
}

static const H264DirectedBlockRecord* h264_directed_find_block(const H264DirectedMbRecord& mb, int blkX4, int blkY4, int sizeIdx, int comp)
{
    int pelX = blkX4 * 4;
    int pelY = blkY4 * 4;
    int blkSize = 4 << sizeIdx;
    if (comp != 0 && g_h264_directed.seq.chromaFormat == 1) {
        pelX /= 2;
        pelY /= 2;
        blkSize /= 2;
    }
    for (size_t i = 0; i < mb.blocks.size(); i++) {
        const H264DirectedBlockRecord& block = mb.blocks[i];
        if ((int)block.info.compID == comp &&
            (int)block.info.blkPelXInMb == pelX &&
            (int)block.info.blkPelYInMb == pelY &&
            (int)block.info.width == blkSize &&
            (int)block.info.height == blkSize) return &block;
    }
    return NULL;
}

static const H264IntraDump::ModeMetric* h264_directed_find_metric(const H264DirectedBlockRecord& block, int modeId)
{
    for (size_t i = 0; i < block.metrics.size(); i++) {
        if ((int)block.metrics[i].modeId == modeId) return &block.metrics[i];
    }
    return block.metrics.empty() ? NULL : &block.metrics[0];
}

static void h264_directed_fill_esrc(struct t_intra_test* tester, const H264DirectedMbRecord& mb)
{
    const H264IntraDump::SeqInfo& seq = g_h264_directed.seq;
    int mbSize = (int)seq.mbSize;
    int esrcSlot = (int)(mb.info.mbAddrX % MVRAM_ME_SRC_SLOT_NUM);
    int yuvFmt = h264_directed_yuv_format(seq);
    (void)tester;
    for (int y = 0; y < mbSize; y++) {
        for (int x = 0; x < mbSize; x++) {
            int addr = vce_rams_me_src_addr_calc(esrcSlot, x, y, yuvFmt, PLANER_Y);
            vceemul_ram_write_elem(VCE_RAM_ID_ESRC, addr, x % 8,
                g_h264_directed.frame.getY((int)mb.info.mbPelX + x, (int)mb.info.mbPelY + y));
        }
    }

    if (seq.chromaFormat == 0) return;
    int chromaW = (seq.chromaFormat == 3) ? mbSize : mbSize / 2;
    int chromaH = (seq.chromaFormat == 1) ? mbSize / 2 : mbSize;
    int baseX = h264_directed_chroma_x(seq, (int)mb.info.mbPelX);
    int baseY = h264_directed_chroma_y(seq, (int)mb.info.mbPelY);
    for (int y = 0; y < chromaH; y++) {
        for (int x = 0; x < chromaW; x++) {
            int addrU = vce_rams_me_src_addr_calc(esrcSlot, x, y, yuvFmt, PLANER_U);
            int addrV = vce_rams_me_src_addr_calc(esrcSlot, x, y, yuvFmt, PLANER_V);
            vceemul_ram_write_elem(VCE_RAM_ID_ESRC, addrU, x % 8, g_h264_directed.frame.getU(baseX + x, baseY + y));
            vceemul_ram_write_elem(VCE_RAM_ID_ESRC, addrV, x % 8, g_h264_directed.frame.getV(baseX + x, baseY + y));
        }
    }
}

static void h264_directed_fill_above_edge(struct t_intra_test* tester, const H264DirectedMbRecord& mb)
{
    const H264IntraDump::SeqInfo& seq = g_h264_directed.seq;
    int mbSize = (int)seq.mbSize;
    int mbX = (int)mb.info.mbPelX / mbSize;
    int mbY = (int)mb.info.mbPelY / mbSize;
    int slot = mbX % INTRA_RAM_CTU_ABOVE_EDGE_SLOT_NUM;
    (void)tester;

    int addr = vce_rams_intra_ram_ctu_above_edge_addr_calc(VCE_PROTOCOL_CODEC_H264, INTRA_RAM_CTU_ABOVE_EDGE_Y, slot);
    for (int i = 0; i < INTRA_RAM_CTU_ABOVE_EDGE_LUMA_PIXS_PER_ADDR; i++) {
        int px = (int)mb.info.mbPelX + i;
        int16_t val = (mbY > 0 && px < (int)g_h264_directed.aboveReconY.size()) ? g_h264_directed.aboveReconY[(size_t)px] : 0;
        vceemul_ram_write_elem(VCE_RAM_ID_INTRA_RAM, addr, i, val);
    }

    if (seq.chromaFormat == 0) return;
    int chromaX = h264_directed_chroma_x(seq, (int)mb.info.mbPelX);
    addr = vce_rams_intra_ram_ctu_above_edge_addr_calc(VCE_PROTOCOL_CODEC_H264, INTRA_RAM_CTU_ABOVE_EDGE_U, slot);
    for (int i = 0; i < INTRA_RAM_CTU_ABOVE_EDGE_CHROMA_PIXS_PER_ADDR; i++) {
        int cx = chromaX + i;
        int16_t val = (mbY > 0 && cx < (int)g_h264_directed.aboveReconU.size()) ? g_h264_directed.aboveReconU[(size_t)cx] : 0;
        vceemul_ram_write_elem(VCE_RAM_ID_INTRA_RAM, addr, i, val);
    }
    addr = vce_rams_intra_ram_ctu_above_edge_addr_calc(VCE_PROTOCOL_CODEC_H264, INTRA_RAM_CTU_ABOVE_EDGE_V, slot);
    for (int i = 0; i < INTRA_RAM_CTU_ABOVE_EDGE_CHROMA_PIXS_PER_ADDR; i++) {
        int cx = chromaX + i;
        int16_t val = (mbY > 0 && cx < (int)g_h264_directed.aboveReconV.size()) ? g_h264_directed.aboveReconV[(size_t)cx] : 0;
        vceemul_ram_write_elem(VCE_RAM_ID_INTRA_RAM, addr, i + 8, val);
    }
}

static void h264_directed_prepare_mb(struct t_intra_test* tester, const H264DirectedMbRecord& mb)
{
    h264_directed_fill_above_edge(tester, mb);
    h264_directed_fill_esrc(tester, mb);
}

static void h264_directed_update_above_recon(const H264DirectedMbRecord& mb)
{
    const H264IntraDump::SeqInfo& seq = g_h264_directed.seq;
    for (size_t i = 0; i < mb.blocks.size(); i++) {
        const H264DirectedBlockRecord& block = mb.blocks[i];
        if (!block.hasRecon || block.reconPels.empty()) continue;
        if (block.info.compID == 0) {
            if (block.info.blkPelYInMb + block.info.height != seq.mbSize) continue;
            uint32_t row = block.reconH - 1;
            uint32_t absX = mb.info.mbPelX + block.info.blkPelXInMb;
            for (uint32_t x = 0; x < block.info.width && absX + x < g_h264_directed.aboveReconY.size(); x++) {
                g_h264_directed.aboveReconY[absX + x] = block.reconPels[row * block.reconW + x];
            }
        } else if (seq.chromaFormat != 0) {
            uint32_t chromaMbH = (seq.chromaFormat == 1) ? seq.mbSize / 2 : seq.mbSize;
            if (block.info.blkPelYInMb + block.info.height != chromaMbH) continue;
            uint32_t row = block.reconH - 1;
            uint32_t absX = (uint32_t)h264_directed_chroma_x(seq, (int)mb.info.mbPelX) + block.info.blkPelXInMb;
            std::vector<int16_t>& dst = (block.info.compID == 1) ? g_h264_directed.aboveReconU : g_h264_directed.aboveReconV;
            for (uint32_t x = 0; x < block.info.width && absX + x < dst.size(); x++) {
                dst[absX + x] = block.reconPels[row * block.reconW + x];
            }
        }
    }
}

static void h264_directed_pmf_response(struct t_intra_test* tester, const t_intra2pmf* req, t_pmf2intra* resp)
{
    init_pmf2intra(resp);
    resp->pmf2intra_src_ack = (req->intra2pmf_src_req != 0) ? 1 : 0;
    resp->pmf2intra_src_sync_idx = ((req->intra2pmf_src_req_pos_y & 0x3f) << 6) | (req->intra2pmf_src_req_pos_x & 0x3f);

    const H264IntraDump::SeqInfo& seq = g_h264_directed.seq;
    const H264DirectedMbRecord& mb = g_h264_directed.curMb;
    int comp = req->intra2pmf_src_req_comp;
    int baseX = (comp == 0) ? (int)mb.info.mbPelX : h264_directed_chroma_x(seq, (int)mb.info.mbPelX);
    int baseY = (comp == 0) ? (int)mb.info.mbPelY : h264_directed_chroma_y(seq, (int)mb.info.mbPelY);
    (void)tester;

    if (req->intra2pmf_src_req_type == INTRA2PMF_SRC_REQ_TYPE_16X1) {
        for (int i = 0; i < 16; i++) {
            int x = baseX + req->intra2pmf_src_req_pos_x + i;
            int y = baseY + req->intra2pmf_src_req_pos_y;
            resp->pmf2intra_src_ack_data[i] = (uint16_t)((comp == 0) ? g_h264_directed.frame.getY(x, y) : (comp == 1) ? g_h264_directed.frame.getU(x, y) : g_h264_directed.frame.getV(x, y));
        }
    } else if (req->intra2pmf_src_req_type == INTRA2PMF_SRC_REQ_TYPE_8X8) {
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                int px = baseX + req->intra2pmf_src_req_pos_x + x;
                int py = baseY + req->intra2pmf_src_req_pos_y + y;
                resp->pmf2intra_src_ack_data[y * 8 + x] = (uint16_t)((comp == 0) ? g_h264_directed.frame.getY(px, py) : (comp == 1) ? g_h264_directed.frame.getU(px, py) : g_h264_directed.frame.getV(px, py));
            }
        }
    } else {
        assert(0);
    }
}

static void h264_directed_tqitq_response(struct t_intra_test* tester, const t_tqitq_in* intra2tq, t_tqitq_out* tq2intra)
{
    init_tqitq_out(tq2intra);
    const t_tqitq_inf& inf = intra2tq->tqitq_inf;
    int sizeIdx = (inf.inf_pu_size == 4) ? 2 : (inf.inf_pu_size == 2) ? 1 : 0;
    const H264DirectedBlockRecord* block = h264_directed_find_block(g_h264_directed.curMb, inf.inf_blk_x, inf.inf_blk_y, sizeIdx, inf.inf_comp);
    const H264IntraDump::ModeMetric* metric = block ? h264_directed_find_metric(*block, inf.inf_blk_mode) : NULL;
    (void)tester;

    tq2intra->tqitq_bit_valid = intra2tq->tqitq_in_valid;
    tq2intra->tqitq_bit_info.bits_blk_x = inf.inf_blk_x;
    tq2intra->tqitq_bit_info.bits_blk_y = inf.inf_blk_y;
    tq2intra->tqitq_bit_info.bits_pu_size = inf.inf_pu_size;
    tq2intra->tqitq_bit_info.inf_blk_mode = inf.inf_blk_mode;
    tq2intra->tqitq_bit_info.bits_rdo_type = inf.inf_rdo_type;
    tq2intra->tqitq_bit_info.bits_comp = inf.inf_comp;
    tq2intra->tqitq_bit_info.bits_transform_skip = inf.inf_transform_skip;
    tq2intra->tqitq_zdist_valid = intra2tq->tqitq_in_valid;
    tq2intra->tqitq_zdist_info = tq2intra->tqitq_bit_info;
    tq2intra->tqitq_bit_data = 0;
    tq2intra->tqitq_bit_ncoeffs = (intra2tq->tqitq_in_valid != 0) ? 16 : 0;
    tq2intra->tqitq_bit_dc_ncoeffs = 0;
    tq2intra->tqitq_bit_dist_data = metric ? (int)std::min<uint64_t>(metric->distortionSatd, 0x7fffffffULL) : 0;
    tq2intra->tqitq_zdist_data = tq2intra->tqitq_bit_dist_data;
}

static void h264_directed_sel_response(struct t_intra_test* tester, const t_intra2sel* intra2sel, t_sel2intra* sel2intra)
{
    init_sel2intra(sel2intra);
    sel2intra->blk_x4 = (uint8_t)intra2sel->Intra2sel_intra_info[0].blk_x4;
    sel2intra->blk_y4 = (uint8_t)intra2sel->Intra2sel_intra_info[0].blk_y4;
    sel2intra->size = convert_size_valid_to_size(intra2sel->size_valid);
    sel2intra->avail = true;
    sel2intra->rdo_type = 0;
    sel2intra->split_flag = (sel2intra->size == 2 /*16x16*/ && g_h264_directed.curMb.endInfo.finalPartWidth != 16) ? 1 : 0;
    (void)tester;

    int blkSize = 4 << sel2intra->size;
    for (int comp = 0; comp < 3; comp++) {
        const H264DirectedBlockRecord* block = h264_directed_find_block(g_h264_directed.curMb, sel2intra->blk_x4, sel2intra->blk_y4, sel2intra->size, comp);
        if (!block || !block->hasRecon || block->reconPels.empty()) continue;
        int edgeSize = (comp != 0 && g_h264_directed.seq.chromaFormat == 1) ? blkSize / 2 : blkSize;
        if (edgeSize < 1) edgeSize = 1;
        for (int i = 0; i < edgeSize && i < 16; i++) {
            int16_t val = (i < (int)block->reconH && block->reconW > 0) ? block->reconPels[(size_t)i * block->reconW + (block->reconW - 1)] : 0;
            sel2intra->recon_data[comp][0].recon[i] = (uint16_t)val;
        }
        for (int i = 0; i < edgeSize && edgeSize + i < 16; i++) {
            int16_t val = (i < (int)block->reconW && block->reconH > 0) ? block->reconPels[(size_t)(block->reconH - 1) * block->reconW + i] : 0;
            sel2intra->recon_data[comp][0].recon[edgeSize + i] = (uint16_t)val;
        }
    }
}

static void h264_directed_verify_mb(struct t_intra_test* tester, const H264DirectedMbRecord& mb)
{
    ASSERT_EQ(tester->intra2vctrl.ctu_end, 1);
    int lumaBlocks = 0;
    int cbBlocks = 0;
    int crBlocks = 0;
    for (size_t i = 0; i < mb.blocks.size(); i++) {
        if (mb.blocks[i].info.compID == 0) lumaBlocks++;
        else if (mb.blocks[i].info.compID == 1) cbBlocks++;
        else if (mb.blocks[i].info.compID == 2) crBlocks++;
    }
    if (lumaBlocks != 21 || cbBlocks != 1 || crBlocks != 1) {
        printf("    [H264-VERIFY] WARN: MB %u block layout Y/Cb/Cr=%d/%d/%d\n",
               mb.info.mbAddrX, lumaBlocks, cbBlocks, crBlocks);
    }
}

#ifdef UT_INTRA_AUTO_CHECK
static void h264_directed_ut_response(struct t_intra_test* tester, const t_intra2ut* req, t_ut2intra* resp)
{
    memset(resp, 0, sizeof(*resp));
    resp->req_info = *req;

    int comp = (int)req->comp;
    int pelX = (int)req->blk_x;
    int pelY = (int)req->blk_y;
    int blkSize = (int)req->blk_size;
    int sizeIdx = (blkSize == 16) ? 2 : (blkSize == 8) ? 1 : 0;

    const H264DirectedBlockRecord* block = h264_directed_find_block(
        g_h264_directed.curMb, pelX / 4, pelY / 4, sizeIdx, comp);

    if (block) {
        size_t count = block->preds.size() < 4 ? 4 : block->preds.size();
        for (size_t i = 0; i < count; i++) {
            resp->pred_info[i].mode = (uint8_t)block->preds[i].modeId;
            resp->pred_info[i].satd = block->preds[i].satd;
        }
        printf("    [H264-UT-DIR] comp=%d blk(%d,%d) size=%d => %zu preds matched\n",
               comp, pelX, pelY, blkSize, block->preds.size());
    } else {
        printf("    [H264-UT-DIR] WARN: no block found for comp=%d blk(%d,%d) size=%d\n",
               comp, pelX, pelY, blkSize);
    }
    (void)tester;
}
#endif