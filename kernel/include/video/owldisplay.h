/*************************************************************************/ /*!
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@License        Strictly Confidential.
*/ /**************************************************************************/

#ifndef __OWL_DISPLAY_H__
#define __OWL_DISPLAY_H__

#define __bool signed char

typedef struct {__u8  alpha;__u8 red;__u8 green; __u8 blue; }__disp_color_t;
typedef struct {__s32 x; __s32 y; __u32 width; __u32 height;}__disp_rect_t;
typedef struct {__u32 width;__u32 height;                   }__disp_rectsz_t;
typedef struct {__s32 x; __s32 y;                           }__disp_pos_t;


#define MAX_CALLBACK_NUM 10

#define OWL_DCQ_DEPTH	 11

typedef void (*callback)(void *, int);

typedef void * callback_arg;

typedef enum
{
    DISP_MOD_INTERLEAVED        =0x1,
    DISP_MOD_NON_MB_PLANAR      =0x0,
    DISP_MOD_NON_MB_UV_COMBINED =0x2,
    DISP_MOD_MB_PLANAR          =0x4,
    DISP_MOD_MB_UV_COMBINED     =0x6,
}__disp_pixel_mod_t;

typedef enum
{
    DISP_SEQ_ARGB   =0x0,
    DISP_SEQ_BGRA   =0x2,
    
    DISP_SEQ_UYVY   =0x3,  
    DISP_SEQ_YUYV   =0x4,
    DISP_SEQ_VYUY   =0x5,
    DISP_SEQ_YVYU   =0x6,
    
    DISP_SEQ_AYUV   =0x7,  
    DISP_SEQ_VUYA   =0x8,
    
    DISP_SEQ_UVUV   =0x9,  
    DISP_SEQ_VUVU   =0xa,
    
    DISP_SEQ_P10    = 0xd,
    DISP_SEQ_P01    = 0xe,
    
    DISP_SEQ_P3210  = 0xf,
    DISP_SEQ_P0123  = 0x10,
    
    DISP_SEQ_P76543210  = 0x11,
    DISP_SEQ_P67452301  = 0x12,
    DISP_SEQ_P10325476  = 0x13,
    DISP_SEQ_P01234567  = 0x14,
    
    DISP_SEQ_2BPP_BIG_BIG       = 0x15,
    DISP_SEQ_2BPP_BIG_LITTER    = 0x16,
    DISP_SEQ_2BPP_LITTER_BIG    = 0x17,
    DISP_SEQ_2BPP_LITTER_LITTER = 0x18,
    
    DISP_SEQ_1BPP_BIG_BIG       = 0x19,
    DISP_SEQ_1BPP_BIG_LITTER    = 0x1a,
    DISP_SEQ_1BPP_LITTER_BIG    = 0x1b,
    DISP_SEQ_1BPP_LITTER_LITTER = 0x1c,
}__disp_pixel_seq_t;

typedef enum
{
    DISP_3D_SRC_MODE_TB = 0x0,
    DISP_3D_SRC_MODE_FP = 0x1,
    DISP_3D_SRC_MODE_SSF = 0x2,
    DISP_3D_SRC_MODE_SSH = 0x3,
    DISP_3D_SRC_MODE_LI = 0x4,
}__disp_3d_src_mode_t;

typedef enum
{
    DISP_3D_OUT_MODE_CI_1 = 0x5,
    DISP_3D_OUT_MODE_CI_2 = 0x6,
    DISP_3D_OUT_MODE_CI_3 = 0x7,
    DISP_3D_OUT_MODE_CI_4 = 0x8,
    DISP_3D_OUT_MODE_LIRGB = 0x9,

    DISP_3D_OUT_MODE_TB = 0x0,
    DISP_3D_OUT_MODE_FP = 0x1,
    DISP_3D_OUT_MODE_SSF = 0x2,
    DISP_3D_OUT_MODE_SSH = 0x3,
    DISP_3D_OUT_MODE_LI = 0x4,
    DISP_3D_OUT_MODE_FA = 0xa,
}__disp_3d_out_mode_t;

typedef enum
{
    DISP_BT601  = 0,
    DISP_BT709  = 1,
    DISP_YCC    = 2,
    DISP_VXYCC  = 3,
}__disp_cs_mode_t;

typedef enum
{
	OWL_TV_MOD_720P_50HZ           = 1,
    OWL_TV_MOD_720P_60HZ           = 2,
    OWL_TV_MOD_1080P_50HZ          = 3,
    OWL_TV_MOD_1080P_60HZ          = 4, 
    OWL_TV_MOD_576P                = 5,
    OWL_TV_MOD_480P                = 6,
    OWL_TV_MOD_DVI 				   = 7,
    OWL_TV_MOD_PAL                 = 8,
    OWL_TV_MOD_NTSC                = 9,
    OWL_TV_MOD_4K_30HZ             = 10,
    OWL_TV_MODE_NUM               =  10,    
}__owl_tv_mode_t;

typedef enum
{
    DISP_LAYER_WORK_MODE_NORMAL     = 0,
    DISP_LAYER_WORK_MODE_PALETTE    = 1,
    DISP_LAYER_WORK_MODE_INTER_BUF  = 2,
    DISP_LAYER_WORK_MODE_GAMMA      = 3,
    DISP_LAYER_WORK_MODE_SCALER     = 4,
}__disp_layer_work_mode_t;

typedef struct
{
    __u32                   addr[3];
    __u64                   buffer_id;
    __disp_rectsz_t         size;
    __u32                   format;
    __disp_pixel_seq_t      seq;
    __disp_pixel_mod_t      mode;
    __bool                  br_swap;
    __disp_cs_mode_t        cs_mode;
    __bool                  b_trd_src;
    __disp_3d_src_mode_t    trd_mode;
    __u32                   trd_right_addr[3];
    __bool                  pre_multiply;
}__disp_fb_t;

typedef struct
{
    __disp_layer_work_mode_t    mode;
    __bool                      b_from_screen;
    __u8                        pipe;
    __u8                        prio;
    __bool                      alpha_en;
    __u16                       alpha_val;
    __bool                      ck_enable;
    __disp_rect_t               src_win;
    __disp_rect_t               scn_win;
    __disp_fb_t                 fb;
    __bool                      b_trd_out;
    __disp_3d_out_mode_t        out_trd_mode;
    __u8                        rotate;
}__disp_layer_info_t;

typedef struct
{   
    int                 post2_layers;
    __disp_layer_info_t layer_info[8];
    __disp_rect_t       fb_scn_win;

    int                 primary_display_layer_num;
    int                 show_black[2];
    int                 time_stamp;
}setup_dispc_data_t;

struct owl_disp_info
{
	spinlock_t info_lock;
	int state;
	int index;
	setup_dispc_data_t psDispcData;
	callback mCallBack;
	callback_arg mCallBackArg;
	struct list_head list;	
};
struct owlfb_dc
{	
	struct device * dev;	   
    struct mutex dc_lock;
    struct work_struct dc_work;
    struct workqueue_struct * dc_workqueue;
    struct owl_disp_info dc_queue[OWL_DCQ_DEPTH];
    struct owl_overlay_manager * primary_manager;
    struct owl_overlay_manager * external_manager;
    struct list_head q_list;
    bool working;  
      	
};

#endif
