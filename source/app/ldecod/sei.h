
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

#if NNPF_ENABLE 
  SEI_NNPFC = 210,
  SEI_NNPFA = 211,
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

#if NNPF_ENABLE
typedef struct
{
  unsigned int nnpfa_target_id;
  Boolean nnpfa_cancel_flag;
  Boolean nnpfa_persistence_flag;
  Boolean nnpfa_target_base_flag;
  Boolean nnpfa_no_prev_clvs_flag;
  Boolean nnpfa_no_foll_clvs_flag;
  unsigned int nnpfa_num_output_entries;
  Boolean *nnpfa_output_flag;
  Boolean nnpfa_prompt_update_flag;
  char nnpfa_prompt[4096];
  Boolean nnpfa_seed_update_flag;
  int nnpfa_seed;
  unsigned char nnpfa_num_input_pic_shift;
  Bitstream *data;
  int payloadSize;
} NNPFASEI;

typedef struct {
  unsigned int nnpfc_purpose; // u(16)
  unsigned int nnpfc_id; // ue(v)
  Boolean nnpfc_base_flag; // u(1)
  unsigned int nnpfc_mode_idc; // ue(v)
  char nnpfc_tag_uri[4096]; // st(v) if mode_idc==1
  char nnpfc_uri[4096]; // st(v) if mode_idc==1
  Boolean nnpfc_property_present_flag; // u(1)
  /* Property Present Fields */
  unsigned int nnpfc_num_input_pics_minus1;   // ue(v)
  Boolean* nnpfc_input_pic_filtering_flag;    // u(1)[num_input_pics_minus1+1]
  Boolean nnpfc_absent_input_pic_zero_flag;   // u(1)
  Boolean nnpfc_out_sub_c_flag;               // u(1)
  unsigned int nnpfc_out_colour_format_idc;   // u(2)
  unsigned int nnpfc_pic_width_num_minus1;    // ue(v)
  unsigned int nnpfc_pic_width_denom_minus1;  // ue(v)
  unsigned int nnpfc_pic_height_num_minus1;   // ue(v)
  unsigned int nnpfc_pic_height_denom_minus1; // ue(v)
  unsigned int* nnpfc_interpolated_pics;      // ue(v)[num_input_pics_minus1]
  unsigned int nnpfc_extrapolated_pics_minus1;// ue(v)
  int nnpfc_spatial_extrapolation_left_offset;              // se(v)
  int nnpfc_spatial_extrapolation_right_offset;             // se(v)
  int nnpfc_spatial_extrapolation_top_offset;               // se(v)
  int nnpfc_spatial_extrapolation_bottom_offset;            // se(v)
  Boolean nnpfc_component_last_flag;          // u(1)
  unsigned int nnpfc_inp_format_idc;          // ue(v)
  unsigned int nnpfc_auxiliary_inp_idc;       // ue(v)
  Boolean nnpfc_inband_prompt_flag;           // u(1)
  char nnpfc_prompt[4096];                    // st(v)
  Boolean nnpfc_inband_seed_flag;             // u(1)
  unsigned int nnpfc_seed;                    // u(16)
  unsigned int nnpfc_inp_order_idc;           // ue(v)
  unsigned int nnpfc_inp_tensor_luma_bitdepth_minus8;       // ue(v)
  unsigned int nnpfc_inp_tensor_chroma_bitdepth_minus8;     // ue(v)
  unsigned int nnpfc_out_format_idc;          // ue(v)
  unsigned int nnpfc_out_order_idc;           // ue(v)
  unsigned int nnpfc_out_tensor_luma_bitdepth_minus8;       // ue(v)
  unsigned int nnpfc_out_tensor_chroma_bitdepth_minus8;     // ue(v)
  Boolean nnpfc_separate_colour_description_present_flag;         // u(1)
  unsigned int nnpfc_colour_primaries;        // u(8)
  unsigned int nnpfc_transfer_characteristics;// u(8)
  unsigned int nnpfc_matrix_coeffs;           // u(8)
  Boolean nnpfc_full_range_flag;              // u(1)
  Boolean nnpfc_chroma_loc_present_flag;      // u(1)
  unsigned int nnpfc_chroma_sample_loc_type_frame;         // ue(v)
  unsigned int nnpfc_overlap;                 // ue(v)
  Boolean nnpfc_constant_patch_size_flag;          // u(1)
  unsigned int nnpfc_patch_width_minus1;             // ue(v)
  unsigned int nnpfc_patch_height_minus1;            // ue(v)
  unsigned int nnpfc_extended_patch_width_cd_delta_minus1;         // ue(v)
  unsigned int nnpfc_extended_patch_height_cd_delta_minus1;        // ue(v)
  unsigned int nnpfc_padding_type;            // ue(v)
  unsigned int nnpfc_luma_padding_val;            // ue(v)
  unsigned int nnpfc_cb_padding_val;              // ue(v)
  unsigned int nnpfc_cr_padding_val;              // ue(v)
  Boolean nnpfc_complexity_info_flag;         // u(1)
  unsigned int nnpfc_parameter_type_idc;              // u(2)
  unsigned int nnpfc_log2_parameter_bit_length_minus3;       // u(2)
  unsigned int nnpfc_num_parameters_idc;              // u(6)
  unsigned int nnpfc_num_kmac_operations_idc;                // ue(v)
  unsigned int nnpfc_total_kilobyte_size;                // ue(v)
  unsigned int nnpfc_num_metadata_extension_bits;       // ue(v)
  Boolean nnpfc_application_purpose_tag_uri_present_flag;         // u(1)
  char nnpfc_application_purpose_tag_uri[4096];           // st(v)
  unsigned int nnpfc_scan_type_idc;               // u(2)
  unsigned int nnpfc_for_human_viewing_idc;           // u(2)
  unsigned int nnpfc_for_machine_analysis_idc;        // u(2)
  unsigned int nnpfc_reserved_metadata_extension;       // u(v)

  /* Mode IDC 0 Payload */
  unsigned char* nnpfc_payload_byte;          // b(8)[]
  unsigned int nnpfc_payload_size;
  char nnpfc_payload_filename[4096];

  Bitstream *data;
  int payloadSize;                            
} NNPFCSEI;
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
#if NNPF_ENABLE
void interpret_nnpfc_info                             ( byte* payload, int size, VideoParameters *p_Vid );
void interpret_nnpfa_info                             ( byte* payload, int size, VideoParameters *p_Vid );
#endif
#if (ENABLE_OUTPUT_TONEMAPPING)
void tone_map               (imgpel** imgX, imgpel* lut, int size_x, int size_y);
void init_tone_mapping_sei  (ToneMappingSEI *seiToneMapping);
void update_tone_mapping_sei(ToneMappingSEI *seiToneMapping);
#endif
#endif
