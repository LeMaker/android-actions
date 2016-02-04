#ifndef __VCE_COMMON_H__
#define __VCE_COMMON_H__

#include "vce_cfg.h"

enum
{
	ENC_PREVIEW,
	ENC_ENCODE,
};

enum
{
	ENC_JPEG,
	ENC_H264,
};

/* encode input format */
enum
{
	ENC_YUV411P = 2,
	ENC_YUV411SP,
	ENC_YUV422P = 5,

	ENC_RGB565 = 6,
	ENC_BGR565 = 7,
	ENC_ARGB8888 = 8,
	ENC_ABGR8888 = 9,
	ENC_RGBA8888 = 10,
	ENC_BGRA8888 = 11,
};

/* for blending area */
typedef struct
{
	int x;
	int y;
	int w;
	int h;
} vce_rect_t;

typedef struct
{
	int x;
	int y;
	int w;
	int h;
	int stride;
	int dstw;
	int dsth;
} vce_blend_t;

enum
{
	BLEND_ARGB8888 = 0x1,
	BLEND_ABGR8888 = 0x2,
	BLEND_RGBA8888 = 0x4,
	BLEND_BGRA8888 = 0x8,
	BLEND_RGB565 = 0x10,
	BLEND_BGR565 = 0x20,
	BLEND_YUV420SP = 0x40,
	BLEND_YVU420SP = 0x80,
	BLEND_YUV422SP = 0x100,
	BLEND_YVU422SP= 0x200,
};

/* alignment */
#define ALIGN_SRC_STRIDE(x)    (((x) + 0xf) & (~0xf))
#define ALIGN_SRC_HEIGHT(x)    (((x) + 0x0) & (~0x7))
#define ALIGN_SRC_WIDTH(x)     (((x) + 0x0) & (~0x7))
#define ALIGN_SRC_OFFX(x)      (((x) + 0x0) & (~0x3f))
#define ALIGN_SRC_OFFY(x)      (((x) + 0x0) & (~0x7))
#define ALIGN_DST_HEIGHT(x)    (((x) + 0xf) & (~0xf))
#define ALIGN_DST_WIDTH(x)     (((x) + 0xf) & (~0xf))
#define ALIGN_16(x)            (((x) + 0xf) & (~0xf))
#define ALIGN_32(x)            (((x) + 0x1f) & (~0x1f))

#endif
