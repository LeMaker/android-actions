/********************************************************************************
 *                              usdk1100
 *                            Module: Face application
 *                 Copyright(c) 2003-2008 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>    <time>           <version >             <desc>
 *       noahkong  2010-03-11 14:47     1.0             build this file
 ********************************************************************************/
/*!
 * \file     faceApp.h
 * \brief    face detection
 * \author   noahkong
 * \version 1.0
 * \date  2010/03/11
 *******************************************************************************/
#ifndef __ALFACE_ON_H__
#define __ALFACE_ON_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*! rectangle area
 */
typedef struct
{
	int x;
	int y;
	int w;
	int h;
} DRect;

/*! point
 */
typedef struct
{
	int x;
	int y;
} DPoint;

/*! mask type
 */
typedef enum
{
	MMM_DISABLE_ALL,
	MMM_PHOTO_FRAME = 0x1, /* full image */
	MMM_SEYE_MASK = 0x101, /* single eye */
	MMM_DEYE_MASK, /* double eye */
	MMM_NOSE_MASK, /* nose */
	MMM_MOUTHUP_MASK, /* mouth up */
	MMM_MOUTH_MASK, /* mouth*/
	MMM_FACE_MASK, /* face */
	MMM_MOUTHDOWN_MASK, /* mouth down */
	MMM_FOREHEAD_MASK, /* forehead */
	MMM_HEAD_MASK, /* head */
	MMM_BREAD_MASK, /* bread */
} ALMasque_type_t;

typedef struct
{
	ALMasque_type_t mas_type;
	int mas_enable;
	short width;
	short height;
	unsigned char *im_yv12;
	unsigned char *im_alpha;
	DPoint CPi[3]; /* the central point of eyes and mouth */
} ALMask_info_t;

typedef struct
{
	int width;
	int height;
	int bk_param;
} ALFace_app_init_t;

typedef struct
{
	/*! reference frame */
	unsigned char *pre_img;
	/*! current frame */
	unsigned char *cur_img;
	/*! width */
	int width;
	/*! height */
	int height;
	/*! detected max faces */
	int max_faces;
	/*! */
	int photoframe_enable;
	/*! backup image width */
	int bk_width;
	/*! backup image width */
	int bk_height;
	/*! buffer of backup image */
	unsigned char *bk_img;
	/*! image format YUV420p or yuv420ps */
	int img_fmt;
	int stride;
#if 1
	/*! new add! for pad rotation and front/back camera */
	int dir;
	int isfront;
#endif
} ALFace_img_t;

/*! face
*/
typedef struct
{
	/*! face area */
	DRect *rt;
	/*! face number */
	int RectNum;

	unsigned int ptr[2];
} ALDface_out_t;

/*! eye
 */
typedef struct
{
	/*! central point of left eye */
	DPoint *lrt;
	/*! central point of right eye */
	DPoint *rrt;
	/*! number of left eye */
	int LRectNum;
	/*! number of right eye */
	int RRectNum;

	unsigned int ptr[2];
} ALDeye_out_t;

/*! mouth
 */
typedef struct
{
	/*! up */
	DPoint tP;
	/*! down */
	DPoint bP;
	/*! left */
	DPoint lP;
	/*! right */
	DPoint rP;
	/*! centre */
	DPoint cP;

	unsigned int ptr[2];
} ALDmouth_out_t;

typedef struct
{
	/* input image buffer */
	unsigned char *buf;
	/* image width */
	unsigned int width;
	/* image height */
	unsigned int height;
	/*! smile */
	int smile_status;
	/*! face index: 0 1..., -1 is no face */
	int faceidx;
	/*! face */
	ALDface_out_t faceout;
	/*! eye */
	ALDeye_out_t eyeout;
	/*! mouth */
	ALDmouth_out_t *mouthout;
	/*! mask number */
	int mask_num;
	/*! mask */
	ALMask_info_t *face_mask;
} ALFace_appout_t;

typedef struct
{
	ALFace_img_t *fim;
	ALFace_appout_t fout;
} ALFace_mask_param_t;

typedef enum
{
	/*! mask */
	FSET_PHOTO_MASK_ENABLE = 0x1,
	FSET_PHOTO_MASK_DISABLE = 0x2,
	FSET_FACE_GEN = 0x100,
	/*! face detection */
	FSET_FACE_DET_A = 0x101,
	/*! face, eye and mouth detection */
	FSET_FACE_DET_B,
	FSET_FACE_DISABLE,
	/*! smile detection */
	FSET_SIMLE_DET,
	FSET_SIMLE_DISABLE,
	/*! close mask */
	FSET_MASK_DISABLE,
	FSET_MASK_ENABLE,
	FGET_STATUS,
	FSET_RESET,

	FSET_DRAW_FACE,
	FSET_FACE_GRAY,
} ALFace_cmd_t;

#if 0
/*! init */
void *face_app_init(ALFace_app_init_t *);
/*! run */
int face_app_run(void *handle, unsigned int cmd, unsigned long args);
/*! release */
int face_app_dispose(void *handle);
int face_mask_run(void *handle, unsigned int cmd, unsigned long args);
#endif
#endif
