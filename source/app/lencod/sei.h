
/*!
 ************************************************************************
 *  \file
 *     sei.h
 *  \brief
 *     definitions for Supplemental Enhanced Information
 *  \author(s)
 *      - Dong Tian                             <tian@cs.tut.fi>
 *      - TBD
 *
 * ************************************************************************
 */

#ifndef SEI_H
#define SEI_H

#define MAX_LAYER_NUMBER 2
#define MAX_DEPENDENT_SUBSEQ 5
// tone mapping information
#define MAX_CODED_BIT_DEPTH  12
#define MAX_SEI_BIT_DEPTH    12
#define MAX_NUM_PIVOTS       (1<<MAX_CODED_BIT_DEPTH)
// This is only temp
//! Buffering Period Information
#define MAX_CPB_CNT_MINUS1 31
#define MAX_PIC_STRUCT_VALUE 16
#define MAX_FN 256

#define AGGREGATION_PACKET_TYPE 4
#define SEI_PACKET_TYPE 5  // Tian Dong: See VCEG-N72, it need updates

#define NORMAL_SEI 0
#define AGGREGATION_SEI 1
#define JVET_AK0107_MODALITY_INFORMATION 1

#if GFV_ENABLE
#define JVET_AJ0207_GFV_SEI 1
#define JVET_AK0239_GFVE_SEI 1
#endif

//! definition of SEI payload type
typedef enum {
  SEI_BUFFERING_PERIOD = 0,
  SEI_PIC_TIMING,
  SEI_PAN_SCAN_RECT,
  SEI_FILLER_PAYLOAD,
  SEI_USER_DATA_REGISTERED_ITU_T_T35,
  SEI_USER_DATA_UNREGISTERED,
  SEI_RECOVERY_POINT,
  SEI_DEC_REF_PIC_MARKING_REPETITION,
  SEI_SPARE_PIC,
  SEI_SCENE_INFO,
  SEI_SUB_SEQ_INFO,
  SEI_SUB_SEQ_LAYER_CHARACTERISTICS,
  SEI_SUB_SEQ_CHARACTERISTICS,
  SEI_FULL_FRAME_FREEZE,
  SEI_FULL_FRAME_FREEZE_RELEASE,
  SEI_FULL_FRAME_SNAPSHOT,
  SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START,
  SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END,
  SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET,
  SEI_FILM_GRAIN_CHARACTERISTICS,
  SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE,
  SEI_STEREO_VIDEO_INFO,
  SEI_POST_FILTER_HINTS,
  SEI_TONE_MAPPING,
  SEI_SCALABILITY_INFO,
  SEI_SUB_PIC_SCALABLE_LAYER,
  SEI_NON_REQUIRED_LAYER_REP,
  SEI_PRIORITY_LAYER_INFO,
  SEI_LAYERS_NOT_PRESENT,
  SEI_LAYER_DEPENDENCY_CHANGE,
  SEI_SCALABLE_NESTING,
  SEI_BASE_LAYER_TEMPORAL_HRD,
  SEI_QUALITY_LAYER_INTEGRITY_CHECK,
  SEI_REDUNDANT_PIC_PROPERTY,
  SEI_TL0_DEP_REP_INDEX,
  SEI_TL_SWITCHING_POINT,
  SEI_PARALLEL_DECODING_INFO,
  SEI_MVC_SCALABLE_NESTING,
  SEI_VIEW_SCALABILITY_INFO,
  SEI_MULTIVIEW_SCENE_INFO,
  SEI_MULTIVIEW_ACQUISITION_INFO,
  SEI_NON_REQUIRED_VIEW_COMPONENT,
  SEI_VIEW_DEPENDENCY_CHANGE,
  SEI_OPERATION_POINTS_NOT_PRESENT,
  SEI_BASE_VIEW_TEMPORAL_HRD,
  SEI_FRAME_PACKING_ARRANGEMENT,
#if JVET_AK0107_MODALITY_INFORMATION
  SEI_MODALITY_INFO = 218,
#endif

#if GFV_ENABLE
#if JVET_AJ0207_GFV_SEI
  SEI_GENERATIVE_FACE_VIDEO = 223,
#endif
#if JVET_AK0239_GFVE_SEI
  SEI_GENERATIVE_FACE_VIDEO_ENHANCEMENT = 224,
#endif
#endif

  SEI_MAX_ELEMENTS  //!< number of maximum syntax elements
} SEI_type;

typedef struct
{
  int seq_parameter_set_id;
  int nal_initial_cpb_removal_delay[MAX_CPB_CNT_MINUS1+1];
  int nal_initial_cpb_removal_delay_offset[MAX_CPB_CNT_MINUS1+1];
  int vcl_initial_cpb_removal_delay[MAX_CPB_CNT_MINUS1+1];
  int vcl_initial_cpb_removal_delay_offset[MAX_CPB_CNT_MINUS1+1];

  Bitstream *data;
  int payloadSize;
} bufferingperiod_information_struct;

//! SEI structure
typedef struct
{
  Boolean available;
  int payloadSize;
  unsigned char subPacketType;
  byte* data;
} sei_struct;

//! Spare Picture
typedef struct
{
  int target_frame_num;
  int num_spare_pics;
  int payloadSize;
  Bitstream* data;
} spare_picture_struct;

//! Subseq Information
typedef struct
{
  int subseq_layer_num;
  int subseq_id;
  unsigned int last_picture_flag;
  unsigned int stored_frame_cnt;

  int payloadSize;
  Bitstream* data;
} subseq_information_struct;

//! Subseq Layer Information
typedef struct
{
  uint16 bit_rate[MAX_LAYER_NUMBER];
  uint16 frame_rate[MAX_LAYER_NUMBER];
  byte data[4*MAX_LAYER_NUMBER];
  int layer_number;
  int payloadSize;
} subseq_layer_information_struct;

//! Subseq Characteristics
typedef struct
{
  int subseq_layer_num;
  int subseq_id;
  int duration_flag;
  unsigned int subseq_duration;
  unsigned int average_rate_flag;
  unsigned int average_bit_rate;
  unsigned int average_frame_rate;
  int num_referenced_subseqs;
  int ref_subseq_layer_num[MAX_DEPENDENT_SUBSEQ];
  int ref_subseq_id[MAX_DEPENDENT_SUBSEQ];

  Bitstream* data;
  int payloadSize;
} subseq_char_information_struct;

typedef struct
{
  int scene_id;
  int scene_transition_type;
  int second_scene_id;

  Bitstream* data;
  int payloadSize;
} scene_information_struct;

//! PanScanRect Information
typedef struct
{
  int pan_scan_rect_id;
  int pan_scan_rect_cancel_flag;
  int pan_scan_cnt_minus1;             //!< ranges from 0 to 2
  int pan_scan_rect_left_offset[3];
  int pan_scan_rect_right_offset[3];
  int pan_scan_rect_top_offset[3];
  int pan_scan_rect_bottom_offset[3];
  int pac_scan_rect_repetition_period;

  Bitstream *data;
  int payloadSize;
} panscanrect_information_struct;
//! User_data_unregistered Information
typedef struct
{
  char *byte;
  int total_byte;
  Bitstream *data;
  int payloadSize;
} user_data_unregistered_information_struct;

//! User_data_registered_itu_t_t35 Information
typedef struct
{
  char *byte;
  int total_byte;
  int itu_t_t35_country_code;
  int itu_t_t35_country_code_extension_byte;
  Bitstream *data;
  int payloadSize;
} user_data_registered_itu_t_t35_information_struct;

//! Recovery Point Information
typedef struct
{
  unsigned int  recovery_frame_cnt;
  unsigned char exact_match_flag;
  unsigned char broken_link_flag;
  unsigned char changing_slice_group_idc;

  Bitstream *data;
  int payloadSize;
} recovery_point_information_struct;

//! Picture timing Information
typedef struct
{
  int cpb_removal_delay;
  int dpb_output_delay;
  int pic_struct;
  Boolean clock_timestamp_flag[MAX_PIC_STRUCT_VALUE];
  int ct_type;
  Boolean nuit_field_based_flag;
  int counting_type;
  Boolean full_timestamp_flag;
  Boolean discontinuity_flag;
  Boolean cnt_dropped_flag;
  int n_frames;
  int seconds_value;
  int minutes_value;
  int hours_value;
  Boolean seconds_flag;
  Boolean minutes_flag;
  Boolean hours_flag;
  int time_offset;

  Bitstream *data;
  int payloadSize;
} pictiming_information_struct;


//! Decoded reference picture marking repetition Information
typedef struct
{
  Boolean original_idr_flag;
  int original_frame_num;
  Boolean original_field_pic_flag;
  Boolean original_bottom_field_flag;
  DecRefPicMarking_t *dec_ref_pic_marking_buffer_saved;

  Bitstream *data;
  int payloadSize;
} drpm_repetition_information_struct;


//! Frame packing arrangement Information
typedef struct
{
  unsigned int  frame_packing_arrangement_id;
  Boolean       frame_packing_arrangement_cancel_flag;
  unsigned char frame_packing_arrangement_type;
  Boolean       quincunx_sampling_flag;
  unsigned char content_interpretation_type;
  Boolean       spatial_flipping_flag;
  Boolean       frame0_flipped_flag;
  Boolean       field_views_flag;
  Boolean       current_frame_is_frame0_flag;
  Boolean       frame0_self_contained_flag;
  Boolean       frame1_self_contained_flag;
  unsigned char frame0_grid_position_x;
  unsigned char frame0_grid_position_y;
  unsigned char frame1_grid_position_x;
  unsigned char frame1_grid_position_y;
  unsigned char frame_packing_arrangement_reserved_byte;
  unsigned int  frame_packing_arrangement_repetition_period;
  Boolean       frame_packing_arrangement_extension_flag;

  Bitstream *data;
  int payloadSize;
} frame_packing_arrangement_information_struct;


//! Post Filter Hints Information
typedef struct
{
  unsigned int  filter_hint_size_y;
  unsigned int  filter_hint_size_x;
  unsigned int  filter_hint_type;
  int           ***filter_hint;
  unsigned int  additional_extension_flag;

  Bitstream *data;
  int payloadSize;
} post_filter_information_struct;

typedef struct
{
  unsigned int  tone_map_id;
  unsigned char tone_map_cancel_flag;
  unsigned int  tone_map_repetition_period;
  unsigned char coded_data_bit_depth;
  unsigned char sei_bit_depth;
  unsigned int  model_id;
  // variables for model 0
  int  min_value;
  int  max_value;
  // variables for model 1
  int  sigmoid_midpoint;
  int  sigmoid_width;
  // variables for model 2
  int start_of_coded_interval[1<<MAX_SEI_BIT_DEPTH];
  // variables for model 3
  int num_pivots;
  int coded_pivot_value[MAX_NUM_PIVOTS];
  int sei_pivot_value[MAX_NUM_PIVOTS];

  Bitstream *data;
  int payloadSize;
} ToneMappingSEI;

 
#if JVET_AK0107_MODALITY_INFORMATION
//! Modality Infomation
typedef struct
{
  Boolean modality_info_cancel_flag;
  Boolean modality_info_persistence_flag;
  int modality_type;
  Boolean spectrum_range_present_flag;
  int min_wavelength_mantissa; 
  int min_wavelength_exponent_plus15; 
  int max_wavelength_mantissa;
  int max_wavelength_exponent_plus15; 
  Bitstream *data;
  int payloadSize;
} modality_information_struct;
#if GFV_ENABLE
#define MAX_NUM_GFV_CNT 65536 

#if JVET_AJ0207_GFV_SEI
//! Generative Face Video
typedef struct {
  unsigned int gfv_number;
  unsigned int gfv_current_id;

  Boolean gfv_base_pic_flag;
  Boolean gfv_nn_present_flag;
  unsigned int gfv_nn_mode_idc;
  char gfv_nn_tag_uri[4096];
  char gfv_nn_uri[4096];

  Boolean gfv_chroma_key_info_present_flag;
  Boolean gfv_chroma_key_value_present_flag[3];
  unsigned int gfv_chroma_key_value[3];
  Boolean gfv_chroma_key_thr_present_flag[2];
  unsigned int gfv_chroma_key_thr_value[2];

  unsigned int gfv_id;
  unsigned int gfv_cnt;
  Boolean gfv_drive_pic_fusion_flag;
  Boolean gfv_low_confidence_face_parameter_flag;

  Boolean gfv_coordinate_present_flag;
  Boolean gfv_coordinate_pred_flag;
  unsigned int gfv_coordinate_quantization_factor;
  unsigned int gfv_coordinate_point_num;
  Boolean gfv_3d_coordinate_flag;
  unsigned int gfv_coordinate_z_max_value;
  double* gfv_coordinate_x;
  double* gfv_coordinate_y;
  double* gfv_coordinate_z;

  Boolean gfv_matrix_present_flag;
  Boolean gfv_matrix_pred_flag;
  unsigned int gfv_matrix_element_precision_factor;
  unsigned int gfv_num_matrix_type;
  unsigned int* gfv_matrix_type_idx;
  Boolean* gfv_num_matrices_to_num_kps_flag;
  unsigned int* gfv_num_matrices_info;
  Boolean* gfv_matrix_3D_space_flag;
  unsigned int* gfv_num_matrices;
  unsigned int* gfv_matrix_width;
  unsigned int* gfv_matrix_height;
  double**** gfv_matrix_element;

  char gfv_payload_filename[4096];
  unsigned long gfv_payload_length;
  unsigned char* gfv_payload_byte;
  
  unsigned int* gfv_num_matrices_store;
  unsigned int* gfv_matrix_width_store;
  unsigned int* gfv_matrix_height_store;

  Bitstream *data;
  int payloadSize;
} generative_face_video_struct;

// Additional coordinate arrays from header file
extern double* base_coordinate_x_rec;
extern double* base_coordinate_y_rec;
extern double* base_coordinate_z_rec;
extern double* prev_coordinate_x_rec;
extern double* prev_coordinate_y_rec;
extern double* prev_coordinate_z_rec;

// Additional matrix arrays from header file
extern unsigned int* base_matrix_width_vec;
extern unsigned int* base_matrix_height_vec;
extern unsigned int* base_num_matrices_vec;

// For matrix element
extern double**** base_matrix_rec;
extern double**** prev_matrix_rec;

// For pred coding
extern Boolean do_update_gfv_coordinate;
extern Boolean do_update_gfv_matrix;
#endif

#if JVET_AK0239_GFVE_SEI
//! Generative Face Video Enhancement
typedef struct {
  unsigned int gfve_number;
  unsigned int gfve_current_id;

  Boolean gfve_base_pic_flag;
  Boolean gfve_nn_present_flag;
  unsigned int gfve_nn_mode_idc;
  char gfve_nn_tag_uri[4096];
  char gfve_nn_uri[4096];

  unsigned int gfve_id;
  unsigned int gfve_gfv_cnt;
  unsigned int gfve_gfv_id;

  Boolean gfve_matrix_present_flag;
  Boolean gfve_matrix_pred_flag;
  unsigned int gfve_matrix_element_precision_factor;
  unsigned int gfve_num_matrices;
  unsigned int* gfve_matrix_width;
  unsigned int* gfve_matrix_height;
  double*** gfve_matrix_element;

  char gfve_payload_filename[4096];
  unsigned long gfve_payload_length;
  unsigned char* gfve_payload_byte;

  unsigned int gfve_pupil_present_idx;
  unsigned int gfve_pupil_coordinate_precision_factor;
  double gfve_pupil_left_eye_coordinate_x;
  double gfve_pupil_left_eye_coordinate_y;
  double gfve_pupil_right_eye_coordinate_x;
  double gfve_pupil_right_eye_coordinate_y;

  Bitstream *data;
  int payloadSize;
} generative_face_video_enhancement_struct;

// Additional matrix arrays from header file
extern unsigned int* base_gfve_matrix_width_vec;
extern unsigned int* base_gfve_matrix_height_vec;

// For matrix element
extern double*** base_gfve_matrix_rec;
extern double*** prev_gfve_matrix_rec;

// For pred coding
extern unsigned int base_gfve_num_matrices;
extern unsigned int base_gfve_matrix_element_precision_factor;
extern Boolean do_update_gfve_matrix;

// For pupil prediction
extern double base_gfve_left_pupil_coordinate_x;
extern double base_gfve_left_pupil_coordinate_y;
extern double base_gfve_right_pupil_coordinate_x;
extern double base_gfve_right_pupil_coordinate_y;
extern double prev_gfve_left_pupil_coordinate_x;
extern double prev_gfve_left_pupil_coordinate_y;
extern double prev_gfve_right_pupil_coordinate_x;
extern double prev_gfve_right_pupil_coordinate_y;

// For pupil pred coding
extern Boolean check_base_pic_pupil_present_idx;
extern Boolean do_update_gfve_pupil_coordinate;
#endif
#endif

// Globals
struct sei_params {
  Boolean seiHasDRPMRepetition_info;
  drpm_repetition_information_struct seiDRPMRepetition;

  //!< sei_message[0]: this struct is to store the sei message packetized independently
  //!< sei_message[1]: this struct is to store the sei message packetized together with slice data
  sei_struct sei_message[2];
  Boolean seiHasSparePicture;
  //extern Boolean sei_has_sp;
  spare_picture_struct seiSparePicturePayload;
  Boolean seiHasSubseqInfo;
  subseq_information_struct seiSubseqInfo[MAX_LAYER_NUMBER];
  Boolean seiHasSubseqLayerInfo;
  subseq_layer_information_struct seiSubseqLayerInfo;
  Boolean seiHasSubseqChar;
  subseq_char_information_struct seiSubseqChar;
  Boolean seiHasSceneInformation;
  scene_information_struct seiSceneInformation;
  Boolean seiHasPanScanRectInfo;
  panscanrect_information_struct seiPanScanRectInfo;
  Boolean seiHasUser_data_unregistered_info;
  user_data_unregistered_information_struct seiUser_data_unregistered;
  Boolean seiHasUser_data_registered_itu_t_t35_info;
  user_data_registered_itu_t_t35_information_struct seiUser_data_registered_itu_t_t35;
  Boolean seiHasRecoveryPoint_info;
  recovery_point_information_struct seiRecoveryPoint;
  Boolean seiHasBufferingPeriod_info;
  bufferingperiod_information_struct seiBufferingPeriod;
  Boolean seiHasPicTiming_info;
  pictiming_information_struct seiPicTiming;
  Boolean seiHasFramePackingArrangement_info;
  frame_packing_arrangement_information_struct seiFramePackingArrangement;

  Boolean seiHasTone_mapping;
  ToneMappingSEI seiToneMapping;
  Boolean seiHasPostFilterHints_info;
  post_filter_information_struct seiPostFilterHints;
#if JVET_AK0107_MODALITY_INFORMATION
  Boolean seiHasModalityInfo; 
  modality_information_struct seiModalityInfo;
#endif

#if JVET_AJ0207_GFV_SEI
  Boolean seiHasGFV_info;
  unsigned int gfv_num_sei;
  generative_face_video_struct seiGFV[MAX_NUM_GFV_CNT];
#endif
#if JVET_AK0239_GFVE_SEI
  Boolean seiHasGFVE_info;
  unsigned int gfve_num_sei;
  generative_face_video_enhancement_struct seiGFVE[MAX_NUM_GFV_CNT];
#endif

  Boolean seiHasTemporal_reference;
  Boolean seiHasClock_timestamp;
  Boolean seiHasPanscan_rect;
  Boolean seiHasHrd_picture;
  Boolean seiHasFiller_payload;
  Boolean seiHasUser_data_registered_itu_t_t35;
  Boolean seiHasUser_data_unregistered;
  Boolean seiHasRef_pic_buffer_management_repetition;
  Boolean seiHasSpare_picture;
  Boolean seiHasSubseq_information;
  Boolean seiHasSubseq_layer_characteristics;
  Boolean seiHasSubseq_characteristics;

};

typedef struct sei_params SEIParameters;

// functions
extern void InitSEIMessages      (VideoParameters *p_Vid, InputParameters *p_Inp);
extern void CloseSEIMessages     (VideoParameters *p_Vid, InputParameters *p_Inp);
extern Boolean HaveAggregationSEI(VideoParameters *p_Vid);

extern void clear_sei_message    (SEIParameters *p_SEI, int id);
extern void AppendTmpbits2Buf    ( Bitstream* dest, Bitstream* source );
extern void PrepareAggregationSEIMessage(VideoParameters *p_Vid);
extern void CalculateSparePicture();
extern Boolean CompressSpareMBMap(VideoParameters *p_Vid, unsigned char **map_sp, Bitstream *bitstream);

extern void InitSubseqInfo(SEIParameters *p_SEI, int currLayer);
extern void UpdateSubseqInfo  (VideoParameters *p_Vid, InputParameters *p_Inp, int currLayer);
extern void CloseSubseqInfo   (SEIParameters *p_SEI, int currLayer);
void CloseSubseqLayerInfo();
extern void UpdateSubseqChar(VideoParameters *p_Vid);

extern void InitSceneInformation        (SEIParameters *p_SEI);
extern void CloseSceneInformation       (SEIParameters *p_SEI);
extern void UpdateSceneInformation      (SEIParameters *p_SEI, Boolean HasSceneInformation, int sceneID, int sceneTransType, int secondSceneID);
extern void FinalizeSceneInformation    (SEIParameters *p_SEI);
extern void UpdatePanScanRectInfo       (VideoParameters *p_Vid);
extern void ClearPanScanRectInfoPayload (SEIParameters *p_SEI);

extern void UpdateUser_data_unregistered(SEIParameters *p_SEI);
extern void UpdateUser_data_registered_itu_t_t35(SEIParameters *p_SEI);

extern void ClearRandomAccess(SEIParameters *p_SEI);
extern void UpdateRandomAccess(VideoParameters *p_Vid);

extern void UpdateToneMapping(SEIParameters *p_SEI);

extern void init_sei(SEIParameters *p_SEI);
extern int  Write_SEI_NALU(VideoParameters *p_Vid, int len);

extern void InitBufferingPeriod    (VideoParameters *p_Vid);
extern void ClearBufferingPeriod   (SEIParameters *p_SEI, seq_parameter_set_rbsp_t *active_sps);
extern void UpdateBufferingPeriod  (VideoParameters *p_Vid, InputParameters *p_Inp);

extern void ClearPicTiming(SEIParameters *p_SEI);
extern void UpdatePicTiming(VideoParameters *p_Vid, InputParameters *p_Inp);

extern void ClearDRPMRepetition(SEIParameters *p_SEI);
extern void UpdateDRPMRepetition(SEIParameters *p_SEI);
extern void free_drpm_buffer( DecRefPicMarking_t *pDRPM );

extern void UpdatePostFilterHints(SEIParameters *p_SEI);

extern void ComposeSparePictureMessage (SEIParameters *p_SEI, int delta_spare_frame_num, int ref_area_indicator, Bitstream *tmpBitstream);

extern void ClearFramePackingArrangement(SEIParameters *p_SEI);
extern void UpdateFramePackingArrangement(VideoParameters *p_Vid, InputParameters *p_Inp);

#if JVET_AK0107_MODALITY_INFORMATION 
extern void UpdateModalityInfo(SEIParameters *p_SEI);
#endif

#if JVET_AJ0207_GFV_SEI
extern void UpdateGFV(SEIParameters *p_SEI, InputParameters *p_Inp);
#endif
#if JVET_AK0239_GFVE_SEI
extern void UpdateGFVE(SEIParameters *p_SEI, InputParameters *p_Inp);
extern double WriteGFVEPupilCoordinate(SEIParameters *p_SEI, unsigned int gfve_idx, double coordinate, double ref_coordinate, int precision_factor, const char* eye, const char* axis);
#endif

// end of temp additions

#endif
