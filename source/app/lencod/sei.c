
/*!
 ************************************************************************
 *  \file
 *     sei.c
 *  \brief
 *     implementation of SEI related functions
 *  \author(s)
 *      - Dong Tian                             <tian@cs.tut.fi>
 *      - Athanasios Leontaris                  <aleon@dolby.com>  
 *
 ************************************************************************
 */

#include "global.h"
#include "memalloc.h"
#include "rtp.h"
#include "mbuffer.h"
#include "sei.h"
#include "vlc.h"
#include "header.h"

static void InitRandomAccess           (SEIParameters *p_SEI);
static void CloseRandomAccess          (SEIParameters *p_SEI);
static void InitToneMapping            (SEIParameters *p_SEI, InputParameters *p_Inp);
static void CloseToneMapping           (SEIParameters *p_SEI);
static void CloseBufferingPeriod       (SEIParameters *p_SEI);
static void InitPicTiming              (SEIParameters *p_SEI);
static void ClosePicTiming             (SEIParameters *p_SEI);
static void InitDRPMRepetition         (SEIParameters *p_SEI);
static void CloseDRPMRepetition        (SEIParameters *p_SEI);
static void FinalizeDRPMRepetition     (VideoParameters *p_Vid);
static void InitSparePicture           (SEIParameters *p_SEI);
static void CloseSparePicture          (SEIParameters *p_SEI);
static void FinalizeSpareMBMap         (VideoParameters *p_Vid);
static void InitSubseqChar             (VideoParameters *p_Vid);
static void ClearSubseqCharPayload     (SEIParameters *p_SEI);
static void CloseSubseqChar            (SEIParameters *p_SEI);
static void InitSubseqLayerInfo        (SEIParameters *p_SEI);
static void InitPanScanRectInfo        (SEIParameters *p_SEI);
static void ClosePanScanRectInfo       (SEIParameters *p_SEI);
static void InitUser_data_unregistered (SEIParameters *p_SEI);
static void InitUser_data_registered_itu_t_t35(SEIParameters *p_SEI);
static void ClearUser_data_unregistered(SEIParameters *p_SEI);
static void CloseUser_data_unregistered(SEIParameters *p_SEI);
static void ClearUser_data_registered_itu_t_t35(SEIParameters *p_SEI);
static void CloseUser_data_registered_itu_t_t35(SEIParameters *p_SEI);
static void InitPostFilterHints        (SEIParameters *p_SEI);
static void ClearPostFilterHints       (SEIParameters *p_SEI);
static void ClosePostFilterHints       (SEIParameters *p_SEI);
static void InitFramePackingArrangement(VideoParameters *p_Vid);
static void CloseFramePackingArrangement(SEIParameters *p_SEI);
#if JVET_AK0107_MODALITY_INFORMATION
static void InitModalityInfo(SEIParameters *p_SEI);
static void ClearModalityInfo(SEIParameters *p_SEI);
static void CloseModalityInfo(SEIParameters *p_SEI);
static void FinalizeModalityInfo(SEIParameters *p_SEI);
#endif

#if GFV_ENABLE
static void write_string(char *name, const char *str, Bitstream *bs);
#if JVET_AJ0207_GFV_SEI
static void InitGFV        (SEIParameters *p_SEI, InputParameters *p_Inp);
static void ClearGFV       (SEIParameters *p_SEI);
static void CloseGFV       (SEIParameters *p_SEI);
static void FinalizeGFV    (SEIParameters *p_SEI, unsigned int gfv_idx);

// Additional coordinate arrays from header file
double* base_coordinate_x_rec = NULL;
double* base_coordinate_y_rec = NULL;
double* base_coordinate_z_rec = NULL;
double* prev_coordinate_x_rec = NULL;
double* prev_coordinate_y_rec = NULL;
double* prev_coordinate_z_rec = NULL;

// Additional matrix arrays from header file
unsigned int* base_matrix_width_vec = NULL;
unsigned int* base_matrix_height_vec = NULL;
unsigned int* base_num_matrices_vec = NULL;

// For matrix element
double**** base_matrix_rec = NULL;
double**** prev_matrix_rec = NULL;

// For pred coding
Boolean do_update_gfv_coordinate = TRUE;
Boolean do_update_gfv_matrix = TRUE;

// #define PRINT_GFV_INFO                             // uncomment to print GFV SEI info
#endif

#if JVET_AK0239_GFVE_SEI
static void InitGFVE       (SEIParameters *p_SEI, InputParameters *p_Inp);
static void ClearGFVE      (SEIParameters *p_SEI);
static void CloseGFVE      (SEIParameters *p_SEI);
static void FinalizeGFVE   (SEIParameters *p_SEI, unsigned int gfve_idx);

// Additional matrix arrays from header file
unsigned int* base_gfve_matrix_width_vec = NULL;
unsigned int* base_gfve_matrix_height_vec = NULL;

// For matrix element
double*** base_gfve_matrix_rec = NULL;
double*** prev_gfve_matrix_rec = NULL;

// For pred coding
unsigned int base_gfve_num_matrices = 0;
unsigned int base_gfve_matrix_element_precision_factor = 0;
Boolean do_update_gfve_matrix = TRUE;

// For pupil prediction
double base_gfve_left_pupil_coordinate_x = 0;
double base_gfve_left_pupil_coordinate_y = 0;
double base_gfve_right_pupil_coordinate_x = 0;
double base_gfve_right_pupil_coordinate_y = 0;
double prev_gfve_left_pupil_coordinate_x = 0;
double prev_gfve_left_pupil_coordinate_y = 0;
double prev_gfve_right_pupil_coordinate_x = 0;
double prev_gfve_right_pupil_coordinate_y = 0;

// For pupil pred coding
Boolean check_base_pic_pupil_present_idx = FALSE;
Boolean do_update_gfve_pupil_coordinate = TRUE;

// #define PRINT_GFVE_INFO                             // uncomment to print GFVE SEI info
#endif
#endif

void init_sei(SEIParameters *p_SEI)
{
  p_SEI->seiHasTemporal_reference=FALSE;
  p_SEI->seiHasClock_timestamp=FALSE;
  p_SEI->seiHasPanscan_rect=FALSE;
  p_SEI->seiHasHrd_picture=FALSE;
  p_SEI->seiHasFiller_payload=FALSE;
  p_SEI->seiHasUser_data_registered_itu_t_t35=FALSE;
  p_SEI->seiHasUser_data_unregistered=FALSE;
  p_SEI->seiHasRecoveryPoint_info=FALSE;
  p_SEI->seiHasRef_pic_buffer_management_repetition=FALSE;
  p_SEI->seiHasSpare_picture=FALSE;
  p_SEI->seiHasBufferingPeriod_info=FALSE;
  p_SEI->seiHasPicTiming_info=FALSE;
  p_SEI->seiHasSceneInformation=FALSE;
  p_SEI->seiHasSubseq_information=FALSE;
  p_SEI->seiHasSubseq_layer_characteristics=FALSE;
  p_SEI->seiHasSubseq_characteristics=FALSE;
  p_SEI->seiHasTone_mapping=FALSE;
  p_SEI->seiHasPostFilterHints_info=FALSE;
  p_SEI->seiHasDRPMRepetition_info=FALSE;
  p_SEI->seiHasSparePicture = FALSE;
  p_SEI->seiHasSubseqChar = FALSE;
  p_SEI->seiHasSubseqInfo = FALSE;
  p_SEI->seiHasSubseqLayerInfo = FALSE;
  p_SEI->seiHasPanScanRectInfo = FALSE;
#if JVET_AK0107_MODALITY_INFORMATION
  p_SEI->seiHasModalityInfo = FALSE;
#endif 
#if GFV_ENABLE
#if JVET_AJ0207_GFV_SEI
  p_SEI->seiHasGFV_info = FALSE;
#endif
#if JVET_AK0239_GFVE_SEI
  p_SEI->seiHasGFVE_info = FALSE;
#endif
#endif
}

/*
 ************************************************************************
 *  \basic functions on supplemental enhancement information
 *  \brief
 *     The implementations are based on FCD
 ************************************************************************
 */
void InitSEIMessages(VideoParameters *p_Vid, InputParameters *p_Inp)
{
  SEIParameters *p_SEI = p_Vid->p_SEI;
  int i;
  for (i=0; i<2; i++)
  {
    p_SEI->sei_message[i].data = malloc(MAXRTPPAYLOADLEN);
    if( p_SEI->sei_message[i].data == NULL ) no_mem_exit("InitSEIMessages: sei_message[i].data");
    p_SEI->sei_message[i].subPacketType = SEI_PACKET_TYPE;
    clear_sei_message(p_SEI, i);
  }

  // init sei messages
  p_SEI->seiSparePicturePayload.data = NULL;
  InitSparePicture(p_SEI);  
  if (p_Inp->NumFramesInELSubSeq != 0)
  {
    InitSubseqLayerInfo(p_SEI);
    InitSubseqChar(p_Vid);
  }
  InitSceneInformation(p_SEI);
  // init panscanrect sei message
  InitPanScanRectInfo(p_SEI);
  // init user_data_unregistered
  InitUser_data_unregistered(p_SEI);
  // init user_data_unregistered
  InitUser_data_registered_itu_t_t35(p_SEI);
  // init user_RandomAccess
  InitRandomAccess(p_SEI);
  // Init tone_mapping
  InitToneMapping(p_SEI, p_Inp);
  // init post_filter_hints
  InitPostFilterHints(p_SEI);
  // init BufferingPeriod
  InitBufferingPeriod(p_Vid);
  // init PicTiming
  InitPicTiming(p_SEI);
  // init DRPM Repetition
  InitDRPMRepetition(p_SEI);
  // init Frame Packing Arrangement
  InitFramePackingArrangement(p_Vid);
#if JVET_AK0107_MODALITY_INFORMATION
  // init Modality Information
  InitModalityInfo(p_SEI);
#endif
#if GFV_ENABLE
#if JVET_AJ0207_GFV_SEI
  // init GFV Info
  InitGFV(p_SEI, p_Inp);
#endif
#if JVET_AK0239_GFVE_SEI
  // init GFVE Info
  InitGFVE(p_SEI, p_Inp);
#endif
#endif
}

void CloseSEIMessages(VideoParameters *p_Vid, InputParameters *p_Inp)
{
  SEIParameters *p_SEI = p_Vid->p_SEI;
  int i;

  if (p_Inp->NumFramesInELSubSeq != 0)
    CloseSubseqLayerInfo();

  CloseSubseqChar(p_SEI);
  CloseSparePicture(p_SEI);
  CloseSceneInformation(p_SEI);
  ClosePanScanRectInfo(p_SEI);
  CloseUser_data_unregistered(p_SEI);
  CloseUser_data_registered_itu_t_t35(p_SEI);
  CloseRandomAccess(p_SEI);
  CloseToneMapping(p_SEI);
  ClosePostFilterHints(p_SEI);
  CloseBufferingPeriod(p_SEI);
  ClosePicTiming(p_SEI);
  CloseDRPMRepetition(p_SEI);
  CloseFramePackingArrangement(p_SEI);
#if JVET_AK0107_MODALITY_INFORMATION
  CloseModalityInfo(p_SEI);
#endif

#if GFV_ENABLE
#if JVET_AJ0207_GFV_SEI
  CloseGFV(p_SEI);
#endif
#if JVET_AK0239_GFVE_SEI
  CloseGFVE(p_SEI);
#endif
#endif

  for (i=0; i<MAX_LAYER_NUMBER; i++)
  {
    if ( p_SEI->sei_message[i].data ) 
      free( p_SEI->sei_message[i].data );
    p_SEI->sei_message[i].data = NULL;
  }
}

Boolean HaveAggregationSEI(VideoParameters *p_Vid)
{
  SEIParameters *p_SEI = p_Vid->p_SEI;
  if (p_SEI->sei_message[AGGREGATION_SEI].available && p_Vid->type != B_SLICE)
    return TRUE;
  if (p_SEI->seiHasSubseqInfo)
    return TRUE;
  if (p_SEI->seiHasSubseqLayerInfo && p_Vid->number == 0)
    return TRUE;
  if (p_SEI->seiHasSubseqChar)
    return TRUE;
  if (p_SEI->seiHasSceneInformation)
    return TRUE;
  if (p_SEI->seiHasPanScanRectInfo)
    return TRUE;
  if (p_SEI->seiHasUser_data_unregistered_info)
    return TRUE;
  if (p_SEI->seiHasUser_data_registered_itu_t_t35_info)
    return TRUE;
  if (p_SEI->seiHasRecoveryPoint_info)
    return TRUE;
  if (p_SEI->seiHasTone_mapping)
    return TRUE;
  if (p_SEI->seiHasPostFilterHints_info)
    return TRUE;
  if (p_SEI->seiHasBufferingPeriod_info)
    return TRUE;
  if (p_SEI->seiHasPicTiming_info)
    return TRUE;
  if (p_SEI->seiHasDRPMRepetition_info)
    return TRUE;
#if JVET_AK0107_MODALITY_INFORMATION
  if (p_SEI->seiHasModalityInfo)
    return TRUE;
#endif

#if GFV_ENABLE
#if JVET_AJ0207_GFV_SEI
  if (p_SEI->seiHasGFV_info)
    return TRUE;
#endif
#if JVET_AK0239_GFVE_SEI
  if (p_SEI->seiHasGFVE_info)
    return TRUE;
#endif
#endif

  return FALSE;
//  return p_Inp->SparePictureOption && ( seiHasSpare_picture || seiHasSubseq_information ||
//    seiHasSubseq_layer_characteristics || seiHasSubseq_characteristics );
}

/*!
 ************************************************************************
 *  \brief
 *     write one sei payload to the sei message
 *  \param p_SEI
 *    SEI message
 *  \param id
 *    0, if this is the normal packet\n
 *    1, if this is a aggregation packet
 *  \param payload
 *    a pointer that point to the sei payload. Note that the bitstream
 *    should have be byte aligned already.
 *  \param payload_size
 *    the size of the sei payload
 *  \param payload_type
 *    the type of the sei payload
 *  \par Output
 *    the content of the sei message (sei_message[id]) is updated.
 ************************************************************************
 */
static void write_sei_message(SEIParameters *p_SEI, int id, byte* payload, int payload_size, int payload_type)
{
  int offset, type, size;
  assert(payload_type >= 0 && payload_type < SEI_MAX_ELEMENTS);

  type = payload_type;
  size = payload_size;
  offset = p_SEI->sei_message[id].payloadSize;

  while ( type > 254 )
  {
    p_SEI->sei_message[id].data[offset++] = 0xFF;
    type = type - 255;
  }
  p_SEI->sei_message[id].data[offset++] = (byte) type;

  while ( size > 254 )
  {
    p_SEI->sei_message[id].data[offset++] = 0xFF;
    size = size - 255;
  }
  p_SEI->sei_message[id].data[offset++] = (byte) size;

  memcpy(p_SEI->sei_message[id].data + offset, payload, payload_size);
  offset += payload_size;

  p_SEI->sei_message[id].payloadSize = offset;
}

/*!
 ************************************************************************
 *  \brief
 *     write rbsp_trailing_bits to the sei message
 *  \param p_SEI
 *    SEI message
 *  \param id
 *    0, if this is the normal packet \n
 *    1, if this is a aggregation packet
 *  \par Output
 *    the content of the sei message is updated and ready for packetisation
 ************************************************************************
 */
static void finalize_sei_message(SEIParameters *p_SEI, int id)
{
  int offset = p_SEI->sei_message[id].payloadSize;

  p_SEI->sei_message[id].data[offset] = 0x80;
  p_SEI->sei_message[id].payloadSize++;

  p_SEI->sei_message[id].available = TRUE;
}

/*!
 ************************************************************************
 *  \brief
 *     empty the sei message buffer
 *  \param p_SEI
 *    the SEI message to be cleared
 *  \param id
 *    0, if this is the normal packet \n
 *    1, if this is a aggregation packet
 *  \par Output
 *    the content of the sei message is cleared and ready for storing new
 *      messages
 ************************************************************************
 */
void clear_sei_message(SEIParameters *p_SEI, int id)
{
  memset( p_SEI->sei_message[id].data, 0, MAXRTPPAYLOADLEN);
  p_SEI->sei_message[id].payloadSize       = 0;
  p_SEI->sei_message[id].available         = FALSE;
}

/*!
 ************************************************************************
 *  \brief
 *     copy the bits from one bitstream buffer to another one
 *  \param dest
 *    pointer to the dest bitstream buffer
 *  \param source
 *    pointer to the source bitstream buffer
 *  \par Output
 *    the content of the dest bitstream is changed.
 ************************************************************************
 */
void AppendTmpbits2Buf( Bitstream* dest, Bitstream* source )
{
  int i, j;
  byte mask;
  int bits_in_last_byte;

  // copy the first bytes in source buffer
  for (i=0; i<source->byte_pos; i++)
  {
    mask = 0x80;
    for (j=0; j<8; j++)
    {
      dest->byte_buf <<= 1;
      if (source->streamBuffer[i] & mask)
        dest->byte_buf |= 1;
      dest->bits_to_go--;
      mask >>= 1;
      if (dest->bits_to_go==0)
      {
        dest->bits_to_go = 8;
        dest->streamBuffer[dest->byte_pos++]=dest->byte_buf;
        dest->byte_buf = 0;
      }
    }
  }
  // copy the last byte, there are still (8-source->bits_to_go) bits in the source buffer
  bits_in_last_byte = 8-source->bits_to_go;
  if ( bits_in_last_byte > 0 )
  {
    mask = (byte) (1 << (bits_in_last_byte-1));
    for (j=0; j<bits_in_last_byte; j++)
    {
      dest->byte_buf <<= 1;
      if (source->byte_buf & mask)
        dest->byte_buf |= 1;
      dest->bits_to_go--;
      mask >>= 1;
      if (dest->bits_to_go==0)
      {
        dest->bits_to_go = 8;
        dest->streamBuffer[dest->byte_pos++]=dest->byte_buf;
        dest->byte_buf = 0;
      }
    }
  }
}

/*
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *  \functions on spare pictures
 *  \brief
 *     implementation of Spare Pictures related functions based on
 *      JVT-D100
 *  \author
 *      Dong Tian                 <tian@cs.tut.fi>
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */


/*!
 ************************************************************************
 *  \brief
 *      Init the global variables for spare picture information
 ************************************************************************
 */
static void InitSparePicture(SEIParameters *p_SEI)
{
  if ( p_SEI->seiSparePicturePayload.data != NULL ) 
    CloseSparePicture(p_SEI);

  p_SEI->seiSparePicturePayload.data = malloc( sizeof(Bitstream) );
  if ( p_SEI->seiSparePicturePayload.data == NULL ) no_mem_exit("InitSparePicture: seiSparePicturePayload.data");
  p_SEI->seiSparePicturePayload.data->streamBuffer = malloc(MAXRTPPAYLOADLEN);
  if ( p_SEI->seiSparePicturePayload.data->streamBuffer == NULL ) no_mem_exit("InitSparePicture: seiSparePicturePayload.data->streamBuffer");
  memset( p_SEI->seiSparePicturePayload.data->streamBuffer, 0, MAXRTPPAYLOADLEN);
  p_SEI->seiSparePicturePayload.num_spare_pics = 0;
  p_SEI->seiSparePicturePayload.target_frame_num = 0;

  p_SEI->seiSparePicturePayload.data->bits_to_go  = 8;
  p_SEI->seiSparePicturePayload.data->byte_pos    = 0;
  p_SEI->seiSparePicturePayload.data->byte_buf    = 0;
}

/*!
 ************************************************************************
 *  \brief
 *      Close the global variables for spare picture information
 ************************************************************************
 */
static void CloseSparePicture(SEIParameters *p_SEI)
{
  if (p_SEI->seiSparePicturePayload.data->streamBuffer)
    free(p_SEI->seiSparePicturePayload.data->streamBuffer);
  p_SEI->seiSparePicturePayload.data->streamBuffer = NULL;
  if (p_SEI->seiSparePicturePayload.data)
    free(p_SEI->seiSparePicturePayload.data);
  p_SEI->seiSparePicturePayload.data = NULL;
  p_SEI->seiSparePicturePayload.num_spare_pics = 0;
  p_SEI->seiSparePicturePayload.target_frame_num = 0;
}

/*!
 ************************************************************************
 *  \brief
 *     Calculate the spare picture info, save the result in map_sp
 *      then compose the spare picture information.
 *  \par Output
 *      the spare picture payload is available in *seiSparePicturePayload*
 *      the syntax elements in the loop (see FCD), excluding the two elements
 *      at the beginning.
 ************************************************************************
 */
void CalculateSparePicture()
{
  /*
  int i, j, tmp, i0, j0, m;
  byte **map_sp;
  int delta_spare_frame_num;
  Bitstream *tmpBitstream;

  int num_of_mb=(p_Vid->height >> 4) * (p_Vid->width >> 4);
  int threshold1 = 16*16*p_Inp->SPDetectionThreshold;
  int threshold2 = num_of_mb * p_Inp->SPPercentageThreshold / 100;
  int ref_area_indicator;
  int CandidateSpareFrameNum, SpareFrameNum;
  int possible_spare_pic_num;

  // define it for debug purpose
  #define WRITE_MAP_IMAGE

#ifdef WRITE_MAP_IMAGE
  byte **y;
  int k;
  FILE* fp;
  static int first = 1;
  char map_file_name[255]="map.yuv";
#endif

  // basic check
  if (fb->picbuf_short[0]->used==0 || fb->picbuf_short[1]->used==0)
  {
#ifdef WRITE_MAP_IMAGE
    fp = fopen( map_file_name, "wb" );
    assert( fp != NULL );
    // write the map image
    for (i=0; i < p_Vid->height; i++)
      for (j=0; j < p_Vid->width; j++)
        fputc(0, fp);

    for (k=0; k < 2; k++)
      for (i=0; i < p_Vid->height >> 1; i++)
        for (j=0; j < p_Vid->width >> 1; j++)
          fputc(128, fp);
    fclose( fp );
#endif
    seiHasSparePicture = FALSE;
    return;
  }
  seiHasSparePicture = TRUE;

  // set the global bitstream memory.
  InitSparePicture();
  seiSparePicturePayload.target_frame_num = p_Vid->number % MAX_FN;
  // init the local bitstream memory.
  tmpBitstream = malloc(sizeof(Bitstream));
  if ( tmpBitstream == NULL ) no_mem_exit("CalculateSparePicture: tmpBitstream");
  tmpBitstream->streamBuffer = malloc(MAXRTPPAYLOADLEN);
  if ( tmpBitstream->streamBuffer == NULL ) no_mem_exit("CalculateSparePicture: tmpBitstream->streamBuffer");
  memset( tmpBitstream->streamBuffer, 0, MAXRTPPAYLOADLEN);

#ifdef WRITE_MAP_IMAGE
  if ( first )
  {
    fp = fopen( map_file_name, "wb" );
    first = 0;
  }
  else
    fp = fopen( map_file_name, "ab" );
  get_mem2D(&y, p_Vid->height, p_Vid->width);
#endif
  get_mem2D(&map_sp, p_Vid->height >> 4, p_Vid->width >> 4);

  if (fb->picbuf_short[2]->used!=0) possible_spare_pic_num = 2;
  else possible_spare_pic_num = 1;
  // loop over the spare pictures
  for (m=0; m<possible_spare_pic_num; m++)
  {
    // clear the temporal bitstream buffer
    tmpBitstream->bits_to_go  = 8;
    tmpBitstream->byte_pos    = 0;
    tmpBitstream->byte_buf    = 0;
    memset( tmpBitstream->streamBuffer, 0, MAXRTPPAYLOADLEN);

    // set delta_spare_frame_num
    // the order of the following lines cannot be changed.
    if (m==0)
      CandidateSpareFrameNum = seiSparePicturePayload.target_frame_num - 1; // TargetFrameNum - 1;
    else
      CandidateSpareFrameNum = SpareFrameNum - 1;
    if ( CandidateSpareFrameNum < 0 ) CandidateSpareFrameNum = MAX_FN - 1;
    SpareFrameNum = fb->picbuf_short[m+1]->frame_num_256;
    delta_spare_frame_num = CandidateSpareFrameNum - SpareFrameNum;
    assert( delta_spare_frame_num == 0 );

    // calculate the spare macroblock map of one spare picture
    // the results are stored into map_sp[][]
    for (i=0; i < p_Vid->height >> 4; i++)
      for (j=0; j < p_Vid->width >> 4; j++)
      {
        tmp = 0;
        for (i0=0; i0<16; i0++)
          for (j0=0; j0<16; j0++)
            tmp+=iabs(fb->picbuf_short[m+1]->Refbuf11[(i*16+i0)*p_Vid->width+j*16+j0]-
                       fb->picbuf_short[0]->Refbuf11[(i*16+i0)*p_Vid->width+j*16+j0]);
        tmp = (tmp<=threshold1? 255 : 0);
        map_sp[i][j] = (tmp==0? 1 : 0);
#ifdef WRITE_MAP_IMAGE
//        if (m==0)
        {
        for (i0=0; i0<16; i0++)
          for (j0=0; j0<16; j0++)
            y[i*16+i0][j*16+j0]=tmp;
        }
#endif
      }

    // based on map_sp[][], compose the spare picture information
    // and write the spare picture information to a temp bitstream
    tmp = 0;
    for (i=0; i < p_Vid->height >> 4; i++)
      for (j=0; j < p_Vid->width >> 4; j++)
        if (map_sp[i][j]==0) tmp++;
    if ( tmp > threshold2 )
      ref_area_indicator = 0;
    else if ( !CompressSpareMBMap(p_Vid, map_sp, tmpBitstream) )
      ref_area_indicator = 1;
    else
      ref_area_indicator = 2;

//    printf( "ref_area_indicator = %d\n", ref_area_indicator );

#ifdef WRITE_MAP_IMAGE
    // write the map to a file
//    if (m==0)
    {
      // write the map image
      for (i=0; i < p_Vid->height; i++)
        for (j=0; j < p_Vid->width; j++)
        {
          if ( ref_area_indicator == 0 ) fputc(255, fp);
          else fputc(y[i][j], fp);
        }

      for (k=0; k < 2; k++)
        for (i=0; i < p_Vid->height >> 1; i++)
          for (j=0; j < p_Vid->width >> 1; j++)
            fputc(128, fp);
    }
#endif

    // Finnally, write the current spare picture information to
    // the global variable: seiSparePicturePayload
    ComposeSparePictureMessage(p_SEI, delta_spare_frame_num, ref_area_indicator, tmpBitstream);
    seiSparePicturePayload.num_spare_pics++;
  }  // END for (m=0; m<2; m++)

  free_mem2D( map_sp );
  free( tmpBitstream->streamBuffer );
  free( tmpBitstream );

#ifdef WRITE_MAP_IMAGE
  free_mem2D( y );
  fclose( fp );
#undef WRITE_MAP_IMAGE
#endif
  */
}

/*!
 ************************************************************************
 *  \brief
 *      compose the spare picture information.
 *  \param p_SEI
 *      SEI message 
 *  \param delta_spare_frame_num
 *      see FCD
 *  \param ref_area_indicator
 *      Indicate how to represent the spare mb map
 *  \param tmpBitstream
 *      pointer to a buffer to save the payload
 *  \par Output
 *      bitstream: the composed spare picture payload are
 *        ready to put into the sei_message.
 ************************************************************************
 */
void ComposeSparePictureMessage(SEIParameters *p_SEI, int delta_spare_frame_num, int ref_area_indicator, Bitstream *tmpBitstream)
{
  Bitstream *bitstream = p_SEI->seiSparePicturePayload.data;
  SyntaxElement sym;

  sym.type = SE_HEADER;
  sym.mapping = ue_linfo;

  sym.value1 = delta_spare_frame_num;
  writeSyntaxElement2Buf_UVLC(&sym, bitstream);
  sym.value1 = ref_area_indicator;
  writeSyntaxElement2Buf_UVLC(&sym, bitstream);

  AppendTmpbits2Buf( bitstream, tmpBitstream );
}

/*!
 ************************************************************************
 *  \brief
 *      test if the compressed spare mb map will occupy less mem and
 *      fill the payload buffer.
 *  \param p_Vid
 *      Image parameters for current picture coding
 *  \param map_sp
 *      in which the spare picture information are stored.
 *  \param bitstream
 *      pointer to a buffer to save the payload
 *  \return
 *      TRUE: If it is compressed version, \n
 *             FALSE: If it is not compressed.
 ************************************************************************
 */
Boolean CompressSpareMBMap(VideoParameters *p_Vid, unsigned char **map_sp, Bitstream *bitstream)
{
  int j, k;
  int noc, bit0, bitc;
  SyntaxElement sym;
  int x, y, left, right, bottom, top, directx, directy;

  // this is the size of the uncompressed mb map:
  int size_uncompressed = (p_Vid->height >> 4) * (p_Vid->width >> 4);
  int size_compressed   = 0;
  Boolean ret;

  // initialization
  sym.type = SE_HEADER;
  sym.mapping = ue_linfo;
  noc = 0;
  bit0 = 0;
  bitc = bit0;

  // compress the map, the result goes to the temporal bitstream buffer
  x = ( (p_Vid->width >> 4) - 1 ) / 2;
  y = ( (p_Vid->height >> 4) - 1 ) / 2;
  left = right = x;
  top = bottom = y;
  directx = 0;
  directy = 1;
  for (j=0; j<p_Vid->height >> 4; j++)
    for (k=0; k<p_Vid->width >> 4; k++)
    {
      // check current mb
      if ( map_sp[y][x] == bitc ) noc++;
      else
      {
        sym.value1 = noc;
        size_compressed += writeSyntaxElement2Buf_UVLC(&sym, bitstream);    // the return value indicate the num of bits written
        noc=0;
      }
      // go to the next mb:
      if ( directx == -1 && directy == 0 )
      {
        if (x > left) x--;
        else if (x == 0)
        {
          y = bottom + 1;
          bottom++;
          directx = 1;
          directy = 0;
        }
        else if (x == left)
        {
          x--;
          left--;
          directx = 0;
          directy = 1;
        }
      }
      else if ( directx == 1 && directy == 0 )
      {
        if (x < right) x++;
        else if (x == (p_Vid->width >> 4) - 1)
        {
          y = top - 1;
          top--;
          directx = -1;
          directy = 0;
        }
        else if (x == right)
        {
          x++;
          right++;
          directx = 0;
          directy = -1;
        }
      }
      else if ( directx == 0 && directy == -1 )
      {
        if ( y > top) y--;
        else if (y == 0)
        {
          x = left - 1;
          left--;
          directx = 0;
          directy = 1;
        }
        else if (y == top)
        {
          y--;
          top--;
          directx = -1;
          directy = 0;
        }
      }
      else if ( directx == 0 && directy == 1 )
      {
        if (y < bottom) y++;
        else if (y == (p_Vid->height >> 4) - 1)
        {
          x = right+1;
          right++;
          directx = 0;
          directy = -1;
        }
        else if (y == bottom)
        {
          y++;
          bottom++;
          directx = 1;
          directy = 0;
        }
      }
    }
  if (noc!=0)
  {
    sym.value1 = noc;
    size_compressed += writeSyntaxElement2Buf_UVLC(&sym, bitstream);
  }

  ret = (size_compressed<size_uncompressed? TRUE : FALSE);
  if ( !ret ) // overwrite the streambuffer with the original mb map
  {
    // write the mb map to payload bit by bit
    bitstream->byte_buf = 0;
    bitstream->bits_to_go = 8;
    bitstream->byte_pos = 0;
    for (j=0; j<p_Vid->height >> 4; j++)
    {
      for (k=0; k<p_Vid->width >> 4; k++)
      {
        bitstream->byte_buf <<= 1;
        if (map_sp[j][k]) bitstream->byte_buf |= 1;
        bitstream->bits_to_go--;
        if (bitstream->bits_to_go==0)
        {
          bitstream->bits_to_go = 8;
          bitstream->streamBuffer[bitstream->byte_pos++]=bitstream->byte_buf;
          bitstream->byte_buf = 0;
        }
      }
    }
  }

  return ret;
}

/*!
 ************************************************************************
 *  \brief
 *      Finalize the spare picture SEI payload.
 *        The spare picture paylaod will be ready for encapsulation, and it
 *        should be called before current picture packetized.
 *  \par Input
 *      seiSparePicturePayload.data: points to the payload starting from
 *        delta_spare_frame_num. (See FCD)
 *  \par Output
 *      seiSparePicturePayload.data is updated, pointing to the whole spare
 *        picture information: spare_picture( PayloadSize ) (See FCD)
 *        Make sure it is byte aligned.
 ************************************************************************
 */
static void FinalizeSpareMBMap(VideoParameters *p_Vid)
{
  SEIParameters *p_SEI = p_Vid->p_SEI;
  int CurrFrameNum = p_Vid->number % MAX_FN;
  int delta_frame_num;
  SyntaxElement sym;
  Bitstream *dest, *source;

  sym.type = SE_HEADER;
  sym.mapping = ue_linfo;

  source = p_SEI->seiSparePicturePayload.data;
  dest = malloc(sizeof(Bitstream));
  if ( dest == NULL ) no_mem_exit("FinalizeSpareMBMap: dest");
  dest->streamBuffer = malloc(MAXRTPPAYLOADLEN);
  if ( dest->streamBuffer == NULL ) no_mem_exit("FinalizeSpareMBMap: dest->streamBuffer");
  dest->bits_to_go  = 8;
  dest->byte_pos    = 0;
  dest->byte_buf    = 0;
  memset( dest->streamBuffer, 0, MAXRTPPAYLOADLEN);

  //    delta_frame_num
  delta_frame_num = CurrFrameNum - p_SEI->seiSparePicturePayload.target_frame_num;
  if ( delta_frame_num < 0 ) delta_frame_num += MAX_FN;
  sym.value1 = delta_frame_num;
  writeSyntaxElement2Buf_UVLC(&sym, dest);

  // num_spare_pics_minus1
  sym.value1 = p_SEI->seiSparePicturePayload.num_spare_pics - 1;
  writeSyntaxElement2Buf_UVLC(&sym, dest);

  // copy the other bits
  AppendTmpbits2Buf( dest, source);

  // make sure the payload is byte aligned, stuff bits are 10..0
  if ( dest->bits_to_go != 8 )
  {
    (dest->byte_buf) <<= 1;
    dest->byte_buf |= 1;
    dest->bits_to_go--;
    if ( dest->bits_to_go != 0 ) (dest->byte_buf) <<= (dest->bits_to_go);
    dest->bits_to_go = 8;
    dest->streamBuffer[dest->byte_pos++]=dest->byte_buf;
    dest->byte_buf = 0;
  }
  p_SEI->seiSparePicturePayload.payloadSize = dest->byte_pos;

  // the payload is ready now
  p_SEI->seiSparePicturePayload.data = dest;
  free( source->streamBuffer );
  free( source );
}

/*
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *  \functions on subseq information sei messages
 *  \brief
 *      JVT-D098
 *  \author
 *      Dong Tian                 <tian@cs.tut.fi>
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */


/*!
 ************************************************************************
 *  \brief
 *      init subseqence info
 ************************************************************************
 */
void InitSubseqInfo(SEIParameters *p_SEI, int currLayer)
{
  static uint16 id = 0;

  p_SEI->seiHasSubseqInfo = TRUE;
  p_SEI->seiSubseqInfo[currLayer].subseq_layer_num = currLayer;
  p_SEI->seiSubseqInfo[currLayer].subseq_id = id++;
  p_SEI->seiSubseqInfo[currLayer].last_picture_flag = 0;
  p_SEI->seiSubseqInfo[currLayer].stored_frame_cnt = (unsigned int) -1;
  p_SEI->seiSubseqInfo[currLayer].payloadSize = 0;

  p_SEI->seiSubseqInfo[currLayer].data = malloc( sizeof(Bitstream) );
  if ( p_SEI->seiSubseqInfo[currLayer].data == NULL ) no_mem_exit("InitSubseqInfo: p_SEI->seiSubseqInfo[currLayer].data");
  p_SEI->seiSubseqInfo[currLayer].data->streamBuffer = malloc( MAXRTPPAYLOADLEN );
  if ( p_SEI->seiSubseqInfo[currLayer].data->streamBuffer == NULL ) no_mem_exit("InitSubseqInfo: p_SEI->seiSubseqInfo[currLayer].data->streamBuffer");
  p_SEI->seiSubseqInfo[currLayer].data->bits_to_go  = 8;
  p_SEI->seiSubseqInfo[currLayer].data->byte_pos    = 0;
  p_SEI->seiSubseqInfo[currLayer].data->byte_buf    = 0;
  memset( p_SEI->seiSubseqInfo[currLayer].data->streamBuffer, 0, MAXRTPPAYLOADLEN );
}

/*!
 ************************************************************************
 *  \brief
 *      update subsequence info
 ************************************************************************
 */
void UpdateSubseqInfo(VideoParameters *p_Vid, InputParameters *p_Inp, int currLayer)
{
  SEIParameters *p_SEI = p_Vid->p_SEI;
  if (p_Vid->type != B_SLICE)
  {
    p_SEI->seiSubseqInfo[currLayer].stored_frame_cnt ++;
    p_SEI->seiSubseqInfo[currLayer].stored_frame_cnt = p_SEI->seiSubseqInfo[currLayer].stored_frame_cnt % MAX_FN;
  }

  if ( currLayer == 0 )
  {
    if ( p_Vid->number == p_Inp->no_frames - 1 )
      p_SEI->seiSubseqInfo[currLayer].last_picture_flag = 1;
    else
      p_SEI->seiSubseqInfo[currLayer].last_picture_flag = 0;
  }
  if ( currLayer == 1 )
  {
    if ( (((p_Vid->curr_frm_idx - p_Vid->last_idr_code_order) % (p_Inp->NumFramesInELSubSeq + 1) == 0) && (p_Inp->NumberBFrames != 0) && ((p_Vid->curr_frm_idx - p_Vid->last_idr_code_order) > 0)) || // there are B frames
      (((p_Vid->curr_frm_idx - p_Vid->last_idr_code_order) % (p_Inp->NumFramesInELSubSeq + 1) == p_Inp->NumFramesInELSubSeq) && (p_Inp->NumberBFrames==0))  // there are no B frames
      )
      p_SEI->seiSubseqInfo[currLayer].last_picture_flag = 1;
    else
      p_SEI->seiSubseqInfo[currLayer].last_picture_flag = 0;
  }
}

/*!
 ************************************************************************
 *  \brief
 *      Finalize subseqence info
 ************************************************************************
 */
static void FinalizeSubseqInfo(SEIParameters *p_SEI, int currLayer)
{
  SyntaxElement sym;
  Bitstream *dest = p_SEI->seiSubseqInfo[currLayer].data;

  sym.type = SE_HEADER;
  sym.mapping = ue_linfo;

  sym.value1 = p_SEI->seiSubseqInfo[currLayer].subseq_layer_num;
  writeSyntaxElement2Buf_UVLC(&sym, dest);
  sym.value1 = p_SEI->seiSubseqInfo[currLayer].subseq_id;
  writeSyntaxElement2Buf_UVLC(&sym, dest);
  sym.bitpattern = p_SEI->seiSubseqInfo[currLayer].last_picture_flag;
  sym.len = 1;
  writeSyntaxElement2Buf_Fixed(&sym, dest);
  sym.value1 = p_SEI->seiSubseqInfo[currLayer].stored_frame_cnt;
  writeSyntaxElement2Buf_UVLC(&sym, dest);

  // make sure the payload is byte aligned, stuff bits are 10..0
  if ( dest->bits_to_go != 8 )
  {
    (dest->byte_buf) <<= 1;
    dest->byte_buf |= 1;
    dest->bits_to_go--;
    if ( dest->bits_to_go != 0 ) (dest->byte_buf) <<= (dest->bits_to_go);
    dest->bits_to_go = 8;
    dest->streamBuffer[dest->byte_pos++]=dest->byte_buf;
    dest->byte_buf = 0;
  }
  p_SEI->seiSubseqInfo[currLayer].payloadSize = dest->byte_pos;

//  printf("layer %d, last picture %d, stored_cnt %d\n", currLayer, p_SEI->seiSubseqInfo[currLayer].last_picture_flag, p_SEI->seiSubseqInfo[currLayer].stored_frame_cnt );
}

/*!
 ************************************************************************
 *  \brief
 *      Clear the payload buffer
 ************************************************************************
 */
static void ClearSubseqInfoPayload(SEIParameters *p_SEI, int currLayer)
{
  p_SEI->seiSubseqInfo[currLayer].data->bits_to_go  = 8;
  p_SEI->seiSubseqInfo[currLayer].data->byte_pos    = 0;
  p_SEI->seiSubseqInfo[currLayer].data->byte_buf    = 0;
  memset( p_SEI->seiSubseqInfo[currLayer].data->streamBuffer, 0, MAXRTPPAYLOADLEN );
  p_SEI->seiSubseqInfo[currLayer].payloadSize = 0;
}

/*!
 ************************************************************************
 *  \brief
 *      Close the global variables for spare picture information
 ************************************************************************
 */
void CloseSubseqInfo(SEIParameters *p_SEI, int currLayer)
{
  p_SEI->seiSubseqInfo[currLayer].stored_frame_cnt = (unsigned int) -1;
  p_SEI->seiSubseqInfo[currLayer].payloadSize = 0;

  free( p_SEI->seiSubseqInfo[currLayer].data->streamBuffer );
  free( p_SEI->seiSubseqInfo[currLayer].data );
}

/*
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *  \functions on subseq layer characteristic sei messages
 *  \brief
 *      JVT-D098
 *  \author
 *      Dong Tian                 <tian@cs.tut.fi>
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

/*!
 ************************************************************************
 *  \brief
 *      Init the global variables for spare picture information
 ************************************************************************
 */
static void InitSubseqLayerInfo(SEIParameters *p_SEI)
{
  int i;
  p_SEI->seiHasSubseqLayerInfo = TRUE;
  p_SEI->seiSubseqLayerInfo.layer_number = 0;
  for (i=0; i<MAX_LAYER_NUMBER; i++)
  {
    p_SEI->seiSubseqLayerInfo.bit_rate[i] = 0;
    p_SEI->seiSubseqLayerInfo.frame_rate[i] = 0;
    p_SEI->seiSubseqLayerInfo.layer_number++;
  }
}

/*!
 ************************************************************************
 *  \brief
 *
 ************************************************************************
 */
void CloseSubseqLayerInfo()
{
}

/*!
 ************************************************************************
 *  \brief
 *      Write the data to buffer, which is byte aligned
 ************************************************************************
 */
static void FinalizeSubseqLayerInfo(SEIParameters *p_SEI)
{
  int i, pos;
  pos = 0;
  p_SEI->seiSubseqLayerInfo.payloadSize = 0;
  for (i=0; i<p_SEI->seiSubseqLayerInfo.layer_number; i++)
  {
    *((uint16*)&(p_SEI->seiSubseqLayerInfo.data[pos])) = p_SEI->seiSubseqLayerInfo.bit_rate[i];
    pos += 2;
    *((uint16*)&(p_SEI->seiSubseqLayerInfo.data[pos])) = p_SEI->seiSubseqLayerInfo.frame_rate[i];
    pos += 2;
    p_SEI->seiSubseqLayerInfo.payloadSize += 4;
  }
}

/*
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *  \functions on subseq characteristic sei messages
 *  \brief
 *      JVT-D098
 *  \author
 *      Dong Tian                 <tian@cs.tut.fi>
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

void InitSubseqChar(VideoParameters *p_Vid)
{
  SEIParameters *p_SEI = p_Vid->p_SEI;
  p_SEI->seiSubseqChar.data = malloc( sizeof(Bitstream) );
  if( p_SEI->seiSubseqChar.data == NULL ) no_mem_exit("InitSubseqChar: p_SEI->seiSubseqChar.data");
  p_SEI->seiSubseqChar.data->streamBuffer = malloc(MAXRTPPAYLOADLEN);
  if( p_SEI->seiSubseqChar.data->streamBuffer == NULL ) no_mem_exit("InitSubseqChar: p_SEI->seiSubseqChar.data->streamBuffer");
  ClearSubseqCharPayload(p_SEI);

  p_SEI->seiSubseqChar.subseq_layer_num = p_Vid->layer;
  p_SEI->seiSubseqChar.subseq_id = p_SEI->seiSubseqInfo[p_Vid->layer].subseq_id;
  p_SEI->seiSubseqChar.duration_flag = 0;
  p_SEI->seiSubseqChar.average_rate_flag = 0;
  p_SEI->seiSubseqChar.num_referenced_subseqs = 0;
}

static void ClearSubseqCharPayload(SEIParameters *p_SEI)
{
  memset( p_SEI->seiSubseqChar.data->streamBuffer, 0, MAXRTPPAYLOADLEN);
  p_SEI->seiSubseqChar.data->bits_to_go  = 8;
  p_SEI->seiSubseqChar.data->byte_pos    = 0;
  p_SEI->seiSubseqChar.data->byte_buf    = 0;
  p_SEI->seiSubseqChar.payloadSize       = 0;

  p_SEI->seiHasSubseqChar = FALSE;
}

void UpdateSubseqChar(VideoParameters *p_Vid)
{
  SEIParameters *p_SEI = p_Vid->p_SEI;

  p_SEI->seiSubseqChar.subseq_layer_num = p_Vid->layer;
  p_SEI->seiSubseqChar.subseq_id = p_SEI->seiSubseqInfo[p_Vid->layer].subseq_id;
  p_SEI->seiSubseqChar.duration_flag = 0;
  p_SEI->seiSubseqChar.average_rate_flag = 0;
  p_SEI->seiSubseqChar.average_bit_rate = 100;
  p_SEI->seiSubseqChar.average_frame_rate = 30;
  p_SEI->seiSubseqChar.num_referenced_subseqs = 0;
  p_SEI->seiSubseqChar.ref_subseq_layer_num[0] = 1;
  p_SEI->seiSubseqChar.ref_subseq_id[0] = 2;
  p_SEI->seiSubseqChar.ref_subseq_layer_num[1] = 3;
  p_SEI->seiSubseqChar.ref_subseq_id[1] = 4;

  p_SEI->seiHasSubseqChar = TRUE;
}

static void FinalizeSubseqChar(SEIParameters *p_SEI)
{
  int i;
  SyntaxElement sym;
  Bitstream *dest = p_SEI->seiSubseqChar.data;

  sym.type = SE_HEADER;
  sym.mapping = ue_linfo;

  sym.value1 = p_SEI->seiSubseqChar.subseq_layer_num;
  writeSyntaxElement2Buf_UVLC(&sym, dest);
  sym.value1 = p_SEI->seiSubseqChar.subseq_id;
  writeSyntaxElement2Buf_UVLC(&sym, dest);
  sym.bitpattern = p_SEI->seiSubseqChar.duration_flag;
  sym.len = 1;
  writeSyntaxElement2Buf_Fixed(&sym, dest);
  if ( p_SEI->seiSubseqChar.duration_flag )
  {
    sym.bitpattern = p_SEI->seiSubseqChar.subseq_duration;
    sym.len = 32;
    writeSyntaxElement2Buf_Fixed(&sym, dest);
  }
  sym.bitpattern = p_SEI->seiSubseqChar.average_rate_flag;
  sym.len = 1;
  writeSyntaxElement2Buf_Fixed(&sym, dest);
  if ( p_SEI->seiSubseqChar.average_rate_flag )
  {
    sym.bitpattern = p_SEI->seiSubseqChar.average_bit_rate;
    sym.len = 16;
    writeSyntaxElement2Buf_Fixed(&sym, dest);
    sym.bitpattern = p_SEI->seiSubseqChar.average_frame_rate;
    sym.len = 16;
    writeSyntaxElement2Buf_Fixed(&sym, dest);
  }
  sym.value1 = p_SEI->seiSubseqChar.num_referenced_subseqs;
  writeSyntaxElement2Buf_UVLC(&sym, dest);
  for (i=0; i<p_SEI->seiSubseqChar.num_referenced_subseqs; i++)
  {
    sym.value1 = p_SEI->seiSubseqChar.ref_subseq_layer_num[i];
    writeSyntaxElement2Buf_UVLC(&sym, dest);
    sym.value1 = p_SEI->seiSubseqChar.ref_subseq_id[i];
    writeSyntaxElement2Buf_UVLC(&sym, dest);
  }

  // make sure the payload is byte aligned, stuff bits are 10..0
  if ( dest->bits_to_go != 8 )
  {
    (dest->byte_buf) <<= 1;
    dest->byte_buf |= 1;
    dest->bits_to_go--;
    if ( dest->bits_to_go != 0 ) (dest->byte_buf) <<= (dest->bits_to_go);
    dest->bits_to_go = 8;
    dest->streamBuffer[dest->byte_pos++]=dest->byte_buf;
    dest->byte_buf = 0;
  }
  p_SEI->seiSubseqChar.payloadSize = dest->byte_pos;
}

static void CloseSubseqChar(SEIParameters *p_SEI)
{
  if (p_SEI->seiSubseqChar.data)
  {
    free(p_SEI->seiSubseqChar.data->streamBuffer);
    free(p_SEI->seiSubseqChar.data);
  }
  p_SEI->seiSubseqChar.data = NULL;
}


// JVT-D099
/*
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *  \functions on scene information SEI message
 *  \brief
 *      JVT-D099
 *  \author
 *      Ye-Kui Wang                 <wyk@ieee.org>
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
void InitSceneInformation(SEIParameters *p_SEI)
{
  p_SEI->seiHasSceneInformation = TRUE;

  p_SEI->seiSceneInformation.scene_id = 0;
  p_SEI->seiSceneInformation.scene_transition_type = 0;
  p_SEI->seiSceneInformation.second_scene_id = -1;

  p_SEI->seiSceneInformation.data = malloc( sizeof(Bitstream) );
  if(p_SEI-> seiSceneInformation.data == NULL ) no_mem_exit("InitSceneInformation: seiSceneInformation.data");
  p_SEI->seiSceneInformation.data->streamBuffer = malloc( MAXRTPPAYLOADLEN );
  if( p_SEI->seiSceneInformation.data->streamBuffer == NULL ) no_mem_exit("InitSceneInformation: seiSceneInformation.data->streamBuffer");
  p_SEI->seiSceneInformation.data->bits_to_go  = 8;
  p_SEI->seiSceneInformation.data->byte_pos    = 0;
  p_SEI->seiSceneInformation.data->byte_buf    = 0;
  memset( p_SEI->seiSceneInformation.data->streamBuffer, 0, MAXRTPPAYLOADLEN );
}

void CloseSceneInformation(SEIParameters *p_SEI)
{
  if (p_SEI->seiSceneInformation.data)
  {
    free(p_SEI->seiSceneInformation.data->streamBuffer);
    free(p_SEI->seiSceneInformation.data);
  }
  p_SEI->seiSceneInformation.data = NULL;
}

void FinalizeSceneInformation(SEIParameters *p_SEI)
{
  SyntaxElement sym;
  Bitstream *dest = p_SEI->seiSceneInformation.data;

  sym.type = SE_HEADER;
  sym.mapping = ue_linfo;

  sym.bitpattern = p_SEI->seiSceneInformation.scene_id;
  sym.len = 8;
  writeSyntaxElement2Buf_Fixed(&sym, dest);

  sym.value1 = p_SEI->seiSceneInformation.scene_transition_type;
  writeSyntaxElement2Buf_UVLC(&sym, dest);

  if(p_SEI->seiSceneInformation.scene_transition_type > 3)
  {
    sym.bitpattern = p_SEI->seiSceneInformation.second_scene_id;
    sym.len = 8;
    writeSyntaxElement2Buf_Fixed(&sym, dest);
  }

  // make sure the payload is byte aligned, stuff bits are 10..0
  if ( dest->bits_to_go != 8 )
  {
    (dest->byte_buf) <<= 1;
    dest->byte_buf |= 1;
    dest->bits_to_go--;
    if ( dest->bits_to_go != 0 ) (dest->byte_buf) <<= (dest->bits_to_go);
    dest->bits_to_go = 8;
    dest->streamBuffer[dest->byte_pos++]=dest->byte_buf;
    dest->byte_buf = 0;
  }
  p_SEI->seiSceneInformation.payloadSize = dest->byte_pos;
}

// HasSceneInformation: To include a scene information SEI into the next slice/DP,
//      set HasSceneInformation to be TRUE when calling this function. Otherwise,
//      set HasSceneInformation to be FALSE.
void UpdateSceneInformation(SEIParameters *p_SEI, Boolean HasSceneInformation, int sceneID, int sceneTransType, int secondSceneID)
{
  p_SEI->seiHasSceneInformation = HasSceneInformation;

  assert (sceneID < 256);
  p_SEI->seiSceneInformation.scene_id = sceneID;

  assert (sceneTransType <= 6 );
  p_SEI->seiSceneInformation.scene_transition_type = sceneTransType;

  if(sceneTransType > 3)
  {
    assert (secondSceneID < 256);
    p_SEI->seiSceneInformation.second_scene_id = secondSceneID;
  }
}
// End JVT-D099


/*
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *  \functions on Pan Scan messages
 *  \brief
 *      Based on FCD
 *  \author
 *      Shankar Regunathan                 <tian@cs.tut.fi>
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */


static void InitPanScanRectInfo(SEIParameters *p_SEI)
{
  p_SEI->seiPanScanRectInfo.data = malloc( sizeof(Bitstream) );
  if( p_SEI->seiPanScanRectInfo.data == NULL ) no_mem_exit("InitPanScanRectInfo: p_SEI->seiPanScanRectInfo.data");
  p_SEI->seiPanScanRectInfo.data->streamBuffer = malloc(MAXRTPPAYLOADLEN);
  if( p_SEI->seiPanScanRectInfo.data->streamBuffer == NULL ) no_mem_exit("InitPanScanRectInfo: p_SEI->seiPanScanRectInfo.data->streamBuffer");
  ClearPanScanRectInfoPayload(p_SEI);

  p_SEI->seiPanScanRectInfo.pan_scan_rect_id = 0;
  p_SEI->seiPanScanRectInfo.pan_scan_rect_cancel_flag = 0;
  p_SEI->seiPanScanRectInfo.pan_scan_cnt_minus1 = 0;
  memset( p_SEI->seiPanScanRectInfo.pan_scan_rect_left_offset, 0, 3 * sizeof ( int ) );
  memset( p_SEI->seiPanScanRectInfo.pan_scan_rect_right_offset, 0, 3 * sizeof ( int ) );
  memset( p_SEI->seiPanScanRectInfo.pan_scan_rect_top_offset, 0, 3 * sizeof ( int ) );
  memset( p_SEI->seiPanScanRectInfo.pan_scan_rect_bottom_offset, 0, 3 * sizeof ( int ) );
  p_SEI->seiPanScanRectInfo.pac_scan_rect_repetition_period = 0;
}


void ClearPanScanRectInfoPayload(SEIParameters *p_SEI)
{
  memset( p_SEI->seiPanScanRectInfo.data->streamBuffer, 0, MAXRTPPAYLOADLEN);
  p_SEI->seiPanScanRectInfo.data->bits_to_go  = 8;
  p_SEI->seiPanScanRectInfo.data->byte_pos    = 0;
  p_SEI->seiPanScanRectInfo.data->byte_buf    = 0;
  p_SEI->seiPanScanRectInfo.payloadSize       = 0;

  p_SEI->seiHasPanScanRectInfo = FALSE;
}

void UpdatePanScanRectInfo( VideoParameters *p_Vid )
{
  SEIParameters *p_SEI = p_Vid->p_SEI;

  {
    p_SEI->seiPanScanRectInfo.pan_scan_rect_id = 0;
    p_SEI->seiPanScanRectInfo.pan_scan_rect_cancel_flag = 0;
    p_SEI->seiPanScanRectInfo.pan_scan_cnt_minus1 = 0;
    p_SEI->seiPanScanRectInfo.pan_scan_rect_left_offset[0] = 0;
    p_SEI->seiPanScanRectInfo.pan_scan_rect_right_offset[0] = 160;
    p_SEI->seiPanScanRectInfo.pan_scan_rect_top_offset[0] = 0;
    p_SEI->seiPanScanRectInfo.pan_scan_rect_bottom_offset[0] = 160;
    p_SEI->seiPanScanRectInfo.pac_scan_rect_repetition_period = 1;
  }

  p_SEI->seiHasPanScanRectInfo = TRUE;
}

void FinalizePanScanRectInfo(SEIParameters *p_SEI)
{
  int i;
  Bitstream *bitstream = p_SEI->seiPanScanRectInfo.data;

  write_ue_v( "SEI: pan_scan_rect_id", p_SEI->seiPanScanRectInfo.pan_scan_rect_id, bitstream );
  write_u_1( "SEI: pan_scan_rect_cancel_flag", p_SEI->seiPanScanRectInfo.pan_scan_rect_cancel_flag, bitstream );
  if ( !(p_SEI->seiPanScanRectInfo.pan_scan_rect_cancel_flag) )
  {
    write_ue_v( "SEI: pan_scan_cnt_minus1", p_SEI->seiPanScanRectInfo.pan_scan_cnt_minus1, bitstream );
    for ( i = 0; i <= p_SEI->seiPanScanRectInfo.pan_scan_cnt_minus1; i++ )
    {
      write_se_v( "SEI: pan_scan_rect_left_offset[X]", p_SEI->seiPanScanRectInfo.pan_scan_rect_left_offset[i], bitstream );
      write_se_v( "SEI: pan_scan_rect_right_offset[X]", p_SEI->seiPanScanRectInfo.pan_scan_rect_right_offset[i], bitstream );
      write_se_v( "SEI: pan_scan_rect_top_offset[X]", p_SEI->seiPanScanRectInfo.pan_scan_rect_top_offset[i], bitstream );
      write_se_v( "SEI: pan_scan_rect_bottom_offset[X]", p_SEI->seiPanScanRectInfo.pan_scan_rect_bottom_offset[i], bitstream );
    }
    write_ue_v( "SEI: pac_scan_rect_repetition_period", p_SEI->seiPanScanRectInfo.pac_scan_rect_repetition_period, bitstream );
  }

  // make sure the payload is byte aligned, stuff bits are 10..0
  if ( bitstream->bits_to_go != 8 )
  {
    (bitstream->byte_buf) <<= 1;
    bitstream->byte_buf |= 1;
    bitstream->bits_to_go--;
    if ( bitstream->bits_to_go != 0 )
      (bitstream->byte_buf) <<= (bitstream->bits_to_go);
    bitstream->bits_to_go = 8;
    bitstream->streamBuffer[bitstream->byte_pos++]=bitstream->byte_buf;
    bitstream->byte_buf = 0;
  }
  p_SEI->seiPanScanRectInfo.payloadSize = bitstream->byte_pos;
}


void ClosePanScanRectInfo(SEIParameters *p_SEI)
{
  if (p_SEI->seiPanScanRectInfo.data)
  {
    free(p_SEI->seiPanScanRectInfo.data->streamBuffer);
    free(p_SEI->seiPanScanRectInfo.data);
  }
  p_SEI->seiPanScanRectInfo.data = NULL;
}

/*
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *  \functions on arbitrary (unregistered) data
 *  \brief
 *      Based on FCD
 *  \author
 *      Shankar Regunathan                 <tian@cs.tut.fi>
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
static void InitUser_data_unregistered(SEIParameters *p_SEI)
{

  p_SEI->seiUser_data_unregistered.data = malloc( sizeof(Bitstream) );
  if( p_SEI->seiUser_data_unregistered.data == NULL ) no_mem_exit("InitUser_data_unregistered: p_SEI->seiUser_data_unregistered.data");
  p_SEI->seiUser_data_unregistered.data->streamBuffer = malloc(MAXRTPPAYLOADLEN);
  if( p_SEI->seiUser_data_unregistered.data->streamBuffer == NULL ) no_mem_exit("InitUser_data_unregistered: p_SEI->seiUser_data_unregistered.data->streamBuffer");
  p_SEI->seiUser_data_unregistered.byte = malloc(MAXRTPPAYLOADLEN);
  if( p_SEI->seiUser_data_unregistered.byte == NULL ) no_mem_exit("InitUser_data_unregistered: p_SEI->seiUser_data_unregistered.byte");
  ClearUser_data_unregistered(p_SEI);

}


static void ClearUser_data_unregistered(SEIParameters *p_SEI)
{
  memset( p_SEI->seiUser_data_unregistered.data->streamBuffer, 0, MAXRTPPAYLOADLEN);
  p_SEI->seiUser_data_unregistered.data->bits_to_go  = 8;
  p_SEI->seiUser_data_unregistered.data->byte_pos    = 0;
  p_SEI->seiUser_data_unregistered.data->byte_buf    = 0;
  p_SEI->seiUser_data_unregistered.payloadSize       = 0;

  memset( p_SEI->seiUser_data_unregistered.byte, 0, MAXRTPPAYLOADLEN);
  p_SEI->seiUser_data_unregistered.total_byte = 0;

  p_SEI->seiHasUser_data_unregistered_info = FALSE;
}

void UpdateUser_data_unregistered(SEIParameters *p_SEI)
{
  int i, temp_data;
  int total_byte;


  total_byte = 7;
  for(i = 0; i < total_byte; i++)
  {
    temp_data = i * 4;
    p_SEI->seiUser_data_unregistered.byte[i] = (char) iClip3(0, 255, temp_data);
  }
  p_SEI->seiUser_data_unregistered.total_byte = total_byte;
}

static void FinalizeUser_data_unregistered(SEIParameters *p_SEI)
{
  int i;
  SyntaxElement sym;
  Bitstream *dest = p_SEI->seiUser_data_unregistered.data;

  sym.type = SE_HEADER;
  sym.mapping = ue_linfo;

// #define PRINT_USER_DATA_UNREGISTERED_INFO
  for( i = 0; i < p_SEI->seiUser_data_unregistered.total_byte; i++)
  {
    sym.bitpattern = p_SEI->seiUser_data_unregistered.byte[i];
    sym.len = 8; // b (8)
    writeSyntaxElement2Buf_Fixed(&sym, dest);
#ifdef PRINT_USER_DATA_UNREGISTERED_INFO
    printf("Unreg data payload_byte = %d\n", p_SEI->seiUser_data_unregistered.byte[i]);
#endif
  }
#ifdef PRINT_USER_DATA_UNREGISTERED_INFO
#undef PRINT_USER_DATA_UNREGISTERED_INFO
#endif
  // make sure the payload is byte aligned, stuff bits are 10..0
  if ( dest->bits_to_go != 8 )
  {
    (dest->byte_buf) <<= 1;
    dest->byte_buf |= 1;
    dest->bits_to_go--;
    if ( dest->bits_to_go != 0 ) (dest->byte_buf) <<= (dest->bits_to_go);
    dest->bits_to_go = 8;
    dest->streamBuffer[dest->byte_pos++]=dest->byte_buf;
    dest->byte_buf = 0;
  }
  p_SEI->seiUser_data_unregistered.payloadSize = dest->byte_pos;
}

static void CloseUser_data_unregistered(SEIParameters *p_SEI)
{
  if (p_SEI->seiUser_data_unregistered.data)
  {
    free(p_SEI->seiUser_data_unregistered.data->streamBuffer);
    free(p_SEI->seiUser_data_unregistered.data);
  }
  p_SEI->seiUser_data_unregistered.data = NULL;
  if(p_SEI->seiUser_data_unregistered.byte)
  {
    free(p_SEI->seiUser_data_unregistered.byte);
  }
}


/*
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *  \functions on registered ITU_T_T35 user data
 *  \brief
 *      Based on FCD
 *  \author
 *      Shankar Regunathan                 <tian@cs.tut.fi>
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
void InitUser_data_registered_itu_t_t35(SEIParameters *p_SEI)
{

  p_SEI->seiUser_data_registered_itu_t_t35.data = malloc( sizeof(Bitstream) );
  if( p_SEI->seiUser_data_registered_itu_t_t35.data == NULL ) no_mem_exit("InitUser_data_unregistered: p_SEI->seiUser_data_registered_itu_t_t35.data");
  p_SEI->seiUser_data_registered_itu_t_t35.data->streamBuffer = malloc(MAXRTPPAYLOADLEN);
  if( p_SEI->seiUser_data_registered_itu_t_t35.data->streamBuffer == NULL ) no_mem_exit("InitUser_data_unregistered: p_SEI->seiUser_data_registered_itu_t_t35.data->streamBuffer");
  p_SEI->seiUser_data_registered_itu_t_t35.byte = malloc(MAXRTPPAYLOADLEN);
  if( p_SEI->seiUser_data_registered_itu_t_t35.data == NULL ) no_mem_exit("InitUser_data_unregistered: p_SEI->seiUser_data_registered_itu_t_t35.byte");
  ClearUser_data_registered_itu_t_t35(p_SEI);

}


static void ClearUser_data_registered_itu_t_t35(SEIParameters *p_SEI)
{
  memset( p_SEI->seiUser_data_registered_itu_t_t35.data->streamBuffer, 0, MAXRTPPAYLOADLEN);
  p_SEI->seiUser_data_registered_itu_t_t35.data->bits_to_go  = 8;
  p_SEI->seiUser_data_registered_itu_t_t35.data->byte_pos    = 0;
  p_SEI->seiUser_data_registered_itu_t_t35.data->byte_buf    = 0;
  p_SEI->seiUser_data_registered_itu_t_t35.payloadSize       = 0;

  memset( p_SEI->seiUser_data_registered_itu_t_t35.byte, 0, MAXRTPPAYLOADLEN);
  p_SEI->seiUser_data_registered_itu_t_t35.total_byte = 0;
  p_SEI->seiUser_data_registered_itu_t_t35.itu_t_t35_country_code = 0;
  p_SEI->seiUser_data_registered_itu_t_t35.itu_t_t35_country_code_extension_byte = 0;

  p_SEI->seiHasUser_data_registered_itu_t_t35_info = FALSE;
}

void UpdateUser_data_registered_itu_t_t35(SEIParameters *p_SEI)
{
  int i, temp_data;
  int total_byte;
  int country_code = 82; // Country_code for India

  if(country_code < 0xFF)
  {
    p_SEI->seiUser_data_registered_itu_t_t35.itu_t_t35_country_code = country_code;
  }
  else
  {
    p_SEI->seiUser_data_registered_itu_t_t35.itu_t_t35_country_code = 0xFF;
    p_SEI->seiUser_data_registered_itu_t_t35.itu_t_t35_country_code_extension_byte = country_code - 0xFF;
  }

  total_byte = 7;
  for(i = 0; i < total_byte; i++)
  {
    temp_data = i * 3;
    p_SEI->seiUser_data_registered_itu_t_t35.byte[i] = (char) iClip3(0, 255, temp_data);
  }
  p_SEI->seiUser_data_registered_itu_t_t35.total_byte = total_byte;
}

static void FinalizeUser_data_registered_itu_t_t35(SEIParameters *p_SEI)
{
  int i;
  SyntaxElement sym;
  Bitstream *dest = p_SEI->seiUser_data_registered_itu_t_t35.data;

  sym.type = SE_HEADER;
  sym.mapping = ue_linfo;

  sym.bitpattern = p_SEI->seiUser_data_registered_itu_t_t35.itu_t_t35_country_code;
  sym.len = 8;
  writeSyntaxElement2Buf_Fixed(&sym, dest);

// #define PRINT_USER_DATA_REGISTERED_ITU_T_T35_INFO
#ifdef PRINT_USER_DATA_REGISTERED_ITU_T_T35_INFO
  printf(" ITU_T_T35_COUNTRTY_CODE %d \n", p_SEI->seiUser_data_registered_itu_t_t35.itu_t_t35_country_code);
#endif

  if(p_SEI->seiUser_data_registered_itu_t_t35.itu_t_t35_country_code == 0xFF)
  {
    sym.bitpattern = p_SEI->seiUser_data_registered_itu_t_t35.itu_t_t35_country_code_extension_byte;
    sym.len = 8;
    writeSyntaxElement2Buf_Fixed(&sym, dest);
#ifdef PRINT_USER_DATA_REGISTERED_ITU_T_T35_INFO
    printf(" ITU_T_T35_COUNTRTY_CODE_EXTENSION_BYTE %d \n", p_SEI->seiUser_data_registered_itu_t_t35.itu_t_t35_country_code_extension_byte);
#endif
  }

  for( i = 0; i < p_SEI->seiUser_data_registered_itu_t_t35.total_byte; i++)
  {
    sym.bitpattern = p_SEI->seiUser_data_registered_itu_t_t35.byte[i];
    sym.len = 8; // b (8)
    writeSyntaxElement2Buf_Fixed(&sym, dest);
#ifdef PRINT_USER_DATA_REGISTERED_ITU_T_T35_INFO
    printf("itu_t_t35 payload_byte = %d\n", p_SEI->seiUser_data_registered_itu_t_t35.byte[i]);
#endif
  }
#ifdef PRINT_USER_DATA_REGISTERED_ITU_T_T35_INFO
#undef PRINT_USER_DATA_REGISTERED_ITU_T_T35_INFO
#endif
  // make sure the payload is byte aligned, stuff bits are 10..0
  if ( dest->bits_to_go != 8 )
  {
    (dest->byte_buf) <<= 1;
    dest->byte_buf |= 1;
    dest->bits_to_go--;
    if ( dest->bits_to_go != 0 ) (dest->byte_buf) <<= (dest->bits_to_go);
    dest->bits_to_go = 8;
    dest->streamBuffer[dest->byte_pos++]=dest->byte_buf;
    dest->byte_buf = 0;
  }
  p_SEI->seiUser_data_registered_itu_t_t35.payloadSize = dest->byte_pos;
}

void CloseUser_data_registered_itu_t_t35(SEIParameters *p_SEI)
{
  if (p_SEI->seiUser_data_registered_itu_t_t35.data)
  {
    free(p_SEI->seiUser_data_registered_itu_t_t35.data->streamBuffer);
    free(p_SEI->seiUser_data_registered_itu_t_t35.data);
  }
  p_SEI->seiUser_data_registered_itu_t_t35.data = NULL;
  if(p_SEI->seiUser_data_registered_itu_t_t35.byte)
  {
    free(p_SEI->seiUser_data_registered_itu_t_t35.byte);
  }
}

/*
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *  \functions on random access message
 *  \brief
 *      Based on FCD
 *  \author
 *      Shankar Regunathan
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
static void InitRandomAccess(SEIParameters *p_SEI)
{
  p_SEI->seiRecoveryPoint.data = malloc( sizeof(Bitstream) );
  if( p_SEI->seiRecoveryPoint.data == NULL ) no_mem_exit("InitRandomAccess: seiRandomAccess.data");
  p_SEI->seiRecoveryPoint.data->streamBuffer = malloc(MAXRTPPAYLOADLEN);
  if( p_SEI->seiRecoveryPoint.data->streamBuffer == NULL ) no_mem_exit("InitRandomAccess: seiRandomAccess.data->streamBuffer");
  ClearRandomAccess(p_SEI);
}


void ClearRandomAccess(SEIParameters *p_SEI)
{
  memset( p_SEI->seiRecoveryPoint.data->streamBuffer, 0, MAXRTPPAYLOADLEN);
  p_SEI->seiRecoveryPoint.data->bits_to_go  = 8;
  p_SEI->seiRecoveryPoint.data->byte_pos    = 0;
  p_SEI->seiRecoveryPoint.data->byte_buf    = 0;
  p_SEI->seiRecoveryPoint.payloadSize       = 0;

  p_SEI->seiRecoveryPoint.recovery_frame_cnt = 0;
  p_SEI->seiRecoveryPoint.broken_link_flag = 0;
  p_SEI->seiRecoveryPoint.exact_match_flag = 0;
  p_SEI->seiRecoveryPoint.changing_slice_group_idc = 0;

  p_SEI->seiHasRecoveryPoint_info = FALSE;
}

void UpdateRandomAccess(VideoParameters *p_Vid)
{
  SEIParameters *p_SEI = p_Vid->p_SEI;
  if(p_Vid->type == I_SLICE)
  {
    p_SEI->seiRecoveryPoint.recovery_frame_cnt = 0;
    p_SEI->seiRecoveryPoint.exact_match_flag = 1;
    p_SEI->seiRecoveryPoint.broken_link_flag = 0;
    p_SEI->seiRecoveryPoint.changing_slice_group_idc = 0;
    p_SEI->seiHasRecoveryPoint_info = TRUE;
  }
  else
  {
    p_SEI->seiHasRecoveryPoint_info = FALSE;
  }
}

static void FinalizeRandomAccess(SEIParameters *p_SEI)
{
  Bitstream *bitstream = p_SEI->seiRecoveryPoint.data;

  write_ue_v(   "SEI: recovery_frame_cnt",       p_SEI->seiRecoveryPoint.recovery_frame_cnt,       bitstream);
  write_u_1 (   "SEI: exact_match_flag",         p_SEI->seiRecoveryPoint.exact_match_flag,         bitstream);
  write_u_1 (   "SEI: broken_link_flag",         p_SEI->seiRecoveryPoint.broken_link_flag,         bitstream);
  write_u_v (2, "SEI: changing_slice_group_idc", p_SEI->seiRecoveryPoint.changing_slice_group_idc, bitstream);


// #define PRINT_RECOVERY_POINT
#ifdef PRINT_RECOVERY_POINT
  printf(" recovery_frame_cnt %d \n",       p_SEI->seiRecoveryPoint.recovery_frame_cnt);
  printf(" exact_match_flag %d \n",         p_SEI->seiRecoveryPoint.exact_match_flag);
  printf(" broken_link_flag %d \n",         p_SEI->seiRecoveryPoint.broken_link_flag);
  printf(" changing_slice_group_idc %d \n", p_SEI->seiRecoveryPoint.changing_slice_group_idc);
  printf(" %d %d \n", bitstream->byte_pos, bitstream->bits_to_go);

#undef PRINT_RECOVERY_POINT
#endif
  // make sure the payload is byte aligned, stuff bits are 10..0
  if ( bitstream->bits_to_go != 8 )
  {
    (bitstream->byte_buf) <<= 1;
    bitstream->byte_buf |= 1;
    bitstream->bits_to_go--;
    if ( bitstream->bits_to_go != 0 )
      (bitstream->byte_buf) <<= (bitstream->bits_to_go);
    bitstream->bits_to_go = 8;
    bitstream->streamBuffer[bitstream->byte_pos++]=bitstream->byte_buf;
    bitstream->byte_buf = 0;
  }
  p_SEI->seiRecoveryPoint.payloadSize = bitstream->byte_pos;
}

void CloseRandomAccess(SEIParameters *p_SEI)
{
  if (p_SEI->seiRecoveryPoint.data)
  {
    free(p_SEI->seiRecoveryPoint.data->streamBuffer);
    free(p_SEI->seiRecoveryPoint.data);
  }
  p_SEI->seiRecoveryPoint.data = NULL;
}

/*
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *  \functions on HDR tone-mapping messages
 *  \brief
 *      Based on JVT-T060
 *  \author
 *      Jane Zhao, sharp labs of america
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
static int ParseToneMappingConfigFile(SEIParameters *p_SEI, InputParameters *p_Inp, ToneMappingSEI *pSeiToneMapping)
{
  int i;
  int ret;
  FILE* fp;
  char buf[1024];
  unsigned int tmp;

  printf ("Parsing Tone mapping cfg file %s ..........\n\n", p_Inp->ToneMappingFile);
  if ((fp = fopen(p_Inp->ToneMappingFile, "r")) == NULL) 
  {
    fprintf(stderr, "Tone mapping config file %s is not found, disable tone mapping SEI\n", p_Inp->ToneMappingFile);
    p_SEI->seiHasTone_mapping=FALSE;

    return 1;
  }

  //read the tone mapping config file
  while (fscanf(fp, "%s", buf)!=EOF) 
  {
    ret = 1;
    if (strcmp(buf, "tone_map_id")==0) 
    {
      ret = fscanf(fp, " = %ud\n", &(pSeiToneMapping->tone_map_id));
    }
    else if (strcmp(buf, "tone_map_cancel_flag")==0) 
    {
      ret = fscanf(fp, " = %ud\n", &tmp);
      pSeiToneMapping->tone_map_cancel_flag = (unsigned char) (tmp ? 1 : 0);
    }
    else if (strcmp(buf, "tone_map_repetition_period")==0) 
    {
      ret = fscanf(fp, " = %ud\n", &(pSeiToneMapping->tone_map_repetition_period));
    }
    else if (strcmp(buf, "coded_data_bit_depth")==0) 
    {
      ret = fscanf(fp, " = %ud\n", &tmp);
      pSeiToneMapping->coded_data_bit_depth = (unsigned char) tmp;
    }
    else if (strcmp(buf, "sei_bit_depth")==0) 
    {
      ret = fscanf(fp, " = %ud\n", &tmp);
      pSeiToneMapping->sei_bit_depth =  (unsigned char) tmp;
    }
    else if (strcmp(buf, "model_id")==0) 
    {
      ret = fscanf(fp, " = %ud\n", &(pSeiToneMapping->model_id));
    }
    //else if (model_id ==0) 
    else if (strcmp(buf, "min_value")==0) 
    {
      ret = fscanf(fp, " = %d\n", &(pSeiToneMapping->min_value));
    }
    else if (strcmp(buf, "max_value")==0) 
    {
      ret = fscanf(fp, " = %d\n", &(pSeiToneMapping->max_value));
    }
    //(model_id == 1)
    else if (strcmp(buf, "sigmoid_midpoint")==0) 
    {
      ret = fscanf(fp, " = %d\n", &(pSeiToneMapping->sigmoid_midpoint));
    }
    else if (strcmp(buf, "sigmoid_width")==0) 
    {
      ret = fscanf(fp, " = %d\n", &(pSeiToneMapping->sigmoid_width));
    }
    // (model_id == 2) 
    else if (strcmp(buf, "start_of_coded_interval")==0) 
    {
      int max_output_num = 1<<(pSeiToneMapping->sei_bit_depth);
      ret = fscanf(fp, " = ");
      if (ret!=0)
      {
        error ("ParseToneMappingConfigFile: error parsing tone mapping config file",500);
      }
      for (i=0; i < max_output_num; i++)
      {
        ret = fscanf(fp, "%d\n", &(pSeiToneMapping->start_of_coded_interval[i]));
        if (ret!=1)
        {
          error ("ParseToneMappingConfigFile: error parsing tone mapping config file",500);
        }
      }
    }
    //(model_id == 3)
    else if (strcmp(buf, "num_pivots")==0) 
    {
      ret = fscanf(fp, " = %d\n", &(pSeiToneMapping->num_pivots));
    }

    else if (strcmp(buf, "coded_pivot_value")==0) 
    {
      ret = fscanf(fp, " = ");
      if (ret!=0)
      {
        error ("ParseToneMappingConfigFile: error parsing tone mapping config file",500);
      }
      for (i=0; i < pSeiToneMapping->num_pivots; i++)
      {
        ret = fscanf(fp, "%d\n", &(pSeiToneMapping->coded_pivot_value[i]));
        if (ret!=1)
        {
          error ("ParseToneMappingConfigFile: error parsing tone mapping config file",500);
        }
      }
    }
    else if (strcmp(buf, "sei_pivot_value")==0) 
    {
      ret = fscanf(fp, " = ");
      if (ret!=0)
      {
        error ("ParseToneMappingConfigFile: error parsing tone mapping config file",500);
      }
      for (i=0; i < pSeiToneMapping->num_pivots; i++)
      {
        ret = fscanf(fp, "%d\n", &(pSeiToneMapping->sei_pivot_value[i]));
        if (ret!=1)
        {
          error ("ParseToneMappingConfigFile: error parsing tone mapping config file",500);
        }
      }
    }
    else
    {
      // read till the line end 
      if (NULL == fgets(buf, sizeof(buf), fp))
      {
        error ("ParseToneMappingConfigFile: error parsing tone mapping config file",500);
      }
    }
    if (ret!=1)
    {
      error ("ParseToneMappingConfigFile: error parsing tone mapping config file",500);
    }
  }

  fclose(fp);

  return 0;
}

static void InitToneMapping(SEIParameters *p_SEI, InputParameters *p_Inp) 
{
  if (p_Inp->ToneMappingSEIPresentFlag == 0)
  {
    p_SEI->seiHasTone_mapping = FALSE;
    return;
  }
  else
    p_SEI->seiHasTone_mapping = TRUE;

  p_SEI->seiToneMapping.data = malloc( sizeof(Bitstream) );
  if( p_SEI->seiToneMapping.data == NULL ) no_mem_exit("InitToneMapping: seiToneMapping.data");
  p_SEI->seiToneMapping.data->streamBuffer = malloc(MAXRTPPAYLOADLEN);
  if( p_SEI->seiToneMapping.data->streamBuffer == NULL ) no_mem_exit("InitToneMapping: seiToneMapping.data->streamBuffer");
  memset( p_SEI->seiToneMapping.data->streamBuffer, 0, MAXRTPPAYLOADLEN);
  p_SEI->seiToneMapping.data->bits_to_go  = 8;
  p_SEI->seiToneMapping.data->byte_pos    = 0;
  p_SEI->seiToneMapping.data->byte_buf    = 0;
  p_SEI->seiToneMapping.payloadSize       = 0;

  // read tone mapping config from file
  ParseToneMappingConfigFile(p_SEI, p_Inp, &p_SEI->seiToneMapping);
}

static void FinalizeToneMapping(VideoParameters *p_Vid)
{
  SEIParameters *p_SEI = p_Vid->p_SEI;

  Bitstream *bitstream = p_SEI->seiToneMapping.data;  
  int i;

  write_ue_v("SEI: tone_map_id"               , p_SEI->seiToneMapping.tone_map_id,             bitstream);
  write_u_1("SEI: tone_map_cancel_flag"       , p_SEI->seiToneMapping.tone_map_cancel_flag,    bitstream);

#ifdef PRINT_TONE_MAPPING
  printf("frame %d: Tone-mapping SEI message\n", p_Vid->frame_num);
  printf("tone_map_id = %d\n", p_SEI->seiToneMapping.tone_map_id);
  printf("tone_map_cancel_flag = %d\n", p_SEI->seiToneMapping.tone_map_cancel_flag);
#endif
  if (!p_SEI->seiToneMapping.tone_map_cancel_flag) 
  {
    write_ue_v(  "SEI: tone_map_repetition_period", p_SEI->seiToneMapping.tone_map_repetition_period, bitstream);
    write_u_v (8,"SEI: coded_data_bit_depth"      , p_SEI->seiToneMapping.coded_data_bit_depth,       bitstream);
    write_u_v (8,"SEI: sei_bit_depth"             , p_SEI->seiToneMapping.sei_bit_depth,              bitstream);
    write_ue_v(  "SEI: model_id"                  , p_SEI->seiToneMapping.model_id,                   bitstream);

#ifdef PRINT_TONE_MAPPING
    printf("tone_map_repetition_period = %d\n", p_SEI->seiToneMapping.tone_map_repetition_period);
    printf("coded_data_bit_depth = %d\n", p_SEI->seiToneMapping.coded_data_bit_depth);
    printf("sei_bit_depth = %d\n", p_SEI->seiToneMapping.sei_bit_depth);
    printf("model_id = %d\n", p_SEI->seiToneMapping.model_id);
#endif
    if (p_SEI->seiToneMapping.model_id == 0) 
    { // linear mapping
      write_u_v (32,"SEI: min_value", p_SEI->seiToneMapping.min_value, bitstream);
      write_u_v (32,"SEI: min_value", p_SEI->seiToneMapping.max_value, bitstream);
#ifdef PRINT_TONE_MAPPING
      printf("min_value = %d, max_value = %d\n", p_SEI->seiToneMapping.min_value, p_SEI->seiToneMapping.max_value);
#endif
    }
    else if (p_SEI->seiToneMapping.model_id == 1) 
    { // sigmoidal mapping
      write_u_v (32,"SEI: sigmoid_midpoint", p_SEI->seiToneMapping.sigmoid_midpoint,   bitstream);
      write_u_v (32,"SEI: sigmoid_width", p_SEI->seiToneMapping.sigmoid_width,         bitstream);
#ifdef PRINT_TONE_MAPPING
      printf("sigmoid_midpoint = %d, sigmoid_width = %d\n", p_SEI->seiToneMapping.sigmoid_midpoint, p_SEI->seiToneMapping.sigmoid_width);
#endif
    }
    else if (p_SEI->seiToneMapping.model_id == 2) 
    { // user defined table mapping
      int bit_depth_val = 1 << p_SEI->seiToneMapping.sei_bit_depth;
      for (i=0; i<bit_depth_val; i++) 
      {
        write_u_v((((p_SEI->seiToneMapping.coded_data_bit_depth+7)>>3)<<3), "SEI: start_of_coded_interval", p_SEI->seiToneMapping.start_of_coded_interval[i], bitstream);
#ifdef PRINT_TONE_MAPPING
        //printf("start_of_coded_interval[%d] = %d\n", i, p_SEI->seiToneMapping.start_of_coded_interval[i]);
#endif
      }
    }
    else if (p_SEI->seiToneMapping.model_id == 3) 
    {  // piece-wise linear mapping
      write_u_v (16,"SEI: num_pivots", p_SEI->seiToneMapping.num_pivots, bitstream);
#ifdef PRINT_TONE_MAPPING
      printf("num_pivots = %d\n", p_SEI->seiToneMapping.num_pivots);
#endif
      for (i=0; i < p_SEI->seiToneMapping.num_pivots; i++) 
      {
        write_u_v( (((p_SEI->seiToneMapping.coded_data_bit_depth+7)>>3)<<3), "SEI: coded_pivot_value",  p_SEI->seiToneMapping.coded_pivot_value[i], bitstream);
        write_u_v( (((p_SEI->seiToneMapping.sei_bit_depth+7)>>3)<<3), "SEI: sei_pivot_value",           p_SEI->seiToneMapping.sei_pivot_value[i],   bitstream);
#ifdef PRINT_TONE_MAPPING
        printf("coded_pivot_value[%d] = %d, sei_pivot_value[%d] = %d\n", i, p_SEI->seiToneMapping.coded_pivot_value[i], i, p_SEI->seiToneMapping.sei_pivot_value[i]);
#endif
      }
    }
  } // end !tone_map_cancel_flag

  // make sure the payload is byte aligned, stuff bits are 10..0
  if ( bitstream->bits_to_go != 8 )
  {
    (bitstream->byte_buf) <<= 1;
    bitstream->byte_buf |= 1;
    bitstream->bits_to_go--;
    if ( bitstream->bits_to_go != 0 ) 
      (bitstream->byte_buf) <<= (bitstream->bits_to_go);
    bitstream->bits_to_go = 8;
    bitstream->streamBuffer[bitstream->byte_pos++]=bitstream->byte_buf;
    bitstream->byte_buf = 0;
  }
  p_SEI->seiToneMapping.payloadSize = bitstream->byte_pos;
}


void UpdateToneMapping(SEIParameters *p_SEI) 
{
  // return;

  // you may manually generate some test case here
}

static void ClearToneMapping(SEIParameters *p_SEI) 
{
  memset( p_SEI->seiToneMapping.data->streamBuffer, 0, MAXRTPPAYLOADLEN);
  p_SEI->seiToneMapping.data->bits_to_go  = 8;
  p_SEI->seiToneMapping.data->byte_pos    = 0;
  p_SEI->seiToneMapping.data->byte_buf    = 0;
  p_SEI->seiToneMapping.payloadSize       = 0;

  p_SEI->seiHasTone_mapping=FALSE;
}

static void CloseToneMapping(SEIParameters *p_SEI) 
{

  if (p_SEI->seiToneMapping.data)
  {
    free(p_SEI->seiToneMapping.data->streamBuffer);
    free(p_SEI->seiToneMapping.data);
  }
  p_SEI->seiToneMapping.data = NULL;
  p_SEI->seiHasTone_mapping = FALSE;
}

/*
 ************************************************************************
 *  \functions on post-filter message
 *  \brief
 *      Based on JVT-U035
 *  \author
 *      Steffen Wittmann <steffen.wittmann@eu.panasonic.com>
 ************************************************************************
 */
static void InitPostFilterHints(SEIParameters *p_SEI)
{
  p_SEI->seiPostFilterHints.data = malloc( sizeof(Bitstream) );
  if( p_SEI->seiPostFilterHints.data == NULL ) no_mem_exit("InitPostFilterHints: p_SEI->seiPostFilterHints.data");
  p_SEI->seiPostFilterHints.data->streamBuffer = malloc(MAXRTPPAYLOADLEN);
  if( p_SEI->seiPostFilterHints.data->streamBuffer == NULL ) no_mem_exit("InitPostFilterHints: p_SEI->seiPostFilterHints.data->streamBuffer");
  ClearPostFilterHints(p_SEI);
}

static void ClearPostFilterHints(SEIParameters *p_SEI)
{
  memset( p_SEI->seiPostFilterHints.data->streamBuffer, 0, MAXRTPPAYLOADLEN);
  p_SEI->seiPostFilterHints.data->bits_to_go  = 8;
  p_SEI->seiPostFilterHints.data->byte_pos    = 0;
  p_SEI->seiPostFilterHints.data->byte_buf    = 0;
  p_SEI->seiPostFilterHints.payloadSize       = 0;

  p_SEI->seiPostFilterHints.filter_hint_size_y        = 0;
  p_SEI->seiPostFilterHints.filter_hint_size_x        = 0;
  p_SEI->seiPostFilterHints.filter_hint_type          = 0;
  p_SEI->seiPostFilterHints.additional_extension_flag = 0;
}

void UpdatePostFilterHints(SEIParameters *p_SEI)
{
  unsigned int color_component, cx, cy;
  p_SEI->seiPostFilterHints.filter_hint_type = 0; //define filter_hint_type here
  p_SEI->seiPostFilterHints.filter_hint_size_y = p_SEI->seiPostFilterHints.filter_hint_size_x = 5; //define filter_hint_size here
  get_mem3Dint(&p_SEI->seiPostFilterHints.filter_hint, 3, p_SEI->seiPostFilterHints.filter_hint_size_y, p_SEI->seiPostFilterHints.filter_hint_size_x);

  for (color_component = 0; color_component < 3; color_component ++)
    for (cy = 0; cy < p_SEI->seiPostFilterHints.filter_hint_size_y; cy ++)
      for (cx = 0; cx < p_SEI->seiPostFilterHints.filter_hint_size_x; cx ++)
        p_SEI->seiPostFilterHints.filter_hint[color_component][cy][cx] = 1; //define filter_hint here

  p_SEI->seiPostFilterHints.additional_extension_flag = 0;
}

static void FinalizePostFilterHints(SEIParameters *p_SEI)
{
  Bitstream *bitstream = p_SEI->seiPostFilterHints.data;
  unsigned int color_component, cx, cy;

  write_ue_v(  "SEI: post_filter_hint_size_y", p_SEI->seiPostFilterHints.filter_hint_size_y, bitstream);
  write_ue_v(  "SEI: post_filter_hint_size_x", p_SEI->seiPostFilterHints.filter_hint_size_x, bitstream);
  write_u_v (2,"SEI: post_filter_hint_type",   p_SEI->seiPostFilterHints.filter_hint_type,   bitstream);

  for (color_component = 0; color_component < 3; color_component ++)
    for (cy = 0; cy < p_SEI->seiPostFilterHints.filter_hint_size_y; cy ++)
      for (cx = 0; cx < p_SEI->seiPostFilterHints.filter_hint_size_x; cx ++)
        write_se_v("SEI: post_filter_hints", p_SEI->seiPostFilterHints.filter_hint[color_component][cy][cx], bitstream);

  write_u_1 ("SEI: post_filter_additional_extension_flag", p_SEI->seiPostFilterHints.additional_extension_flag, bitstream);

// #define PRINT_POST_FILTER_HINTS
#ifdef PRINT_POST_FILTER_HINTS
  printf(" post_filter_hint_size_y %d \n", p_SEI->seiPostFilterHints.filter_hint_size_y);
  printf(" post_filter_hint_size_x %d \n", p_SEI->seiPostFilterHints.filter_hint_size_x);
  printf(" post_filter_hint_type %d \n",   p_SEI->seiPostFilterHints.filter_hint_type);
  for (color_component = 0; color_component < 3; color_component ++)
    for (cy = 0; cy < p_SEI->seiPostFilterHints.filter_hint_size_y; cy ++)
      for (cx = 0; cx < p_SEI->seiPostFilterHints.filter_hint_size_x; cx ++)
        printf(" post_filter_hint[%d][%d][%d] %d \n", color_component, cy, cx, filter_hint[color_component][cy][cx]);

  printf(" additional_extension_flag %d \n", p_SEI->seiPostFilterHints.additional_extension_flag);

#undef PRINT_POST_FILTER_HINTS
#endif
  // make sure the payload is byte aligned, stuff bits are 10..0
  if ( bitstream->bits_to_go != 8 )
  {
    (bitstream->byte_buf) <<= 1;
    bitstream->byte_buf |= 1;
    bitstream->bits_to_go--;
    if ( bitstream->bits_to_go != 0 ) 
      (bitstream->byte_buf) <<= (bitstream->bits_to_go);
    bitstream->bits_to_go = 8;
    bitstream->streamBuffer[bitstream->byte_pos++]=bitstream->byte_buf;
    bitstream->byte_buf = 0;
  }
  p_SEI->seiPostFilterHints.payloadSize = bitstream->byte_pos;
}

static void ClosePostFilterHints(SEIParameters *p_SEI)
{
  if (p_SEI->seiPostFilterHints.data)
  {
    free(p_SEI->seiPostFilterHints.data->streamBuffer);
    free(p_SEI->seiPostFilterHints.data);  
    if (p_SEI->seiPostFilterHints.filter_hint)
      free_mem3Dint(p_SEI->seiPostFilterHints.filter_hint);
  }
  p_SEI->seiPostFilterHints.data = NULL;
}

/*
**++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*  \functions to write SEI message into NAL
*  \brief     
**++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
int Write_SEI_NALU(VideoParameters *p_Vid, int len)
{  
  NALU_t *nalu = NULL;
  int RBSPlen = 0;
  byte *rbsp;

  if (HaveAggregationSEI(p_Vid))
  {
    SEIParameters *p_SEI = p_Vid->p_SEI;

    nalu = AllocNALU(MAXNALUSIZE);
    rbsp = p_SEI->sei_message[AGGREGATION_SEI].data;
    RBSPlen = p_SEI->sei_message[AGGREGATION_SEI].payloadSize;
    RBSPtoNALU (rbsp, nalu, RBSPlen, NALU_TYPE_SEI, NALU_PRIORITY_DISPOSABLE, 1);
    nalu->startcodeprefix_len = 4;

    len += p_Vid->WriteNALU (p_Vid, nalu, p_Vid->f_out);
    FreeNALU (nalu);
  }  

  return len;
}

/*
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *  \functions on buffering period SEI message
 *  \brief
 *      Based on final Recommendation
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
void InitBufferingPeriod(VideoParameters *p_Vid)
{
  SEIParameters *p_SEI = p_Vid->p_SEI;
  p_SEI->seiBufferingPeriod.data = malloc( sizeof(Bitstream) );
  if( p_SEI->seiBufferingPeriod.data == NULL ) 
    no_mem_exit("InitBufferingPeriod: seiBufferingPeriod.data");

  p_SEI->seiBufferingPeriod.data->streamBuffer = malloc(MAXRTPPAYLOADLEN);
  if( p_SEI->seiBufferingPeriod.data->streamBuffer == NULL ) 
    no_mem_exit("InitBufferingPeriod: seiBufferingPeriod.data->streamBuffer");

  ClearBufferingPeriod(p_SEI, p_Vid->active_sps);
}

void ClearBufferingPeriod(SEIParameters *p_SEI, seq_parameter_set_rbsp_t *active_sps)
{
  unsigned int SchedSelIdx;
  memset( p_SEI->seiBufferingPeriod.data->streamBuffer, 0, MAXRTPPAYLOADLEN);

  p_SEI->seiBufferingPeriod.data->bits_to_go  = 8;
  p_SEI->seiBufferingPeriod.data->byte_pos    = 0;
  p_SEI->seiBufferingPeriod.data->byte_buf    = 0;
  p_SEI->seiBufferingPeriod.payloadSize       = 0;

  p_SEI->seiBufferingPeriod.seq_parameter_set_id = active_sps->seq_parameter_set_id;
  if ( active_sps->vui_seq_parameters.nal_hrd_parameters_present_flag )
  {
    for ( SchedSelIdx = 0; SchedSelIdx <= active_sps->vui_seq_parameters.nal_hrd_parameters.cpb_cnt_minus1; SchedSelIdx++ )
    {
      p_SEI->seiBufferingPeriod.nal_initial_cpb_removal_delay[SchedSelIdx] = 0;
      p_SEI->seiBufferingPeriod.nal_initial_cpb_removal_delay_offset[SchedSelIdx] = 0;
    }
  }
  if ( active_sps->vui_seq_parameters.vcl_hrd_parameters_present_flag )
  {
    for ( SchedSelIdx = 0; SchedSelIdx <= active_sps->vui_seq_parameters.vcl_hrd_parameters.cpb_cnt_minus1; SchedSelIdx++ )
    {
      p_SEI->seiBufferingPeriod.vcl_initial_cpb_removal_delay[SchedSelIdx] = 0;
      p_SEI->seiBufferingPeriod.vcl_initial_cpb_removal_delay_offset[SchedSelIdx] = 0;
    }
  }

  p_SEI->seiHasBufferingPeriod_info = FALSE;
}

void UpdateBufferingPeriod(VideoParameters *p_Vid, InputParameters *p_Inp)
{
  SEIParameters *p_SEI = p_Vid->p_SEI;
  p_SEI->seiHasBufferingPeriod_info = FALSE;
}

static void FinalizeBufferingPeriod(SEIParameters *p_SEI, seq_parameter_set_rbsp_t *active_sps)
{
  unsigned int SchedSelIdx;
  Bitstream *bitstream = p_SEI->seiBufferingPeriod.data;

  write_ue_v(   "SEI: seq_parameter_set_id",     p_SEI->seiBufferingPeriod.seq_parameter_set_id,   bitstream);
  if ( active_sps->vui_seq_parameters.nal_hrd_parameters_present_flag )
  {
    for ( SchedSelIdx = 0; SchedSelIdx <= active_sps->vui_seq_parameters.nal_hrd_parameters.cpb_cnt_minus1; SchedSelIdx++ )
    {
      write_u_v( active_sps->vui_seq_parameters.nal_hrd_parameters.initial_cpb_removal_delay_length_minus1 + 1,
        "SEI: initial_cpb_removal_delay",     p_SEI->seiBufferingPeriod.nal_initial_cpb_removal_delay[SchedSelIdx],   bitstream);
      write_u_v( active_sps->vui_seq_parameters.nal_hrd_parameters.initial_cpb_removal_delay_length_minus1 + 1,
        "SEI: initial_cpb_removal_delay_offset",     p_SEI->seiBufferingPeriod.nal_initial_cpb_removal_delay_offset[SchedSelIdx],   bitstream);
    }
  }
  if ( active_sps->vui_seq_parameters.vcl_hrd_parameters_present_flag )
  {
    for ( SchedSelIdx = 0; SchedSelIdx <= active_sps->vui_seq_parameters.vcl_hrd_parameters.cpb_cnt_minus1; SchedSelIdx++ )
    {
      write_u_v( active_sps->vui_seq_parameters.vcl_hrd_parameters.initial_cpb_removal_delay_length_minus1 + 1,
        "SEI: initial_cpb_removal_delay",     p_SEI->seiBufferingPeriod.vcl_initial_cpb_removal_delay[SchedSelIdx],   bitstream);
      write_u_v( active_sps->vui_seq_parameters.vcl_hrd_parameters.initial_cpb_removal_delay_length_minus1 + 1,
        "SEI: initial_cpb_removal_delay_offset",     p_SEI->seiBufferingPeriod.vcl_initial_cpb_removal_delay_offset[SchedSelIdx],   bitstream);
    }
  }

  // make sure the payload is byte aligned, stuff bits are 10..0
  if ( bitstream->bits_to_go != 8 )
  {
    (bitstream->byte_buf) <<= 1;
    bitstream->byte_buf |= 1;
    bitstream->bits_to_go--;
    if ( bitstream->bits_to_go != 0 )
      (bitstream->byte_buf) <<= (bitstream->bits_to_go);
    bitstream->bits_to_go = 8;
    bitstream->streamBuffer[bitstream->byte_pos++]=bitstream->byte_buf;
    bitstream->byte_buf = 0;
  }
  p_SEI->seiBufferingPeriod.payloadSize = bitstream->byte_pos;
}

static void CloseBufferingPeriod(SEIParameters *p_SEI)
{
  if (p_SEI->seiBufferingPeriod.data)
  {
    free(p_SEI->seiBufferingPeriod.data->streamBuffer);
    free(p_SEI->seiBufferingPeriod.data);
  }
  p_SEI->seiBufferingPeriod.data = NULL;
}

/*
 ************************************************************************
 * \brief
 *    Initialize Picture Timing SEI data 
 ************************************************************************
 */
void InitPicTiming(SEIParameters *p_SEI)
{
  p_SEI->seiPicTiming.data = malloc( sizeof(Bitstream) );
  if( p_SEI->seiPicTiming.data == NULL ) 
    no_mem_exit("InitPicTiming: seiPicTiming.data");

  p_SEI->seiPicTiming.data->streamBuffer = malloc(MAXRTPPAYLOADLEN);
  if( p_SEI->seiPicTiming.data->streamBuffer == NULL ) 
    no_mem_exit("InitPicTiming: seiPicTiming.data->streamBuffer");

  ClearPicTiming(p_SEI);
}

/*
 ************************************************************************
 * \brief
 *    Clear Picture Timing SEI data
 ************************************************************************
 */
void ClearPicTiming(SEIParameters *p_SEI)
{
  memset( p_SEI->seiPicTiming.data->streamBuffer, 0, MAXRTPPAYLOADLEN);

  p_SEI->seiPicTiming.data->bits_to_go  = 8;
  p_SEI->seiPicTiming.data->byte_pos    = 0;
  p_SEI->seiPicTiming.data->byte_buf    = 0;
  p_SEI->seiPicTiming.payloadSize       = 0;

  // initialization
  p_SEI->seiPicTiming.cpb_removal_delay = 0;
  p_SEI->seiPicTiming.dpb_output_delay = 0;
  p_SEI->seiPicTiming.pic_struct = 0;
  memset(p_SEI->seiPicTiming.clock_timestamp_flag, 0, MAX_PIC_STRUCT_VALUE * sizeof(Boolean) ); // 0 == FALSE
  p_SEI->seiPicTiming.ct_type = 0;
  p_SEI->seiPicTiming.nuit_field_based_flag = FALSE;
  p_SEI->seiPicTiming.counting_type = 0;
  p_SEI->seiPicTiming.full_timestamp_flag = FALSE;
  p_SEI->seiPicTiming.discontinuity_flag = FALSE;
  p_SEI->seiPicTiming.cnt_dropped_flag = FALSE;
  p_SEI->seiPicTiming.n_frames = 0;
  p_SEI->seiPicTiming.seconds_value = 0;
  p_SEI->seiPicTiming.minutes_value = 0;
  p_SEI->seiPicTiming.hours_value = 0;
  p_SEI->seiPicTiming.seconds_flag = FALSE;
  p_SEI->seiPicTiming.minutes_flag = FALSE;
  p_SEI->seiPicTiming.hours_flag = FALSE;
  p_SEI->seiPicTiming.time_offset = 0;  

  p_SEI->seiHasPicTiming_info = FALSE;
}

/*
 ************************************************************************
 * \brief
 *    Derive pic_struct flag (Table D-1)
 ************************************************************************
 */
static int get_pic_struct( VideoParameters *p_Vid, int frame_no, int field_pic_flag, int top )
{
  InputParameters *p_Inp = p_Vid->p_Inp;

  if ( field_pic_flag )
  {
    if ( top )
    {
      return 1;
    }
    else
    {
      return 2;
    }
  }
  else
  {
    if ( !p_Inp->SEIVUI32Pulldown )
    {
      return 0;
    }
    else
    {
      int order = (frame_no % 4);
      int pic_struct_a[4] = {0,5,6,0};
      int pic_struct_b[4] = {0,0,7,0};
      int pic_struct_c[4] = {3,5,6,3};
      int pic_struct_d[4] = {0,5,4,6};
      int pic_struct_e[4] = {3,5,4,6};

      switch( p_Inp->SEIVUI32Pulldown )
      {
      default:
      case 1:
        return pic_struct_a[order];
        break;
      case 2:
        return pic_struct_b[order];
        break;
      case 3:
        return pic_struct_c[order];
        break;
      case 4:
        return pic_struct_d[order];
        break;
      case 5:
        return pic_struct_e[order];
        break;
      }
    }
  }
}


/*
 ************************************************************************
 * \brief
 *    Update Picture Timing SEI data
 ************************************************************************
 */
void UpdatePicTiming(VideoParameters *p_Vid, InputParameters *p_Inp)
{
  SEIParameters *p_SEI = p_Vid->p_SEI;
  seq_parameter_set_rbsp_t *active_sps = p_Vid->active_sps;

  // initialize the values
  if ( p_Inp->SEIVUI32Pulldown )
  {
    int fld_flag = p_Vid->fld_flag;
    // pic_struct
    p_Vid->pic_struct = get_pic_struct( p_Vid, p_Vid->frm_no_in_file, fld_flag, (p_Vid->structure == TOP_FIELD) ? 1 : 0 );

    if ( active_sps->vui_seq_parameters.pic_struct_present_flag )
    {
      int NumClockTS = 0, i;
      p_SEI->seiPicTiming.pic_struct = p_Vid->pic_struct;
      // interpret pic_struct
      switch( p_SEI->seiPicTiming.pic_struct )
      {
      case 0:
      default:
        // frame
        assert( fld_flag == FALSE );
        NumClockTS = 1;
        break;
      case 1:
        // top field
        assert( (p_Vid->fld_flag == TRUE) && (p_Vid->structure != BOTTOM_FIELD) );
        NumClockTS = 1;
        break;
      case 2:
        // bottom field
        assert( (p_Vid->fld_flag == TRUE) && (p_Vid->structure == BOTTOM_FIELD) );
        NumClockTS = 1;
        break;
      case 3:
        // top field, bottom field, in that order
      case 4:
        // bottom field, top field, in that order
        assert( p_Vid->fld_flag == FALSE );
        NumClockTS = 2;
        break;
      case 5:
        // top field, bottom field, top field repeated, in that order
      case 6:
        // bottom field, top field, bottom field repeated, in that order
        assert( p_Vid->fld_flag == FALSE );
        NumClockTS = 3;
      case 7:
        // frame doubling
        assert( (p_Vid->fld_flag == FALSE) && active_sps->vui_seq_parameters.fixed_frame_rate_flag == 1 );
        NumClockTS = 2;
        break;
      case 8:
        // frame tripling
        assert( (p_Vid->fld_flag == FALSE) && active_sps->vui_seq_parameters.fixed_frame_rate_flag == 1 );
        NumClockTS = 3;
        break;
      }
      for ( i = 0; i < NumClockTS; i++ )
      {
        p_SEI->seiPicTiming.clock_timestamp_flag[i] = FALSE;
        if ( p_SEI->seiPicTiming.clock_timestamp_flag[i] )
        {
          int seconds = 0;
          int minutes = 0;
          int hours = 0;
          int time_offset_length;
          assert( seconds >= 0 && seconds <= 59 );
          assert( minutes >= 0 && minutes <= 59 );
          assert( hours >= 0 && hours <= 23 );

          p_SEI->seiPicTiming.ct_type               = 0;
          p_SEI->seiPicTiming.nuit_field_based_flag = FALSE;
          p_SEI->seiPicTiming.counting_type         = 0;
          p_SEI->seiPicTiming.full_timestamp_flag   = FALSE;
          p_SEI->seiPicTiming.discontinuity_flag    = FALSE;
          p_SEI->seiPicTiming.cnt_dropped_flag      = FALSE;
          p_SEI->seiPicTiming.n_frames              = 0;

          if ( p_SEI->seiPicTiming.full_timestamp_flag )
          {      
            p_SEI->seiPicTiming.seconds_value = seconds;
            p_SEI->seiPicTiming.minutes_value = minutes;
            p_SEI->seiPicTiming.hours_value   = hours;
          }
          else
          {            
            p_SEI->seiPicTiming.seconds_flag = FALSE;
            if (p_SEI->seiPicTiming.seconds_flag)
            {
              p_SEI->seiPicTiming.seconds_value = seconds;
              p_SEI->seiPicTiming.minutes_flag = FALSE;
              if (p_SEI->seiPicTiming.minutes_flag)
              {
                p_SEI->seiPicTiming.minutes_value = minutes;
                p_SEI->seiPicTiming.hours_flag = FALSE;
                if (p_SEI->seiPicTiming.hours_flag)
                {
                  p_SEI->seiPicTiming.hours_value   = hours;
                }
              }
            }
          }
          if (active_sps->vui_seq_parameters.vcl_hrd_parameters_present_flag)
            time_offset_length = active_sps->vui_seq_parameters.vcl_hrd_parameters.time_offset_length;
          else if (active_sps->vui_seq_parameters.nal_hrd_parameters_present_flag)
            time_offset_length = active_sps->vui_seq_parameters.nal_hrd_parameters.time_offset_length;
          else
            time_offset_length = 24;
          if ( time_offset_length )
          {
            p_SEI->seiPicTiming.time_offset = 0;
          }
        }
      }
    }
    p_SEI->seiHasPicTiming_info = TRUE;
  }
  else
  {
    p_SEI->seiHasPicTiming_info = FALSE;
  }
}

/*
 ************************************************************************
 * \brief
 *    Finalize Picture Timing SEI data
 ************************************************************************
 */
static void FinalizePicTiming(VideoParameters *p_Vid)
{
  SEIParameters *p_SEI = p_Vid->p_SEI;
  seq_parameter_set_rbsp_t *active_sps = p_Vid->active_sps;

  Bitstream *bitstream = p_SEI->seiPicTiming.data;
  // CpbDpbDelaysPresentFlag can also be set "by some means not specified in this Recommendation | International Standard"
  Boolean CpbDpbDelaysPresentFlag =  (Boolean) (active_sps->vui_parameters_present_flag
                              && (   (active_sps->vui_seq_parameters.nal_hrd_parameters_present_flag != 0)
                                   ||(active_sps->vui_seq_parameters.vcl_hrd_parameters_present_flag != 0)));
  hrd_parameters_t *hrd = NULL;

  if (active_sps->vui_seq_parameters.vcl_hrd_parameters_present_flag)
  {
    hrd = &(active_sps->vui_seq_parameters.vcl_hrd_parameters);
  }
  else if (active_sps->vui_seq_parameters.nal_hrd_parameters_present_flag)
  {
    hrd = &(active_sps->vui_seq_parameters.nal_hrd_parameters);
  }
  else
  {
    hrd = NULL;
  }

  if ( CpbDpbDelaysPresentFlag )
  {
    write_u_v( hrd->cpb_removal_delay_length_minus1 + 1, "SEI: cpb_removal_delay", p_SEI->seiPicTiming.cpb_removal_delay, bitstream);
    write_u_v( hrd->dpb_output_delay_length_minus1  + 1, "SEI: dpb_output_delay",  p_SEI->seiPicTiming.dpb_output_delay,  bitstream);
  }
  if ( active_sps->vui_seq_parameters.pic_struct_present_flag )
  {
    int NumClockTS = 0, i;

    write_u_v( 4, "SEI: pic_struct", p_SEI->seiPicTiming.pic_struct, bitstream);
    // interpret pic_struct
    switch( p_SEI->seiPicTiming.pic_struct )
    {
    case 0:
    default:
      // frame
      assert( p_Vid->fld_flag == FALSE );
      NumClockTS = 1;
      break;
    case 1:
      // top field
      assert( (p_Vid->fld_flag == TRUE) && (p_Vid->structure != BOTTOM_FIELD) );
      NumClockTS = 1;
      break;
    case 2:
      // bottom field
      assert( (p_Vid->fld_flag == TRUE) && (p_Vid->structure == BOTTOM_FIELD) );
      NumClockTS = 1;
      break;
    case 3:
      // top field, bottom field, in that order
    case 4:
      // bottom field, top field, in that order
      assert( p_Vid->fld_flag == FALSE );
      NumClockTS = 2;
      break;
    case 5:
      // top field, bottom field, top field repeated, in that order
    case 6:
      // bottom field, top field, bottom field repeated, in that order
      assert( p_Vid->fld_flag == FALSE );
      NumClockTS = 3;
      break;
    case 7:
      // frame doubling
      assert( (p_Vid->fld_flag == FALSE) && active_sps->vui_seq_parameters.fixed_frame_rate_flag == 1 );
      NumClockTS = 2;
      break;
    case 8:
      // frame tripling
      assert( (p_Vid->fld_flag == FALSE) && active_sps->vui_seq_parameters.fixed_frame_rate_flag == 1 );
      NumClockTS = 3;
      break;
    }
    for ( i = 0; i < NumClockTS; i++ )
    {
      write_u_1( "SEI: clock_timestamp_flag", p_SEI->seiPicTiming.clock_timestamp_flag[i], bitstream);
      if ( p_SEI->seiPicTiming.clock_timestamp_flag[i] )
      {
        write_u_v( 2, "SEI: ct_type", p_SEI->seiPicTiming.ct_type, bitstream);
        write_u_1( "SEI: nuit_field_based_flag", p_SEI->seiPicTiming.nuit_field_based_flag, bitstream);
        write_u_v( 5, "SEI: counting_type", p_SEI->seiPicTiming.counting_type, bitstream);
        write_u_1( "SEI: full_timestamp_flag", p_SEI->seiPicTiming.full_timestamp_flag, bitstream);
        write_u_1( "SEI: discontinuity_flag", p_SEI->seiPicTiming.discontinuity_flag, bitstream);
        write_u_1( "SEI: cnt_dropped_flag", p_SEI->seiPicTiming.cnt_dropped_flag, bitstream);
        write_u_v( 8, "SEI: n_frames", p_SEI->seiPicTiming.n_frames, bitstream);

        if ( p_SEI->seiPicTiming.full_timestamp_flag )
        {      
          write_u_v( 6, "SEI: seconds_value", p_SEI->seiPicTiming.seconds_value, bitstream);
          write_u_v( 6, "SEI: minutes_value", p_SEI->seiPicTiming.minutes_value, bitstream);
          write_u_v( 5, "SEI: hours_value",   p_SEI->seiPicTiming.hours_value, bitstream);
        }
        else
        {            
          write_u_1( "SEI: seconds_flag", p_SEI->seiPicTiming.seconds_flag, bitstream);
          if (p_SEI->seiPicTiming.seconds_flag)
          {
            write_u_v( 6, "SEI: seconds_value", p_SEI->seiPicTiming.seconds_value, bitstream);
            write_u_1( "SEI: minutes_flag", p_SEI->seiPicTiming.minutes_flag, bitstream);
            if (p_SEI->seiPicTiming.minutes_flag)
            {
              write_u_v( 6, "SEI: minutes_value", p_SEI->seiPicTiming.minutes_value, bitstream);
              write_u_1( "SEI: hours_flag", p_SEI->seiPicTiming.hours_flag, bitstream);
              if (p_SEI->seiPicTiming.hours_flag)
              {
                write_u_v( 5, "SEI: hours_value",   p_SEI->seiPicTiming.hours_value, bitstream);
              }
            }
          }
        }
        if ( hrd == NULL )
        {
          write_u_v( 24, "SEI: time_offset", p_SEI->seiPicTiming.time_offset, bitstream);
        }
        else
        {
          write_u_v( hrd->time_offset_length, "SEI: time_offset", p_SEI->seiPicTiming.time_offset, bitstream);
        }
      }
    }
  }

  // make sure the payload is byte aligned, stuff bits are 10..0
  if ( bitstream->bits_to_go != 8 )
  {
    (bitstream->byte_buf) <<= 1;
    bitstream->byte_buf |= 1;
    bitstream->bits_to_go--;
    if ( bitstream->bits_to_go != 0 )
      (bitstream->byte_buf) <<= (bitstream->bits_to_go);
    bitstream->bits_to_go = 8;
    bitstream->streamBuffer[bitstream->byte_pos++]=bitstream->byte_buf;
    bitstream->byte_buf = 0;
  }
  p_SEI->seiPicTiming.payloadSize = bitstream->byte_pos;
}

/*
 ************************************************************************
 * \brief
 *    Close Picture Timing SEI data
 ************************************************************************
 */
static void ClosePicTiming(SEIParameters *p_SEI)
{
  if (p_SEI->seiPicTiming.data)
  {
    free(p_SEI->seiPicTiming.data->streamBuffer);
    free(p_SEI->seiPicTiming.data);
  }
  p_SEI->seiPicTiming.data = NULL;
}

/*
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *  \functions on frame packing arrangement SEI message
 *  \brief
 *      Based on pre-published Recommendation
 **++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

//! Frame packing arrangement Information
static void InitFramePackingArrangement(VideoParameters *p_Vid)
{
  SEIParameters *p_SEI = p_Vid->p_SEI;

  p_SEI->seiFramePackingArrangement.data = malloc( sizeof(Bitstream) );
  if( p_SEI->seiFramePackingArrangement.data == NULL ) 
    no_mem_exit("InitFramePackingArrangement: seiFramePackingArrangement.data");

  p_SEI->seiFramePackingArrangement.data->streamBuffer = malloc(MAXRTPPAYLOADLEN);
  if( p_SEI->seiFramePackingArrangement.data->streamBuffer == NULL ) 
    no_mem_exit("InitFramePackingArrangement: seiFramePackingArrangement.data->streamBuffer");

  ClearFramePackingArrangement(p_SEI);
}

void ClearFramePackingArrangement(SEIParameters *p_SEI)
{
  memset( p_SEI->seiFramePackingArrangement.data->streamBuffer, 0, MAXRTPPAYLOADLEN);

  p_SEI->seiFramePackingArrangement.data->bits_to_go  = 8;
  p_SEI->seiFramePackingArrangement.data->byte_pos    = 0;
  p_SEI->seiFramePackingArrangement.data->byte_buf    = 0;
  p_SEI->seiFramePackingArrangement.payloadSize       = 0;

  p_SEI->seiFramePackingArrangement.frame_packing_arrangement_id = 0;
  p_SEI->seiFramePackingArrangement.frame_packing_arrangement_cancel_flag = FALSE;
  p_SEI->seiFramePackingArrangement.frame_packing_arrangement_type = 0;
  p_SEI->seiFramePackingArrangement.quincunx_sampling_flag = FALSE;
  p_SEI->seiFramePackingArrangement.content_interpretation_type = 0;
  p_SEI->seiFramePackingArrangement.spatial_flipping_flag = FALSE;
  p_SEI->seiFramePackingArrangement.frame0_flipped_flag = FALSE;
  p_SEI->seiFramePackingArrangement.field_views_flag = FALSE;
  p_SEI->seiFramePackingArrangement.current_frame_is_frame0_flag = FALSE;
  p_SEI->seiFramePackingArrangement.frame0_self_contained_flag = FALSE;
  p_SEI->seiFramePackingArrangement.frame1_self_contained_flag = FALSE;
  /*p_SEI->seiFramePackingArrangement.frame0_grid_position_x = 0;
  p_SEI->seiFramePackingArrangement.frame0_grid_position_y = 0;
  p_SEI->seiFramePackingArrangement.frame1_grid_position_x = 0;
  p_SEI->seiFramePackingArrangement.frame1_grid_position_y = 0;*/
  p_SEI->seiFramePackingArrangement.frame_packing_arrangement_reserved_byte = 0;
  p_SEI->seiFramePackingArrangement.frame_packing_arrangement_repetition_period = 0;
  p_SEI->seiFramePackingArrangement.frame_packing_arrangement_extension_flag = FALSE;

  p_SEI->seiHasFramePackingArrangement_info = FALSE;
}

void UpdateFramePackingArrangement(VideoParameters *p_Vid, InputParameters *p_Inp)
{
  SEIParameters *p_SEI = p_Vid->p_SEI;

  p_SEI->seiHasFramePackingArrangement_info = FALSE;
  if ( p_Inp->SEIFPAType != -1 )
  {
    p_SEI->seiHasFramePackingArrangement_info = TRUE;
    p_SEI->seiFramePackingArrangement.frame_packing_arrangement_type = p_Inp->SEIFPAType;
  }
}

static void FinalizeFramePackingArrangement(SEIParameters *p_SEI, InputParameters *p_Inp)
{
  Bitstream *bitstream = p_SEI->seiFramePackingArrangement.data;

  write_ue_v( "SEI: frame_packing_arrangement_id", (int)p_SEI->seiFramePackingArrangement.frame_packing_arrangement_id, bitstream );
  write_u_1( "SEI: frame_packing_arrangement_cancel_flag", (int)p_SEI->seiFramePackingArrangement.frame_packing_arrangement_cancel_flag, bitstream );
  if ( p_SEI->seiFramePackingArrangement.frame_packing_arrangement_cancel_flag == FALSE )
  {
    write_u_v( 7, "SEI: frame_packing_arrangement_type", (int)p_SEI->seiFramePackingArrangement.frame_packing_arrangement_type, bitstream );
    write_u_1( "SEI: quincunx_sampling_flag", (int)p_SEI->seiFramePackingArrangement.quincunx_sampling_flag, bitstream );
    write_u_v( 6, "SEI: content_interpretation_type", (int)p_SEI->seiFramePackingArrangement.content_interpretation_type, bitstream );
    write_u_1( "SEI: spatial_flipping_flag", (int)p_SEI->seiFramePackingArrangement.spatial_flipping_flag, bitstream );
    write_u_1( "SEI: frame0_flipped_flag", (int)p_SEI->seiFramePackingArrangement.frame0_flipped_flag, bitstream );
    write_u_1( "SEI: field_views_flag", (int)p_SEI->seiFramePackingArrangement.field_views_flag, bitstream );
    write_u_1( "SEI: current_frame_is_frame0_flag", (int)p_SEI->seiFramePackingArrangement.current_frame_is_frame0_flag, bitstream );
    write_u_1( "SEI: frame0_self_contained_flag", (int)p_SEI->seiFramePackingArrangement.frame0_self_contained_flag, bitstream );
    write_u_1( "SEI: frame1_self_contained_flag", (int)p_SEI->seiFramePackingArrangement.frame1_self_contained_flag, bitstream );
    if ( p_SEI->seiFramePackingArrangement.quincunx_sampling_flag == FALSE && p_SEI->seiFramePackingArrangement.frame_packing_arrangement_type != 5 )
    {
      write_u_v( 4, "SEI: frame0_grid_position_x", (int)p_SEI->seiFramePackingArrangement.frame0_grid_position_x, bitstream );
      write_u_v( 4, "SEI: frame0_grid_position_y", (int)p_SEI->seiFramePackingArrangement.frame0_grid_position_y, bitstream );
      write_u_v( 4, "SEI: frame1_grid_position_x", (int)p_SEI->seiFramePackingArrangement.frame1_grid_position_x, bitstream );
      write_u_v( 4, "SEI: frame1_grid_position_y", (int)p_SEI->seiFramePackingArrangement.frame1_grid_position_y, bitstream );
    }

    
    write_u_v( 8, "SEI: frame_packing_arrangement_reserved_byte", (int)p_SEI->seiFramePackingArrangement.frame_packing_arrangement_reserved_byte, bitstream );
    write_ue_v( "SEI: frame_packing_arrangement_repetition_period", (int)p_SEI->seiFramePackingArrangement.frame_packing_arrangement_repetition_period, bitstream );
  }
  write_u_1( "SEI: frame_packing_arrangement_extension_flag", (int)p_SEI->seiFramePackingArrangement.frame_packing_arrangement_extension_flag, bitstream );

  // make sure the payload is byte aligned, stuff bits are 10..0
  if ( bitstream->bits_to_go != 8 )
  {
    (bitstream->byte_buf) <<= 1;
    bitstream->byte_buf |= 1;
    bitstream->bits_to_go--;
    if ( bitstream->bits_to_go != 0 )
      (bitstream->byte_buf) <<= (bitstream->bits_to_go);
    bitstream->bits_to_go = 8;
    bitstream->streamBuffer[bitstream->byte_pos++]=bitstream->byte_buf;
    bitstream->byte_buf = 0;
  }
  p_SEI->seiFramePackingArrangement.payloadSize = bitstream->byte_pos;
}

static void CloseFramePackingArrangement(SEIParameters *p_SEI)
{
  if (p_SEI->seiFramePackingArrangement.data)
  {
    free(p_SEI->seiFramePackingArrangement.data->streamBuffer);
    free(p_SEI->seiFramePackingArrangement.data);
  }
  p_SEI->seiFramePackingArrangement.data = NULL;
}

/*
 ************************************************************************
 * \brief
 *    Initialize dec_ref_pic_marking Repetition SEI data 
 ************************************************************************
 */
static void InitDRPMRepetition(SEIParameters *p_SEI)
{
  p_SEI->seiDRPMRepetition.data = malloc( sizeof(Bitstream) );
  if( p_SEI->seiDRPMRepetition.data == NULL ) 
    no_mem_exit("InitDRPMRepetition: p_SEI->seiDRPMRepetition.data");

  p_SEI->seiDRPMRepetition.data->streamBuffer = malloc(MAXRTPPAYLOADLEN);
  if( p_SEI->seiDRPMRepetition.data->streamBuffer == NULL ) 
    no_mem_exit("InitDRPMRepetition: p_SEI->seiDRPMRepetition.data->streamBuffer");

  ClearDRPMRepetition(p_SEI);
}

/*
 ************************************************************************
 * \brief
 *    Clear dec_ref_pic_marking Repetition SEI data
 ************************************************************************
 */
void ClearDRPMRepetition(SEIParameters *p_SEI)
{
  memset( p_SEI->seiDRPMRepetition.data->streamBuffer, 0, MAXRTPPAYLOADLEN);

  p_SEI->seiDRPMRepetition.data->bits_to_go = 8;
  p_SEI->seiDRPMRepetition.data->byte_pos   = 0;
  p_SEI->seiDRPMRepetition.data->byte_buf   = 0;
  p_SEI->seiDRPMRepetition.payloadSize      = 0;

  p_SEI->seiDRPMRepetition.original_bottom_field_flag = FALSE;
  p_SEI->seiDRPMRepetition.original_field_pic_flag    = FALSE;
  p_SEI->seiDRPMRepetition.original_frame_num         = 0;
  p_SEI->seiDRPMRepetition.original_idr_flag          = FALSE;
  if ( p_SEI->seiDRPMRepetition.dec_ref_pic_marking_buffer_saved != NULL )
  {
    free_drpm_buffer( p_SEI->seiDRPMRepetition.dec_ref_pic_marking_buffer_saved );
    p_SEI->seiDRPMRepetition.dec_ref_pic_marking_buffer_saved = NULL;
  }

  p_SEI->seiHasDRPMRepetition_info = FALSE;
}

/*
 ************************************************************************
 * \brief
 *    Update dec_ref_pic_marking Repetition data
 ************************************************************************
 */
void UpdateDRPMRepetition(SEIParameters *p_SEI)
{
  if ( p_SEI->seiDRPMRepetition.dec_ref_pic_marking_buffer_saved != NULL )
  {
    p_SEI->seiHasDRPMRepetition_info = TRUE;
  }
  else
  {
    p_SEI->seiHasDRPMRepetition_info = FALSE;
  }
}

/*
 ************************************************************************
 * \brief
 *    Finalize dec_ref_pic_marking Repetition SEI data
 ************************************************************************
 */
static void FinalizeDRPMRepetition(VideoParameters *p_Vid)
{
  SEIParameters *p_SEI = p_Vid->p_SEI;
  seq_parameter_set_rbsp_t *active_sps = p_Vid->active_sps;

  Bitstream *bitstream = p_SEI->seiDRPMRepetition.data;

  // SEI message bits
  write_u_1( "SEI: original_idr_flag", p_SEI->seiDRPMRepetition.original_idr_flag, bitstream);
  write_ue_v( "SEI: original_frame_num", p_SEI->seiDRPMRepetition.original_frame_num, bitstream);
  if ( !active_sps->frame_mbs_only_flag )
  {
    write_u_1( "SEI: original_field_pic_flag", p_SEI->seiDRPMRepetition.original_field_pic_flag, bitstream);
    if ( p_SEI->seiDRPMRepetition.original_field_pic_flag )
    {
      write_u_1( "SEI: original_bottom_field_flag", p_SEI->seiDRPMRepetition.original_bottom_field_flag, bitstream);
    }
  }
  // now repeat dec_ref_pic_marking_buffer info
  dec_ref_pic_marking( bitstream, 
    p_SEI->seiDRPMRepetition.dec_ref_pic_marking_buffer_saved, 
    p_SEI->seiDRPMRepetition.original_idr_flag, 0, 0);

  // make sure the payload is byte aligned, stuff bits are 10..0
  if ( bitstream->bits_to_go != 8 )
  {
    (bitstream->byte_buf) <<= 1;
    bitstream->byte_buf |= 1;
    bitstream->bits_to_go--;
    if ( bitstream->bits_to_go != 0 )
      (bitstream->byte_buf) <<= (bitstream->bits_to_go);
    bitstream->bits_to_go = 8;
    bitstream->streamBuffer[bitstream->byte_pos++]=bitstream->byte_buf;
    bitstream->byte_buf = 0;
  }
  p_SEI->seiDRPMRepetition.payloadSize = bitstream->byte_pos;
}

#if JVET_AK0107_MODALITY_INFORMATION
/*
************************************************************************
*  \functions on modality information SEI message
*  \brief
*      Based on JVET-AK0107
*  \author
*      Jingying Gao <jingying.gao@sg.panasonic.com>
************************************************************************
*/
static void InitModalityInfo(SEIParameters *p_SEI)
{
  p_SEI->seiModalityInfo.data = malloc( sizeof(Bitstream) );
  if( p_SEI->seiModalityInfo.data == NULL ) no_mem_exit("InitModalityInfo: p_SEI->seiModalityInfo.data");
  p_SEI->seiModalityInfo.data->streamBuffer = malloc(MAXRTPPAYLOADLEN);
  if( p_SEI->seiModalityInfo.data->streamBuffer == NULL ) no_mem_exit("InitModalityInfo: p_SEI->seiModalityInfo.data->streamBuffer");
  ClearModalityInfo(p_SEI);
}

static void ClearModalityInfo(SEIParameters *p_SEI)
{
  memset( p_SEI->seiModalityInfo.data->streamBuffer, 0, MAXRTPPAYLOADLEN);
  p_SEI->seiModalityInfo.data->bits_to_go  = 8;
  p_SEI->seiModalityInfo.data->byte_pos    = 0;
  p_SEI->seiModalityInfo.data->byte_buf    = 0;
  p_SEI->seiModalityInfo.payloadSize       = 0;

  p_SEI->seiModalityInfo.modality_info_cancel_flag = 0;
  p_SEI->seiModalityInfo.modality_info_persistence_flag = 0;
  p_SEI->seiModalityInfo.modality_type = 0;
  p_SEI->seiModalityInfo.spectrum_range_present_flag = 0;
  p_SEI->seiModalityInfo.min_wavelength_mantissa = 0;
  p_SEI->seiModalityInfo.min_wavelength_exponent_plus15 = 0;
  p_SEI->seiModalityInfo.max_wavelength_mantissa = 0;
  p_SEI->seiModalityInfo.max_wavelength_exponent_plus15 = 0;

  p_SEI->seiHasModalityInfo = FALSE;
}

void UpdateModalityInfo(SEIParameters *p_SEI)
{
  p_SEI->seiModalityInfo.modality_info_cancel_flag = 0;
  p_SEI->seiModalityInfo.modality_info_persistence_flag = 0;
  p_SEI->seiModalityInfo.modality_type = 1;
  p_SEI->seiModalityInfo.spectrum_range_present_flag = 1;
  p_SEI->seiModalityInfo.min_wavelength_mantissa = 2;
  p_SEI->seiModalityInfo.min_wavelength_exponent_plus15 = 0;
  p_SEI->seiModalityInfo.max_wavelength_mantissa = 0;
  p_SEI->seiModalityInfo.max_wavelength_exponent_plus15 = 0;

  p_SEI->seiHasModalityInfo  = TRUE;
}

static void FinalizeModalityInfo(SEIParameters *p_SEI)
{
  Bitstream *bitstream = p_SEI->seiModalityInfo.data;
  write_u_1( "SEI: modality_info_cancel_flag", p_SEI->seiModalityInfo.modality_info_cancel_flag, bitstream );
  if ( !p_SEI->seiModalityInfo.modality_info_cancel_flag )
  {
    write_u_1("SEI: modality_info_persistence_flag", p_SEI->seiModalityInfo.modality_info_persistence_flag, bitstream);
    write_u_v(5, "SEI: modality_type", p_SEI->seiModalityInfo.modality_type, bitstream);
    write_u_1("SEI: spectrum_range_present_flag", p_SEI->seiModalityInfo.spectrum_range_present_flag, bitstream );
    if ( p_SEI->seiModalityInfo.spectrum_range_present_flag )
    {
      write_u_v(11, "SEI: min_wavelength_mantissa", p_SEI->seiModalityInfo.min_wavelength_mantissa, bitstream);
      write_u_v(5, "SEI: min_wavelength_exponent_plus15", p_SEI->seiModalityInfo.min_wavelength_exponent_plus15, bitstream);
      write_u_v(11, "SEI: max_wavelength_mantissa", p_SEI->seiModalityInfo.max_wavelength_mantissa, bitstream);
      write_u_v(5, "SEI: max_wavelength_exponent_plus15", p_SEI->seiModalityInfo.max_wavelength_exponent_plus15, bitstream);
    }
    write_ue_v( "SEI: modality_type_extension_bits", 0,  bitstream ); // modality_type_extension_bits shall be equal to 0 in the current edition 
  }
  // make sure the payload is byte aligned, stuff bits are 10..0
  if ( bitstream->bits_to_go != 8 )
  {
    (bitstream->byte_buf) <<= 1;
    bitstream->byte_buf |= 1;
    bitstream->bits_to_go--;
    if ( bitstream->bits_to_go != 0 ) 
      (bitstream->byte_buf) <<= (bitstream->bits_to_go);
    bitstream->bits_to_go = 8;
    bitstream->streamBuffer[bitstream->byte_pos++]=bitstream->byte_buf;
    bitstream->byte_buf = 0;
  }
  p_SEI->seiModalityInfo.payloadSize = bitstream->byte_pos;
}

static void CloseModalityInfo(SEIParameters *p_SEI)
{
  if (p_SEI->seiModalityInfo.data)
  {
    free(p_SEI->seiModalityInfo.data->streamBuffer);
    free(p_SEI->seiModalityInfo.data);
  }
  p_SEI->seiModalityInfo.data = NULL;
}
#endif

/*
 ************************************************************************
 * \brief
 *    Close dec_ref_pic_marking Repetition SEI data
 ************************************************************************
 */
static void CloseDRPMRepetition(SEIParameters *p_SEI)
{
  if (p_SEI->seiDRPMRepetition.data)
  {
    free(p_SEI->seiDRPMRepetition.data->streamBuffer);
    free(p_SEI->seiDRPMRepetition.data);
  }
  p_SEI->seiDRPMRepetition.data = NULL;
  if ( p_SEI->seiDRPMRepetition.dec_ref_pic_marking_buffer_saved != NULL )
  {
    free_drpm_buffer( p_SEI->seiDRPMRepetition.dec_ref_pic_marking_buffer_saved );
    p_SEI->seiDRPMRepetition.dec_ref_pic_marking_buffer_saved = NULL;
  }
}

/*!
 *****************************************************************************
 * \brief
 *    Prepare the aggregation sei message.
 *
 * \date
 *    September 10, 2002
 *
 * \author
 *    Dong Tian   tian@cs.tut.fi
 *****************************************************************************/
void PrepareAggregationSEIMessage(VideoParameters *p_Vid)
{
  SEIParameters *p_SEI = p_Vid->p_SEI;
  Boolean has_aggregation_sei_message = FALSE;

  clear_sei_message(p_SEI, AGGREGATION_SEI);

  // prepare the sei message here
  // write the spare picture sei payload to the aggregation sei message
  if (p_SEI->seiHasSparePicture && p_Vid->type != B_SLICE)
  {
    FinalizeSpareMBMap(p_Vid);
    assert(p_SEI->seiSparePicturePayload.data->byte_pos == p_SEI->seiSparePicturePayload.payloadSize);
    write_sei_message(p_SEI, AGGREGATION_SEI, p_SEI->seiSparePicturePayload.data->streamBuffer, p_SEI->seiSparePicturePayload.payloadSize, SEI_SPARE_PIC);
    has_aggregation_sei_message = TRUE;
  }
  // write the sub sequence information sei paylaod to the aggregation sei message
  if (p_SEI->seiHasSubseqInfo)
  {
    FinalizeSubseqInfo(p_SEI, p_Vid->layer);
    write_sei_message(p_SEI, AGGREGATION_SEI, p_SEI->seiSubseqInfo[p_Vid->layer].data->streamBuffer, p_SEI->seiSubseqInfo[p_Vid->layer].payloadSize, SEI_SUB_SEQ_INFO);
    ClearSubseqInfoPayload(p_SEI, p_Vid->layer);
    has_aggregation_sei_message = TRUE;
  }
  // write the sub sequence layer information sei paylaod to the aggregation sei message
  if (p_SEI->seiHasSubseqLayerInfo && p_Vid->number == 0)
  {
    FinalizeSubseqLayerInfo(p_SEI);
    write_sei_message(p_SEI, AGGREGATION_SEI, p_SEI->seiSubseqLayerInfo.data, p_SEI->seiSubseqLayerInfo.payloadSize, SEI_SUB_SEQ_LAYER_CHARACTERISTICS);
    p_SEI->seiHasSubseqLayerInfo = FALSE;
    has_aggregation_sei_message = TRUE;
  }
  // write the sub sequence characteristics payload to the aggregation sei message
  if (p_SEI->seiHasSubseqChar)
  {
    FinalizeSubseqChar(p_SEI);
    write_sei_message(p_SEI, AGGREGATION_SEI, p_SEI->seiSubseqChar.data->streamBuffer, p_SEI->seiSubseqChar.payloadSize, SEI_SUB_SEQ_CHARACTERISTICS);
    ClearSubseqCharPayload(p_SEI);
    has_aggregation_sei_message = TRUE;
  }
  // write the pan scan rectangle info sei playload to the aggregation sei message
  if (p_SEI->seiHasPanScanRectInfo)
  {
    FinalizePanScanRectInfo(p_SEI);
    write_sei_message(p_SEI, AGGREGATION_SEI, p_SEI->seiPanScanRectInfo.data->streamBuffer, p_SEI->seiPanScanRectInfo.payloadSize, SEI_PAN_SCAN_RECT);
    ClearPanScanRectInfoPayload(p_SEI);
    has_aggregation_sei_message = TRUE;
  }
  // write the arbitrary (unregistered) info sei playload to the aggregation sei message
  if (p_SEI->seiHasUser_data_unregistered_info)
  {
    FinalizeUser_data_unregistered(p_SEI);
    write_sei_message(p_SEI, AGGREGATION_SEI, p_SEI->seiUser_data_unregistered.data->streamBuffer, p_SEI->seiUser_data_unregistered.payloadSize, SEI_USER_DATA_UNREGISTERED);
    ClearUser_data_unregistered(p_SEI);
    has_aggregation_sei_message = TRUE;
  }
  // write the arbitrary (unregistered) info sei playload to the aggregation sei message
  if (p_SEI->seiHasUser_data_registered_itu_t_t35_info)
  {
    FinalizeUser_data_registered_itu_t_t35(p_SEI);
    write_sei_message(p_SEI, AGGREGATION_SEI, p_SEI->seiUser_data_registered_itu_t_t35.data->streamBuffer, p_SEI->seiUser_data_registered_itu_t_t35.payloadSize, SEI_USER_DATA_REGISTERED_ITU_T_T35);
    ClearUser_data_registered_itu_t_t35(p_SEI);
    has_aggregation_sei_message = TRUE;
  }  
  // more aggregation sei payload is written here...

  // write the scene information SEI payload
  if (p_SEI->seiHasSceneInformation)
  {
    FinalizeSceneInformation(p_SEI);
    write_sei_message(p_SEI, AGGREGATION_SEI, p_SEI->seiSceneInformation.data->streamBuffer, p_SEI->seiSceneInformation.payloadSize, SEI_SCENE_INFO);
    has_aggregation_sei_message = TRUE;
  }

  if (p_SEI->seiHasTone_mapping)
  {
    FinalizeToneMapping(p_Vid);
    write_sei_message(p_SEI, AGGREGATION_SEI, p_SEI->seiToneMapping.data->streamBuffer, p_SEI->seiToneMapping.payloadSize, SEI_TONE_MAPPING);
    ClearToneMapping(p_SEI);
    has_aggregation_sei_message = TRUE;
  }

  if (p_SEI->seiHasPostFilterHints_info)
  {
    FinalizePostFilterHints(p_SEI);
    write_sei_message(p_SEI, AGGREGATION_SEI, p_SEI->seiPostFilterHints.data->streamBuffer, p_SEI->seiPostFilterHints.payloadSize, SEI_POST_FILTER_HINTS);
    has_aggregation_sei_message = TRUE;
  }

  if (p_SEI->seiHasBufferingPeriod_info)
  {
    FinalizeBufferingPeriod(p_SEI, p_Vid->active_sps);
    write_sei_message(p_SEI, AGGREGATION_SEI, p_SEI->seiBufferingPeriod.data->streamBuffer, p_SEI->seiBufferingPeriod.payloadSize, SEI_BUFFERING_PERIOD);
    has_aggregation_sei_message = TRUE;
  }

  if (p_SEI->seiHasPicTiming_info)
  {
    FinalizePicTiming(p_Vid);
    write_sei_message(p_SEI, AGGREGATION_SEI, p_SEI->seiPicTiming.data->streamBuffer, p_SEI->seiPicTiming.payloadSize, SEI_PIC_TIMING);
    has_aggregation_sei_message = TRUE;
  }

  if (p_SEI->seiHasRecoveryPoint_info)
  {
    FinalizeRandomAccess(p_SEI);
    write_sei_message(p_SEI, AGGREGATION_SEI, p_SEI->seiRecoveryPoint.data->streamBuffer, p_SEI->seiRecoveryPoint.payloadSize, SEI_RECOVERY_POINT);
    ClearRandomAccess(p_SEI);
    has_aggregation_sei_message = TRUE;
  }

  if (p_SEI->seiHasDRPMRepetition_info)
  {
    FinalizeDRPMRepetition(p_Vid);
    write_sei_message(p_SEI, AGGREGATION_SEI, p_SEI->seiDRPMRepetition.data->streamBuffer, p_SEI->seiDRPMRepetition.payloadSize, SEI_DEC_REF_PIC_MARKING_REPETITION);
    has_aggregation_sei_message = TRUE;
    ClearDRPMRepetition(p_SEI); // to reset the drpm buffer into NULL
  }

  if (p_SEI->seiHasFramePackingArrangement_info)
  {
    FinalizeFramePackingArrangement(p_SEI, p_Vid->p_Inp);
    write_sei_message(p_SEI, AGGREGATION_SEI, p_SEI->seiFramePackingArrangement.data->streamBuffer, p_SEI->seiFramePackingArrangement.payloadSize, SEI_FRAME_PACKING_ARRANGEMENT);
    has_aggregation_sei_message = TRUE;
  }

#if JVET_AK0107_MODALITY_INFORMATION
  if (p_SEI->seiHasModalityInfo)
  {
    FinalizeModalityInfo(p_SEI);
    write_sei_message(p_SEI, AGGREGATION_SEI, p_SEI->seiModalityInfo.data->streamBuffer, p_SEI->seiModalityInfo.payloadSize, SEI_MODALITY_INFO);
    has_aggregation_sei_message = TRUE;
  }
#endif

#if GFV_ENABLE
#if JVET_AJ0207_GFV_SEI
  if (p_SEI->seiHasGFV_info)
  {
    for (unsigned int gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
    {
      FinalizeGFV(p_SEI, gfv_idx);
      write_sei_message(p_SEI, AGGREGATION_SEI, p_SEI->seiGFV[gfv_idx].data->streamBuffer, p_SEI->seiGFV[gfv_idx].payloadSize, SEI_GENERATIVE_FACE_VIDEO);
    }
    has_aggregation_sei_message = TRUE;
    ClearGFV(p_SEI);
  }
#endif

#if JVET_AK0239_GFVE_SEI
  if (p_SEI->seiHasGFVE_info)
  {
    for (unsigned int gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
    {
      FinalizeGFVE(p_SEI, gfve_idx);
      write_sei_message(p_SEI, AGGREGATION_SEI, p_SEI->seiGFVE[gfve_idx].data->streamBuffer, p_SEI->seiGFVE[gfve_idx].payloadSize, SEI_GENERATIVE_FACE_VIDEO_ENHANCEMENT);
    }
    has_aggregation_sei_message = TRUE;
    ClearGFVE(p_SEI);
  }
#endif
#endif

  // after all the sei payload is written
  if (has_aggregation_sei_message)
  {
    finalize_sei_message(p_SEI, AGGREGATION_SEI);
  }
}

/*!
 ************************************************************************
 * \brief
 *    Free dec_ref_pic_marking_bufffer structure
 ************************************************************************
 */
void free_drpm_buffer( DecRefPicMarking_t *pDRPM )
{
  DecRefPicMarking_t *pTmp = pDRPM, *pPrev;

  if (pTmp == NULL)
    return;

  while( pTmp->Next != NULL )
  {
    pPrev = pTmp;
    pTmp = pTmp->Next;
    free( pPrev );
  }
  free( pTmp );
}

#if GFV_ENABLE
// From NNPF: https://vcgit.hhi.fraunhofer.de/jvet/JM/-/merge_requests/12/, can be removed when NNPF is merged in
static void write_string(char *name, const char *str, Bitstream *bs)
{
  for (int i=0; str[i]!='\0'; ++i)
    write_u_v(8, name, str[i], bs);
  write_u_v(8, name, '\0', bs); // NULL terminator
}

#if JVET_AJ0207_GFV_SEI
/*
 ************************************************************************
 *  \functions on Generative Face Video (GFV) SEI message
 *  \brief
 *    Based on JVET-AJ0207
 *  \author
 *    Jing Yuan Thong                  <jingyuan.thong@sg.panasonic.com>
 ************************************************************************
 */

static int ParseGFVConfigFile(SEIParameters *p_SEI, InputParameters *p_Inp, generative_face_video_struct *pSeiGFV)
{
  // loop counters
  unsigned int gfv_idx;
  unsigned int i, j, k, l;

  // array buffers
  int ret;
  FILE* fp;
  char buf[4096];
  char temp[4096];
  memset(temp, 0, sizeof(temp));
  
  // tmp int and double for reading from cfg file
  unsigned int tmp_int;
  double tmp_double;

  // for matrix elements
  unsigned int matrixWidth = 0;
  unsigned int matrixHeight = 0;
  unsigned int numMatrices = 0;

  printf ("Parsing GFV cfg file %s ..........\n\n", p_Inp->GFVFile);
  if ((fp = fopen(p_Inp->GFVFile, "r")) == NULL) 
  {
    fprintf(stderr, "GFV config file %s is not found, disable GFV SEI\n", p_Inp->GFVFile);
    p_SEI->seiHasGFV_info = FALSE;
    p_SEI->gfv_num_sei = 0;
    return 1;
  }

  //read the GFV config file
  while (fscanf(fp, "%s", buf) != EOF) 
  {
    ret = 1;
    if (strcmp(buf, "gfv_number") == 0) 
    {
      ret = fscanf(fp, " = %u\n", &(p_SEI->gfv_num_sei));
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        pSeiGFV[gfv_idx].gfv_number = p_SEI->gfv_num_sei;
        pSeiGFV[gfv_idx].gfv_current_id = gfv_idx;
      }
#ifdef PRINT_GFV_INFO
      printf("gfv_number = %u\n", pSeiGFV[0].gfv_number);
      printf("ret = %u\n", ret);
#endif
    }
    else if (strcmp(buf, "gfv_base_pic_flag") == 0) 
    {
      ret = fscanf(fp, " = %u\n", &tmp_int);
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        pSeiGFV[gfv_idx].gfv_base_pic_flag = tmp_int ? TRUE : FALSE;
      }
#ifdef PRINT_GFV_INFO
      printf("gfv_base_pic_flag = %u\n", tmp_int);
      printf("ret = %u\n", ret);
#endif
    }
    else if (strcmp(buf, "gfv_nn_present_flag") == 0) 
    {
      ret = fscanf(fp, " = %u\n", &tmp_int);
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        pSeiGFV[gfv_idx].gfv_nn_present_flag = tmp_int ? TRUE : FALSE;
      }
#ifdef PRINT_GFV_INFO
      printf("gfv_nn_present_flag = %u\n", tmp_int);
      printf("ret = %u\n", ret);
#endif
    }
    else if (strcmp(buf, "gfv_nn_mode_idc") == 0) {
      ret = fscanf(fp, " = %u\n", &tmp_int);
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        pSeiGFV[gfv_idx].gfv_nn_mode_idc = tmp_int;
      }
#ifdef PRINT_GFV_INFO
      printf("gfv_nn_mode_idc = %u\n", tmp_int);
      printf("ret = %u\n", ret);
#endif
    }
    else if (strcmp(buf, "gfv_nn_tag_uri") == 0) {
      ret = fscanf(fp, " = %s\n", temp);
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        // prevent buffer overflow
        strncpy(pSeiGFV[gfv_idx].gfv_nn_tag_uri, temp, sizeof(pSeiGFV[gfv_idx].gfv_nn_tag_uri) - 1);
        pSeiGFV[gfv_idx].gfv_nn_tag_uri[sizeof(pSeiGFV[gfv_idx].gfv_nn_tag_uri) - 1] = '\0';
      }
#ifdef PRINT_GFV_INFO
      printf("gfv_nn_tag_uri = %s\n", temp);
      printf("ret = %u\n", ret);
#endif
      memset(temp, 0, sizeof(temp));
    }
    else if (strcmp(buf, "gfv_nn_uri") == 0) {
      ret = fscanf(fp, " = %s\n", temp);
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        // prevent buffer overflow
        strncpy(pSeiGFV[gfv_idx].gfv_nn_uri, temp, sizeof(pSeiGFV[gfv_idx].gfv_nn_uri) - 1);
        pSeiGFV[gfv_idx].gfv_nn_uri[sizeof(pSeiGFV[gfv_idx].gfv_nn_uri) - 1] = '\0';
      }
#ifdef PRINT_GFV_INFO
      printf("gfv_nn_uri = %s\n", temp);
      printf("ret = %u\n", ret);
#endif
      memset(temp, 0, sizeof(temp));
    }
    else if (strcmp(buf, "gfv_id") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_id", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_id = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        ret &= fscanf(fp, "%u", &(pSeiGFV[gfv_idx].gfv_id));
#ifdef PRINT_GFV_INFO
        printf("%u ", pSeiGFV[gfv_idx].gfv_id);
#endif
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_cnt") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_cnt", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_cnt = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        ret &= fscanf(fp, "%u", &(pSeiGFV[gfv_idx].gfv_cnt));
#ifdef PRINT_GFV_INFO
        printf("%u ", pSeiGFV[gfv_idx].gfv_cnt);
#endif
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_drive_pic_fusion_flag") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_drive_pic_fusion_flag", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_drive_pic_fusion_flag = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        ret &= fscanf(fp, "%u", &tmp_int);
        pSeiGFV[gfv_idx].gfv_drive_pic_fusion_flag = tmp_int ? TRUE : FALSE;
#ifdef PRINT_GFV_INFO
        printf("%u ", pSeiGFV[gfv_idx].gfv_drive_pic_fusion_flag);
#endif
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_low_confidence_face_parameter_flag") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_low_confidence_face_parameter_flag", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_low_confidence_face_parameter_flag = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        ret &= fscanf(fp, "%u", &tmp_int);
        pSeiGFV[gfv_idx].gfv_low_confidence_face_parameter_flag = tmp_int ? TRUE : FALSE;
#ifdef PRINT_GFV_INFO
        printf("%u ", pSeiGFV[gfv_idx].gfv_low_confidence_face_parameter_flag);
#endif
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_chroma_key_info_present_flag") == 0) 
    {
      ret = fscanf(fp, " = %u\n", &tmp_int);
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        pSeiGFV[gfv_idx].gfv_chroma_key_info_present_flag = tmp_int ? TRUE : FALSE;
      }
#ifdef PRINT_GFV_INFO
      printf("gfv_chroma_key_info_present_flag = %u\n", pSeiGFV[0].gfv_chroma_key_info_present_flag);
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_chroma_key_value_present_flag") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_chroma_key_value_present_flag", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_chroma_key_value_present_flag = ");
#endif
      for (i = 0; i < 3; i++)
      {
        ret &= fscanf(fp, "%u", &tmp_int);
        for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
        {
          pSeiGFV[gfv_idx].gfv_chroma_key_value_present_flag[i] = tmp_int ? TRUE : FALSE;
        }
#ifdef PRINT_GFV_INFO
        printf("%u ", tmp_int);
#endif
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_chroma_key_value") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_chroma_key_value", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_chroma_key_value = ");
#endif
      for (i = 0; i < 3; i++)
      {
        ret &= fscanf(fp, "%u", &tmp_int);
        for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
        {
          pSeiGFV[gfv_idx].gfv_chroma_key_value[i] = tmp_int;
        }
#ifdef PRINT_GFV_INFO
        printf("%u ", tmp_int);
#endif
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_chroma_key_thr_present_flag") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_chroma_key_thr_present_flag", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_chroma_key_thr_present_flag = ");
#endif
      for (i = 0; i < 2; i++)
      {
        ret &= fscanf(fp, "%u", &tmp_int);
        for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
        {
          pSeiGFV[gfv_idx].gfv_chroma_key_thr_present_flag[i] = tmp_int ? TRUE : FALSE;
        }
#ifdef PRINT_GFV_INFO
        printf("%u ", tmp_int);
#endif
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_chroma_key_thr_value") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_chroma_key_thr_value", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_chroma_key_thr_value = ");
#endif
      for (i = 0; i < 2; i++)
      {
        ret &= fscanf(fp, "%u", &tmp_int);
        for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
        {
          pSeiGFV[gfv_idx].gfv_chroma_key_thr_value[i] = tmp_int;
        }
#ifdef PRINT_GFV_INFO
        printf("%u ", tmp_int);
#endif
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_coordinate_present_flag") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_coordinate_present_flag", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_coordinate_present_flag = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        ret &= fscanf(fp, "%u", &tmp_int);
        pSeiGFV[gfv_idx].gfv_coordinate_present_flag = tmp_int ? TRUE : FALSE;
#ifdef PRINT_GFV_INFO
        printf("%u ", pSeiGFV[gfv_idx].gfv_coordinate_present_flag);
#endif
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_coordinate_pred_flag") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_coordinate_pred_flag", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_coordinate_pred_flag = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        ret &= fscanf(fp, "%u", &tmp_int);
        pSeiGFV[gfv_idx].gfv_coordinate_pred_flag = tmp_int ? TRUE : FALSE;
#ifdef PRINT_GFV_INFO
        printf("%u ", pSeiGFV[gfv_idx].gfv_coordinate_pred_flag);
#endif
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_coordinate_quantization_factor") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_coordinate_quantization_factor", 500);
      }
      ret = 1;
 #ifdef PRINT_GFV_INFO
      printf("gfv_coordinate_quantization_factor = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        ret &= fscanf(fp, "%u", &(pSeiGFV[gfv_idx].gfv_coordinate_quantization_factor));
#ifdef PRINT_GFV_INFO
        printf("%u ", pSeiGFV[gfv_idx].gfv_coordinate_quantization_factor);
#endif
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_coordinate_point_num") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_coordinate_point_num", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_coordinate_point_num = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        ret &= fscanf(fp, "%u", &(pSeiGFV[gfv_idx].gfv_coordinate_point_num));
        if (pSeiGFV[gfv_idx].gfv_coordinate_present_flag && pSeiGFV[gfv_idx].gfv_coordinate_point_num > 0)
        {
          pSeiGFV[gfv_idx].gfv_coordinate_x = calloc((pSeiGFV[gfv_idx].gfv_coordinate_point_num), sizeof(double));
          if (!pSeiGFV[gfv_idx].gfv_coordinate_x) {
            no_mem_exit("ParseGFVConfigFile: alloc gfv_coordinate_x");
          }
          pSeiGFV[gfv_idx].gfv_coordinate_y = calloc((pSeiGFV[gfv_idx].gfv_coordinate_point_num), sizeof(double));
          if (!pSeiGFV[gfv_idx].gfv_coordinate_y) {
            no_mem_exit("ParseGFVConfigFile: alloc gfv_coordinate_y");
          }
          if (pSeiGFV[gfv_idx].gfv_3d_coordinate_flag && !pSeiGFV[gfv_idx].gfv_coordinate_z)
          {
            pSeiGFV[gfv_idx].gfv_coordinate_z = calloc((pSeiGFV[gfv_idx].gfv_coordinate_point_num), sizeof(double));
            if (!pSeiGFV[gfv_idx].gfv_coordinate_z) {
              no_mem_exit("ParseGFVConfigFile: alloc gfv_coordinate_z");
            }
          }
        }
#ifdef PRINT_GFV_INFO
        printf("%u ", pSeiGFV[gfv_idx].gfv_coordinate_point_num);
#endif
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_3d_coordinate_flag") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_3d_coordinate_flag", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_3d_coordinate_flag = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        ret &= fscanf(fp, "%u", &tmp_int);
        pSeiGFV[gfv_idx].gfv_3d_coordinate_flag = tmp_int ? TRUE : FALSE;
        if (pSeiGFV[gfv_idx].gfv_coordinate_present_flag && pSeiGFV[gfv_idx].gfv_coordinate_point_num > 0 && pSeiGFV[gfv_idx].gfv_3d_coordinate_flag && !pSeiGFV[gfv_idx].gfv_coordinate_z)
        {
          pSeiGFV[gfv_idx].gfv_coordinate_z = calloc((pSeiGFV[gfv_idx].gfv_coordinate_point_num), sizeof(double));
          if (!pSeiGFV[gfv_idx].gfv_coordinate_z) {
            no_mem_exit("ParseGFVConfigFile: alloc gfv_coordinate_z");
          }
        }
#ifdef PRINT_GFV_INFO
        printf("%u ", pSeiGFV[gfv_idx].gfv_3d_coordinate_flag);
#endif
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_coordinate_z_max_value") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_coordinate_z_max_value", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_coordinate_z_max_value = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        ret &= fscanf(fp, "%u", &(pSeiGFV[gfv_idx].gfv_coordinate_z_max_value));
#ifdef PRINT_GFV_INFO
        printf("%u ", pSeiGFV[gfv_idx].gfv_coordinate_z_max_value);
#endif
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_coordinate_x") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_coordinate_x", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_coordinate_x = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        if (pSeiGFV[gfv_idx].gfv_coordinate_present_flag)
        {
          for (i = 0; i < pSeiGFV[gfv_idx].gfv_coordinate_point_num; i++)
          {
            ret &= fscanf(fp, "%lf", &tmp_double);
            pSeiGFV[gfv_idx].gfv_coordinate_x[i] = tmp_double;
#ifdef PRINT_GFV_INFO
            printf("%lf ", pSeiGFV[gfv_idx].gfv_coordinate_x[i]);
#endif
          }
        }
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_coordinate_y") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_coordinate_y", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_coordinate_y = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        if (pSeiGFV[gfv_idx].gfv_coordinate_present_flag)
        {
          for (i = 0; i < pSeiGFV[gfv_idx].gfv_coordinate_point_num; i++)
          {
            ret &= fscanf(fp, "%lf", &tmp_double);
            pSeiGFV[gfv_idx].gfv_coordinate_y[i] = tmp_double;
#ifdef PRINT_GFV_INFO
            printf("%lf ", pSeiGFV[gfv_idx].gfv_coordinate_y[i]);
#endif
          }
        }
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_coordinate_z") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_coordinate_z", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_coordinate_z = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        if (pSeiGFV[gfv_idx].gfv_coordinate_present_flag && pSeiGFV[gfv_idx].gfv_3d_coordinate_flag)
        {
          for (i = 0; i < pSeiGFV[gfv_idx].gfv_coordinate_point_num; i++)
          {
            ret &= fscanf(fp, "%lf", &tmp_double);
            pSeiGFV[gfv_idx].gfv_coordinate_z[i] = tmp_double;
#ifdef PRINT_GFV_INFO
            printf("%lf ", pSeiGFV[gfv_idx].gfv_coordinate_z[i]);
#endif
          }
        }
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_matrix_present_flag") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_matrix_present_flag", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_matrix_present_flag = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        ret &= fscanf(fp, "%u", &tmp_int);
        pSeiGFV[gfv_idx].gfv_matrix_present_flag = tmp_int ? TRUE : FALSE;
#ifdef PRINT_GFV_INFO
        printf("%u ", pSeiGFV[gfv_idx].gfv_matrix_present_flag);
#endif
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_matrix_pred_flag") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_matrix_pred_flag", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_matrix_pred_flag = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        ret &= fscanf(fp, "%u", &tmp_int);
        pSeiGFV[gfv_idx].gfv_matrix_pred_flag = tmp_int ? TRUE : FALSE;
#ifdef PRINT_GFV_INFO
        printf("%u ", pSeiGFV[gfv_idx].gfv_matrix_pred_flag);
#endif
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_matrix_element_precision_factor") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_matrix_element_precision_factor", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_matrix_element_precision_factor = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        ret &= fscanf(fp, "%u", &(pSeiGFV[gfv_idx].gfv_matrix_element_precision_factor));
#ifdef PRINT_GFV_INFO
        printf("%u ", pSeiGFV[gfv_idx].gfv_matrix_element_precision_factor);
#endif
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_num_matrix_type") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_num_matrix_type", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_num_matrix_type = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        ret &= fscanf(fp, "%u", &(pSeiGFV[gfv_idx].gfv_num_matrix_type));
#ifdef PRINT_GFV_INFO
        printf("%u ", pSeiGFV[gfv_idx].gfv_num_matrix_type);
#endif
        if (pSeiGFV[gfv_idx].gfv_matrix_present_flag && pSeiGFV[gfv_idx].gfv_num_matrix_type > 0)
        {
          pSeiGFV[gfv_idx].gfv_matrix_type_idx = calloc((pSeiGFV[gfv_idx].gfv_num_matrix_type), sizeof(unsigned int));
          if (!pSeiGFV[gfv_idx].gfv_matrix_type_idx) {
            no_mem_exit("ParseGFVConfigFile: alloc gfv_matrix_type_idx");
          }
          pSeiGFV[gfv_idx].gfv_num_matrices_to_num_kps_flag = calloc((pSeiGFV[gfv_idx].gfv_num_matrix_type), sizeof(unsigned int));
          if (!pSeiGFV[gfv_idx].gfv_num_matrices_to_num_kps_flag) {
            no_mem_exit("ParseGFVConfigFile: alloc gfv_num_matrices_to_num_kps_flag");
          }
          pSeiGFV[gfv_idx].gfv_num_matrices_info = calloc((pSeiGFV[gfv_idx].gfv_num_matrix_type), sizeof(unsigned int));
          if (!pSeiGFV[gfv_idx].gfv_num_matrices_info) {
            no_mem_exit("ParseGFVConfigFile: alloc gfv_num_matrices_info");
          }
          pSeiGFV[gfv_idx].gfv_matrix_3D_space_flag = calloc((pSeiGFV[gfv_idx].gfv_num_matrix_type), sizeof(unsigned int));
          if (!pSeiGFV[gfv_idx].gfv_matrix_3D_space_flag) {
            no_mem_exit("ParseGFVConfigFile: alloc gfv_matrix_3D_space_flag");
          }
          pSeiGFV[gfv_idx].gfv_num_matrices = calloc((pSeiGFV[gfv_idx].gfv_num_matrix_type), sizeof(unsigned int));
          if (!pSeiGFV[gfv_idx].gfv_num_matrices) {
            no_mem_exit("ParseGFVConfigFile: alloc gfv_num_matrices");
          }
          pSeiGFV[gfv_idx].gfv_matrix_width = calloc((pSeiGFV[gfv_idx].gfv_num_matrix_type), sizeof(unsigned int));
          if (!pSeiGFV[gfv_idx].gfv_matrix_width) {
            no_mem_exit("ParseGFVConfigFile: alloc gfv_matrix_width");
          }
          pSeiGFV[gfv_idx].gfv_matrix_height = calloc((pSeiGFV[gfv_idx].gfv_num_matrix_type), sizeof(unsigned int));
          if (!pSeiGFV[gfv_idx].gfv_matrix_height) {
            no_mem_exit("ParseGFVConfigFile: alloc gfv_matrix_height");
          }
          pSeiGFV[gfv_idx].gfv_num_matrices_store = calloc((pSeiGFV[gfv_idx].gfv_num_matrix_type), sizeof(unsigned int));
          if (!pSeiGFV[gfv_idx].gfv_num_matrices_store) {
            no_mem_exit("ParseGFVConfigFile: alloc gfv_num_matrices_store");
          }
          pSeiGFV[gfv_idx].gfv_matrix_width_store = calloc((pSeiGFV[gfv_idx].gfv_num_matrix_type), sizeof(unsigned int));
          if (!pSeiGFV[gfv_idx].gfv_matrix_width_store) {
            no_mem_exit("ParseGFVConfigFile: alloc gfv_matrix_width_store");
          }
          pSeiGFV[gfv_idx].gfv_matrix_height_store = calloc((pSeiGFV[gfv_idx].gfv_num_matrix_type), sizeof(unsigned int));
          if (!pSeiGFV[gfv_idx].gfv_matrix_height_store) {
            no_mem_exit("ParseGFVConfigFile: alloc gfv_matrix_height_store");
          }
          pSeiGFV[gfv_idx].gfv_matrix_element = (double****) calloc((pSeiGFV[gfv_idx].gfv_num_matrix_type), sizeof(double***));
          if (!pSeiGFV[gfv_idx].gfv_matrix_element) {
            no_mem_exit("ParseGFVConfigFile: alloc gfv_matrix_element");
          }
        }
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_matrix_type_idx") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_matrix_type_idx", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_matrix_type_idx = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        if (pSeiGFV[gfv_idx].gfv_matrix_present_flag)
        {
          for (i = 0; i < pSeiGFV[gfv_idx].gfv_num_matrix_type; i++)
          {
            ret &= fscanf(fp, "%u", &tmp_int);
            pSeiGFV[gfv_idx].gfv_matrix_type_idx[i] = tmp_int;
#ifdef PRINT_GFV_INFO
            printf("%u ", pSeiGFV[gfv_idx].gfv_matrix_type_idx[i]);
#endif
          }
        }
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_num_matrices_to_num_kps_flag") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_num_matrices_to_num_kps_flag", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_num_matrices_to_num_kps_flag = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        if (pSeiGFV[gfv_idx].gfv_matrix_present_flag)
        {
          ret &= fscanf(fp, "%u", &tmp_int);
          for (i = 0; i < pSeiGFV[gfv_idx].gfv_num_matrix_type; i++)
          {
            pSeiGFV[gfv_idx].gfv_num_matrices_to_num_kps_flag[i] = tmp_int ? TRUE : FALSE;
#ifdef PRINT_GFV_INFO
            printf("%u ", pSeiGFV[gfv_idx].gfv_num_matrices_to_num_kps_flag[i]);
#endif
          }
        }
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_num_matrices_info") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_num_matrices_info", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_num_matrices_info = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        if (pSeiGFV[gfv_idx].gfv_matrix_present_flag)
        {
          ret &= fscanf(fp, "%u", &tmp_int);
          for (i = 0; i < pSeiGFV[gfv_idx].gfv_num_matrix_type; i++)
          {
            pSeiGFV[gfv_idx].gfv_num_matrices_info[i] = tmp_int;
#ifdef PRINT_GFV_INFO
            printf("%u ", pSeiGFV[gfv_idx].gfv_num_matrices_info[i]);
#endif
          }
        }
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_matrix_3D_space_flag") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_matrix_3D_space_flag", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_matrix_3D_space_flag = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        if (pSeiGFV[gfv_idx].gfv_matrix_present_flag)
        {
          ret &= fscanf(fp, "%u", &tmp_int);
          for (i = 0; i < pSeiGFV[gfv_idx].gfv_num_matrix_type; i++)
          {
            pSeiGFV[gfv_idx].gfv_matrix_3D_space_flag[i] = tmp_int ? TRUE : FALSE;
#ifdef PRINT_GFV_INFO
            printf("%u ", pSeiGFV[gfv_idx].gfv_matrix_3D_space_flag[i]);
#endif
          }
        }
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_num_matrices") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_num_matrices", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_num_matrices = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        if (pSeiGFV[gfv_idx].gfv_matrix_present_flag)
        {
          for (i = 0; i < pSeiGFV[gfv_idx].gfv_num_matrix_type; i++)
          {
            ret &= fscanf(fp, "%u", &tmp_int);
            pSeiGFV[gfv_idx].gfv_num_matrices[i] = tmp_int;
#ifdef PRINT_GFV_INFO
            printf("%u ", pSeiGFV[gfv_idx].gfv_num_matrices[i]);
#endif
            if (pSeiGFV[gfv_idx].gfv_matrix_type_idx[i] == 0 || pSeiGFV[gfv_idx].gfv_matrix_type_idx[i] == 1) 
            {
              if (pSeiGFV[gfv_idx].gfv_coordinate_present_flag)
              {
                numMatrices = pSeiGFV[gfv_idx].gfv_num_matrices_to_num_kps_flag[i] ? pSeiGFV[gfv_idx].gfv_coordinate_point_num : (pSeiGFV[gfv_idx].gfv_num_matrices_info[i] < (pSeiGFV[gfv_idx].gfv_coordinate_point_num - 1) ? (pSeiGFV[gfv_idx].gfv_num_matrices_info[i] + 1) : (pSeiGFV[gfv_idx].gfv_num_matrices_info[i] + 2));
              } 
              else 
              {
                numMatrices = pSeiGFV[gfv_idx].gfv_num_matrices_info[i] + 1;
              }
            }
            else if (pSeiGFV[gfv_idx].gfv_matrix_type_idx[i] >= 2 && pSeiGFV[gfv_idx].gfv_matrix_type_idx[i] < 7)
            {
                numMatrices = 1;
            }
            else
            {
                numMatrices = pSeiGFV[gfv_idx].gfv_num_matrices[i];
            }
            pSeiGFV[gfv_idx].gfv_num_matrices_store[i] = numMatrices;
#ifdef PRINT_GFV_INFO
            printf("(%u) ", pSeiGFV[gfv_idx].gfv_num_matrices_store[i]);
#endif
          }
        }
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_matrix_width") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_matrix_width", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_matrix_width = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        if (pSeiGFV[gfv_idx].gfv_matrix_present_flag)
        {
          for (i = 0; i < pSeiGFV[gfv_idx].gfv_num_matrix_type; i++)
          {
            ret &= fscanf(fp, "%u", &tmp_int);
            pSeiGFV[gfv_idx].gfv_matrix_width[i] = tmp_int;
#ifdef PRINT_GFV_INFO
            printf("%u ", pSeiGFV[gfv_idx].gfv_matrix_width[i]);
#endif
            if (pSeiGFV[gfv_idx].gfv_matrix_type_idx[i] == 0 || pSeiGFV[gfv_idx].gfv_matrix_type_idx[i] == 1) 
            {
              matrixWidth = pSeiGFV[gfv_idx].gfv_3d_coordinate_flag + 2;
            }
            else if (pSeiGFV[gfv_idx].gfv_matrix_type_idx[i] == 4)
            {
              matrixWidth = (pSeiGFV[gfv_idx].gfv_coordinate_present_flag ? pSeiGFV[gfv_idx].gfv_3d_coordinate_flag : pSeiGFV[gfv_idx].gfv_matrix_3D_space_flag[i]) + 2;
            }
            else if (pSeiGFV[gfv_idx].gfv_matrix_type_idx[i] == 5 || pSeiGFV[gfv_idx].gfv_matrix_type_idx[i] == 6)
            {
              matrixWidth = 1;
            }
            else
            {
              matrixWidth = pSeiGFV[gfv_idx].gfv_matrix_width[i];
            }
            pSeiGFV[gfv_idx].gfv_matrix_width_store[i] = matrixWidth;
#ifdef PRINT_GFV_INFO
            printf("(%u) ", pSeiGFV[gfv_idx].gfv_matrix_width_store[i]);
#endif
          }
        }
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_matrix_height") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_matrix_height", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_matrix_height = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        if (pSeiGFV[gfv_idx].gfv_matrix_present_flag)
        {
          for (i = 0; i < pSeiGFV[gfv_idx].gfv_num_matrix_type; i++)
          {
            ret &= fscanf(fp, "%u", &tmp_int);
            pSeiGFV[gfv_idx].gfv_matrix_height[i] = tmp_int;
#ifdef PRINT_GFV_INFO
            printf("%u ", pSeiGFV[gfv_idx].gfv_matrix_height[i]);
#endif
            if (pSeiGFV[gfv_idx].gfv_matrix_type_idx[i] == 0 || pSeiGFV[gfv_idx].gfv_matrix_type_idx[i] == 1) 
            {
              matrixHeight = pSeiGFV[gfv_idx].gfv_3d_coordinate_flag + 2;
            }
            else if (pSeiGFV[gfv_idx].gfv_matrix_type_idx[i] >= 4 && pSeiGFV[gfv_idx].gfv_matrix_type_idx[i] <= 6)
            {
              matrixHeight = (pSeiGFV[gfv_idx].gfv_coordinate_present_flag ? pSeiGFV[gfv_idx].gfv_3d_coordinate_flag : pSeiGFV[gfv_idx].gfv_matrix_3D_space_flag[i]) + 2;
            }
            else
            {
              matrixHeight = pSeiGFV[gfv_idx].gfv_matrix_height[i];
            }
            pSeiGFV[gfv_idx].gfv_matrix_height_store[i] = matrixHeight;
#ifdef PRINT_GFV_INFO
            printf("(%u) ", pSeiGFV[gfv_idx].gfv_matrix_height_store[i]);
#endif
          }
        }
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_matrix_element") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVConfigFile: error parsing gfv_matrix_element", 500);
      }
      ret = 1;
#ifdef PRINT_GFV_INFO
      printf("gfv_matrix_element = ");
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++)
      {
        if (pSeiGFV[gfv_idx].gfv_matrix_present_flag)
        {
          for (i = 0; i < pSeiGFV[gfv_idx].gfv_num_matrix_type; i++)
          {
            numMatrices = pSeiGFV[gfv_idx].gfv_num_matrices_store[i];
            matrixHeight = pSeiGFV[gfv_idx].gfv_matrix_height_store[i];
            matrixWidth = pSeiGFV[gfv_idx].gfv_matrix_width_store[i];

            // malloc for numMatrices
            pSeiGFV[gfv_idx].gfv_matrix_element[i] = (double***) calloc((numMatrices), sizeof(double**));
            if (!pSeiGFV[gfv_idx].gfv_matrix_element[i]) {
              no_mem_exit("ParseGFVConfigFile: alloc gfv_matrix_element, numMatrices");
            }

            // over each matrix
            for (j = 0; j < numMatrices; j++)
            {
              // malloc for matrixHeight
              pSeiGFV[gfv_idx].gfv_matrix_element[i][j] = (double**) calloc((matrixHeight), sizeof(double*));
              if (!pSeiGFV[gfv_idx].gfv_matrix_element[i][j]) {
                no_mem_exit("ParseGFVConfigFile: alloc gfv_matrix_element, matrixHeight");
              }

              // over matrixHeight
              for (k = 0; k < matrixHeight; k++)
              {
                // malloc for matrixWidth
                pSeiGFV[gfv_idx].gfv_matrix_element[i][j][k] = (double*) calloc((matrixWidth), sizeof(double));
                if (!pSeiGFV[gfv_idx].gfv_matrix_element[i][j][k]) {
                  no_mem_exit("ParseGFVConfigFile: alloc gfv_matrix_element, matrixWidth");
                }

                // over matrixWidth
                for (l = 0; l < matrixWidth; l++)
                {
                  ret &= fscanf(fp, "%lf", &tmp_double);
                  pSeiGFV[gfv_idx].gfv_matrix_element[i][j][k][l] = tmp_double;
#ifdef PRINT_GFV_INFO
                  printf("%lf ", pSeiGFV[gfv_idx].gfv_matrix_element[i][j][k][l]);
#endif
                }
              }
            }
          }
        }
      }
#ifdef PRINT_GFV_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfv_payload_filename") == 0) 
    {
      ret = fscanf(fp, " = %4095[^\n]\n", temp);
#ifdef PRINT_GFV_INFO
      printf("gfv_payload_filename = %s\n", temp);
#endif
      for (gfv_idx = 0; gfv_idx < p_SEI->gfv_num_sei; gfv_idx++) 
      {
        if (pSeiGFV[gfv_idx].gfv_nn_present_flag)
        {
          if (pSeiGFV[gfv_idx].gfv_nn_mode_idc == 0)
          {
            FILE* file;
            if ((file = fopen(temp, "rb")) == NULL) 
            {
              fprintf(stderr, "GFV NN payload file %s is not found.\n", temp);
              return 1;
            }
  
            if (fseek(file, 0, SEEK_END) != 0) {
              fclose(file);
              return -1;
            }
  
            long payload_size = ftell(file);
            if (payload_size == -1) {
              fclose(file);
              return -1;
            }
            rewind(file);

            pSeiGFV[gfv_idx].gfv_payload_length = payload_size;
  
            pSeiGFV[gfv_idx].gfv_payload_byte = malloc(payload_size);
            if (!pSeiGFV[gfv_idx].gfv_payload_byte) {
              no_mem_exit("ParseGFVConfigFile: alloc gfv_payload_byte");
            }

            size_t bytes_read = fread(pSeiGFV[gfv_idx].gfv_payload_byte, 1, payload_size, file);
            if (bytes_read != (size_t)payload_size) {
                free(pSeiGFV[gfv_idx].gfv_payload_byte);
                pSeiGFV[gfv_idx].gfv_payload_byte = NULL;
                fclose(file);
                return -1;
            }

            fclose(file);
          }
        }
      }
#ifdef PRINT_GFV_INFO     
      printf("ret = %u\n", ret);
#endif
    }

    else
    {
      // read till the line end 
      if (NULL == fgets(buf, sizeof(buf), fp))
      {
        error ("ParseGFVConfigFile: error parsing GFV config file, fgets returns null",500);
      }
    }
    if (ret != 1)
    {
      error ("ParseGFVConfigFile: error parsing GFV config file, format error",500);
    }
  }

  fclose(fp);

  return 0;
}

static void InitGFV(SEIParameters *p_SEI, InputParameters *p_Inp) 
{
  if (p_Inp->GFVSEIPresentFlag == 0)
  {
    p_SEI->seiHasGFV_info = FALSE;
    p_SEI->gfv_num_sei = 0;
    return;
  }

#ifdef PRINT_GFV_INFO
  printf("InitGFV, start\n");
#endif
  
  for (int gfv_idx = 0; gfv_idx < MAX_NUM_GFV_CNT; gfv_idx++)
  {
    p_SEI->seiGFV[gfv_idx].data = malloc( sizeof(Bitstream) );
    if( p_SEI->seiGFV[gfv_idx].data == NULL ) no_mem_exit("InitGFV: seiGFV[i].data");
    p_SEI->seiGFV[gfv_idx].data->streamBuffer = malloc(MAXRTPPAYLOADLEN);
    if( p_SEI->seiGFV[gfv_idx].data->streamBuffer == NULL ) no_mem_exit("InitGFV: seiGFV[i].data->streamBuffer");
  }
  
  ClearGFV(p_SEI);

  // read GFV config from file
  ParseGFVConfigFile(p_SEI, p_Inp, p_SEI->seiGFV);
  
  // all initialized, set GFV presence to true
  p_SEI->seiHasGFV_info = TRUE;

#ifdef PRINT_GFV_INFO
  printf("InitGFV, finish with %u SEI\n", p_SEI->gfv_num_sei);
#endif
}

static void ClearGFV(SEIParameters *p_SEI) 
{
#ifdef PRINT_GFV_INFO
  printf("ClearGFV, start\n");
#endif
  for (int gfv_idx = 0; gfv_idx < MAX_NUM_GFV_CNT; gfv_idx++)
  {
    memset( p_SEI->seiGFV[gfv_idx].data->streamBuffer, 0, MAXRTPPAYLOADLEN);
    p_SEI->seiGFV[gfv_idx].data->bits_to_go  = 8;
    p_SEI->seiGFV[gfv_idx].data->byte_pos    = 0;
    p_SEI->seiGFV[gfv_idx].data->byte_buf    = 0;
    p_SEI->seiGFV[gfv_idx].payloadSize       = 0;

    
    p_SEI->seiGFV[gfv_idx].gfv_number = 0;
    p_SEI->seiGFV[gfv_idx].gfv_current_id = 0;
    p_SEI->seiGFV[gfv_idx].gfv_base_pic_flag = TRUE;
    p_SEI->seiGFV[gfv_idx].gfv_nn_present_flag = FALSE;
    p_SEI->seiGFV[gfv_idx].gfv_nn_mode_idc = 1;
    memset(p_SEI->seiGFV[gfv_idx].gfv_nn_tag_uri, 0, sizeof(p_SEI->seiGFV[gfv_idx].gfv_nn_tag_uri));
    memset(p_SEI->seiGFV[gfv_idx].gfv_nn_uri, 0, sizeof(p_SEI->seiGFV[gfv_idx].gfv_nn_uri));

    p_SEI->seiGFV[gfv_idx].gfv_chroma_key_info_present_flag = FALSE;
    memset(p_SEI->seiGFV[gfv_idx].gfv_chroma_key_value_present_flag, FALSE, sizeof(p_SEI->seiGFV[gfv_idx].gfv_chroma_key_value_present_flag));
    memset(p_SEI->seiGFV[gfv_idx].gfv_chroma_key_value, 0, sizeof(p_SEI->seiGFV[gfv_idx].gfv_chroma_key_value));
    memset(p_SEI->seiGFV[gfv_idx].gfv_chroma_key_thr_present_flag, FALSE, sizeof(p_SEI->seiGFV[gfv_idx].gfv_chroma_key_thr_present_flag));
    memset(p_SEI->seiGFV[gfv_idx].gfv_chroma_key_thr_value, 0, sizeof(p_SEI->seiGFV[gfv_idx].gfv_chroma_key_thr_value));

    p_SEI->seiGFV[gfv_idx].gfv_id = 0;
    p_SEI->seiGFV[gfv_idx].gfv_cnt = 0;
    p_SEI->seiGFV[gfv_idx].gfv_drive_pic_fusion_flag = FALSE;
    p_SEI->seiGFV[gfv_idx].gfv_low_confidence_face_parameter_flag = TRUE;

    p_SEI->seiGFV[gfv_idx].gfv_coordinate_present_flag = FALSE;
    p_SEI->seiGFV[gfv_idx].gfv_coordinate_pred_flag = FALSE;
    p_SEI->seiGFV[gfv_idx].gfv_coordinate_quantization_factor = 0;
    p_SEI->seiGFV[gfv_idx].gfv_coordinate_point_num = 0;
    p_SEI->seiGFV[gfv_idx].gfv_3d_coordinate_flag = FALSE;
    p_SEI->seiGFV[gfv_idx].gfv_coordinate_z_max_value = 0;

    p_SEI->seiGFV[gfv_idx].gfv_matrix_present_flag = FALSE;
    p_SEI->seiGFV[gfv_idx].gfv_matrix_pred_flag = FALSE;
    p_SEI->seiGFV[gfv_idx].gfv_matrix_element_precision_factor = 0;
    p_SEI->seiGFV[gfv_idx].gfv_num_matrix_type = 0;

    memset(p_SEI->seiGFV[gfv_idx].gfv_payload_filename, 0, sizeof(p_SEI->seiGFV[gfv_idx].gfv_payload_filename));
    p_SEI->seiGFV[gfv_idx].gfv_payload_length = 0;
  }

  p_SEI->seiHasGFV_info = FALSE;
  p_SEI->gfv_num_sei = 0;  

#ifdef PRINT_GFV_INFO
  printf("ClearGFV, finish\n");
#endif
}

static void CloseGFV(SEIParameters *p_SEI) 
{
  unsigned int matrixHeight = 0;
  unsigned int numMatrices = 0;

  unsigned int i, j, k;
#ifdef PRINT_GFV_INFO
  printf("CloseGFV, start\n");
#endif

  for (int gfv_idx = 0; gfv_idx < MAX_NUM_GFV_CNT; gfv_idx++)
  {
    if (p_SEI->seiGFV[gfv_idx].data)
    {
      free(p_SEI->seiGFV[gfv_idx].data->streamBuffer);
      free(p_SEI->seiGFV[gfv_idx].data);
    }
      
    // free additional pointers in GFV struct
    if (p_SEI->seiGFV[gfv_idx].gfv_coordinate_x)
    {
      free(p_SEI->seiGFV[gfv_idx].gfv_coordinate_x);
      p_SEI->seiGFV[gfv_idx].gfv_coordinate_x = NULL;
    }

    if (p_SEI->seiGFV[gfv_idx].gfv_coordinate_y)
    {
      free(p_SEI->seiGFV[gfv_idx].gfv_coordinate_y);
      p_SEI->seiGFV[gfv_idx].gfv_coordinate_y = NULL;
    }

    if (p_SEI->seiGFV[gfv_idx].gfv_coordinate_z)
    {
      free(p_SEI->seiGFV[gfv_idx].gfv_coordinate_z);
      p_SEI->seiGFV[gfv_idx].gfv_coordinate_z = NULL;
    }

    if (p_SEI->seiGFV[gfv_idx].gfv_num_matrices_to_num_kps_flag)
    {
      free(p_SEI->seiGFV[gfv_idx].gfv_num_matrices_to_num_kps_flag);
      p_SEI->seiGFV[gfv_idx].gfv_num_matrices_to_num_kps_flag = NULL;
    }

    if (p_SEI->seiGFV[gfv_idx].gfv_num_matrices_info)
    {
      free(p_SEI->seiGFV[gfv_idx].gfv_num_matrices_info);
      p_SEI->seiGFV[gfv_idx].gfv_num_matrices_info = NULL;
    }

    if (p_SEI->seiGFV[gfv_idx].gfv_matrix_3D_space_flag)
    {
      free(p_SEI->seiGFV[gfv_idx].gfv_matrix_3D_space_flag);
      p_SEI->seiGFV[gfv_idx].gfv_matrix_3D_space_flag = NULL;
    }

    if (p_SEI->seiGFV[gfv_idx].gfv_num_matrices)
    {
      free(p_SEI->seiGFV[gfv_idx].gfv_num_matrices);
      p_SEI->seiGFV[gfv_idx].gfv_num_matrices = NULL;
    }

    if (p_SEI->seiGFV[gfv_idx].gfv_matrix_width)
    {
      free(p_SEI->seiGFV[gfv_idx].gfv_matrix_width);
      p_SEI->seiGFV[gfv_idx].gfv_matrix_width = NULL;
    }

    if (p_SEI->seiGFV[gfv_idx].gfv_matrix_height)
    {
      free(p_SEI->seiGFV[gfv_idx].gfv_matrix_height);
      p_SEI->seiGFV[gfv_idx].gfv_matrix_height = NULL;
    }

    if (p_SEI->seiGFV[gfv_idx].gfv_matrix_type_idx)
    {
      free(p_SEI->seiGFV[gfv_idx].gfv_matrix_type_idx);
      p_SEI->seiGFV[gfv_idx].gfv_matrix_type_idx = NULL;
    }

    if (p_SEI->seiGFV[gfv_idx].gfv_matrix_element) 
    {
      for (i = 0; i < p_SEI->seiGFV[gfv_idx].gfv_num_matrix_type; i++)
      {
        numMatrices = p_SEI->seiGFV[gfv_idx].gfv_num_matrices_store[i];
        matrixHeight = p_SEI->seiGFV[gfv_idx].gfv_matrix_height_store[i];

        if (p_SEI->seiGFV[gfv_idx].gfv_matrix_element[i])
        {
          for (j = 0; j < numMatrices; j++)
          {
            if (p_SEI->seiGFV[gfv_idx].gfv_matrix_element[i][j])
            {
              for (k = 0; k < matrixHeight; k++)
              {
                if (p_SEI->seiGFV[gfv_idx].gfv_matrix_element[i][j][k]) 
                {
                  free(p_SEI->seiGFV[gfv_idx].gfv_matrix_element[i][j][k]);
                }
              }
              free(p_SEI->seiGFV[gfv_idx].gfv_matrix_element[i][j]);
            }
          }
          free(p_SEI->seiGFV[gfv_idx].gfv_matrix_element[i]);
        }
      }
      
      free(p_SEI->seiGFV[gfv_idx].gfv_num_matrices_store);
      p_SEI->seiGFV[gfv_idx].gfv_num_matrices_store = NULL;

      free(p_SEI->seiGFV[gfv_idx].gfv_matrix_height_store);
      p_SEI->seiGFV[gfv_idx].gfv_matrix_height_store = NULL;

      free(p_SEI->seiGFV[gfv_idx].gfv_matrix_width_store);
      p_SEI->seiGFV[gfv_idx].gfv_matrix_width_store = NULL;

      free(p_SEI->seiGFV[gfv_idx].gfv_matrix_element);
      p_SEI->seiGFV[gfv_idx].gfv_matrix_element = NULL;
    }

    if (p_SEI->seiGFV[gfv_idx].gfv_payload_byte)
    {
      free(p_SEI->seiGFV[gfv_idx].gfv_payload_byte);
      p_SEI->seiGFV[gfv_idx].gfv_payload_byte = NULL;
    }

    // reset pointers to null
    p_SEI->seiGFV[gfv_idx].data = NULL;
  }
  p_SEI->seiHasGFV_info = FALSE;
  p_SEI->gfv_num_sei = 0;

#ifdef PRINT_GFV_INFO
  printf("CloseGFV, finish\n");
#endif
}

static void FinalizeGFV(SEIParameters *p_SEI, unsigned int gfv_idx)
{
  generative_face_video_struct *sei = &p_SEI->seiGFV[gfv_idx];
  Bitstream *bitstream = sei->data;  
  unsigned int i;

  unsigned int matrixId, j, k, l, chromac, chromai;
  unsigned int gfv_coordinate_precision_factor_minus1;
  unsigned int gfv_num_kps_minus1;
  unsigned int gfv_coordinate_z_max_value_minus1;

  int cur_coordinate_x_int, cur_coordinate_y_int, cur_coordinate_z_int;
  int cur_coordinate_x_int_abs, cur_coordinate_y_int_abs, cur_coordinate_z_int_abs;
  Boolean signflag_x_coord, signflag_y_coord, signflag_z_coord;
  double coordinate_x_tensor_abs_rec, coordinate_y_tensor_abs_rec, coordinate_z_tensor_abs_rec;
  
  unsigned int gfv_matrix_element_precision_factor_minus1;
  unsigned int gfv_num_matrix_types_minus1;
  unsigned int gfv_num_matrices_minus1;
  unsigned int gfv_matrix_width_minus1;
  unsigned int gfv_matrix_height_minus1;
  
  double cur_matrix_element_abs, cur_matrix_element_abs_decimal;
  int cur_matrix_element_abs_int, cur_matrix_element_abs_dec_int_value;
  Boolean signflag_matrix_element;
  double matrix_element_abs_rec;


  // Arrays for coordinates
  double* coordinate_x_rec = NULL;
  double* coordinate_y_rec = NULL;
  double* coordinate_z_rec = NULL;
  
  // Arrays for matrices
  unsigned int* matrix_width_vec = NULL;
  unsigned int* matrix_height_vec = NULL;
  unsigned int* num_matrices_vec = NULL;

  // Matrix element
  double**** matrix_element_rec = NULL;

  // Base picture
  Boolean basePicFlag = FALSE;
  
#ifdef PRINT_GFV_INFO
  printf("FinalizeGFV, start idx %u\n", gfv_idx);
#endif

  // start
  write_ue_v("SEI: gfv_id", sei->gfv_id, bitstream);
  write_ue_v("SEI: gfv_cnt", sei->gfv_cnt, bitstream);

  // GFV base picture
  if (sei->gfv_cnt == 0)
  {
    write_u_1("SEI: gfv_base_picture_flag", sei->gfv_base_pic_flag, bitstream);
    basePicFlag = sei->gfv_base_pic_flag;
  }
  else 
  {
    basePicFlag = FALSE;
  }

#ifdef PRINT_GFV_INFO
  printf("gfv_id = %u\n", sei->gfv_id);
  printf("gfv_cnt = %u\n", sei->gfv_cnt);
  printf("gfv_base_pic_flag = %u\n", basePicFlag);
#endif

  if (basePicFlag)
  {
    write_u_1("SEI: gfv_nn_present_flag", sei->gfv_nn_present_flag, bitstream);
    if (sei->gfv_nn_present_flag)
    {
      write_ue_v("SEI: gfv_mode_idc", sei->gfv_nn_mode_idc, bitstream);

#ifdef PRINT_GFV_INFO
      printf("gfv_nn_mode_idc = %u\n", sei->gfv_nn_mode_idc);
#endif

      // Mode IDC 1 processing, to signal NN
      if (sei->gfv_nn_mode_idc == 1) 
      {
        // Byte alignment
        while (!(bitstream->bits_to_go==8))
        {
          write_u_1("SEI: gfv_nn_alignment_zero_bit_a", 0, bitstream);
        }
        // String fields
        write_string("SEI: gfv_nn_tag_uri", sei->gfv_nn_tag_uri, bitstream);
        write_string("SEI: gfv_nn_uri", sei->gfv_nn_uri, bitstream);

#ifdef PRINT_GFV_INFO
        printf("gfv_nn_tag_uri = %s\n", sei->gfv_nn_tag_uri);
        printf("gfv_nn_uri = %s\n", sei->gfv_nn_uri);
#endif
      }
    }
    
    write_u_1("SEI: gfv_chroma_key_info_present_flag", sei->gfv_chroma_key_info_present_flag, bitstream);
#ifdef PRINT_GFV_INFO
    printf("gfv_chroma_key_info_present_flag = %u\n", sei->gfv_chroma_key_info_present_flag);
#endif
    if (sei->gfv_chroma_key_info_present_flag)
    {
      for (chromac = 0; chromac < 3; chromac++)
      {
        write_u_1("SEI: gfv_chroma_key_value_present_flag[c]", sei->gfv_chroma_key_value_present_flag[chromac], bitstream);
#ifdef PRINT_GFV_INFO
        printf("gfv_chroma_key_value_present_flag[c] = %u\n", sei->gfv_chroma_key_value_present_flag[chromac]);
#endif
        if (sei->gfv_chroma_key_value_present_flag[chromac])
        {
          write_u_v(8, "SEI: gfv_chroma_key_value[chromac]", sei->gfv_chroma_key_value[chromac], bitstream);
#ifdef PRINT_GFV_INFO
          printf("gfv_chroma_key_value[c] = %u\n", sei->gfv_chroma_key_value[chromac]);
#endif
        }
      }
      for (chromai = 0; chromai < 2; chromai++)
      {
        write_u_1("SEI: gfv_chroma_key_thr_present_flag[i]", sei->gfv_chroma_key_thr_present_flag[chromai], bitstream);
#ifdef PRINT_GFV_INFO
        printf("gfv_chroma_key_thr_present_flag[i] = %u\n", sei->gfv_chroma_key_thr_present_flag[chromai]);
#endif
        if (sei->gfv_chroma_key_thr_present_flag[chromai])
        {
          write_ue_v("SEI: gfv_chroma_key_thr_value[i]", sei->gfv_chroma_key_thr_value[chromai], bitstream);
#ifdef PRINT_GFV_INFO
        printf("gfv_chroma_key_thr_value[i] = %u\n", sei->gfv_chroma_key_thr_value[chromai]);
#endif
        }
      }
    }
  }
  else
  {
    // Not base picture, check for drive pic fusion
    write_u_1("SEI: gfv_drive_picture_fusion_flag", sei->gfv_drive_pic_fusion_flag, bitstream);
#ifdef PRINT_GFV_INFO
    printf("gfv_drive_picture_fusion_flag = %u\n", sei->gfv_drive_pic_fusion_flag);
#endif
  }

  // low confidence face
  write_u_1("SEI: gfv_low_confidence_face_parameter_flag", sei->gfv_low_confidence_face_parameter_flag, bitstream);
#ifdef PRINT_GFV_INFO
  printf("gfv_low_confidence_face_parameter_flag = %u\n", sei->gfv_low_confidence_face_parameter_flag);
#endif
  
  // face keypoint coordinates
  write_u_1("SEI: gfv_coordinate_present_flag", sei->gfv_coordinate_present_flag, bitstream);
#ifdef PRINT_GFV_INFO
  printf("gfv_coordinate_present_flag = %u\n", sei->gfv_coordinate_present_flag);
#endif
  if (sei->gfv_coordinate_present_flag)
  {
    write_u_1("SEI: gfv_kps_pred_flag", sei->gfv_coordinate_pred_flag, bitstream);
#ifdef PRINT_GFV_INFO
    printf("gfv_kps_pred_flag = %u\n", sei->gfv_coordinate_pred_flag);
#endif
    if ( basePicFlag || !sei->gfv_coordinate_pred_flag )
    {
      gfv_coordinate_precision_factor_minus1 = sei->gfv_coordinate_quantization_factor - 1;
      // CHECK: The value of gfv_coordinate_precision_factor_minus1 shall be in the range of 0 to 31, inclusive
      assert( gfv_coordinate_precision_factor_minus1 >= 0 && gfv_coordinate_precision_factor_minus1 <= 31 );
      write_ue_v("SEI: gfv_coordinate_precision_factor_minus1", gfv_coordinate_precision_factor_minus1, bitstream);

      gfv_num_kps_minus1 = sei->gfv_coordinate_point_num - 1;
      write_ue_v("SEI: gfv_num_kps_minus1", gfv_num_kps_minus1, bitstream);

      write_u_1("SEI: gfv_coordinate_z_present_flag", sei->gfv_3d_coordinate_flag, bitstream);

#ifdef PRINT_GFV_INFO
      printf("gfv_coordinate_precision_factor_minus1 = %u\n", gfv_coordinate_precision_factor_minus1);
      printf("gfv_num_kps_minus1 = %u\n", gfv_num_kps_minus1);
      printf("gfv_coordinate_z_present_flag = %u\n", sei->gfv_3d_coordinate_flag);
#endif
      if (sei->gfv_3d_coordinate_flag)
      {
        gfv_coordinate_z_max_value_minus1 = sei->gfv_coordinate_z_max_value - 1;
        // CHECK: The value of gfv_coordinate_z_max_value_minus1 shall be in the range of 0 to 2^(16) - 1, inclusive
        assert( gfv_coordinate_z_max_value_minus1 >= 0 && gfv_coordinate_z_max_value_minus1 <= ((1 << 16) - 1) );
        write_ue_v("SEI: gfv_coordinate_z_max_value_minus1", gfv_coordinate_z_max_value_minus1, bitstream);
#ifdef PRINT_GFV_INFO
        printf("gfv_coordinate_z_max_value_minus1 = %u\n", gfv_coordinate_z_max_value_minus1);
#endif
      }
    }

    // Allocate memory for coordinate arrays
    coordinate_x_rec = (double*)malloc(sei->gfv_coordinate_point_num * sizeof(double));
    if (!coordinate_x_rec) no_mem_exit("FinalizeGFV: coordinate_x_rec");
    coordinate_y_rec = (double*)malloc(sei->gfv_coordinate_point_num * sizeof(double));
    if (!coordinate_y_rec) no_mem_exit("FinalizeGFV: coordinate_y_rec");
    coordinate_z_rec = (double*)malloc(sei->gfv_coordinate_point_num * sizeof(double));
    if (!coordinate_z_rec) no_mem_exit("FinalizeGFV: coordinate_z_rec");

    // For base pic, free and reallocate base coordinates
    if ( basePicFlag )
    {
      if (base_coordinate_x_rec) free(base_coordinate_x_rec);
      base_coordinate_x_rec = (double*)malloc(sei->gfv_coordinate_point_num * sizeof(double));
      if (!base_coordinate_x_rec) no_mem_exit("FinalizeGFV: base_coordinate_x_rec");
      
      if (prev_coordinate_x_rec) free(prev_coordinate_x_rec);
      prev_coordinate_x_rec = (double*)malloc(sei->gfv_coordinate_point_num * sizeof(double));
      if (!prev_coordinate_x_rec) no_mem_exit("FinalizeGFV: prev_coordinate_x_rec");
      
      if (base_coordinate_y_rec) free(base_coordinate_y_rec);
      base_coordinate_y_rec = (double*)malloc(sei->gfv_coordinate_point_num * sizeof(double));
      if (!base_coordinate_y_rec) no_mem_exit("FinalizeGFV: base_coordinate_y_rec");

      if (prev_coordinate_y_rec) free(prev_coordinate_y_rec);
      prev_coordinate_y_rec = (double*)malloc(sei->gfv_coordinate_point_num * sizeof(double));
      if (!prev_coordinate_y_rec) no_mem_exit("FinalizeGFV: prev_coordinate_y_rec");

      if (base_coordinate_z_rec) free(base_coordinate_z_rec);
      base_coordinate_z_rec = (double*)malloc(sei->gfv_coordinate_point_num * sizeof(double));
      if (!base_coordinate_z_rec) no_mem_exit("FinalizeGFV: base_coordinate_z_rec");
      
      if (prev_coordinate_z_rec) free(prev_coordinate_z_rec);
      prev_coordinate_z_rec = (double*)malloc(sei->gfv_coordinate_point_num * sizeof(double));
      if (!prev_coordinate_z_rec) no_mem_exit("FinalizeGFV: prev_coordinate_z_rec");
    }

#ifdef PRINT_GFV_INFO
    printf("gfv_coordinate_xyz = ");
#endif
    // X_coordinate_tensor && Y_coordinate_tensor  && Z_coordinate_tensor
    for (i = 0; i < sei->gfv_coordinate_point_num; i++)
    {
      // absolute coords
      if (!sei->gfv_coordinate_pred_flag)
      {
        // X_coordinate_tensor
        cur_coordinate_x_int = (int)(sei->gfv_coordinate_x[i] * (1 << sei->gfv_coordinate_quantization_factor) + 0.5);
        cur_coordinate_x_int_abs = abs(cur_coordinate_x_int);
        write_ue_v("SEI: gfv_coordinate_x_abs[ i ]", cur_coordinate_x_int_abs, bitstream);
        if (cur_coordinate_x_int_abs)
        {
          signflag_x_coord = (cur_coordinate_x_int <= 0) ? TRUE : FALSE;
          write_u_1("SEI: gfv_coordinate_x_sign_flag[ i ]", signflag_x_coord, bitstream);
        }
        coordinate_x_tensor_abs_rec = ((double)cur_coordinate_x_int) / (1 << sei->gfv_coordinate_quantization_factor);
        coordinate_x_rec[i] = coordinate_x_tensor_abs_rec;

        // Y_coordinate_tensor
        cur_coordinate_y_int = (int)(sei->gfv_coordinate_y[i] * (1 << sei->gfv_coordinate_quantization_factor) + 0.5);
        cur_coordinate_y_int_abs = abs(cur_coordinate_y_int);
        write_ue_v("SEI: gfv_coordinate_y_abs[ i ]", cur_coordinate_y_int_abs, bitstream);
        if (cur_coordinate_y_int_abs)
        {
          signflag_y_coord = (cur_coordinate_y_int <= 0) ? TRUE : FALSE;
          write_u_1("SEI: gfv_coordinate_y_sign_flag[ i ]", signflag_y_coord, bitstream);
        }
        coordinate_y_tensor_abs_rec = ((double)cur_coordinate_y_int) / (1 << sei->gfv_coordinate_quantization_factor);
        coordinate_y_rec[i] = coordinate_y_tensor_abs_rec;

        // Z_coordinate_tensor
        if (sei->gfv_3d_coordinate_flag)
        {
          cur_coordinate_z_int = (int)(sei->gfv_coordinate_z[i] * (1 << sei->gfv_coordinate_quantization_factor) + 0.5);
          cur_coordinate_z_int_abs = abs(cur_coordinate_z_int);
          write_ue_v("SEI: gfv_coordinate_z_abs[ i ]", cur_coordinate_z_int_abs, bitstream);
          if (cur_coordinate_z_int_abs)
          {
            signflag_z_coord = (cur_coordinate_z_int <= 0) ? TRUE : FALSE;
            write_u_1("SEI: gfv_coordinate_z_sign_flag[ i ]", signflag_z_coord, bitstream);
          }
          coordinate_z_tensor_abs_rec = ((double)cur_coordinate_z_int) / (1 << sei->gfv_coordinate_quantization_factor);
          coordinate_z_rec[i] = coordinate_z_tensor_abs_rec;
        }
      }
      // inter-frame difference with dx
      else
      {
        // X_coordinate_pred_tensor
        cur_coordinate_x_int = (int)((sei->gfv_coordinate_x[i] - (basePicFlag ? (i == 0 ? 0 : coordinate_x_rec[i - 1]) : (sei->gfv_cnt == 0 ? base_coordinate_x_rec[i] : prev_coordinate_x_rec[i]))) * (1 << sei->gfv_coordinate_quantization_factor) + 0.5);
        coordinate_x_tensor_abs_rec = ((double)cur_coordinate_x_int) / (1 << sei->gfv_coordinate_quantization_factor);
        coordinate_x_rec[i] = coordinate_x_tensor_abs_rec + (basePicFlag ? (i == 0 ? 0 : coordinate_x_rec[i - 1]) : (sei->gfv_cnt == 0 ? base_coordinate_x_rec[i] : prev_coordinate_x_rec[i]));
        cur_coordinate_x_int_abs = abs(cur_coordinate_x_int);
        write_ue_v("SEI: gfv_coordinate_dx_abs[ i ]", cur_coordinate_x_int_abs, bitstream);
        if (cur_coordinate_x_int_abs)
        {
          signflag_x_coord = (cur_coordinate_x_int <= 0) ? TRUE : FALSE;
          write_u_1("SEI: gfv_coordinate_dx_sign_flag[ i ]", signflag_x_coord, bitstream);
        }
        
        // Y_coordinate_pred_tensor
        cur_coordinate_y_int = (int)((sei->gfv_coordinate_y[i] - (basePicFlag ? (i == 0 ? 0 : coordinate_y_rec[i - 1]) : (sei->gfv_cnt == 0 ? base_coordinate_y_rec[i] : prev_coordinate_y_rec[i]))) * (1 << sei->gfv_coordinate_quantization_factor) + 0.5);
        coordinate_y_tensor_abs_rec = ((double)cur_coordinate_y_int) / (1 << sei->gfv_coordinate_quantization_factor);
        coordinate_y_rec[i] = coordinate_y_tensor_abs_rec + (basePicFlag ? (i == 0 ? 0 : coordinate_y_rec[i - 1]) : (sei->gfv_cnt == 0 ? base_coordinate_y_rec[i] : prev_coordinate_y_rec[i]));
        cur_coordinate_y_int_abs = abs(cur_coordinate_y_int);
        write_ue_v("SEI: gfv_coordinate_dy_abs[ i ]", cur_coordinate_y_int_abs, bitstream);
        if (cur_coordinate_y_int_abs)
        {
          signflag_y_coord = (cur_coordinate_y_int <= 0) ? TRUE : FALSE;
          write_u_1("SEI: gfv_coordinate_dy_sign_flag[ i ]", signflag_y_coord, bitstream);
        }

        // Z_coordinate_pred_tensor
        if (sei->gfv_3d_coordinate_flag)
        {
          cur_coordinate_z_int = (int)((sei->gfv_coordinate_z[i] - (basePicFlag ? (i == 0 ? 0 : coordinate_z_rec[i - 1]) : (sei->gfv_cnt == 0 ? base_coordinate_z_rec[i] : prev_coordinate_z_rec[i]))) * (1 << sei->gfv_coordinate_quantization_factor) + 0.5);
          coordinate_z_tensor_abs_rec = ((double)cur_coordinate_z_int) / (1 << sei->gfv_coordinate_quantization_factor);
          coordinate_z_rec[i] = coordinate_z_tensor_abs_rec + (basePicFlag ? (i == 0 ? 0 : coordinate_z_rec[i - 1]) : (sei->gfv_cnt == 0 ? base_coordinate_z_rec[i] : prev_coordinate_z_rec[i]));
          cur_coordinate_z_int_abs = abs(cur_coordinate_z_int);
          write_ue_v("SEI: gfv_coordinate_dz_abs[ i ]", cur_coordinate_z_int_abs, bitstream);
          if (cur_coordinate_z_int_abs)
          {
            signflag_z_coord = (cur_coordinate_z_int <= 0) ? TRUE : FALSE;
            write_u_1("SEI: gfv_coordinate_dz_sign_flag[ i ]", signflag_z_coord, bitstream);
          }
        }
      }
#ifdef PRINT_GFV_INFO
      printf("%d (%d) ", cur_coordinate_x_int_abs, cur_coordinate_y_int_abs);
      if (sei->gfv_3d_coordinate_flag)
      {
        printf("[%d] ", cur_coordinate_z_int_abs);
      }
#endif      
    }
#ifdef PRINT_GFV_INFO
    printf("\n");
#endif

    if (do_update_gfv_coordinate)
    {
      for (i = 0; i < sei->gfv_coordinate_point_num; i++) 
      {
        prev_coordinate_x_rec[i] = coordinate_x_rec[i];
        prev_coordinate_y_rec[i] = coordinate_y_rec[i];
        if (sei->gfv_3d_coordinate_flag == 1) {
          prev_coordinate_z_rec[i] = coordinate_z_rec[i];
        }
      }
      if (sei->gfv_base_pic_flag) 
      {
        for (i = 0; i < sei->gfv_coordinate_point_num; i++) 
        {
          base_coordinate_x_rec[i] = coordinate_x_rec[i];
          base_coordinate_y_rec[i] = coordinate_y_rec[i];
          if (sei->gfv_3d_coordinate_flag == 1) {
            base_coordinate_z_rec[i] = coordinate_z_rec[i];
          }
        }
      }
      do_update_gfv_coordinate = FALSE;
    }
    else 
    {
      do_update_gfv_coordinate = TRUE;
    }
  }

  // Matrix parameters

  // CHECK: When gfv_coordinate_present_flag is equal to 0, gfv_matrix_present_flag shall be equal to 1
  assert( (sei->gfv_coordinate_present_flag) || (sei->gfv_matrix_present_flag) );
  write_u_1("SEI: gfv_matrix_present_flag", sei->gfv_matrix_present_flag, bitstream);
#ifdef PRINT_GFV_INFO
  printf("gfv_matrix_present_flag = %u\n", sei->gfv_matrix_present_flag);
#endif
  if (sei->gfv_matrix_present_flag) 
  {
    if (!basePicFlag)
    {
      write_u_1("SEI: gfv_matrix_pred_flag", sei->gfv_matrix_pred_flag, bitstream);
#ifdef PRINT_GFV_INFO
      printf("gfv_matrix_pred_flag = %u\n", sei->gfv_matrix_pred_flag);
#endif
    }
    
    if (basePicFlag)
    {
      // For base pic, free and reallocate base matrix info
      if (base_num_matrices_vec) free(base_num_matrices_vec);
      base_num_matrices_vec = (unsigned int*)malloc(sei->gfv_num_matrix_type * sizeof(unsigned int));
      if (!base_num_matrices_vec) no_mem_exit("FinalizeGFV: base_num_matrices_vec");

      if (base_matrix_width_vec) free(base_matrix_width_vec);
      base_matrix_width_vec = (unsigned int*)malloc(sei->gfv_num_matrix_type * sizeof(unsigned int));
      if (!base_matrix_width_vec) no_mem_exit("FinalizeGFV: base_matrix_width_vec");
      
      if (base_matrix_height_vec) free(base_matrix_height_vec);
      base_matrix_height_vec = (unsigned int*)malloc(sei->gfv_num_matrix_type * sizeof(unsigned int));
      if (!base_matrix_height_vec) no_mem_exit("FinalizeGFV: base_matrix_height_vec");
    }
    
    if (!sei->gfv_matrix_pred_flag)
    {
      gfv_matrix_element_precision_factor_minus1 = sei->gfv_matrix_element_precision_factor - 1;
      // CHECK: The value of gfv_matrix_element_precision_factor_minus1 shall be in the range of 0 to 31, inclusive
      assert( gfv_matrix_element_precision_factor_minus1 >= 0 && gfv_matrix_element_precision_factor_minus1 <= 31 );
      write_ue_v("SEI: gfv_matrix_element_precision_factor_minus1", gfv_matrix_element_precision_factor_minus1, bitstream);

      gfv_num_matrix_types_minus1 = sei->gfv_num_matrix_type - 1;
      write_ue_v("SEI: gfv_num_matrix_types_minus1", gfv_num_matrix_types_minus1, bitstream);

#ifdef PRINT_GFV_INFO
      printf("gfv_matrix_element_precision_factor_minus1 = %u\n", gfv_matrix_element_precision_factor_minus1);
      printf("gfv_num_matrix_types_minus1 = %u\n", gfv_num_matrix_types_minus1);
#endif
      
      matrix_width_vec = (unsigned int*)malloc(sei->gfv_num_matrix_type * sizeof(unsigned int));
      if (!matrix_width_vec) no_mem_exit("FinalizeGFV: matrix_width_vec");
      matrix_height_vec = (unsigned int*)malloc(sei->gfv_num_matrix_type * sizeof(unsigned int));
      if (!matrix_height_vec) no_mem_exit("FinalizeGFV: matrix_height_vec");
      num_matrices_vec = (unsigned int*)malloc(sei->gfv_num_matrix_type * sizeof(unsigned int));
      if (!num_matrices_vec) no_mem_exit("FinalizeGFV: num_matrices_vec");

      for (matrixId = 0; matrixId < sei->gfv_num_matrix_type; matrixId++)
      {
        // CHECK: The value of gfv_matrix_type_idx shall be in the range of 0 to 63, inclusive
        assert( sei->gfv_matrix_type_idx[matrixId] >= 0 && sei->gfv_matrix_type_idx[matrixId] <= 63 );
        write_u_v(6, "SEI: gfv_matrix_type_idx", sei->gfv_matrix_type_idx[matrixId], bitstream);

        if (sei->gfv_matrix_type_idx[matrixId] == 0 || sei->gfv_matrix_type_idx[matrixId] == 1)
        {
          // CHECK: coordinatePresentFlag shall be 1 when matrix type is 0 or 1
          assert( sei->gfv_coordinate_present_flag == TRUE );
          write_u_1("SEI: gfv_num_matrices_equal_to_num_kps_flag", sei->gfv_num_matrices_to_num_kps_flag[matrixId], bitstream);
          if (!sei->gfv_num_matrices_to_num_kps_flag[matrixId])
          {
            // CHECK: The value of gfv_num_matrices_info shall be in the range of 0 to 2^(10) - 1, inclusive
            assert( sei->gfv_num_matrices_info[matrixId] >= 0 && sei->gfv_num_matrices_info[matrixId] <= (1 << 10) - 1 );
            write_ue_v("SEI: gfv_num_matrices_info", sei->gfv_num_matrices_info[matrixId], bitstream);
          }
        }
        else if (sei->gfv_matrix_type_idx[matrixId] == 2 || sei->gfv_matrix_type_idx[matrixId] == 3 || sei->gfv_matrix_type_idx[matrixId] >= 7)
        {
          if (sei->gfv_matrix_type_idx[matrixId] >= 7)
          {
            gfv_num_matrices_minus1 = sei->gfv_num_matrices[matrixId] - 1;
            // CHECK: The value of gfv_num_matrices_minus1 shall be in the range of 0 to 2^(10) - 1, inclusive
            assert( gfv_num_matrices_minus1 >= 0 && gfv_num_matrices_minus1 <= ((1 << 10) - 1) );
            write_ue_v("SEI: gfv_num_matrices_minus1", gfv_num_matrices_minus1, bitstream);
          }
          gfv_matrix_width_minus1 = sei->gfv_matrix_width[matrixId] - 1;
          // CHECK: The value of gfv_matrix_width_minus1 shall be in the range of 0 to 2^(10) - 1, inclusive
          assert( gfv_matrix_width_minus1 >= 0 && gfv_matrix_width_minus1 <= ((1 << 10) - 1));
          write_ue_v("SEI: gfv_matrix_width_minus1", gfv_matrix_width_minus1, bitstream);

          gfv_matrix_height_minus1 = sei->gfv_matrix_height[matrixId] - 1;
          // CHECK: The value of gfv_matrix_height_minus1 shall be in the range of 0 to 2^(10) - 1, inclusive
          assert( gfv_matrix_height_minus1 >= 0 && gfv_matrix_height_minus1 <= ((1 << 10) - 1) );
          write_ue_v("SEI: gfv_matrix_height_minus1", gfv_matrix_height_minus1, bitstream);
        }
        else if (sei->gfv_matrix_type_idx[matrixId] >= 4 && sei->gfv_matrix_type_idx[matrixId] <= 6)
        {
          if (!sei->gfv_coordinate_present_flag)
          {
            write_u_1("SEI: gfv_Matrix3DSpaceFlag", sei->gfv_matrix_3D_space_flag[matrixId], bitstream);
          }
        }
      }
    }
    // Pred coding for matrix
    else
    {
      // Allocate memory for matrix info
      matrix_width_vec = (unsigned int*)malloc(sei->gfv_num_matrix_type * sizeof(unsigned int));
      if (!matrix_width_vec) no_mem_exit("FinalizeGFV: matrix_width_vec");
      matrix_height_vec = (unsigned int*)malloc(sei->gfv_num_matrix_type * sizeof(unsigned int));
      if (!matrix_height_vec) no_mem_exit("FinalizeGFV: matrix_height_vec");
      num_matrices_vec = (unsigned int*)malloc(sei->gfv_num_matrix_type * sizeof(unsigned int));
      if (!num_matrices_vec) no_mem_exit("FinalizeGFV: num_matrices_vec");
    }
    
    if (sei->gfv_matrix_pred_flag)
    {
      for (matrixId = 0; matrixId < sei->gfv_num_matrix_type; matrixId++)
      {
        num_matrices_vec[matrixId] = base_num_matrices_vec[matrixId];
        matrix_height_vec[matrixId] = base_matrix_height_vec[matrixId];
        matrix_width_vec[matrixId] = base_matrix_width_vec[matrixId];
      }
    }
    else
    {
      for (matrixId = 0; matrixId < sei->gfv_num_matrix_type; matrixId++)
      {
        num_matrices_vec[matrixId] = sei->gfv_num_matrices_store[matrixId];
        matrix_height_vec[matrixId] = sei->gfv_matrix_height_store[matrixId];
        matrix_width_vec[matrixId] = sei->gfv_matrix_width_store[matrixId];
      }
    }

    if (basePicFlag)
    {
      for (matrixId = 0; matrixId < sei->gfv_num_matrix_type; matrixId++) {
          base_num_matrices_vec[matrixId] = num_matrices_vec[matrixId];
          base_matrix_height_vec[matrixId] = matrix_height_vec[matrixId];
          base_matrix_width_vec[matrixId] = matrix_width_vec[matrixId];
        }
    }

#ifdef PRINT_GFV_INFO
    printf("gfv_matrix_type_idx = ");
    for (matrixId = 0; matrixId < sei->gfv_num_matrix_type; matrixId++)
    {
      printf("%u ", sei->gfv_matrix_type_idx[matrixId]);
    }
    printf("\n");

    printf("gfv_num_matrices = ");
    for (matrixId = 0; matrixId < sei->gfv_num_matrix_type; matrixId++)
    {
      printf("%u (%u) ", num_matrices_vec[matrixId], base_num_matrices_vec[matrixId]);
    }
    printf("\n");

    printf("gfv_matrix_width = ");
    for (matrixId = 0; matrixId < sei->gfv_num_matrix_type; matrixId++)
    {
      printf("%u (%u) ", matrix_width_vec[matrixId], base_matrix_width_vec[matrixId]);
    }
    printf("\n");

    printf("gfv_matrix_height = ");
    for (matrixId = 0; matrixId < sei->gfv_num_matrix_type; matrixId++)
    {
      printf("%u (%u) ", matrix_height_vec[matrixId], base_matrix_height_vec[matrixId]);
    }
    printf("\n");
#endif

    // For base pic, free and reallocate base matrix
    if ( basePicFlag ) 
    {
      if ( base_matrix_rec )
      {
        for (matrixId = 0; matrixId < sei->gfv_num_matrix_type; matrixId++)
        {
          for (j = 0; j < base_num_matrices_vec[matrixId]; j++)
          {
            for (k = 0; k < base_matrix_height_vec[matrixId]; k++)
            {
              free(base_matrix_rec[matrixId][j][k]);
            }
            free(base_matrix_rec[matrixId][j]);
          }
          free(base_matrix_rec[matrixId]);
        }
      }

      if ( prev_matrix_rec )
      {
        for (matrixId = 0; matrixId < sei->gfv_num_matrix_type; matrixId++)
        {
          for (j = 0; j < num_matrices_vec[matrixId]; j++)
          {
            for (k = 0; k < matrix_height_vec[matrixId]; k++)
            {
              free(prev_matrix_rec[matrixId][j][k]);
            }
            free(prev_matrix_rec[matrixId][j]);
          }
          free(prev_matrix_rec[matrixId]);
        }
      }

      base_matrix_rec = (double****) calloc((sei->gfv_num_matrix_type), sizeof(double***));
      if (!base_matrix_rec) no_mem_exit("FinalizeGFV: base_matrix_rec");

      for (matrixId = 0; matrixId < sei->gfv_num_matrix_type; matrixId++)
      {
        base_matrix_rec[matrixId] = (double***) calloc((base_num_matrices_vec[matrixId]), sizeof(double**));
        if (!base_matrix_rec[matrixId]) no_mem_exit("FinalizeGFV: base_matrix_rec[matrixId]");

        for (j = 0; j < base_num_matrices_vec[matrixId]; j++)
        {
          base_matrix_rec[matrixId][j] = (double**) calloc((base_matrix_height_vec[matrixId]), sizeof(double*));
          if (!base_matrix_rec[matrixId][j]) no_mem_exit("FinalizeGFV: base_matrix_rec[matrixId][j]");

          for (k = 0; k < base_matrix_height_vec[matrixId]; k++)
          {
            base_matrix_rec[matrixId][j][k] = (double*) calloc((base_matrix_width_vec[matrixId]), sizeof(double));
            if (!base_matrix_rec[matrixId][j][k]) no_mem_exit("FinalizeGFV: base_matrix_rec[matrixId][j][k]");
          }
        }
      }

      prev_matrix_rec = (double****) calloc((sei->gfv_num_matrix_type), sizeof(double***));
      if (!prev_matrix_rec) no_mem_exit("FinalizeGFV: prev_matrix_rec");

      for (matrixId = 0; matrixId < sei->gfv_num_matrix_type; matrixId++)
      {
        prev_matrix_rec[matrixId] = (double***) calloc((num_matrices_vec[matrixId]), sizeof(double**));
        if (!prev_matrix_rec[matrixId]) no_mem_exit("FinalizeGFV: prev_matrix[matrixId]");

        for (j = 0; j < num_matrices_vec[matrixId]; j++)
        {
          prev_matrix_rec[matrixId][j] = (double**) calloc((matrix_height_vec[matrixId]), sizeof(double*));
          if (!prev_matrix_rec[matrixId][j]) no_mem_exit("FinalizeGFV: prev_matrix[matrixId][j]");

          for (k = 0; k < matrix_height_vec[matrixId]; k++)
          {
            prev_matrix_rec[matrixId][j][k] = (double*) calloc((matrix_width_vec[matrixId]), sizeof(double));
            if (!prev_matrix_rec[matrixId][j][k]) no_mem_exit("FinalizeGFV: prev_matrix_rec[matrixId][j][k]");
          }
        }
      }
    }

    // Allocate 4D matrix element array
    matrix_element_rec = (double****) calloc((sei->gfv_num_matrix_type), sizeof(double***));
    if (!matrix_element_rec) printf("FinalizeGFV: matrix_element_rec");
#ifdef PRINT_GFV_INFO
    printf("matrix_element_rec = ");
#endif
    for (matrixId = 0; matrixId < sei->gfv_num_matrix_type; matrixId++)
    {
      matrix_element_rec[matrixId] = (double***) calloc((num_matrices_vec[matrixId]), sizeof(double**));
      if (!matrix_element_rec[matrixId]) no_mem_exit("FinalizeGFV: matrix_element_rec[matrixId]");

      for (j = 0; j < num_matrices_vec[matrixId]; j++)
      {
        matrix_element_rec[matrixId][j] = (double**) calloc((matrix_height_vec[matrixId]), sizeof(double*));
        if (!matrix_element_rec[matrixId][j]) no_mem_exit("FinalizeGFV: matrix_element_rec[matrixId][j]");

        for (k = 0; k < matrix_height_vec[matrixId]; k++)
        {
          matrix_element_rec[matrixId][j][k] = (double*) calloc((matrix_width_vec[matrixId]), sizeof(double));
          if (!matrix_element_rec[matrixId][j][k]) no_mem_exit("FinalizeGFV: matrix_element_rec[matrixId][j][k]");

          for (l = 0; l < matrix_width_vec[matrixId]; l++)
          {
            // absolute matrix
            if (!sei->gfv_matrix_pred_flag)
            {
              cur_matrix_element_abs = fabs(sei->gfv_matrix_element[matrixId][j][k][l]);
              cur_matrix_element_abs_int = (int)(cur_matrix_element_abs);
              // CHECK: The value of gfv_matrix_element_int[ i ][ j ][ k ][ m ] shall be in the range of 0 to 2^(32) - 2, inclusive
              assert( cur_matrix_element_abs_int >= 0 && (unsigned int)cur_matrix_element_abs_int <= (4294967296 - 2) );
              write_ue_v("SEI: gfv_matrix_element_int", cur_matrix_element_abs_int, bitstream);

              cur_matrix_element_abs_decimal = cur_matrix_element_abs - cur_matrix_element_abs_int;
              assert( cur_matrix_element_abs_decimal >= 0 );
              cur_matrix_element_abs_dec_int_value = iClip3(0, (1 << sei->gfv_matrix_element_precision_factor) - 1, (int)(cur_matrix_element_abs_decimal * (1 << sei->gfv_matrix_element_precision_factor) + 0.5));
              write_u_v(sei->gfv_matrix_element_precision_factor, "SEI: gfv_matrix_element_dec", cur_matrix_element_abs_dec_int_value, bitstream);

              signflag_matrix_element = (sei->gfv_matrix_element[matrixId][j][k][l] < 0) ? TRUE : FALSE;
              if (cur_matrix_element_abs_int || cur_matrix_element_abs_dec_int_value)
              {
                write_u_1("SEI: gfv_matrix_element_sign_flag", signflag_matrix_element, bitstream);
              }

              matrix_element_abs_rec = (double)(cur_matrix_element_abs_int + (((double)cur_matrix_element_abs_dec_int_value) / (1 << sei->gfv_matrix_element_precision_factor)));
              matrix_element_rec[matrixId][j][k][l] = signflag_matrix_element ? -matrix_element_abs_rec : matrix_element_abs_rec;
            }
            // inter-frame difference
            else
            {
              cur_matrix_element_abs = fabs(sei->gfv_matrix_element[matrixId][j][k][l] - (sei->gfv_cnt== 0 ? base_matrix_rec[matrixId][j][k][l] : prev_matrix_rec[matrixId][j][k][l]));
              cur_matrix_element_abs_int = (int)(cur_matrix_element_abs);
              write_ue_v("SEI: gfv_matrix_delta_element_int", cur_matrix_element_abs_int, bitstream);

              cur_matrix_element_abs_decimal = cur_matrix_element_abs - cur_matrix_element_abs_int;
              assert( cur_matrix_element_abs_decimal >= 0 );
              cur_matrix_element_abs_dec_int_value = (int)(cur_matrix_element_abs_decimal * (1 << sei->gfv_matrix_element_precision_factor) + 0.5);
              write_ue_v("SEI: gfv_matrix_delta_element_dec", cur_matrix_element_abs_dec_int_value, bitstream);

              signflag_matrix_element = ((sei->gfv_matrix_element[matrixId][j][k][l] - (sei->gfv_cnt == 0 ? base_matrix_rec[matrixId][j][k][l] : prev_matrix_rec[matrixId][j][k][l])) < 0) ? TRUE : FALSE;
              if (cur_matrix_element_abs_int || cur_matrix_element_abs_dec_int_value)
              {
                write_u_1("SEI: gfv_matrix_delta_element_sign_flag", signflag_matrix_element, bitstream);
              }

              matrix_element_abs_rec = (double)(cur_matrix_element_abs_int + (((double)cur_matrix_element_abs_dec_int_value) / (1 << sei->gfv_matrix_element_precision_factor)));
              matrix_element_rec[matrixId][j][k][l] = ((signflag_matrix_element ? -matrix_element_abs_rec : matrix_element_abs_rec) + (sei->gfv_cnt == 0 ? base_matrix_rec[matrixId][j][k][l] : prev_matrix_rec[matrixId][j][k][l]));
            }
#ifdef PRINT_GFV_INFO
            printf("%d (%d) ", cur_matrix_element_abs_int, cur_matrix_element_abs_dec_int_value);
#endif
          }
        }
      }
    }
#ifdef PRINT_GFV_INFO
    printf("\n");
#endif

    if (do_update_gfv_matrix)
    {
      for (matrixId = 0; matrixId < sei->gfv_num_matrix_type; matrixId++) 
      {
        for (j = 0; j < num_matrices_vec[matrixId]; j++) 
        {
          for (k = 0; k < matrix_height_vec[matrixId]; k++) 
          {
            for (l = 0; l < matrix_width_vec[matrixId]; l++) 
            {
              prev_matrix_rec[matrixId][j][k][l] = matrix_element_rec[matrixId][j][k][l];
              if (basePicFlag) 
              {
                base_matrix_rec[matrixId][j][k][l] = matrix_element_rec[matrixId][j][k][l];
              }
            }
          }
        }
      }
      if (basePicFlag)
      {
        for (matrixId = 0; matrixId < sei->gfv_num_matrix_type; matrixId++) {
          base_num_matrices_vec[matrixId] = num_matrices_vec[matrixId];
          base_matrix_height_vec[matrixId] = matrix_height_vec[matrixId];
          base_matrix_width_vec[matrixId] = matrix_width_vec[matrixId];
        }
      }
      do_update_gfv_matrix = FALSE;
    }
    else
    {
      do_update_gfv_matrix = TRUE;
    }
  }

  if (sei->gfv_nn_present_flag)
  {
    if (sei->gfv_nn_mode_idc == 0)
    {
      // Byte alignment
      while (!(bitstream->bits_to_go==8))
      {
        write_u_1("SEI: gfv_nn_alignment_zero_bit_b", 0, bitstream);
      }
      for (unsigned long p = 0; p < sei->gfv_payload_length; p++)
      {
        write_u_v(8, "SEI: gfv_nn_payload_byte[i]", sei->gfv_payload_byte[p], bitstream);
      }
    }
  }

  // make sure the payload is byte aligned, stuff bits are 10..0
  if ( bitstream->bits_to_go != 8 )
  {
    (bitstream->byte_buf) <<= 1;
    bitstream->byte_buf |= 1;
    bitstream->bits_to_go--;
    if ( bitstream->bits_to_go != 0 ) 
      (bitstream->byte_buf) <<= (bitstream->bits_to_go);
    bitstream->bits_to_go = 8;
    bitstream->streamBuffer[bitstream->byte_pos++]=bitstream->byte_buf;
    bitstream->byte_buf = 0;
  }
  sei->payloadSize = bitstream->byte_pos;

  // Free allocated memory
  if (matrix_element_rec) 
  {
    for (matrixId = 0; matrixId < sei->gfv_num_matrix_type; matrixId++)
    {
      if (matrix_element_rec[matrixId])
      {
        for (j = 0; j < num_matrices_vec[matrixId]; j++)
        {
          if (matrix_element_rec[matrixId][j])
          {
            for (k = 0; k < matrix_height_vec[matrixId]; k++)
            {
              if (matrix_element_rec[matrixId][j][k]) 
              {
                free(matrix_element_rec[matrixId][j][k]);
              }
            }
            free(matrix_element_rec[matrixId][j]);
          }
        }
        free(matrix_element_rec[matrixId]);
      }
    }
    free(matrix_element_rec);
  }

  if (coordinate_x_rec) free(coordinate_x_rec);
  if (coordinate_y_rec) free(coordinate_y_rec);
  if (coordinate_z_rec) free(coordinate_z_rec);
  
  if (matrix_width_vec) free(matrix_width_vec);
  if (matrix_height_vec) free(matrix_height_vec);
  if (num_matrices_vec) free(num_matrices_vec);

#ifdef PRINT_GFV_INFO
  printf("FinalizeGFV, finish idx %u\n", gfv_idx);
#endif
}


void UpdateGFV(SEIParameters *p_SEI, InputParameters *p_Inp) 
{
#ifdef PRINT_GFV_INFO
  printf("UpdateGFV, start\n");
#endif

  // uncomment to update SEI to another configuration
  // sprintf(p_Inp->GFVFile, "cfg/generative_face_video3.cfg");

  // read GFV config from file
  ParseGFVConfigFile(p_SEI, p_Inp, p_SEI->seiGFV);
  
  // all initialized, set GFV presence to true
  p_SEI->seiHasGFV_info = TRUE;

#ifdef PRINT_GFV_INFO
    printf("UpdateGFV, finish\n");
#endif
}
#endif

#if JVET_AK0239_GFVE_SEI
/*
 ************************************************************************
 *  \functions on Generative Face Video Enhancement (GFVE) SEI message
 *  \brief
 *    Based on JVET-AK0239
 *  \author
 *    Jing Yuan Thong                  <jingyuan.thong@sg.panasonic.com>
 ************************************************************************
 */

static int ParseGFVEConfigFile(SEIParameters *p_SEI, InputParameters *p_Inp, generative_face_video_enhancement_struct *pSeiGFVE)
{
  // loop counters
  unsigned int gfve_idx;
  unsigned int i, j, k;

  // array buffers
  int ret;
  FILE* fp;
  char buf[4096];
  char temp[4096];
  memset(temp, 0, sizeof(temp));
  
  // tmp int and double for reading from cfg file
  unsigned int tmp_int;
  double tmp_double;

  // for matrix elements
  unsigned int matrixWidth = 0;
  unsigned int matrixHeight = 0;
  unsigned int numMatrices = 0;

  printf ("Parsing GFVE cfg file %s ..........\n\n", p_Inp->GFVEFile);
  if ((fp = fopen(p_Inp->GFVEFile, "r")) == NULL) 
  {
    fprintf(stderr, "GFVE config file %s is not found, disable GFVE SEI\n", p_Inp->GFVEFile);
    p_SEI->seiHasGFVE_info = FALSE;
    p_SEI->gfve_num_sei = 0;
    return 1;
  }
  
  //read the GFVE config file
  while (fscanf(fp, "%s", buf) != EOF) 
  {
    ret = 1;
    if (strcmp(buf, "gfve_number") == 0) 
    {
      ret = fscanf(fp, " = %u\n", &(p_SEI->gfve_num_sei));
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
      {
        pSeiGFVE[gfve_idx].gfve_number = p_SEI->gfve_num_sei;
        pSeiGFVE[gfve_idx].gfve_current_id = gfve_idx;
      }
#ifdef PRINT_GFVE_INFO
      printf("gfve_number = %u\n", pSeiGFVE[0].gfve_number);
      printf("ret = %u\n", ret);
#endif
    }
    else if (strcmp(buf, "gfve_base_pic_flag") == 0) 
    {
      ret = fscanf(fp, " = %u\n", &tmp_int);
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
      {
        pSeiGFVE[gfve_idx].gfve_base_pic_flag = tmp_int ? TRUE : FALSE;
      }
#ifdef PRINT_GFVE_INFO
      printf("gfve_base_pic_flag = %u\n", tmp_int);
      printf("ret = %u\n", ret);
#endif
    }
    else if (strcmp(buf, "gfve_nn_present_flag") == 0) 
    {
      ret = fscanf(fp, " = %u\n", &tmp_int);
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
      {
        pSeiGFVE[gfve_idx].gfve_nn_present_flag = tmp_int ? TRUE : FALSE;
      }
#ifdef PRINT_GFVE_INFO
      printf("gfve_nn_present_flag = %u\n", tmp_int);
      printf("ret = %u\n", ret);
#endif
    }
    else if (strcmp(buf, "gfve_nn_mode_idc") == 0) {
      ret = fscanf(fp, " = %u\n", &tmp_int);
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
      {
        pSeiGFVE[gfve_idx].gfve_nn_mode_idc = tmp_int;
      }
#ifdef PRINT_GFVE_INFO
      printf("gfve_nn_mode_idc = %u\n", tmp_int);
      printf("ret = %u\n", ret);
#endif
    }
    else if (strcmp(buf, "gfve_nn_tag_uri") == 0) {
      ret = fscanf(fp, " = %s\n", temp);
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
      {
        // prevent buffer overflow
        strncpy(pSeiGFVE[gfve_idx].gfve_nn_tag_uri, temp, sizeof(pSeiGFVE[gfve_idx].gfve_nn_tag_uri) - 1);
        pSeiGFVE[gfve_idx].gfve_nn_tag_uri[sizeof(pSeiGFVE[gfve_idx].gfve_nn_tag_uri) - 1] = '\0';
      }
#ifdef PRINT_GFVE_INFO
      printf("gfve_nn_tag_uri = %s\n", temp);
      printf("ret = %u\n", ret);
#endif
      memset(temp, 0, sizeof(temp));
    }
    else if (strcmp(buf, "gfve_nn_uri") == 0) {
      ret = fscanf(fp, " = %s\n", temp);
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
      {
        // prevent buffer overflow
        strncpy(pSeiGFVE[gfve_idx].gfve_nn_uri, temp, sizeof(pSeiGFVE[gfve_idx].gfve_nn_uri) - 1);
        pSeiGFVE[gfve_idx].gfve_nn_uri[sizeof(pSeiGFVE[gfve_idx].gfve_nn_uri) - 1] = '\0';
      }
#ifdef PRINT_GFVE_INFO
      printf("gfve_nn_uri = %s\n", temp);
      printf("ret = %u\n", ret);
#endif
      memset(temp, 0, sizeof(temp));
    }
    else if (strcmp(buf, "gfve_payload_filename") == 0) 
    {
      ret = fscanf(fp, " = %4095[^\n]\n", temp);
#ifdef PRINT_GFVE_INFO
      printf("gfve_payload_filename = %s\n", temp);
#endif
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++) 
      {
        if (pSeiGFVE[gfve_idx].gfve_nn_present_flag)
        {
          if (pSeiGFVE[gfve_idx].gfve_nn_mode_idc == 0)
          {
            FILE* file;
            if ((file = fopen(temp, "rb")) == NULL) 
            {
              fprintf(stderr, "GFVE NN payload file %s is not found.\n", temp);
              return 1;
            }
  
            if (fseek(file, 0, SEEK_END) != 0) {
              fclose(file);
              return -1;
            }
  
            long payload_size = ftell(file);
            if (payload_size == -1) {
              fclose(file);
              return -1;
            }
            rewind(file);

            pSeiGFVE[gfve_idx].gfve_payload_length = payload_size;
  
            pSeiGFVE[gfve_idx].gfve_payload_byte = malloc(payload_size);
            if (!pSeiGFVE[gfve_idx].gfve_payload_byte) {
              no_mem_exit("ParseGFVEConfigFile: alloc gfve_payload_byte");
            }

            size_t bytes_read = fread(pSeiGFVE[gfve_idx].gfve_payload_byte, 1, payload_size, file);
            if (bytes_read != (size_t)payload_size) {
                free(pSeiGFVE[gfve_idx].gfve_payload_byte);
                pSeiGFVE[gfve_idx].gfve_payload_byte = NULL;
                fclose(file);
                return -1;
            }

            fclose(file);
          }
        }
      }
#ifdef PRINT_GFVE_INFO     
      printf("ret = %u\n", ret);
#endif
    }
    else if (strcmp(buf, "gfve_id") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVEConfigFile: error parsing gfve_id", 500);
      }
      ret = 1;
#ifdef PRINT_GFVE_INFO
      printf("gfve_id = ");
#endif
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
      {
        ret &= fscanf(fp, "%u", &(pSeiGFVE[gfve_idx].gfve_id));
#ifdef PRINT_GFVE_INFO
        printf("%u ", pSeiGFVE[gfve_idx].gfve_id);
#endif
      }
#ifdef PRINT_GFVE_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfve_gfv_id") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVEConfigFile: error parsing gfve_gfv_id", 500);
      }
      ret = 1;
#ifdef PRINT_GFVE_INFO
      printf("gfve_gfv_id = ");
#endif
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
      {
        ret &= fscanf(fp, "%u", &(pSeiGFVE[gfve_idx].gfve_gfv_id));
#ifdef PRINT_GFVE_INFO
        printf("%u ", pSeiGFVE[gfve_idx].gfve_gfv_id);
#endif
      }
#ifdef PRINT_GFVE_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfve_gfv_cnt") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVEConfigFile: error parsing gfve_gfv_cnt", 500);
      }
      ret = 1;
#ifdef PRINT_GFVE_INFO
      printf("gfve_gfv_cnt = ");
#endif
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
      {
        ret &= fscanf(fp, "%u", &(pSeiGFVE[gfve_idx].gfve_gfv_cnt));
#ifdef PRINT_GFVE_INFO
        printf("%u ", pSeiGFVE[gfve_idx].gfve_gfv_cnt);
#endif
      }
#ifdef PRINT_GFVE_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfve_matrix_present_flag") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVEConfigFile: error parsing gfve_matrix_present_flag", 500);
      }
      ret = 1;
#ifdef PRINT_GFVE_INFO
      printf("gfve_matrix_present_flag = ");
#endif
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
      {
        ret &= fscanf(fp, "%u", &tmp_int);
        pSeiGFVE[gfve_idx].gfve_matrix_present_flag = tmp_int ? TRUE : FALSE;
#ifdef PRINT_GFVE_INFO
        printf("%u ", pSeiGFVE[gfve_idx].gfve_matrix_present_flag);
#endif
      }
#ifdef PRINT_GFVE_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfve_matrix_pred_flag") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVEConfigFile: error parsing gfve_matrix_pred_flag", 500);
      }
      ret = 1;
#ifdef PRINT_GFVE_INFO
      printf("gfve_matrix_pred_flag = ");
#endif
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
      {
        ret &= fscanf(fp, "%u", &tmp_int);
        pSeiGFVE[gfve_idx].gfve_matrix_pred_flag = tmp_int ? TRUE : FALSE;
#ifdef PRINT_GFVE_INFO
        printf("%u ", pSeiGFVE[gfve_idx].gfve_matrix_pred_flag);
#endif
      }
#ifdef PRINT_GFVE_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfve_matrix_element_precision_factor") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVEConfigFile: error parsing gfve_matrix_element_precision_factor", 500);
      }
      ret = 1;
#ifdef PRINT_GFVE_INFO
      printf("gfve_matrix_element_precision_factor = ");
#endif
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
      {
        ret &= fscanf(fp, "%u", &(pSeiGFVE[gfve_idx].gfve_matrix_element_precision_factor));
#ifdef PRINT_GFVE_INFO
        printf("%u ", pSeiGFVE[gfve_idx].gfve_matrix_element_precision_factor);
#endif
      }
#ifdef PRINT_GFVE_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfve_num_matrices") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVEConfigFile: error parsing gfve_num_matrices", 500);
      }
      ret = 1;
#ifdef PRINT_GFVE_INFO
      printf("gfve_num_matrices = ");
#endif
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
      {
        ret &= fscanf(fp, "%u", &(pSeiGFVE[gfve_idx].gfve_num_matrices));
#ifdef PRINT_GFVE_INFO
        printf("%u ", pSeiGFVE[gfve_idx].gfve_num_matrices);
#endif
        if (pSeiGFVE[gfve_idx].gfve_matrix_present_flag && pSeiGFVE[gfve_idx].gfve_num_matrices > 0)
        {
          pSeiGFVE[gfve_idx].gfve_matrix_width = calloc((pSeiGFVE[gfve_idx].gfve_num_matrices), sizeof(unsigned int));
          if (!pSeiGFVE[gfve_idx].gfve_matrix_width) {
            no_mem_exit("ParseGFVEConfigFile: alloc gfve_matrix_width");
          }
          pSeiGFVE[gfve_idx].gfve_matrix_height = calloc((pSeiGFVE[gfve_idx].gfve_num_matrices), sizeof(unsigned int));
          if (!pSeiGFVE[gfve_idx].gfve_matrix_height) {
            no_mem_exit("ParseGFVEConfigFile: alloc gfve_matrix_height");
          }
          pSeiGFVE[gfve_idx].gfve_matrix_element = (double***) calloc((pSeiGFVE[gfve_idx].gfve_num_matrices), sizeof(double**));
          if (!pSeiGFVE[gfve_idx].gfve_matrix_element) {
            no_mem_exit("ParseGFVEConfigFile: alloc gfve_matrix_element");
          }
        }
      }
#ifdef PRINT_GFVE_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfve_matrix_width") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVEConfigFile: error parsing gfve_matrix_width", 500);
      }
      ret = 1;
#ifdef PRINT_GFVE_INFO
      printf("gfve_matrix_width = ");
#endif
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
      {
        if (pSeiGFVE[gfve_idx].gfve_matrix_present_flag)
        {
          for (i = 0; i < pSeiGFVE[gfve_idx].gfve_num_matrices; i++)
          {
            ret &= fscanf(fp, "%u", &tmp_int);
            pSeiGFVE[gfve_idx].gfve_matrix_width[i] = tmp_int;
#ifdef PRINT_GFVE_INFO
            printf("%u ", pSeiGFVE[gfve_idx].gfve_matrix_width[i]);
#endif
          }
        }
      }
#ifdef PRINT_GFVE_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfve_matrix_height") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVEConfigFile: error parsing gfve_matrix_height", 500);
      }
      ret = 1;
#ifdef PRINT_GFVE_INFO
      printf("gfve_matrix_height = ");
#endif
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
      {
        if (pSeiGFVE[gfve_idx].gfve_matrix_present_flag)
        {
          for (i = 0; i < pSeiGFVE[gfve_idx].gfve_num_matrices; i++)
          {
            ret &= fscanf(fp, "%u", &tmp_int);
            pSeiGFVE[gfve_idx].gfve_matrix_height[i] = tmp_int;
#ifdef PRINT_GFV_INFO
            printf("%u ", pSeiGFVE[gfve_idx].gfve_matrix_height[i]);
#endif
          }
        }
      }
#ifdef PRINT_GFVE_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfve_matrix_element") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVEConfigFile: error parsing gfve_matrix_element", 500);
      }
      ret = 1;
#ifdef PRINT_GFVE_INFO
      printf("gfve_matrix_element = ");
#endif
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
      {
        if (pSeiGFVE[gfve_idx].gfve_matrix_present_flag)
        {
          numMatrices = pSeiGFVE[gfve_idx].gfve_num_matrices;
          // over each matrix
          for (i = 0; i < numMatrices; i++)
          {
            matrixHeight = pSeiGFVE[gfve_idx].gfve_matrix_height[i];
            matrixWidth = pSeiGFVE[gfve_idx].gfve_matrix_width[i];
            
            // malloc for matrixHeight
            pSeiGFVE[gfve_idx].gfve_matrix_element[i] = (double**) calloc((matrixHeight), sizeof(double*));
            if (!pSeiGFVE[gfve_idx].gfve_matrix_element[i]) {
              no_mem_exit("ParseGFVEConfigFile: alloc gfve_matrix_element, matrixHeight");
            }

            // over matrixHeight
            for (j = 0; j < matrixHeight; j++)
            {
              // malloc for matrixWidth
              pSeiGFVE[gfve_idx].gfve_matrix_element[i][j] = (double*) calloc((matrixWidth), sizeof(double));
              if (!pSeiGFVE[gfve_idx].gfve_matrix_element[i][j]) {
                no_mem_exit("ParseGFVEConfigFile: alloc gfve_matrix_element, matrixWidth");
              }

              // over matrixWidth
              for (k = 0; k < matrixWidth; k++)
              {
                ret &= fscanf(fp, "%lf", &tmp_double);
                pSeiGFVE[gfve_idx].gfve_matrix_element[i][j][k] = tmp_double;
#ifdef PRINT_GFVE_INFO
                printf("%lf ", pSeiGFVE[gfve_idx].gfve_matrix_element[i][j][k]);
#endif
              }
            }
          }
        }
      }
#ifdef PRINT_GFVE_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfve_pupil_present_idx") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVEConfigFile: error parsing gfve_pupil_present_idx", 500);
      }
      ret = 1;
#ifdef PRINT_GFVE_INFO
      printf("gfve_pupil_present_idx = ");
#endif
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
      {
        ret &= fscanf(fp, "%u", &(pSeiGFVE[gfve_idx].gfve_pupil_present_idx));
#ifdef PRINT_GFVE_INFO
        printf("%u ", pSeiGFVE[gfve_idx].gfve_pupil_present_idx);
#endif
      }
#ifdef PRINT_GFVE_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfve_pupil_coordinate_precision_factor") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVEConfigFile: error parsing gfve_pupil_coordinate_precision_factor", 500);
      }
      ret = 1;
#ifdef PRINT_GFVE_INFO
      printf("gfve_pupil_coordinate_precision_factor = ");
#endif
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
      {
        ret &= fscanf(fp, "%u", &(pSeiGFVE[gfve_idx].gfve_pupil_coordinate_precision_factor));
#ifdef PRINT_GFVE_INFO
        printf("%u ", pSeiGFVE[gfve_idx].gfve_pupil_coordinate_precision_factor);
#endif
      }
#ifdef PRINT_GFVE_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfve_pupil_left_eye_coordinate_x") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVEConfigFile: error parsing gfve_pupil_left_eye_coordinate_x", 500);
      }
      ret = 1;
#ifdef PRINT_GFVE_INFO
      printf("gfve_pupil_left_eye_coordinate_x = ");
#endif
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
      {
        ret &= fscanf(fp, "%lf", &(pSeiGFVE[gfve_idx].gfve_pupil_left_eye_coordinate_x));
#ifdef PRINT_GFVE_INFO
        printf("%lf ", pSeiGFVE[gfve_idx].gfve_pupil_left_eye_coordinate_x);
#endif
      }
#ifdef PRINT_GFVE_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfve_pupil_left_eye_coordinate_y") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVEConfigFile: error parsing gfve_pupil_left_eye_coordinate_y", 500);
      }
      ret = 1;
#ifdef PRINT_GFVE_INFO
      printf("gfve_pupil_left_eye_coordinate_y = ");
#endif
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
      {
        ret &= fscanf(fp, "%lf", &(pSeiGFVE[gfve_idx].gfve_pupil_left_eye_coordinate_y));
#ifdef PRINT_GFVE_INFO
        printf("%lf ", pSeiGFVE[gfve_idx].gfve_pupil_left_eye_coordinate_y);
#endif
      }
#ifdef PRINT_GFVE_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfve_pupil_right_eye_coordinate_x") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVEConfigFile: error parsing gfve_pupil_right_eye_coordinate_x", 500);
      }
      ret = 1;
#ifdef PRINT_GFVE_INFO
      printf("gfve_pupil_right_eye_coordinate_x = ");
#endif
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
      {
        ret &= fscanf(fp, "%lf", &(pSeiGFVE[gfve_idx].gfve_pupil_right_eye_coordinate_x));
#ifdef PRINT_GFVE_INFO
        printf("%lf ", pSeiGFVE[gfve_idx].gfve_pupil_right_eye_coordinate_x);
#endif
      }
#ifdef PRINT_GFVE_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }
    else if (strcmp(buf, "gfve_pupil_right_eye_coordinate_y") == 0) 
    {
      ret = fscanf(fp, " = ");
      if (ret != 0)
      {
        error ("ParseGFVEConfigFile: error parsing gfve_pupil_right_eye_coordinate_y", 500);
      }
      ret = 1;
#ifdef PRINT_GFVE_INFO
      printf("gfve_pupil_right_eye_coordinate_y = ");
#endif
      for (gfve_idx = 0; gfve_idx < p_SEI->gfve_num_sei; gfve_idx++)
      {
        ret &= fscanf(fp, "%lf", &(pSeiGFVE[gfve_idx].gfve_pupil_right_eye_coordinate_y));
#ifdef PRINT_GFVE_INFO
        printf("%lf ", pSeiGFVE[gfve_idx].gfve_pupil_right_eye_coordinate_y);
#endif
      }
#ifdef PRINT_GFVE_INFO
      printf("\n");
      printf("ret = %u\n", ret);
#endif
      ret = (ret == 1);
    }

    else
    {
      // read till the line end 
      if (NULL == fgets(buf, sizeof(buf), fp))
      {
        error ("ParseGFVConfigFile: error parsing GFVE config file, fgets returns null",500);
      }
    }
    if (ret != 1)
    {
      error ("ParseGFVConfigFile: error parsing GFVE config file, format error",500);
    }
  }

  fclose(fp);

  return 0;
}

static void InitGFVE(SEIParameters *p_SEI, InputParameters *p_Inp) 
{
  if (p_Inp->GFVESEIPresentFlag == 0)
  {
    p_SEI->seiHasGFVE_info = FALSE;
    p_SEI->gfve_num_sei = 0;
    return;
  }

#ifdef PRINT_GFVE_INFO
  printf("InitGFVE, start\n");
#endif
  
  for (int gfve_idx = 0; gfve_idx < MAX_NUM_GFV_CNT; gfve_idx++)
  {
    p_SEI->seiGFVE[gfve_idx].data = malloc( sizeof(Bitstream) );
    if( p_SEI->seiGFVE[gfve_idx].data == NULL ) no_mem_exit("InitGFVE: seiGFVE[i].data");
    p_SEI->seiGFVE[gfve_idx].data->streamBuffer = malloc(MAXRTPPAYLOADLEN);
    if( p_SEI->seiGFVE[gfve_idx].data->streamBuffer == NULL ) no_mem_exit("InitGFVE: seiGFVE[i].data->streamBuffer");
  }
  
  ClearGFVE(p_SEI);

  // read GFVE config from file
  ParseGFVEConfigFile(p_SEI, p_Inp, p_SEI->seiGFVE);
  
  // all initialized, set GFVE presence to true
  p_SEI->seiHasGFVE_info = TRUE;

#ifdef PRINT_GFVE_INFO
  printf("InitGFVE, finish with %u SEI\n", p_SEI->gfve_num_sei);
#endif
}

static void ClearGFVE(SEIParameters *p_SEI) 
{
#ifdef PRINT_GFVE_INFO
  printf("ClearGFVE, start\n");
#endif
  for (int gfve_idx = 0; gfve_idx < MAX_NUM_GFV_CNT; gfve_idx++)
  {
    memset( p_SEI->seiGFVE[gfve_idx].data->streamBuffer, 0, MAXRTPPAYLOADLEN);
    p_SEI->seiGFVE[gfve_idx].data->bits_to_go  = 8;
    p_SEI->seiGFVE[gfve_idx].data->byte_pos    = 0;
    p_SEI->seiGFVE[gfve_idx].data->byte_buf    = 0;
    p_SEI->seiGFVE[gfve_idx].payloadSize       = 0;

    
    p_SEI->seiGFVE[gfve_idx].gfve_number = 0;
    p_SEI->seiGFVE[gfve_idx].gfve_current_id = 0;
    p_SEI->seiGFVE[gfve_idx].gfve_base_pic_flag = TRUE;
    p_SEI->seiGFVE[gfve_idx].gfve_nn_present_flag = FALSE;
    p_SEI->seiGFVE[gfve_idx].gfve_nn_mode_idc = 1;
    memset(p_SEI->seiGFVE[gfve_idx].gfve_nn_tag_uri, 0, sizeof(p_SEI->seiGFVE[gfve_idx].gfve_nn_tag_uri));
    memset(p_SEI->seiGFVE[gfve_idx].gfve_nn_uri, 0, sizeof(p_SEI->seiGFVE[gfve_idx].gfve_nn_uri));
   
    p_SEI->seiGFVE[gfve_idx].gfve_id = 0;
    p_SEI->seiGFVE[gfve_idx].gfve_gfv_id = 0;
    p_SEI->seiGFVE[gfve_idx].gfve_gfv_cnt = 0;

    p_SEI->seiGFVE[gfve_idx].gfve_matrix_present_flag = FALSE;
    p_SEI->seiGFVE[gfve_idx].gfve_matrix_pred_flag = FALSE;
    p_SEI->seiGFVE[gfve_idx].gfve_matrix_element_precision_factor = 0;
    p_SEI->seiGFVE[gfve_idx].gfve_num_matrices = 0;

    p_SEI->seiGFVE[gfve_idx].gfve_pupil_present_idx = 0;
    p_SEI->seiGFVE[gfve_idx].gfve_pupil_coordinate_precision_factor = 0;
    p_SEI->seiGFVE[gfve_idx].gfve_pupil_left_eye_coordinate_x = 0;
    p_SEI->seiGFVE[gfve_idx].gfve_pupil_left_eye_coordinate_y = 0;
    p_SEI->seiGFVE[gfve_idx].gfve_pupil_right_eye_coordinate_x = 0;
    p_SEI->seiGFVE[gfve_idx].gfve_pupil_right_eye_coordinate_y = 0;

    memset(p_SEI->seiGFVE[gfve_idx].gfve_payload_filename, 0, sizeof(p_SEI->seiGFVE[gfve_idx].gfve_payload_filename));
    p_SEI->seiGFVE[gfve_idx].gfve_payload_length = 0;
  }

  p_SEI->seiHasGFVE_info = FALSE;
  p_SEI->gfve_num_sei = 0;  

#ifdef PRINT_GFVE_INFO
  printf("ClearGFVE, finish\n");
#endif
}

static void CloseGFVE(SEIParameters *p_SEI) 
{
  unsigned int matrixHeight = 0;
  unsigned int numMatrices = 0;

  unsigned int i, j;
#ifdef PRINT_GFVE_INFO
  printf("CloseGFVE, start\n");
#endif

  for (int gfve_idx = 0; gfve_idx < MAX_NUM_GFV_CNT; gfve_idx++)
  {
    if (p_SEI->seiGFVE[gfve_idx].data)
    {
      free(p_SEI->seiGFVE[gfve_idx].data->streamBuffer);
      free(p_SEI->seiGFVE[gfve_idx].data);
    }
      
    // free additional pointers in GFVE struct
    if (p_SEI->seiGFVE[gfve_idx].gfve_matrix_element) 
    {
      numMatrices = p_SEI->seiGFVE[gfve_idx].gfve_num_matrices;
      for (i = 0; i < numMatrices; i++)
      {
        matrixHeight = p_SEI->seiGFVE[gfve_idx].gfve_matrix_height[i];
        if (p_SEI->seiGFVE[gfve_idx].gfve_matrix_element[i])
        {
          for (j = 0; j < matrixHeight; j++)
          {
            if (p_SEI->seiGFVE[gfve_idx].gfve_matrix_element[i][j])
            {
              free(p_SEI->seiGFVE[gfve_idx].gfve_matrix_element[i][j]);
            }
          }
          free(p_SEI->seiGFVE[gfve_idx].gfve_matrix_element[i]);
        }
      }

      free(p_SEI->seiGFVE[gfve_idx].gfve_matrix_element);
      p_SEI->seiGFVE[gfve_idx].gfve_matrix_element = NULL;
      
      free(p_SEI->seiGFVE[gfve_idx].gfve_matrix_width);
      p_SEI->seiGFVE[gfve_idx].gfve_matrix_width = NULL;

      free(p_SEI->seiGFVE[gfve_idx].gfve_matrix_height);
      p_SEI->seiGFVE[gfve_idx].gfve_matrix_height = NULL;
    }

    if (p_SEI->seiGFVE[gfve_idx].gfve_payload_byte)
    {
      free(p_SEI->seiGFVE[gfve_idx].gfve_payload_byte);
      p_SEI->seiGFVE[gfve_idx].gfve_payload_byte = NULL;
    }

    // reset pointers to null
    p_SEI->seiGFVE[gfve_idx].data = NULL;
  }
  p_SEI->seiHasGFVE_info = FALSE;
  p_SEI->gfve_num_sei = 0;

#ifdef PRINT_GFVE_INFO
  printf("CloseGFVE, finish\n");
#endif
}

static void FinalizeGFVE(SEIParameters *p_SEI, unsigned int gfve_idx)
{
  generative_face_video_enhancement_struct *sei = &p_SEI->seiGFVE[gfve_idx];
  Bitstream *bitstream = sei->data;  

  unsigned int matrixId, j, k;

  unsigned int gfve_matrix_element_precision_factor_minus1, gfve_matrix_element_precision_factor;
  unsigned int gfve_num_matrices_minus1, gfve_num_matrices;
  unsigned int gfve_matrix_width_minus1;
  unsigned int gfve_matrix_height_minus1;

  double gfve_cur_matrix_element_abs, gfve_cur_matrix_element_abs_decimal;
  int gfve_cur_matrix_element_abs_int, gfve_cur_matrix_element_abs_dec_int_value;
  Boolean gfve_signflag_matrix_element;
  double gfve_matrix_element_abs_rec;

  double gfve_left_pupil_coordinate_x_rec = 0, gfve_left_pupil_coordinate_y_rec = 0;
  double gfve_right_pupil_coordinate_x_rec = 0, gfve_right_pupil_coordinate_y_rec = 0;
  double gfve_left_pupil_coordinate_x_ref = 0, gfve_left_pupil_coordinate_y_ref = 0;
  double gfve_right_pupil_coordinate_x_ref = 0, gfve_right_pupil_coordinate_y_ref = 0;
  unsigned int gfve_pupil_coordinate_precision_factor_minus1;
  
  // Arrays for matrices
  unsigned int* gfve_matrix_width_vec = NULL;
  unsigned int* gfve_matrix_height_vec = NULL;

  // Matrix element
  double*** gfve_matrix_element_rec = NULL;

  // Base picture
  Boolean gfve_basePicFlag = FALSE;

#ifdef PRINT_GFVE_INFO
  printf("FinalizeGFVE, start idx %u\n", gfve_idx);
#endif

  // start
  write_ue_v("SEI: gfve_id", sei->gfve_id, bitstream);
  write_ue_v("SEI: gfve_gfv_id", sei->gfve_gfv_id, bitstream);
  write_ue_v("SEI: gfve_gfv_cnt", sei->gfve_gfv_cnt, bitstream);

  // GFVE base picture
  if (sei->gfve_gfv_cnt == 0)
  {
    write_u_1("SEI: gfve_base_picture_flag", sei->gfve_base_pic_flag, bitstream);
    gfve_basePicFlag = sei->gfve_base_pic_flag;
  }
  else 
  {
    gfve_basePicFlag = FALSE;
  }

#ifdef PRINT_GFVE_INFO
  printf("gfve_id = %u\n", sei->gfve_id);
  printf("gfve_gfv_id = %u\n", sei->gfve_gfv_id);
  printf("gfve_gfv_cnt = %u\n", sei->gfve_gfv_cnt);
  printf("gfve_base_pic_flag = %u\n", gfve_basePicFlag);
#endif  

  if (gfve_basePicFlag)
  {
    write_u_1("SEI: gfve_nn_present_flag", sei->gfve_nn_present_flag, bitstream);
    if (sei->gfve_nn_present_flag)
    {
      write_ue_v("SEI: gfve_mode_idc", sei->gfve_nn_mode_idc, bitstream);

  #ifdef PRINT_GFVE_INFO
      printf("gfve_nn_mode_idc = %u\n", sei->gfve_nn_mode_idc);
  #endif

      // Mode IDC 1 processing, to signal NN
      if (sei->gfve_nn_mode_idc == 1) 
      {
        // Byte alignment
        while (!(bitstream->bits_to_go==8))
        {
          write_u_1("SEI: gfve_nn_alignment_zero_bit_a", 0, bitstream);
        }
        // String fields
        write_string("SEI: gfve_nn_tag_uri", sei->gfve_nn_tag_uri, bitstream);
        write_string("SEI: gfve_nn_uri", sei->gfve_nn_uri, bitstream);

  #ifdef PRINT_GFVE_INFO
        printf("gfve_nn_tag_uri = %s\n", sei->gfve_nn_tag_uri);
        printf("gfve_nn_uri = %s\n", sei->gfve_nn_uri);
  #endif
      }
    }
  }

  // Matrix parameters

  write_u_1("SEI: gfve_matrix_present_flag", sei->gfve_matrix_present_flag, bitstream);
#ifdef PRINT_GFVE_INFO
  printf("gfve_matrix_present_flag = %u\n", sei->gfve_matrix_present_flag);
#endif
  if (sei->gfve_matrix_present_flag) 
  {
    if (!gfve_basePicFlag)
    {
      write_u_1("SEI: gfve_matrix_pred_flag", sei->gfve_matrix_pred_flag, bitstream);
#ifdef PRINT_GFVE_INFO
      printf("gfve_matrix_pred_flag = %u\n", sei->gfve_matrix_pred_flag);
#endif
    }

    if (gfve_basePicFlag)
    {
      // For base pic, free and reallocate base matrix info
      if (base_gfve_matrix_width_vec) free(base_gfve_matrix_width_vec);
      base_gfve_matrix_width_vec = (unsigned int*)malloc(sei->gfve_num_matrices * sizeof(unsigned int));
      if (!base_gfve_matrix_width_vec) no_mem_exit("FinalizeGFVE: base_gfve_matrix_width_vec");
      
      if (base_gfve_matrix_height_vec) free(base_gfve_matrix_height_vec);
      base_gfve_matrix_height_vec = (unsigned int*)malloc(sei->gfve_num_matrices * sizeof(unsigned int));
      if (!base_gfve_matrix_height_vec) no_mem_exit("FinalizeGFVE: base_gfve_matrix_height_vec");
    }

    if (!sei->gfve_matrix_pred_flag)
    {
      gfve_matrix_element_precision_factor_minus1 = sei->gfve_matrix_element_precision_factor - 1;
      // CHECK: The value of gfve_matrix_element_precision_factor_minus1 shall be in the range of 0 to 31, inclusive
      assert( gfve_matrix_element_precision_factor_minus1 >= 0 && gfve_matrix_element_precision_factor_minus1 <= 31 );
      write_ue_v("SEI: gfve_matrix_element_precision_factor_minus1", gfve_matrix_element_precision_factor_minus1, bitstream);

      gfve_num_matrices_minus1 = sei->gfve_num_matrices - 1;
      // CHECK: The value of gfve_num_matrices_minus1 shall be in the range of 0 to 2^(10) - 1, inclusive
      assert( gfve_num_matrices_minus1 >= 0 && gfve_num_matrices_minus1 <= (1 << 10) - 1 );
      write_ue_v("SEI: gfve_num_matrices_minus1", gfve_num_matrices_minus1, bitstream);

      gfve_matrix_element_precision_factor = sei->gfve_matrix_element_precision_factor;
      gfve_num_matrices = sei->gfve_num_matrices;

      if (gfve_basePicFlag)
      {
        base_gfve_matrix_element_precision_factor = gfve_matrix_element_precision_factor;
        base_gfve_num_matrices = gfve_num_matrices;
      }

#ifdef PRINT_GFVE_INFO
      printf("gfve_matrix_element_precision_factor_minus1 = %u\n", gfve_matrix_element_precision_factor_minus1);
      printf("gfve_num_matrices_minus1 = %u\n", gfve_num_matrices_minus1);
#endif
      
      gfve_matrix_width_vec = (unsigned int*)malloc(sei->gfve_num_matrices * sizeof(unsigned int));
      if (!gfve_matrix_width_vec) no_mem_exit("FinalizeGFVE: gfve_matrix_width_vec");
      gfve_matrix_height_vec = (unsigned int*)malloc(sei->gfve_num_matrices * sizeof(unsigned int));
      if (!gfve_matrix_height_vec) no_mem_exit("FinalizeGFVE: gfve_matrix_height_vec");

      for (matrixId = 0; matrixId < sei->gfve_num_matrices; matrixId++)
      {
        gfve_matrix_height_minus1 = sei->gfve_matrix_height[matrixId] - 1;
        write_ue_v("SEI: gfve_matrix_height_minus1", gfve_matrix_height_minus1, bitstream);
        
        gfve_matrix_width_minus1 = sei->gfve_matrix_width[matrixId] - 1;
        write_ue_v("SEI: gfve_matrix_width_minus1", gfve_matrix_width_minus1, bitstream);
      }
    }
    // Pred coding for matrix
    else
    {
      if (!gfve_basePicFlag)
      {
        gfve_num_matrices = base_gfve_num_matrices;
        gfve_matrix_element_precision_factor = base_gfve_matrix_element_precision_factor;
      }
      else
      {
        gfve_matrix_element_precision_factor = sei->gfve_matrix_element_precision_factor;
        gfve_num_matrices = sei->gfve_num_matrices;

        base_gfve_matrix_element_precision_factor = gfve_matrix_element_precision_factor;
        base_gfve_num_matrices = gfve_num_matrices;
      }

      // Allocate memory for matrix info
      gfve_matrix_width_vec = (unsigned int*)malloc(gfve_num_matrices * sizeof(unsigned int));
      if (!gfve_matrix_width_vec) no_mem_exit("FinalizeGFVE: gfve_matrix_width_vec");
      gfve_matrix_height_vec = (unsigned int*)malloc(gfve_num_matrices * sizeof(unsigned int));
      if (!gfve_matrix_height_vec) no_mem_exit("FinalizeGFVE: gfve_matrix_height_vec");
    }

    if (gfve_basePicFlag)
    {
      for (matrixId = 0; matrixId < gfve_num_matrices; matrixId++) {
          base_gfve_matrix_height_vec[matrixId] = sei->gfve_matrix_height[matrixId];
          base_gfve_matrix_width_vec[matrixId] = sei->gfve_matrix_width[matrixId];
        }
    }
    
    if (sei->gfve_matrix_pred_flag)
    {
      for (matrixId = 0; matrixId < gfve_num_matrices; matrixId++)
      {
        gfve_matrix_height_vec[matrixId] = base_gfve_matrix_height_vec[matrixId];
        gfve_matrix_width_vec[matrixId] = base_gfve_matrix_width_vec[matrixId];
      }
    }
    else
    {
      for (matrixId = 0; matrixId < gfve_num_matrices; matrixId++)
      {
        gfve_matrix_height_vec[matrixId] = sei->gfve_matrix_height[matrixId];
        gfve_matrix_width_vec[matrixId] = sei->gfve_matrix_width[matrixId];
      }
    }

#ifdef PRINT_GFVE_INFO
    printf("gfve_matrix_width = ");
    for (matrixId = 0; matrixId < gfve_num_matrices; matrixId++)
    {
      printf("%u (%u) ", gfve_matrix_width_vec[matrixId], base_gfve_matrix_width_vec[matrixId]);
    }
    printf("\n");

    printf("gfve_matrix_height = ");
    for (matrixId = 0; matrixId < gfve_num_matrices; matrixId++)
    {
      printf("%u (%u) ", gfve_matrix_height_vec[matrixId], base_gfve_matrix_height_vec[matrixId]);
    }
    printf("\n");
#endif

    // For base pic, free and reallocate base matrix
    if ( gfve_basePicFlag ) 
    {
      if ( base_gfve_matrix_rec )
      {
        for (matrixId = 0; matrixId < gfve_num_matrices; matrixId++)
        {
          for (j = 0; j < base_gfve_matrix_height_vec[matrixId]; j++)
          {
            free(base_gfve_matrix_rec[matrixId][j]);
          }
          free(base_gfve_matrix_rec[matrixId]);
        }
      }

      if ( prev_gfve_matrix_rec )
      {
        for (matrixId = 0; matrixId < gfve_num_matrices; matrixId++)
        {
          for (j = 0; j < gfve_matrix_height_vec[matrixId]; j++)
          {
            free(prev_gfve_matrix_rec[matrixId][j]);
          }
          free(prev_gfve_matrix_rec[matrixId]);
        }
      }

      base_gfve_matrix_rec = (double***) calloc((gfve_num_matrices), sizeof(double**));
      if (!base_gfve_matrix_rec) no_mem_exit("FinalizeGFVE: base_gfve_matrix_rec");

      for (matrixId = 0; matrixId < gfve_num_matrices; matrixId++)
      {
        base_gfve_matrix_rec[matrixId] = (double**) calloc((base_gfve_matrix_height_vec[matrixId]), sizeof(double*));
        if (!base_gfve_matrix_rec[matrixId]) no_mem_exit("FinalizeGFVE: base_gfve_matrix_rec[matrixId]");

        for (j = 0; j < base_gfve_matrix_height_vec[matrixId]; j++)
        {
          base_gfve_matrix_rec[matrixId][j] = (double*) calloc((base_gfve_matrix_width_vec[matrixId]), sizeof(double));
          if (!base_gfve_matrix_rec[matrixId][j]) no_mem_exit("FinalizeGFVE: base_gfve_matrix_rec[matrixId][j]");
        }
      }

      prev_gfve_matrix_rec = (double***) calloc((gfve_num_matrices), sizeof(double**));
      if (!prev_gfve_matrix_rec) no_mem_exit("FinalizeGFVE: prev_gfve_matrix_rec");

      for (matrixId = 0; matrixId < gfve_num_matrices; matrixId++)
      {
        prev_gfve_matrix_rec[matrixId] = (double**) calloc((gfve_matrix_height_vec[matrixId]), sizeof(double*));
        if (!prev_gfve_matrix_rec[matrixId]) no_mem_exit("FinalizeGFVE: prev_gfve_matrix_rec[matrixId]");

        for (j = 0; j < gfve_matrix_height_vec[matrixId]; j++)
        {
          prev_gfve_matrix_rec[matrixId][j] = (double*) calloc((gfve_matrix_width_vec[matrixId]), sizeof(double));
          if (!prev_gfve_matrix_rec[matrixId][j]) no_mem_exit("FinalizeGFVE: prev_gfve_matrix_rec[matrixId][j]");
        }
      }
    }

    // Allocate 3D matrix element array
    gfve_matrix_element_rec = (double***) calloc((gfve_num_matrices), sizeof(double**));
    if (!gfve_matrix_element_rec) printf("FinalizeGFVE: gfve_matrix_element_rec");
#ifdef PRINT_GFVE_INFO
    printf("gfve_matrix_element_rec = ");
#endif
    for (matrixId = 0; matrixId < gfve_num_matrices; matrixId++)
    {
      gfve_matrix_element_rec[matrixId] = (double**) calloc((gfve_matrix_height_vec[matrixId]), sizeof(double*));
      if (!gfve_matrix_element_rec[matrixId]) no_mem_exit("FinalizeGFVE: gfve_matrix_element_rec[matrixId]");

      for (j = 0; j < gfve_matrix_height_vec[matrixId]; j++)
      {
        gfve_matrix_element_rec[matrixId][j] = (double*) calloc((gfve_matrix_width_vec[matrixId]), sizeof(double));
        if (!gfve_matrix_element_rec[matrixId][j]) no_mem_exit("FinalizeGFVE: gfve_matrix_element_rec[matrixId][j]");

        for (k = 0; k < gfve_matrix_width_vec[matrixId]; k++)
        {
          // absolute matrix
          if (!sei->gfve_matrix_pred_flag)
          {
            gfve_cur_matrix_element_abs = fabs(sei->gfve_matrix_element[matrixId][j][k]);
            gfve_cur_matrix_element_abs_int = (int)(gfve_cur_matrix_element_abs);
            // CHECK: The value of gfve_matrix_element_int shall be in the range of 0 to 2^(32) - 2, inclusive"
            assert( gfve_cur_matrix_element_abs_int >= 0 && (unsigned int)gfve_cur_matrix_element_abs_int <= (4294967296 - 2) );
            write_ue_v("SEI: gfve_matrix_element_int", gfve_cur_matrix_element_abs_int, bitstream);

            gfve_cur_matrix_element_abs_decimal = gfve_cur_matrix_element_abs - gfve_cur_matrix_element_abs_int;
            assert( gfve_cur_matrix_element_abs_decimal >= 0 );
            gfve_cur_matrix_element_abs_dec_int_value = iClip3(0, (1 << gfve_matrix_element_precision_factor) - 1, (int)(gfve_cur_matrix_element_abs_decimal * (1 << gfve_matrix_element_precision_factor) + 0.5));
            write_u_v(gfve_matrix_element_precision_factor, "SEI: gfve_matrix_element_dec", gfve_cur_matrix_element_abs_dec_int_value, bitstream);

            gfve_signflag_matrix_element = (sei->gfve_matrix_element[matrixId][j][k] < 0) ? TRUE : FALSE;
            if (gfve_cur_matrix_element_abs_int || gfve_cur_matrix_element_abs_dec_int_value)
            {
              write_u_1("SEI: gfve_matrix_element_sign_flag", gfve_signflag_matrix_element, bitstream);
            }

            gfve_matrix_element_abs_rec = (double)(gfve_cur_matrix_element_abs_int + (((double)gfve_cur_matrix_element_abs_dec_int_value) / (1 << gfve_matrix_element_precision_factor)));
            gfve_matrix_element_rec[matrixId][j][k] = gfve_signflag_matrix_element ? -gfve_matrix_element_abs_rec : gfve_matrix_element_abs_rec;
          }
          // inter-frame difference
          else
          {
            gfve_cur_matrix_element_abs = fabs(sei->gfve_matrix_element[matrixId][j][k] - (sei->gfve_gfv_cnt == 0 ? base_gfve_matrix_rec[matrixId][j][k] : prev_gfve_matrix_rec[matrixId][j][k]));
            gfve_cur_matrix_element_abs_int = (int)(gfve_cur_matrix_element_abs);
            // CHECK: The value of gfve_matrix_element_int shall be in the range of 0 to 2^(32) - 2, inclusive"
            assert( gfve_cur_matrix_element_abs_int >= 0 && (unsigned int)gfve_cur_matrix_element_abs_int <= (4294967296 - 2) );
            write_ue_v("SEI: gfve_matrix_delta_element_int", gfve_cur_matrix_element_abs_int, bitstream);

            gfve_cur_matrix_element_abs_decimal = gfve_cur_matrix_element_abs - gfve_cur_matrix_element_abs_int;
            assert( gfve_cur_matrix_element_abs_decimal >= 0 );
            gfve_cur_matrix_element_abs_dec_int_value = (int)(gfve_cur_matrix_element_abs_decimal * (1 << gfve_matrix_element_precision_factor) + 0.5);
            write_u_v(gfve_matrix_element_precision_factor, "SEI: gfve_matrix_delta_element_dec", gfve_cur_matrix_element_abs_dec_int_value, bitstream);

            gfve_signflag_matrix_element = ((sei->gfve_matrix_element[matrixId][j][k] - (sei->gfve_gfv_cnt == 0 ? base_gfve_matrix_rec[matrixId][j][k] : prev_gfve_matrix_rec[matrixId][j][k])) < 0) ? TRUE : FALSE;
            if (gfve_cur_matrix_element_abs_int || gfve_cur_matrix_element_abs_dec_int_value)
            {
              write_u_1("SEI: gfve_matrix_delta_element_sign_flag", gfve_signflag_matrix_element, bitstream);
            }

            gfve_matrix_element_abs_rec = (double)(gfve_cur_matrix_element_abs_int + (((double)gfve_cur_matrix_element_abs_dec_int_value) / (1 << gfve_matrix_element_precision_factor)));
            gfve_matrix_element_rec[matrixId][j][k] = ((gfve_signflag_matrix_element ? -gfve_matrix_element_abs_rec : gfve_matrix_element_abs_rec) + (sei->gfve_gfv_cnt == 0 ? base_gfve_matrix_rec[matrixId][j][k] : prev_gfve_matrix_rec[matrixId][j][k]));
          }
#ifdef PRINT_GFVE_INFO
          printf("%d (%d) ", gfve_cur_matrix_element_abs_int, gfve_cur_matrix_element_abs_dec_int_value);
#endif
        }
      }
    }
#ifdef PRINT_GFV_INFO
    printf("\n");
#endif

    if (do_update_gfve_matrix)
    {
      for (matrixId = 0; matrixId < gfve_num_matrices; matrixId++) 
      {
        for (j = 0; j < gfve_matrix_height_vec[matrixId]; j++) 
        {
          for (k = 0; k < gfve_matrix_width_vec[matrixId]; k++) 
          {
            prev_gfve_matrix_rec[matrixId][j][k] = gfve_matrix_element_rec[matrixId][j][k];
            if (gfve_basePicFlag) 
            {
              base_gfve_matrix_rec[matrixId][j][k] = gfve_matrix_element_rec[matrixId][j][k];
            }
          }
        }
      }
      if (gfve_basePicFlag)
      {
        for (matrixId = 0; matrixId < gfve_num_matrices; matrixId++) {
          base_gfve_matrix_height_vec[matrixId] = gfve_matrix_height_vec[matrixId];
          base_gfve_matrix_width_vec[matrixId] = gfve_matrix_width_vec[matrixId];
        }
      }
      do_update_gfve_matrix = FALSE;
    }
    else
    {
      do_update_gfve_matrix = TRUE;
    }
  }

  // Pupils in GFVE

  // "The possible values of gfve_pupil_coordinate_present_idx are 0, 1, 2, and 3"
  assert( sei->gfve_pupil_present_idx >= 0 && sei->gfve_pupil_present_idx <= 3);
  write_u_v(2, "SEI: gfve_pupil_coordinate_present_idx", sei->gfve_pupil_present_idx, bitstream);
#ifdef PRINT_GFVE_INFO
  printf("gfve_pupil_coordinate_present_idx = %u\n", sei->gfve_pupil_present_idx);
#endif

  if ( sei->gfve_pupil_present_idx != 0 )
  {
    if ( gfve_basePicFlag )
    {
      check_base_pic_pupil_present_idx = TRUE;

      gfve_pupil_coordinate_precision_factor_minus1 = sei->gfve_pupil_coordinate_precision_factor - 1;
      // CHECK: The value of gfve_pupil_coordinate_precision_factor_minus1 shall be in the range of 0 to 31, inclusive
      assert( gfve_pupil_coordinate_precision_factor_minus1 >= 0 && gfve_pupil_coordinate_precision_factor_minus1 <= 31 );
      write_ue_v("SEI: gfve_pupil_coordinate_precision_factor_minus1", gfve_pupil_coordinate_precision_factor_minus1, bitstream);
#ifdef PRINT_GFVE_INFO
      printf("gfve_pupil_coordinate_precision_factor_minus1 = %u\n", gfve_pupil_coordinate_precision_factor_minus1);
#endif
    }

    // CHECK: The gfve_pupil_coordinate_present _idx for the first frame shall not be 0
    assert( check_base_pic_pupil_present_idx );
  }

  if ( check_base_pic_pupil_present_idx )
  {
    // set the reference coordinate for this frame
    if ( sei->gfve_gfv_cnt == 0 )
    {
      if ( !gfve_basePicFlag )
      {
        gfve_left_pupil_coordinate_x_ref = base_gfve_left_pupil_coordinate_x;
        gfve_left_pupil_coordinate_y_ref = base_gfve_left_pupil_coordinate_y;
        gfve_right_pupil_coordinate_x_ref = base_gfve_right_pupil_coordinate_x;
        gfve_right_pupil_coordinate_y_ref = base_gfve_right_pupil_coordinate_y;
      }
    }
    else
    {
      gfve_left_pupil_coordinate_x_ref = prev_gfve_left_pupil_coordinate_x;
      gfve_left_pupil_coordinate_y_ref = prev_gfve_left_pupil_coordinate_y;
      gfve_right_pupil_coordinate_x_ref = prev_gfve_right_pupil_coordinate_x;
      gfve_right_pupil_coordinate_y_ref = prev_gfve_right_pupil_coordinate_y;
    }

    // Left Pupil: for idx 1 and 3
    if ( sei->gfve_pupil_present_idx == 1 || sei->gfve_pupil_present_idx == 3 )
    {
      gfve_left_pupil_coordinate_x_rec = WriteGFVEPupilCoordinate(p_SEI, gfve_idx, sei->gfve_pupil_left_eye_coordinate_x, gfve_left_pupil_coordinate_x_ref, sei->gfve_pupil_coordinate_precision_factor, "left", "x");
      gfve_left_pupil_coordinate_y_rec = WriteGFVEPupilCoordinate(p_SEI, gfve_idx, sei->gfve_pupil_left_eye_coordinate_y, gfve_left_pupil_coordinate_y_ref, sei->gfve_pupil_coordinate_precision_factor, "left", "y");
    }
    else
    {
      gfve_left_pupil_coordinate_x_rec = gfve_left_pupil_coordinate_x_ref;
      gfve_left_pupil_coordinate_y_rec = gfve_left_pupil_coordinate_y_ref;
    }

    // right pupil reference from left pupil in base pic
    if ( gfve_basePicFlag )
    {
      gfve_right_pupil_coordinate_x_ref = gfve_left_pupil_coordinate_x_rec;
      gfve_right_pupil_coordinate_y_ref = gfve_left_pupil_coordinate_y_rec;
    }
    
    // RIght Pupil: for idx 2 and 3
    if ( sei->gfve_pupil_present_idx == 2 || sei->gfve_pupil_present_idx == 3 )
    {
      gfve_right_pupil_coordinate_x_rec = WriteGFVEPupilCoordinate(p_SEI, gfve_idx, sei->gfve_pupil_right_eye_coordinate_x, gfve_right_pupil_coordinate_x_ref, sei->gfve_pupil_coordinate_precision_factor, "right", "x");
      gfve_right_pupil_coordinate_y_rec = WriteGFVEPupilCoordinate(p_SEI, gfve_idx, sei->gfve_pupil_right_eye_coordinate_y, gfve_right_pupil_coordinate_y_ref, sei->gfve_pupil_coordinate_precision_factor, "right", "y");
    }
    else
    {
      gfve_right_pupil_coordinate_x_rec = gfve_right_pupil_coordinate_x_ref;
      gfve_right_pupil_coordinate_y_rec = gfve_right_pupil_coordinate_y_ref;
    }

#ifdef PRINT_GFVE_INFO
    printf("Pupils: Left -- [%lf , %lf], ", gfve_left_pupil_coordinate_x_rec, gfve_left_pupil_coordinate_y_rec);
    printf("Right -- [%lf , %lf]\n", gfve_right_pupil_coordinate_x_rec, gfve_right_pupil_coordinate_y_rec);
#endif

    if (do_update_gfve_pupil_coordinate)
    {
      if (gfve_basePicFlag)
      {
        base_gfve_left_pupil_coordinate_x = gfve_left_pupil_coordinate_x_rec;
        base_gfve_left_pupil_coordinate_y = gfve_left_pupil_coordinate_y_rec;
        base_gfve_right_pupil_coordinate_x = gfve_right_pupil_coordinate_x_rec;
        base_gfve_right_pupil_coordinate_y = gfve_right_pupil_coordinate_y_rec;
      }

      prev_gfve_left_pupil_coordinate_x = gfve_left_pupil_coordinate_x_rec;
      prev_gfve_left_pupil_coordinate_y = gfve_left_pupil_coordinate_y_rec;
      prev_gfve_right_pupil_coordinate_x = gfve_right_pupil_coordinate_x_rec;
      prev_gfve_right_pupil_coordinate_y = gfve_right_pupil_coordinate_y_rec;

      do_update_gfve_pupil_coordinate = FALSE;
    }
    else 
    {
      do_update_gfve_pupil_coordinate = TRUE;
    }
  }
  
  if (sei->gfve_nn_present_flag)
  {
    if (sei->gfve_nn_mode_idc == 0)
    {
      // Byte alignment
      while (!(bitstream->bits_to_go==8))
      {
        write_u_1("SEI: gfve_nn_alignment_zero_bit_b", 0, bitstream);
      }
      for (unsigned long p = 0; p < sei->gfve_payload_length; p++)
      {
        write_u_v(8, "SEI: gfve_nn_payload_byte[i]", sei->gfve_payload_byte[p], bitstream);
      }
    }
  }

  // make sure the payload is byte aligned, stuff bits are 10..0
  if ( bitstream->bits_to_go != 8 )
  {
    (bitstream->byte_buf) <<= 1;
    bitstream->byte_buf |= 1;
    bitstream->bits_to_go--;
    if ( bitstream->bits_to_go != 0 ) 
      (bitstream->byte_buf) <<= (bitstream->bits_to_go);
    bitstream->bits_to_go = 8;
    bitstream->streamBuffer[bitstream->byte_pos++]=bitstream->byte_buf;
    bitstream->byte_buf = 0;
  }
  sei->payloadSize = bitstream->byte_pos;


  // Free allocated memory
  if (gfve_matrix_element_rec) 
  {
    for (matrixId = 0; matrixId < gfve_num_matrices; matrixId++)
    {
      if (gfve_matrix_element_rec[matrixId])
      {
        for (j = 0; j < gfve_matrix_height_vec[matrixId]; j++)
        {
          if (gfve_matrix_element_rec[matrixId][j]) 
          {
            free(gfve_matrix_element_rec[matrixId][j]);
          }
        }
        free(gfve_matrix_element_rec[matrixId]);
      }
    }
    free(gfve_matrix_element_rec);
  }
  
  if (gfve_matrix_width_vec) free(gfve_matrix_width_vec);
  if (gfve_matrix_height_vec) free(gfve_matrix_height_vec);

#ifdef PRINT_GFVE_INFO
  printf("FinalizeGFVE, finish idx %u\n", gfve_idx);
#endif
}

double WriteGFVEPupilCoordinate(SEIParameters *p_SEI, unsigned int gfve_idx, double coordinate, double ref_coordinate, int precision_factor, const char* eye, const char* axis)
{
  generative_face_video_enhancement_struct *sei = &p_SEI->seiGFVE[gfve_idx];
  Bitstream *bitstream = sei->data;  

  double gfve_pupil_delta_abs, gfve_pupil_delta_abs_rec;
  int gfve_pupil_abs_int_value;
  char gfve_pupil_check_message[256];
  char gfve_pupil_abs_symbol_name[256];
  char gfve_pupil_sign_symbol_name[256];
  Boolean gfve_pupil_signflag;

  gfve_pupil_delta_abs = fabs(coordinate - ref_coordinate);
  gfve_pupil_abs_int_value = (int)(gfve_pupil_delta_abs * (1 << precision_factor) + 0.5);

  // CHECK: Invalid value for 'eye'. Allowed values are 'left' or 'right'.
  assert( strcmp(eye, "left") == 0 || strcmp(eye, "right") == 0 );
  // CHECK: Invalid value for 'axis'. Allowed values are 'x' or 'y'.
  assert( strcmp(axis, "x") == 0 || strcmp(axis, "y") == 0 );

  snprintf(gfve_pupil_check_message, sizeof(gfve_pupil_check_message), "The value of gfve_pupil_%s_eye_d%s_coordinate_abs shall be in the range of 0 to 1 << (gfve_pupil_coordinate_precision_factor_minus1 + 2), inclusive", eye, axis);
  if (gfve_pupil_abs_int_value < 0 || gfve_pupil_abs_int_value > (1 << (precision_factor + 1))) {
    error(gfve_pupil_check_message, 500);
  }

  snprintf(gfve_pupil_abs_symbol_name, sizeof(gfve_pupil_abs_symbol_name), "SEI: gfve_pupil_%s_eye_d%s_coordinate_abs", eye, axis);
  write_ue_v(gfve_pupil_abs_symbol_name, gfve_pupil_abs_int_value, bitstream);

#ifdef PRINT_GFVE_INFO
  printf("%s (%d)\n", gfve_pupil_abs_symbol_name, gfve_pupil_abs_int_value);
#endif

  gfve_pupil_signflag = ((coordinate - ref_coordinate) < 0) ? TRUE : FALSE;
  if ( gfve_pupil_abs_int_value )
  {
    snprintf(gfve_pupil_sign_symbol_name, sizeof(gfve_pupil_sign_symbol_name), "SEI: gfve_pupil_%s_eye_d%s_coordinate_sign_flag", eye, axis);
    write_u_1(gfve_pupil_sign_symbol_name, gfve_pupil_signflag, bitstream);
  }

  gfve_pupil_delta_abs_rec = (((double)gfve_pupil_abs_int_value) / (1 << precision_factor));
  return (gfve_pupil_signflag ? -gfve_pupil_delta_abs_rec : gfve_pupil_delta_abs_rec) + ref_coordinate;
}

void UpdateGFVE(SEIParameters *p_SEI, InputParameters *p_Inp) 
{
#ifdef PRINT_GFVE_INFO
  printf("UpdateGFVE, start\n");
#endif

  // uncomment to update SEI to another configuration
  // sprintf(p_Inp->GFVFile, "cfg/generative_face_video_enhancement3.cfg");

  // read GFVE config from file
  ParseGFVEConfigFile(p_SEI, p_Inp, p_SEI->seiGFVE);
  
  // all initialized, set GFVE presence to true
  p_SEI->seiHasGFVE_info = TRUE;

#ifdef PRINT_GFVE_INFO
    printf("UpdateGFVE, finish\n");
#endif
}
#endif
#endif
