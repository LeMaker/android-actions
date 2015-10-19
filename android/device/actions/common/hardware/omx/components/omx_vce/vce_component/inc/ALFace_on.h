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
 * \brief    定义与中间件的接口及参数
 * \author   孔文海
 * \version 1.0
 * \date  2010/03/11
 *******************************************************************************/
#ifndef __ALFACE_ON_H__
#define	__ALFACE_ON_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*!
 * \brief
 *      矩形区域
 */
typedef struct
{
	/*! 左上角横向座标 */
	int x;
	/*! 左上角纵向座标 */
	int y;
	/* 矩形宽 */
	int w;
	/* 矩形高 */
	int h;
} DRect;

/*!
 * \brief
 *      点
 */
typedef struct
{
	/*! 横向座标 */
	int x;
	/*! 纵向座标 */
	int y;
} DPoint;

/*!
 * \brief
 *      人脸特效类型
 *      默认标示眼嘴胡子等的中心坐标，图像区域为紧凑掩模区域
 */
typedef enum
{
	/*! 无特效 */
	MMM_DISABLE_ALL,
	/*! 大头贴 */
	MMM_PHOTO_FRAME = 0x1,
	/*! 单眼特效 */
	MMM_SEYE_MASK = 0x101,
	/*! 双眼特效，如眼镜等 */
	MMM_DEYE_MASK,
	/*! 面具特效，用眼和嘴中心位置标定 */
	MMM_NOSE_MASK,
	/*! 嘴巴特效，用嘴巴中心标定 */
	MMM_MOUTHUP_MASK,
	/*! 下胡子特效，用下胡子上边缘中心标定 */
	MMM_MOUTH_MASK,
	/*! 上胡子特效 ，用胡子中心标定（暂不支持）*/
	MMM_FACE_MASK,
	/*! 鼻子特效，用鼻中心标定 （暂不支持）*/
	MMM_MOUTHDOWN_MASK,
	/*! 额头特效，（暂不支持） */
	MMM_FOREHEAD_MASK,
	/*! 头饰特效， （暂不支持）*/
	MMM_HEAD_MASK,
	/*! 脸颊胡子 */
	MMM_BREAD_MASK,
} ALMasque_type_t;

/*!
 * \brief
 *      掩模特效参数，同时需要作为JPG encoder初始化参数
 */
typedef struct
{
	ALMasque_type_t mas_type;
	int mas_enable;
	short width;
	short height;
	unsigned char *im_yv12;
	unsigned char *im_alpha;
	/*! 双眼中心，嘴中心 */
	DPoint CPi[3];
} ALMask_info_t;

/*!
 * \brief
 *      人脸特效应用初始化结构
 */
typedef struct
{
	/*! 图像宽 */
	int width;
	/*! 图像高 */
	int height;
	/*! 备份参数 */
	int bk_param;
} ALFace_app_init_t;

/*!
 * \brief
 *      运行使输入待检测含人脸图像
 */
typedef struct
{
	/*! 上一帧图像 */
	unsigned char *pre_img;
	/*! 当前图像 */
	unsigned char *cur_img;
	/*! 图像宽 */
	int width;
	/*! 图像高 */
	int height;
	/*! 最大人脸数目 */
	int max_faces;
	/**/
	int photoframe_enable;
	/*! 背景图像宽 */
	int bk_width;
	/*! 背景图像高 */
	int bk_height;
	/*! 背景图像buffer */
	unsigned char *bk_img;
	/*! image format YUV420p or yuv420ps */
	int img_fmt;
	int stride;
#if 1
	int dir;         /*pad屏的旋转角度*/
	int isfront;  /*是否是前置摄像头*/
#endif
} ALFace_img_t;

/*!
 * \brief
 *      人脸检测输出
 */
typedef struct
{
	/*! 人脸区域 */
	DRect *rt;
	/*! 检测到的人脸个数 */
	int RectNum;
	/*！备用指针 */
	unsigned int ptr[2];
} ALDface_out_t;

/*!
 * \brief
 *      人眼检测输出
 */
typedef struct
{
	/*! 左眼候选中心 */
	DPoint *lrt;
	/*! 右眼候选中心 */
	DPoint *rrt;
	/*! 左眼个数 */
	int LRectNum;
	/*! 右眼个数 */
	int RRectNum;
	/*! 备用指针 */
	unsigned int ptr[2];
} ALDeye_out_t;

/*!
 * \brief
 *      人嘴检测输出
 */
typedef struct
{
	/*! 上唇 */
	DPoint tP;
	/*! 下唇 */
	DPoint bP;
	/*! 左嘴角 */
	DPoint lP;
	/*! 右嘴角 */
	DPoint rP;
	/*! 中心 */
	DPoint cP;
	/*! 备用指针 */
	unsigned int ptr[2];
} ALDmouth_out_t;

/*!
 * \brief
 *      检测到特征参考信息
 */
typedef struct
{
	/* 输入图像buf */
	unsigned char *buf;
	/* 检测图像宽 */
	unsigned int width;
	/* 检测图像高 */
	unsigned int height;
	/*! 笑脸状态 */
	int smile_status;
	/*! 被检测人脸序号 0...;-1为人脸区域 */
	int faceidx;
	/*! 被检测人脸信息 */
	ALDface_out_t faceout;
	/*! 被检测人眼信息 */
	ALDeye_out_t eyeout;
	/*! 被检测人嘴信息 */
	ALDmouth_out_t *mouthout;
	/*! 有效掩模总数 */
	int mask_num;
	/*! 掩模信息 */
	ALMask_info_t *face_mask;
} ALFace_appout_t;

typedef struct
{
	ALFace_img_t *fim;
	ALFace_appout_t fout;
} ALFace_mask_param_t;

typedef enum
{
	/*! 使能大头贴 */
	FSET_PHOTO_MASK_ENABLE = 0x1,
	FSET_PHOTO_MASK_DISABLE = 0x2,
	FSET_FACE_GEN = 0x100,
	/*! 使能人脸检测 */
	FSET_FACE_DET_A = 0x101,
	/*! HAAR检测 */
	FSET_FACE_DET_B,
	FSET_FACE_DISABLE,
	/*! 使能笑脸检测 */
	FSET_SIMLE_DET,
	FSET_SIMLE_DISABLE,
	/*! 关闭所有特效 */
	FSET_MASK_DISABLE,
	FSET_MASK_ENABLE,
	FGET_STATUS,
	FSET_RESET,
	/*! 眼镜特效 */
	//	FSET_EYE_MASK,
	//	/*! 面具特效 */
	//	FSET_FACE_MASK,
	//	/*! 鼻子特效 */
	//	FSET_NOSE_MASK,
	//	/*! 嘴巴特效 */
	//	FSET_MOUTH_MASK,
	//	/*! 胡子特效 */
	//	FSET_MOUTHUP_MASK,
	//	/*! 胡子特效 */
	//	FSET_MOUTHDOWN_MASK,
	//	/*! 额头特效 */
	//	FSET_FOREHEAD_MASK,
	//	/*! 头饰特效 */
	//	FSET_HEAD_MASK,
	/*! 画人脸框 */
	FSET_DRAW_FACE,
	FSET_FACE_GRAY,
} ALFace_cmd_t;

#if 0
/*! 人脸应用初始化 */
void *face_app_init(ALFace_app_init_t *);
/*! 人脸应用运行 */
int face_app_run(void *handle,unsigned int cmd,unsigned long args);
/*! 人脸应用释放资源 */
int face_app_dispose(void *handle);
int face_mask_run(void *handle,unsigned int cmd,unsigned long args);
#endif
#endif
