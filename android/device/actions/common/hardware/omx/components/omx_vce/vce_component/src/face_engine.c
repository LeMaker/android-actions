#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <asm/unistd.h>
#include "tsemaphore.h"
#include "omx_comp_debug_levels.h"
#include "face_engine.h"
#include "log.h"

typedef struct
{
	pthread_t face_thread;
	tsem_t cmd_mutex;
	tsem_t task_syn;
	int cmd_pending;
	int cmd;
	unsigned int tv;
	unsigned long args;
	unsigned int status;
	int result;

	void *facehandle;
	ALFace_appout_t face_appout;

	int face_det_ok;
	face_input_crop crop;

	ALMask_info_t maskinfo[16];
	unsigned char mask_enable[12];
	int is_mask_en;
	int is_smile_en;
	unsigned long smile_args;
	unsigned int smile_status;
	int face_status;
} face_engine_t;

#define  KEEP_FACES

#ifdef    KEEP_FACES
#define  MAX_KP_FS  2
ALFace_appout_t faces_keep;
int keepfs[64];

static int is_overlap(DRect* r1, DRect* r2)
{
	int x0, y0;
	int x1, y1;
	x0 = r1->x;
	if (x0 < r2->x)
		x0 = r2->x;
	y0 = r1->y;
	if (y0 < r2->y)
		y0 = r2->y;

	x1 = r1->x + r1->w;
	if (x1 > (r2->x + r2->w))
		x1 = (r2->x + r2->w);
	y1 = r1->y + r1->h;
	if (y1 > (r2->y + r2->h))
		y1 = (r2->y + r2->h);

	if (x1 <= x0 || y1 <= y0)
		return 0;

	int r = (x1 - x0) * (y1 - y0) * 1000 / (r2->w * r2->h);
	int t = (x1 - x0) * (y1 - y0) * 1000 / (r1->w * r1->h);
	if(r < t) r = t;

	return r;
}

/*是否用r2替代r1*/
#ifndef   F_ABS
#define  F_ABS(x)  ((x)>0?(x):(-(x)))
#endif
static int is_replace(DRect* r1, DRect* r2)
{
	int cx1 = r1->w / 2 + r1->x;
	int cy1 = r1->h / 2 + r1->y;
	int cx2 = r2->w / 2 + r2->x;
	int cy2 = r2->h / 2 + r2->y;

	 if(F_ABS(cx1 - cx2)*10 > r1->w*1  ||  F_ABS(cy1 - cy2)*10 > r1->h*1 ||  r2->w*10 > r1->w*13 ||  r2->w*13 < r1->w*10)
	{
		return 1;
	}

	return 0;
}

static void face_keep_replace(DRect* rtkeep, int idx, DRect* rtout)
{
	memcpy(&rtkeep[idx], rtout, sizeof(DRect));
	keepfs[idx] = MAX_KP_FS + 1;
}

static void face_keep_add(DRect* rtkeep, int* nfacekeep, DRect* rtout)
{
	if (*nfacekeep < 32)
	{
		memcpy(&rtkeep[*nfacekeep], rtout, sizeof(DRect));
		keepfs[*nfacekeep] = MAX_KP_FS + 1;
		*nfacekeep = *nfacekeep + 1;
	}
}

static void face_keep_del(DRect* rtkeep, int* nfacekeep, int idx)
{
	int i;
	if (*nfacekeep > 0 && idx < *nfacekeep)
	{
		for (i = idx; i < *nfacekeep; i++)
		{
			memcpy(&rtkeep[i], &rtkeep[i + 1], sizeof(DRect));
			keepfs[i] = keepfs[i + 1];
		}
		keepfs[*nfacekeep - 1] = 0;
		*nfacekeep = *nfacekeep - 1;
	}
}

static void face_keep_checkup(DRect* rtkeep, int* nfacekeep)
{
	int i;
	int n = *nfacekeep;
	if (n > 0)
	{
		for (i = 0; i < *nfacekeep;)
		{
			if (keepfs[i] <= 0)
			{
				face_keep_del(rtkeep, nfacekeep, i);
			}
			else
			{
				i++;
			}
		}
	}
}

static void face_keep_find(DRect* rtout, DRect* rtkeep, int* nfacekeep)
{
	int i;
	int iso;
	int maxo = 0;
	int idx = 0;

	for (i = 0; i < *nfacekeep; i++)
	{
		iso = is_overlap(&rtkeep[i], rtout);
		if (iso > maxo)
		{
			maxo = iso;
			idx = i;
		}
	}

	if (maxo > 300)
	{
		if (is_replace(&rtkeep[idx], rtout))
		{
			//DEBUG(DEB_LEV_ERR,"is_replace!!!\n");
			face_keep_replace(rtkeep, idx, rtout);
		}
		else
		{
			keepfs[idx] = MAX_KP_FS + 1;
			//DEBUG(DEB_LEV_ERR,"no_replace!!!\n");
		}
	}
	else
	{
		face_keep_add(rtkeep, nfacekeep, rtout);
	}
}

static void faces_keep_run(ALFace_appout_t* face_appout)
{
	int i, j;
	int nfacekeep = faces_keep.faceout.RectNum;
	int nfaceout = face_appout->faceout.RectNum;
	DRect* rtkeep = faces_keep.faceout.rt;
	DRect* rtout = face_appout->faceout.rt;

	if (nfacekeep == 0 || nfaceout == 0)
	{
		if (nfaceout)
		{
			nfacekeep = nfaceout;
			faces_keep.faceout.RectNum = nfaceout;
			memcpy(rtkeep, rtout, sizeof(DRect) * nfaceout);
			for (i = 0; i < nfacekeep; i++)
			{
				keepfs[i] = MAX_KP_FS;
			}
		}
		else if (nfacekeep)
		{
			for (i = 0; i < nfacekeep; i++)
			{
				--keepfs[i];
			}
		}
	}
	else
	{
		for (i = 0; i < nfaceout; i++)
		{
			face_keep_find(&rtout[i], rtkeep, &nfacekeep);
		}

		faces_keep.faceout.RectNum = nfacekeep;

		for (i = 0; i < nfacekeep; i++)
		{
			--keepfs[i];
		}
	}

	//DEBUG(DEB_LEV_ERR,"rtkeep:%p,nfacekeep:%d!\n",rtkeep,nfacekeep);
	//checkup
	face_keep_checkup(rtkeep, &nfacekeep);
	faces_keep.faceout.RectNum = nfacekeep;

	/*修改结果*/
	face_appout->faceout.RectNum = nfacekeep;
	if (nfacekeep)
	{
		memcpy(rtout, rtkeep, sizeof(DRect) * nfacekeep);
	}
}

static int faces_keep_init()
{
	memset(&faces_keep, 0, sizeof(ALFace_appout_t));
	memset(&keepfs, 0, sizeof(int) * 64);
	faces_keep.faceout.rt = (DRect*) malloc(sizeof(DRect) * 64);
	if(faces_keep.faceout.rt == NULL)
	{
		DEBUG(DEB_LEV_ERR, "err!malloc fail!%s,%d\n", __FILE__, __LINE__);
		return -1;
	}

	return 0;
}

static void faces_keep_dispose()
{
	if (faces_keep.faceout.rt)
	{
		free(faces_keep.faceout.rt);
		faces_keep.faceout.rt = NULL;
	}
}

#endif

static void * face_loop(void *param)
{
	int j;
	face_engine_t * face_engine = (face_engine_t *) param;
	face_engine->status = SFE_IDLE;

	DEBUG(DEB_LEV_FUNCTION_NAME, "in to face_loop!\n");
	tsem_up(&face_engine->task_syn);

	while (1)
	{
		if (face_engine->cmd_pending == 1)
		{
			switch (face_engine->cmd)
			{
				case FE_CLOSE:
				{
					if (face_engine->facehandle)
					{
						face_app_dispose(face_engine->facehandle);
						face_engine->facehandle = NULL;
					}
					memset(&face_engine->face_appout, 0, sizeof(ALFace_appout_t));

					face_engine->face_det_ok = 0;
					memset(face_engine->mask_enable, 0, 12);
					memset(face_engine->maskinfo, 0, 16 * sizeof(ALMask_info_t));
					face_engine->status = SFE_IDLE;
					break;
				}

				case FE_RUN:
				{
					face_engine->status = SFE_DATA_READY;
				}
				break;

				case FE_STOP:
				face_engine->status = SFE_STOPED;
				break;

				default:
				{
					break;
				}
			}
			face_engine->cmd_pending = 0;
			tsem_up(&face_engine->task_syn);
		}

		if (face_engine->status == SFE_IDLE)
		{
			usleep((int) face_engine->tv);
		}

		if (face_engine->status == SFE_DATA_READY)
		{
			face_engine->status = SFE_BUSY;

			DEBUG(/*DEB_LEV_ERR*/DEB_LEV_SIMPLE_SEQ, "face det start\n");
			face_app_run(face_engine->facehandle, FSET_FACE_DET_A, 0);
			face_app_run(face_engine->facehandle, FGET_STATUS, (unsigned long) (&face_engine->face_appout));
			DEBUG(/*DEB_LEV_ERR*/DEB_LEV_SIMPLE_SEQ, "face det finish\n");

#ifdef    KEEP_FACES
			faces_keep_run(&face_engine->face_appout);
#endif

			/*
			//Crop前结果打印
			DEBUG(DEB_LEV_ERR,"face det,b4 crop!number_of_faces:%d!\n",face_engine->face_appout.faceout.RectNum);
			for(j = 0; j < face_engine->face_appout.faceout.RectNum; j++)
			{
				DEBUG(DEB_LEV_ERR,"face det,b4 crop!x:%d  y:%d  w:%d  h:%d\n",
				    face_engine->face_appout.faceout.rt[j].x,face_engine->face_appout.faceout.rt[j].y,
					face_engine->face_appout.faceout.rt[j].w,face_engine->face_appout.faceout.rt[j].h);
			}
			*/

			face_engine->face_det_ok = 1;

			/*由于输入为Crop所导致的输出窗口修正*/
			int CropFaceIdx[32];
			memset(CropFaceIdx, 0, 32 * sizeof(int));
			int RectNum = face_engine->face_appout.faceout.RectNum;
			int facesNum = 0;
			DRect * facert = face_engine->face_appout.faceout.rt;
			face_input_crop * facecrop = &(face_engine->crop);
			int xscale = (facecrop->dstw << 8) / (facecrop->cropw);
			int yscale = (facecrop->dsth << 8) / (facecrop->croph);

			/*找出在Crop内的窗口*/
			for (j = 0; j < RectNum; j++)
			{
				facert[j].x -= facecrop->x;
				facert[j].y -= facecrop->y;

#if 0
				/*人脸框x和y方向必须全部都在Crop区域才显示*/
				if(facert[j].x >= 0 && facert[j].y >= 0 &&
						(facert[j].x + facert[j].w) < facecrop->cropw && (facert[j].y + facert[j].h) < facecrop->croph )
				{
					CropFaceIdx[facesNum] = j;  /*记录序列号*/
					facesNum++;
					if(facesNum>31)break;
				}
#else
				/*只要人脸框x和y方向有一半以上在Crop区域都应显示*/
				if ((facert[j].x + facert[j].w / 2) > 0 && (facert[j].y + facert[j].h / 2) > 0 && (facert[j].x
						+ facert[j].w / 2) < facecrop->cropw && (facert[j].y + facert[j].h / 2) < facecrop->croph)
				{
					if (facert[j].x < 0)
					{
						facert[j].w += facert[j].x;
						facert[j].x = 0;
					}
					if (facert[j].y < 0)
					{
						facert[j].h += facert[j].y;
						facert[j].y = 0;
					}
					if ((facert[j].x + facert[j].w) > facecrop->cropw)
						facert[j].w = facecrop->cropw - facert[j].x;
					if ((facert[j].y + facert[j].h) > facecrop->croph)
						facert[j].h = facecrop->croph - facert[j].y;

					CropFaceIdx[facesNum] = j; /*记录序列号*/
					facesNum++;
					if (facesNum > 31)
						break;
				}
#endif
			}

			/*保存在Crop内的窗口*/
			for (j = 0; j < facesNum; j++)
			{
				if (j != CropFaceIdx[j])
					facert[j] = facert[CropFaceIdx[j]];

				/*归一化*/
				facert[j].x = (facert[j].x - facecrop->cropw / 2) * 2000 / facecrop->cropw;
				facert[j].y = (facert[j].y - facecrop->croph / 2) * 2000 / facecrop->croph;
				facert[j].w = (facert[j].w) * 2000 / facecrop->cropw;
				facert[j].h = (facert[j].h) * 2000 / facecrop->croph;
			}
			face_engine->face_appout.faceout.RectNum = facesNum;

			/*
			DEBUG(DEB_LEV_ERR,"face det,aft crop!number_of_faces:%d!\n",face_engine->face_appout.faceout.RectNum);
			for(j = 0; j < face_engine->face_appout.faceout.RectNum; j++)
			{
				DEBUG(DEB_LEV_ERR,"face det,aft crop!x:%d  y:%d  w:%d  h:%d\n",
			     face_engine->face_appout.faceout.rt[j].x,face_engine->face_appout.faceout.rt[j].y,
					face_engine->face_appout.faceout.rt[j].w,face_engine->face_appout.faceout.rt[j].h);
			}
			*/

			face_engine->status = SFE_IDLE;
		}

		if (face_engine->status == SFE_STOPED)
		{
			DEBUG(DEB_LEV_SIMPLE_SEQ, "SFE_STOPED!\n");
			break;
		}

		usleep((int) face_engine->tv / 4);
	}

	DEBUG(DEB_LEV_FUNCTION_NAME, "out of face_loop\n");

	return NULL;
}

int face_cmd(void *engine, int cmd, unsigned long args)
{
	face_engine_t * face_engine = (face_engine_t *) engine;
	int result = 0x00;

	switch (cmd)
	{
		case FE_OPEN:
		{
			DEBUG(DEB_LEV_SIMPLE_SEQ, "face!face_cmd:FE_OPEN\n");
			tsem_down(&face_engine->cmd_mutex);
			if ((face_engine->status == SFE_IDLE)) /*stopped状态不能open?*/
			{
				face_engine->facehandle = face_app_init(NULL);
				if (face_engine->facehandle)
				{
					//face_app_run(face_engine->facehandle,FSET_DRAW_FACE,1);
				}
				else
				{
					DEBUG(DEB_LEV_ERR, "err!face_app_init fail!%s,%d\n", __FILE__, __LINE__);
					result = -1;
				}
			}
			tsem_up(&face_engine->cmd_mutex);
			break;
		}

		case FE_SMILE:
		{
			tsem_down(&face_engine->cmd_mutex);
			if ((face_engine->status == SFE_IDLE))
			{
				if (face_engine->facehandle)/*只有先打开人脸检测才能进行下一步的笑脸检测*/
				{
					face_app_run(face_engine->facehandle, FSET_SIMLE_DET, args);
					face_engine->is_smile_en = 0;
				}
			}
			else
			{
				face_engine->smile_args = args;
				face_engine->is_smile_en = 1;
			}
			tsem_up(&face_engine->cmd_mutex);
			break;
		}

		case FE_MASK:
		{
			tsem_down(&face_engine->cmd_mutex);
			if ((face_engine->status == SFE_IDLE))
			{
				if (face_engine->facehandle)
				{
					ALMask_info_t *maskinfo = (ALMask_info_t*) args;
					if ((maskinfo->mas_type != MMM_DISABLE_ALL) && (face_engine->mask_enable[8] == 1))
					{
						face_engine->mask_enable[8] = 0;
					}

					if (maskinfo->mas_type == MMM_SEYE_MASK || maskinfo->mas_type == MMM_DEYE_MASK)
					{
						face_engine->mask_enable[0] = 0;
						face_app_run(face_engine->facehandle, FSET_MASK_ENABLE, (unsigned long) maskinfo);
					}
					else if (maskinfo->mas_type == MMM_NOSE_MASK)
					{
						face_engine->mask_enable[1] = 0;
						face_app_run(face_engine->facehandle, FSET_MASK_ENABLE, (unsigned long) maskinfo);
					}
					else if (maskinfo->mas_type == MMM_MOUTHUP_MASK)
					{
						face_engine->mask_enable[2] = 0;
						face_app_run(face_engine->facehandle, FSET_MASK_ENABLE, (unsigned long) maskinfo);
					}
					else if (maskinfo->mas_type == MMM_MOUTH_MASK)
					{
						face_engine->mask_enable[3] = 0;
						face_app_run(face_engine->facehandle, FSET_MASK_ENABLE, (unsigned long) maskinfo);
					}
					else if (maskinfo->mas_type == MMM_FACE_MASK)
					{
						face_engine->mask_enable[4] = 0;
						face_app_run(face_engine->facehandle, FSET_MASK_ENABLE, (unsigned long) maskinfo);
					}
					else if (maskinfo->mas_type == MMM_MOUTHDOWN_MASK)
					{
						face_engine->mask_enable[5] = 0;
						face_app_run(face_engine->facehandle, FSET_MASK_ENABLE, (unsigned long) maskinfo);
					}
					else if (maskinfo->mas_type == MMM_FOREHEAD_MASK)
					{
						face_engine->mask_enable[6] = 0;
						face_app_run(face_engine->facehandle, FSET_MASK_ENABLE, (unsigned long) maskinfo);
					}
					else if (maskinfo->mas_type == MMM_HEAD_MASK)
					{
						face_engine->mask_enable[7] = 0;
						face_app_run(face_engine->facehandle, FSET_MASK_ENABLE, (unsigned long) maskinfo);
					}
					else if (maskinfo->mas_type == MMM_BREAD_MASK)
					{
						face_engine->mask_enable[7] = 0;
						face_app_run(face_engine->facehandle, FSET_MASK_ENABLE, (unsigned long) maskinfo);
					}
					else if (maskinfo->mas_type == MMM_DISABLE_ALL)
					{
						face_engine->mask_enable[8] = 0;
						face_app_run(face_engine->facehandle, FSET_MASK_DISABLE, (unsigned long) maskinfo);
					}
				}
				face_engine->is_smile_en = 0;
			}
			else
			{
				ALMask_info_t *maskinfo = (ALMask_info_t*) args;
				if ((maskinfo->mas_type != MMM_DISABLE_ALL) && (face_engine->mask_enable[8] == 1))
				{
					face_engine->mask_enable[8] = 0;
				}
				if (maskinfo->mas_type == MMM_SEYE_MASK || maskinfo->mas_type == MMM_DEYE_MASK)
				{
					face_engine->mask_enable[0] = 1;
					face_engine->maskinfo[0] = *maskinfo;
				}
				else if (maskinfo->mas_type == MMM_NOSE_MASK)
				{
					face_engine->mask_enable[1] = 1;
					face_engine->maskinfo[1] = *maskinfo;
				}
				else if (maskinfo->mas_type == MMM_MOUTHUP_MASK)
				{
					face_engine->mask_enable[2] = 1;
					face_engine->maskinfo[2] = *maskinfo;
				}
				else if (maskinfo->mas_type == MMM_MOUTH_MASK)
				{
					face_engine->mask_enable[3] = 1;
					face_engine->maskinfo[3] = *maskinfo;
				}
				else if (maskinfo->mas_type == MMM_FACE_MASK)
				{
					face_engine->mask_enable[4] = 1;
					face_engine->maskinfo[4] = *maskinfo;
				}
				else if (maskinfo->mas_type == MMM_MOUTHDOWN_MASK)
				{
					face_engine->mask_enable[5] = 1;
					face_engine->maskinfo[5] = *maskinfo;
				}
				else if (maskinfo->mas_type == MMM_FOREHEAD_MASK)
				{
					face_engine->mask_enable[6] = 1;
					face_engine->maskinfo[6] = *maskinfo;
				}
				else if (maskinfo->mas_type == MMM_HEAD_MASK)
				{
					face_engine->mask_enable[7] = 1;
					face_engine->maskinfo[7] = *maskinfo;
				}
				else if (maskinfo->mas_type == MMM_BREAD_MASK)
				{
					face_engine->mask_enable[7] = 1;
					face_engine->maskinfo[7] = *maskinfo;
				}
				else if (maskinfo->mas_type == MMM_DISABLE_ALL)
				{
					face_engine->mask_enable[8] = 1;
					face_engine->maskinfo[8] = *maskinfo;
				}
				face_engine->is_mask_en = 1;
			}
			tsem_up(&face_engine->cmd_mutex);
			break;
		}

		case FE_DRAW:
		{
			tsem_down(&face_engine->cmd_mutex);
			face_mask_run(face_engine->facehandle, FSET_FACE_DET_B, args);
			tsem_up(&face_engine->cmd_mutex);
			break;
		}

		case FE_RESET:
		{
			tsem_down(&face_engine->cmd_mutex);
			face_app_run(face_engine->facehandle, FSET_RESET, 0);
			tsem_up(&face_engine->cmd_mutex);
			break;
		}

		case FE_STATUS:
		{
			tsem_down(&face_engine->cmd_mutex);
			*(unsigned int*) args = face_engine->status;
			tsem_up(&face_engine->cmd_mutex);
			break;
		}

		case FE_CROP:
		{
			tsem_down(&face_engine->cmd_mutex);
			face_engine->crop = *(face_input_crop*) args;
			tsem_up(&face_engine->cmd_mutex);
			break;
		}

		case FE_DATAOUT:
		{
			DEBUG(DEB_LEV_SIMPLE_SEQ, "face!face_cmd:FE_DATAOUT\n");
			tsem_down(&face_engine->cmd_mutex);

			if (face_engine->face_det_ok)
			{
				*(ALFace_appout_t*) args = face_engine->face_appout;
				result = 0;
			}
			else
			{
				((ALFace_appout_t*) args)->faceout.RectNum = 0;
				result = 0;
			}

			tsem_up(&face_engine->cmd_mutex);
			break;
		}

		case FE_DATAIN:
		{
			DEBUG(DEB_LEV_SIMPLE_SEQ, "face!face_cmd:FE_DATAIN\n");
			tsem_down(&face_engine->cmd_mutex);
			face_engine->status = SFE_BUSY;

#if 0
			if(face_engine->is_mask_en)
			{
				ALMask_info_t *maskinfo = NULL;
				int i;
				for(i = 0; i < 8; i++)
				{
					maskinfo = &face_engine->maskinfo[i];
					if(face_engine->mask_enable[i] == 1)
					{
						face_app_run(face_engine->facehandle,FSET_MASK_ENABLE,(unsigned long)maskinfo);
						face_engine->mask_enable[i] = 0;
					}
				}
				maskinfo = &face_engine->maskinfo[8];
				if(face_engine->mask_enable[8] == 1)
				{
					face_app_run(face_engine->facehandle,FSET_MASK_DISABLE,(unsigned long)maskinfo);
					face_engine->mask_enable[8] = 0;
				}
				face_engine->is_mask_en = 0;
			}

			if(face_engine->is_smile_en)
			{
				face_app_run(face_engine->facehandle,FSET_SIMLE_DET,face_engine->smile_args);
				face_engine->is_smile_en = 0;
			}
#endif

			face_app_run(face_engine->facehandle, FSET_FACE_GEN, args);
			tsem_up(&face_engine->cmd_mutex);
			break;
		}

		default:
		{
			tsem_down(&face_engine->cmd_mutex);
			face_engine->args = args;
			face_engine->cmd = cmd;
			face_engine->cmd_pending = 1;
			tsem_down(&face_engine->task_syn);
			result = face_engine->result;
			tsem_up(&face_engine->cmd_mutex);
			break;
		}
	}

	return result;
}

int face_dispose(void * engine)
{
	DEBUG(DEB_LEV_FUNCTION_NAME, "face!in to face_dispose\n");
	face_engine_t * face_engine = (face_engine_t *) engine;
	int ret = 0;

	if (face_engine == NULL)
	{
		DEBUG(DEB_LEV_ERR, "err!the face_engine is NULL!File: %s,%d\n", __FILE__, __LINE__);
		return -1;
	}

	tsem_down(&face_engine->cmd_mutex);
	if (face_engine->status != SFE_STOPED)
	{
		face_engine->cmd = FE_STOP;
		face_engine->cmd_pending = 1;
		tsem_down(&face_engine->task_syn);
	}
	tsem_up(&face_engine->cmd_mutex);

	ret = pthread_join(face_engine->face_thread, NULL);
	if (ret != 0)
	{
		DEBUG(DEB_LEV_ERR, "err!In %s pthread_join returned err =%d\n", __func__, ret);
	}

	if (face_engine->facehandle)
	{
		face_app_dispose(face_engine->facehandle);
		face_engine->facehandle = 0;
	}

	if (face_engine)
	{
		tsem_deinit(&face_engine->cmd_mutex);
		tsem_deinit(&face_engine->task_syn);
		free(face_engine);
	}

#ifdef    KEEP_FACES
	faces_keep_dispose();
#endif

	DEBUG(DEB_LEV_FUNCTION_NAME, "out of  face_dispose\n");
	return 0;
}

void *face_open()
{
	int result;
	face_engine_t *face_engine = NULL;
	DEBUG(DEB_LEV_FUNCTION_NAME, "face!in to face_open\n");

#ifdef    KEEP_FACES
	result = faces_keep_init();
	if (result < 0)
	{
		DEBUG(DEB_LEV_ERR, "err!faces_keep_init fail!%s,%d\n", __FILE__, __LINE__);
		goto open_fail0;
	}
#endif

	face_engine = (face_engine_t *) calloc(1, sizeof(face_engine_t));
	if (face_engine == NULL)
	{
		DEBUG(DEB_LEV_ERR, "err!the face_engine calloc fail!%s,%d\n", __FILE__, __LINE__);
		goto open_fail1;
	}
	memset(face_engine, 0, sizeof(face_engine_t));

	face_engine->status = SFE_STOPED;
	face_engine->tv = 30;
	face_engine->face_appout.faceidx = -1;
	face_engine->face_det_ok = 0;

	/*创建任务同步使用的信号量和取景任务*/
	tsem_init(&face_engine->task_syn, 0);
	tsem_init(&face_engine->cmd_mutex, 1);

	result = pthread_create(&face_engine->face_thread, NULL, face_loop, (void *) face_engine);
	if (result != 0)
	{
		DEBUG(DEB_LEV_ERR, "err!the face_thread create fail!%s,%d\n", __FILE__, __LINE__);
		goto open_fail2;
	}
	tsem_down(&face_engine->task_syn);

	return (void*) face_engine;

open_fail2:
    free(face_engine);
	face_engine = NULL;

open_fail1:
#ifdef    KEEP_FACES
	faces_keep_dispose();
#endif

open_fail0:
    DEBUG(DEB_LEV_ERR,"err!open video face_engine fail!%s,%d\n",__FILE__,__LINE__);
	return NULL;
}

