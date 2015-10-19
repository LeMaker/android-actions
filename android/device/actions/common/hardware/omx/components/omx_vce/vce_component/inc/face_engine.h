#ifndef __FACE_ENGINE_H__
#define __FACE_ENGINE_H__

#include "ALFace_on.h"

typedef enum
{
  FE_OPEN,
  FE_SMILE,
  FE_MASK,
  FE_DATAIN,
  FE_DRAW,
  FE_RESET,
  FE_STATUS,
  FE_DATAOUT,
  FE_CLOSE,
  FE_STOP,
  FE_RUN,
  FE_CROP,
}face_ctrl_t;

typedef enum
{
  SFE_OPENED,
  SFE_IDLE,
  SFE_DATA_READY,
  SFE_BUSY,
  SFE_STOPED,
  SFE_ERR,
}face_status_t;

typedef struct face_input_crop
{
	int dstw, dsth;
	int cropw, croph;
	int x, y;
} face_input_crop;

int face_cmd(void *engine, int cmd, unsigned long args);
int face_dispose(void * engine);
void *face_open();

extern void * (*face_app_init)(ALFace_app_init_t *);
extern int (*face_app_run)(void *, unsigned int, unsigned long);
extern int (*face_mask_run)(void *, unsigned int, unsigned long);
extern int (*face_app_dispose)(void *);

#endif
