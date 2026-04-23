/*!
 *************************************************************************************
 * \file sei.h
 *
 * \brief
 *    Prototypes for sei.c
 *************************************************************************************
 */

#ifndef SEI_H
#define SEI_H

#define JVET_AK0107_MODALITY_INFORMATION 1
#if GFV_ENABLE
#define JVET_AJ0207_GFV_SEI 1
#define JVET_AK0239_GFVE_SEI 1
#endif

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
  SEI_GREEN_METADATA=56,
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

#define MAX_FN 256
// tone mapping information
#define MAX_CODED_BIT_DEPTH  12
#define MAX_SEI_BIT_DEPTH    12
#define MAX_NUM_PIVOTS     (1<<MAX_CODED_BIT_DEPTH)

#if (ENABLE_OUTPUT_TONEMAPPING)
typedef struct tone_mapping_struct_s
{
  Boolean seiHasTone_mapping;
  unsigned int  tone_map_repetition_period;
  unsigned char coded_data_bit_depth;
  unsigned char sei_bit_depth;
  unsigned int  model_id;
  unsigned int count;
  
  imgpel lut[1<<MAX_CODED_BIT_DEPTH];                 //<! look up table for mapping the coded data value to output data value

  Bitstream *data;
  int payloadSize;
} ToneMappingSEI;
#endif

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
} frame_packing_arrangement_information_struct;


//! Green metada Information
typedef struct
{
  unsigned char  green_metadata_type;
  unsigned char  period_type;
  unsigned short num_seconds;
  unsigned short num_pictures;
  unsigned char percent_non_zero_macroblocks;
  unsigned char percent_intra_coded_macroblocks;
  unsigned char percent_six_tap_filtering;
  unsigned char percent_alpha_point_deblocking_instance;
  unsigned char xsd_metric_type;
  unsigned short xsd_metric_value;
} Green_metadata_information_struct;

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
   int modality_type_extension_bits; 
 } modality_information_struct;
#endif 

#if GFV_ENABLE
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
extern double* base_coordinate_x;
extern double* base_coordinate_y;
extern double* base_coordinate_z;
extern double* prev_coordinate_x;
extern double* prev_coordinate_y;
extern double* prev_coordinate_z;

// Additional matrix arrays from header file
extern unsigned int* base_matrix_width_vec;
extern unsigned int* base_matrix_height_vec;
extern unsigned int* base_num_matrices_vec;

// For matrix element
extern double**** base_matrix;
extern double**** prev_matrix;

// For pred coding
extern unsigned int base_coordinate_quantization_factor;
extern unsigned int base_coordinate_point_num;
extern Boolean base_3d_coordinate_flag;
extern unsigned int base_coordinate_z_max_value;
extern unsigned int base_matrix_element_precision_factor;
extern unsigned int base_num_matrix_type;
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
extern double*** base_gfve_matrix;
extern double*** prev_gfve_matrix;

// For pred coding
extern unsigned int base_gfve_num_matrices;
extern unsigned int base_gfve_matrix_element_precision_factor;

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
extern unsigned int base_gfve_pupil_coordinate_precision_factor;
#endif
#endif


void InterpretSEIMessage                                ( byte* payload, int size, VideoParameters *p_Vid, Slice *pSlice );
void interpret_spare_pic                                ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_subsequence_info                         ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_subsequence_layer_characteristics_info   ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_subsequence_characteristics_info         ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_scene_information                        ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_user_data_registered_itu_t_t35_info      ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_user_data_unregistered_info              ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_pan_scan_rect_info                       ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_recovery_point_info                      ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_filler_payload_info                      ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_dec_ref_pic_marking_repetition_info      ( byte* payload, int size, VideoParameters *p_Vid, Slice *pSlice );
void interpret_full_frame_freeze_info                   ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_full_frame_freeze_release_info           ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_full_frame_snapshot_info                 ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_progressive_refinement_start_info        ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_progressive_refinement_end_info          ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_motion_constrained_slice_group_set_info  ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_reserved_info                            ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_buffering_period_info                    ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_picture_timing_info                      ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_film_grain_characteristics_info          ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_deblocking_filter_display_preference_info( byte* payload, int size, VideoParameters *p_Vid );
void interpret_stereo_video_info_info                   ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_post_filter_hints_info                   ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_tone_mapping                             ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_frame_packing_arrangement_info           ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_green_metadata_info                       (byte* payload, int size, VideoParameters *p_Vid );
#if JVET_AK0107_MODALITY_INFORMATION
void interpret_modality_info                            ( byte* payload, int size, VideoParameters *p_Vid );
#endif
#if GFV_ENABLE
#if JVET_AJ0207_GFV_SEI
void interpret_gfv_info                                 ( byte* payload, int size, VideoParameters *p_Vid );
#endif
#if JVET_AK0239_GFVE_SEI
void interpret_gfve_info                                ( byte* payload, int size, VideoParameters *p_Vid );
extern double ReadGFVEPupilCoordinate                   (Bitstream **ptr_buf, double ref_coordinate, int precision_factor, const char* eye, const char* axis);
#endif
#endif

#if (ENABLE_OUTPUT_TONEMAPPING)
void tone_map               (imgpel** imgX, imgpel* lut, int size_x, int size_y);
void init_tone_mapping_sei  (ToneMappingSEI *seiToneMapping);
void update_tone_mapping_sei(ToneMappingSEI *seiToneMapping);
#endif
#endif
