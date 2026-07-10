/*!
 ***************************************************************************
 * \file md_high.c
 *
 * \brief
 *    Main macroblock mode decision functions and helpers
 *
 **************************************************************************
 */

#include <math.h>
#include <limits.h>
#include <float.h>

#include "global.h"
#include "rdopt_coding_state.h"
#include "intrarefresh.h"
#include "image.h"
#include "ratectl.h"
#include "mode_decision.h"
#include "mode_decision_p8x8.h"
#include "fmo.h"
#include "me_umhex.h"
#include "me_umhexsmp.h"
#include "macroblock.h"
#include "md_common.h"
#include "conformance.h"
#include "vlc.h"
#include "rdopt.h"
#include "mv_search.h"
#include "intra_dump.h"

static uint8_t intra_dump_chroma_mode_kind(int mode)
{
  switch (mode)
  {
  case DC_PRED_8:   return MODE_KIND_DC;
  case HOR_PRED_8:  return MODE_KIND_HOR;
  case VERT_PRED_8: return MODE_KIND_VER;
  case PLANE_8:     return MODE_KIND_PLANE;
  default:          return MODE_KIND_DC;
  }
}

static uint64_t intra_dump_compute_satd_chroma(imgpel** org, int orgX, imgpel** pred, int predX, int w, int h)
{
  uint64_t satd = 0;
#if USE_SATD_FOR_DISTORTION
  for (int y = 0; y < h; y += BLOCK_SIZE_8x8)
  {
    for (int x = 0; x < w; x += BLOCK_SIZE_8x8)
    {
      satd += intra_dump_compute_satd_8x8(org + y, orgX + x, pred + y, predX + x);
    }
  }
#else
  for (int y = 0; y < h; y += BLOCK_SIZE)
  {
    for (int x = 0; x < w; x += BLOCK_SIZE)
    {
      satd += intra_dump_compute_satd_4x4(org + y, orgX + x, pred + y, predX + x);
    }
  }
#endif
  return satd;
}

static void intra_dump_fill_chroma_ref(Macroblock *currMB, int uv, imgpel *ref, int *refLen)
{
  VideoParameters *p_Vid = currMB->p_Vid;
  InputParameters *p_Inp = currMB->p_Inp;
  imgpel **img = p_Vid->enc_picture->imgUV[uv];
  PixelPos pix_left, pix_up, pix_up_left;
  int available_left, available_up, available_up_left;
  int w = p_Vid->mb_cr_size_x;
  int h = p_Vid->mb_cr_size_y;
  int idx = 0;

  p_Vid->getNeighbour(currMB, -1, -1, p_Vid->mb_size[IS_CHROMA], &pix_up_left);
  p_Vid->getNeighbour(currMB,  0, -1, p_Vid->mb_size[IS_CHROMA], &pix_up);
  p_Vid->getNeighbour(currMB, -1,  0, p_Vid->mb_size[IS_CHROMA], &pix_left);

  available_left = pix_left.available;
  available_up = pix_up.available;
  available_up_left = pix_up_left.available;

  if (p_Inp->UseConstrainedIntraPred)
  {
    available_left = pix_left.available ? p_Vid->intra_block[pix_left.mb_addr] : 0;
    available_up = pix_up.available ? p_Vid->intra_block[pix_up.mb_addr] : 0;
    available_up_left = pix_up_left.available ? p_Vid->intra_block[pix_up_left.mb_addr] : 0;
  }

  ref[idx++] = available_up_left ? img[pix_up_left.pos_y][pix_up_left.pos_x] : (imgpel)p_Vid->dc_pred_value_comp[uv + 1];
  for (int x = 0; x < w; x++)
    ref[idx++] = available_up ? img[pix_up.pos_y][pix_up.pos_x + x] : (imgpel)p_Vid->dc_pred_value_comp[uv + 1];
  for (int y = 0; y < h; y++)
    ref[idx++] = available_left ? img[pix_left.pos_y + y][pix_left.pos_x] : (imgpel)p_Vid->dc_pred_value_comp[uv + 1];

  *refLen = idx;
}

static void intra_dump_dump_chroma_blocks(Macroblock *currMB, IntraDumper *dumper)
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters *p_Vid = currMB->p_Vid;
  int w = p_Vid->mb_cr_size_x;
  int h = p_Vid->mb_cr_size_y;
  int leftAvailable, upAvailable, allAvailable;

  if (!dumper->enabled || p_Vid->yuv_format == YUV400 || currSlice->P444_joined)
    return;

  if (p_Vid->mb_aff_frame_flag && p_Vid->field_mode)
  {
    PixelPos pix_b;
    PixelPos pix_a[17];
    int *mb_size = p_Vid->mb_size[IS_CHROMA];

    for (int i = 0; i < h + 1; ++i)
      p_Vid->getNeighbour(currMB, -1, i - 1, mb_size, &pix_a[i]);
    p_Vid->getNeighbour(currMB, 0, -1, mb_size, &pix_b);

    if (currMB->p_Inp->UseConstrainedIntraPred == 0)
    {
      upAvailable = pix_b.available;
      leftAvailable = pix_a[1].available;
      allAvailable = pix_a[0].available;
    }
    else
    {
      upAvailable = pix_b.available ? p_Vid->intra_block[pix_b.mb_addr] : 0;
      leftAvailable = 1;
      for (int i = 1; i < h + 1; ++i)
        leftAvailable &= pix_a[i].available ? p_Vid->intra_block[pix_a[i].mb_addr] : 0;
      allAvailable = pix_a[0].available ? p_Vid->intra_block[pix_a[0].mb_addr] : 0;
    }
  }
  else
  {
    PixelPos pix_c;
    PixelPos pix_d;
    PixelPos pix_a;
    int *mb_size = p_Vid->mb_size[IS_CHROMA];

    p_Vid->getNeighbour(currMB, -1, -1, mb_size, &pix_d);
    p_Vid->getNeighbour(currMB, -1, 0, mb_size, &pix_a);
    p_Vid->getNeighbour(currMB, 0, -1, mb_size, &pix_c);

    upAvailable = pix_c.available;
    leftAvailable = pix_a.available;
    allAvailable = pix_d.available;

    if (currMB->p_Inp->UseConstrainedIntraPred)
    {
      upAvailable = pix_c.available ? p_Vid->intra_block[pix_c.mb_addr] : 0;
      leftAvailable = pix_a.available ? p_Vid->intra_block[pix_a.mb_addr] : 0;
      allAvailable = pix_d.available ? p_Vid->intra_block[pix_d.mb_addr] : 0;
    }
  }

  for (int uv = 0; uv < 2; uv++)
  {
    IntraDumpBlockKey blockKey;
    imgpel ref[33];
    int refLen = 0;
    uint64_t bestSatd = 0;
    imgpel ***curr_mpr_16x16 = currSlice->mpr_16x16[uv + 1];

    memset(&blockKey, 0, sizeof(blockKey));
    blockKey.blockUid = intra_dumper_alloc_block_uid(dumper);
    blockKey.parentUid = 0xFFFFFFFF;
    blockKey.mbAddrX = (uint32_t)currMB->mbAddrX;
    blockKey.mbPelX = (uint32_t)currMB->pix_c_x;
    blockKey.mbPelY = (uint32_t)currMB->pix_c_y;
    blockKey.blkPelXInMb = 0;
    blockKey.blkPelYInMb = 0;
    blockKey.width = (uint32_t)w;
    blockKey.height = (uint32_t)h;
    blockKey.compID = (uint8_t)(uv + 1);
    blockKey.mbMode = 2;
    blockKey.intraMode = (uint8_t)currMB->c_ipred_mode;
    blockKey.partIdx = 0;

    intra_dumper_begin_block(dumper, &blockKey);
    intra_dump_fill_chroma_ref(currMB, uv, ref, &refLen);
    intra_dumper_dump_ref_samples(dumper, ref, (uint32_t)refLen, 0);

    for (int mode = DC_PRED_8; mode <= PLANE_8; mode++)
    {
      imgpel **predRows;
      int modeAvailable = (mode == DC_PRED_8)
        || (mode == VERT_PRED_8 && upAvailable)
        || (mode == HOR_PRED_8 && leftAvailable)
        || (mode == PLANE_8 && leftAvailable && upAvailable && allAvailable);

      if (!modeAvailable)
        continue;

      predRows = curr_mpr_16x16[mode];
      uint64_t satd = intra_dump_compute_satd_chroma(
        &p_Vid->pImgOrg[uv + 1][currMB->opix_c_y], currMB->pix_c_x,
        predRows, 0, w, h);
      IntraDumpModeMetric metric;
      intra_dumper_dump_pred_pels(dumper, (uint32_t)mode, intra_dump_chroma_mode_kind(mode),
        predRows, 0, (uint32_t)w, (uint32_t)h, satd);
      metric.modeId = (uint32_t)mode;
      metric.modeKind = intra_dump_chroma_mode_kind(mode);
      metric.mbMode = 2;
      metric.distortionSatd = satd;
      intra_dumper_dump_mode_metric(dumper, &metric);
      if (mode == currMB->c_ipred_mode)
        bestSatd = satd;
    }

    IntraDumpFinalMode final;
    final.modeId = (uint32_t)currMB->c_ipred_mode;
    final.modeKind = intra_dump_chroma_mode_kind(currMB->c_ipred_mode);
    final.mbMode = 2;
    final.distortionSatd = bestSatd;
    intra_dumper_dump_final_mode(dumper, &final);
    intra_dumper_dump_recon_pels(dumper, &p_Vid->enc_picture->imgUV[uv][currMB->pix_c_y], (uint32_t)currMB->pix_c_x, (uint32_t)w, (uint32_t)h);
    intra_dumper_end_block(dumper);
  }
}

static void intra_dump_fill_mb_end(Macroblock *currMB, IntraDumpMbEnd *end)
{
  memset(end, 0, sizeof(*end));
  end->mbType = (uint32_t)currMB->mb_type;
  end->lumaTransformSize8x8Flag = (uint32_t)currMB->luma_transform_size_8x8_flag;

  switch (currMB->mb_type)
  {
  case I4MB:
  case SI4MB:
    end->finalPartWidth = 4;
    end->finalPartHeight = 4;
    break;
  case I8MB:
  case P8x8:
    end->finalPartWidth = 8;
    end->finalPartHeight = 8;
    break;
  case P16x8:
    end->finalPartWidth = 16;
    end->finalPartHeight = 8;
    break;
  case P8x16:
    end->finalPartWidth = 8;
    end->finalPartHeight = 16;
    break;
  default:
    end->finalPartWidth = 16;
    end->finalPartHeight = 16;
    break;
  }
}

/*!
*************************************************************************************
* \brief
*    Mode Decision for a macroblock
*************************************************************************************
*/
void encode_one_macroblock_high (Macroblock *currMB)
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters *p_Vid = currMB->p_Vid;
  InputParameters *p_Inp = currMB->p_Inp;
  PicMotionParams **motion = p_Vid->enc_picture->mv_info;
  RDOPTStructure  *p_RDO = currSlice->p_RDO;

  IntraDumper* dumper = intra_dumper_get_instance();
  IntraDumpMbKey mbKey;
  if (dumper->enabled) {
    mbKey.picIdx = (uint32_t)p_Vid->frame_no;
    mbKey.mbAddrX = (uint32_t)currMB->mbAddrX;
    mbKey.mbPelX = (uint32_t)currMB->pix_x;
    mbKey.mbPelY = (uint32_t)currMB->pix_y;
    mbKey.sliceType = (uint32_t)currSlice->slice_type;
    mbKey.sliceQp = (int8_t)currMB->qp;
    intra_dumper_begin_mb(dumper, &mbKey);
  }

  int         max_index = 9;
  int         block, index, mode, i, j;
  RD_PARAMS   enc_mb;
  distblk     bmcost[5] = {DISTBLK_MAX};
  distblk     cost=0;
  distblk     min_cost = DISTBLK_MAX;
  int         intra1 = 0;
  int         mb_available[3];

  short       bslice      = (short) (currSlice->slice_type == B_SLICE);
  short       pslice      = (short) ((currSlice->slice_type == P_SLICE) || (currSlice->slice_type == SP_SLICE));
  short       intra       = (short) ((currSlice->slice_type == I_SLICE) || (currSlice->slice_type == SI_SLICE) || (pslice && currMB->mb_y == p_Vid->mb_y_upd && p_Vid->mb_y_upd != p_Vid->mb_y_intra));
  int         lambda_mf[3];

  imgpel    **mb_pred  = currSlice->mb_pred[0];
  Block8x8Info *b8x8info = p_Vid->b8x8info;

  char        chroma_pred_mode_range[2];
  short       inter_skip = 0;
  BestMode    md_best;
  Info8x8     best;


  init_md_best(&md_best);

  // Init best (need to create simple function)
  best.pdir = 0;
  best.bipred = 0;
  best.ref[LIST_0] = 0;
  best.ref[LIST_1] = -1;

  intra |= RandomIntra (p_Vid, currMB->mbAddrX);    // Forced Pseudo-Random Intra

  //===== Setup Macroblock encoding parameters =====
  init_enc_mb_params(currMB, &enc_mb, intra);
  if (p_Inp->AdaptiveRounding)
  {
    reset_adaptive_rounding(p_Vid);
  }

  if (currSlice->mb_aff_frame_flag)
  {
    reset_mb_nz_coeff(p_Vid, currMB->mbAddrX);
  }

  //=====   S T O R E   C O D I N G   S T A T E   =====
  //---------------------------------------------------
  currSlice->store_coding_state (currMB, currSlice->p_RDO->cs_cm);

  if (!intra)
  {
    //===== set skip/direct motion vectors =====
    if (enc_mb.valid[0])
    {
      if (bslice)
        currSlice->Get_Direct_Motion_Vectors (currMB);
      else 
        FindSkipModeMotionVector (currMB);
    }
    if (p_Inp->CtxAdptLagrangeMult == 1)
    {
      get_initial_mb16x16_cost(currMB);
    }

    //===== MOTION ESTIMATION FOR 16x16, 16x8, 8x16 BLOCKS =====
    for (mode = 1; mode < 4; mode++)
    {
      best.mode = (char) mode;
      best.bipred = 0;
      b8x8info->best[mode][0].bipred = 0;

      if (enc_mb.valid[mode])
      {
        for (cost=0, block=0; block<(mode==1?1:2); block++)
        {
          update_lambda_costs(currMB, &enc_mb, lambda_mf);
          PartitionMotionSearch (currMB, mode, block, lambda_mf);

          //--- set 4x4 block indices (for getting MV) ---
          j = (block==1 && mode==2 ? 2 : 0);
          i = (block==1 && mode==3 ? 2 : 0);

          //--- get cost and reference frame for List 0 prediction ---
          bmcost[LIST_0] = DISTBLK_MAX;
          list_prediction_cost(currMB, LIST_0, block, mode, &enc_mb, bmcost, best.ref);

          if (bslice)
          {
            //--- get cost and reference frame for List 1 prediction ---
            bmcost[LIST_1] = DISTBLK_MAX;
            list_prediction_cost(currMB, LIST_1, block, mode, &enc_mb, bmcost, best.ref);

            // Compute bipredictive cost between best list 0 and best list 1 references
            list_prediction_cost(currMB, BI_PRED, block, mode, &enc_mb, bmcost, best.ref);

            // currently Bi predictive ME is only supported for modes 1, 2, 3 and ref 0
            if (is_bipred_enabled(p_Vid, mode))
            {
              get_bipred_cost(currMB, mode, block, i, j, &best, &enc_mb, bmcost);
            }
            else
            {
              bmcost[BI_PRED_L0] = DISTBLK_MAX;
              bmcost[BI_PRED_L1] = DISTBLK_MAX;
            }

            // Determine prediction list based on mode cost
            determine_prediction_list(bmcost, &best, &cost);
          }
          else // if (bslice)
          {
            best.pdir = 0;
            cost      += bmcost[LIST_0];
          }

          assign_enc_picture_params(currMB, mode, &best, 2 * block);

          //----- set reference frame and direction parameters -----
          set_block8x8_info(b8x8info, mode, block, &best);

          //--- set reference frames and motion vectors ---
          if (mode>1 && block == 0)
            currSlice->set_ref_and_motion_vectors (currMB, motion, &best, block);
        } // for (block=0; block<(mode==1?1:2); block++)
        if (cost < min_cost)
        {
          md_best.mode = (byte) mode;
          md_best.cost = cost;
          currMB->best_mode = (short) mode;
          min_cost  = cost;
          if (p_Inp->CtxAdptLagrangeMult == 1)
          {
            adjust_mb16x16_cost(currMB, cost);
          }
        }
      } // if (enc_mb.valid[mode])
    } // for (mode=1; mode<4; mode++)

    if (enc_mb.valid[P8x8])
    {    
      currMB->valid_8x8 = FALSE;

      if (p_Inp->Transform8x8Mode)
      {
        ResetRD8x8Data(p_Vid, p_RDO->tr8x8);
        currMB->luma_transform_size_8x8_flag = TRUE; //switch to 8x8 transform size
        //===========================================================
        // Check 8x8 partition with transform size 8x8
        //===========================================================
        //=====  LOOP OVER 8x8 SUB-PARTITIONS  (Motion Estimation & Mode Decision) =====
        for (block = 0; block < 4; block++)
        {
          currSlice->submacroblock_mode_decision(currMB, &enc_mb, p_RDO->tr8x8, p_RDO->cofAC8x8ts[block], block, &cost);
          if(!currMB->valid_8x8)
            break;
          set_subblock8x8_info(b8x8info, P8x8, block, p_RDO->tr8x8);
        } 

      }// if (p_Inp->Transform8x8Mode)

      currMB->valid_4x4 = FALSE;
      if (p_Inp->Transform8x8Mode != 2)
      {
        currMB->luma_transform_size_8x8_flag = FALSE; //switch to 8x8 transform size
        ResetRD8x8Data(p_Vid, p_RDO->tr4x4);
        //=================================================================
        // Check 8x8, 8x4, 4x8 and 4x4 partitions with transform size 4x4
        //=================================================================
        //=====  LOOP OVER 8x8 SUB-PARTITIONS  (Motion Estimation & Mode Decision) =====
        for (block = 0; block < 4; block++)
        {
          currSlice->submacroblock_mode_decision(currMB, &enc_mb, p_RDO->tr4x4, p_RDO->coefAC8x8[block], block, &cost);
          if(!currMB->valid_4x4)
            break;
          set_subblock8x8_info(b8x8info, P8x8, block, p_RDO->tr4x4);
        }
      }// if (p_Inp->Transform8x8Mode != 2)

      if (p_Inp->RCEnable)
        rc_store_diff(currSlice->diffy, &p_Vid->pCurImg[currMB->opix_y], currMB->pix_x, mb_pred);

      p_Vid->giRDOpt_B8OnlyFlag = FALSE;
    }
  }
  else // if (!intra)
  {
    min_cost = DISTBLK_MAX;
  }

  // Set Chroma mode
  set_chroma_pred_mode(currMB, enc_mb, mb_available, chroma_pred_mode_range);

  //========= C H O O S E   B E S T   M A C R O B L O C K   M O D E =========
  //-------------------------------------------------------------------------

  for (currMB->c_ipred_mode = chroma_pred_mode_range[0]; currMB->c_ipred_mode<=chroma_pred_mode_range[1]; currMB->c_ipred_mode++)
  {
    // bypass if c_ipred_mode is not allowed
    if ( (p_Vid->yuv_format != YUV400) &&
      (  ((!intra || !p_Inp->IntraDisableInterOnly) && p_Inp->ChromaIntraDisable == 1 && currMB->c_ipred_mode!=DC_PRED_8) 
      || (currMB->c_ipred_mode == VERT_PRED_8 && !mb_available[0]) 
      || (currMB->c_ipred_mode == HOR_PRED_8  && !mb_available[1]) 
      || (currMB->c_ipred_mode == PLANE_8     && (!mb_available[1] || !mb_available[0] || !mb_available[2]))))
      continue;        

    //===== GET BEST MACROBLOCK MODE =====
    for (index=0; index < max_index; index++)
    {
      mode = mb_mode_table[index];
      //printf("mode %d %7.3f", mode, (double) currMB->min_rdcost);
      if (enc_mb.valid[mode])
      {
        //printf(" mode %d is valid", mode);
        if (p_Vid->yuv_format != YUV400)
        {           
          currMB->i16mode = 0; 
        }

        // Skip intra modes in inter slices if best mode is inter <P8x8 with cbp equal to 0    
        if (currSlice->P444_joined)
        {
          if (p_Inp->SkipIntraInInterSlices && !intra && mode >= I16MB 
            && currMB->best_mode <=3 && currMB->best_cbp == 0 && currSlice->cmp_cbp[1] == 0 && currSlice->cmp_cbp[2] == 0 && (currMB->min_rdcost < weighted_cost(enc_mb.lambda_mdfp,5)))
            continue;
        }
        else
        {
          if (p_Inp->SkipIntraInInterSlices)
          {
            if (!intra && mode >= I4MB)
            {
              if (currMB->best_mode <=3 && currMB->best_cbp == 0 && (currMB->min_rdcost < weighted_cost(enc_mb.lambda_mdfp, 5)))
              {
                continue;
              }
              else if (currMB->best_mode == 0 && (currMB->min_rdcost < weighted_cost(enc_mb.lambda_mdfp,6)))
              {
                continue;
              }
            }
          }
        }

        compute_mode_RD_cost(currMB, &enc_mb, (short) mode, &inter_skip);
        
      }
      //printf(" best %d %7.2f\n", currMB->best_mode, (double) currMB->min_rdcost);
    }// for (index=0; index<max_index; index++)
  }// for (currMB->c_ipred_mode=DC_PRED_8; currMB->c_ipred_mode<=chroma_pred_mode_range[1]; currMB->c_ipred_mode++)                     

  restore_nz_coeff(currMB);

  intra1 = is_intra(currMB);

  //=====  S E T   F I N A L   M A C R O B L O C K   P A R A M E T E R S ======
  //---------------------------------------------------------------------------
  update_qp_cbp_tmp(currMB, p_RDO->cbp);
  currSlice->set_stored_mb_parameters (currMB);

  // Rate control
  if(p_Inp->RCEnable && p_Inp->RCUpdateMode <= MAX_RC_MODE)
    rc_store_mad(currMB);


  //===== Decide if this MB will restrict the reference frames =====
  if (p_Inp->RestrictRef)
    update_refresh_map(currMB, intra, intra1);

  if (dumper->enabled) {
    IntraDumpMbEnd mbEnd;
    intra_dump_fill_mb_end(currMB, &mbEnd);
    intra_dump_dump_chroma_blocks(currMB, dumper);
    intra_dumper_end_mb(dumper, &mbEnd);
  }
}


