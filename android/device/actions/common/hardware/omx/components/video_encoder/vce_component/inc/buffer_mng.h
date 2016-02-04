#ifndef  __BUFFER_MNG_H__
#define  __BUFFER_MNG_H__

#include "OMX_Core.h"
#include "OMX_Types.h"
#include "vce_cfg.h"

#define MAX_VCE_MNG_Buffers 120

typedef struct OMX_VCE_Buffer_Info
{
	OMX_U32 IsValid;
	OMX_PTR InPutAddr;
	OMX_PTR PhyAddr;
	OMX_PTR VirAddr;
	OMX_S32 Ion_Size;
} OMX_VCE_Buffer_Info;

typedef struct OMX_VCE_Buffers_List
{
	OMX_VCE_Buffer_Info pList[MAX_VCE_MNG_Buffers];
	OMX_S32 Actual_Buffers;
} OMX_VCE_Buffers_List;

OMX_ERRORTYPE Add_UseBuffer_BuffersMng(OMX_VCE_Buffers_List *pBuffersMng_List, OMX_BUFFERHEADERTYPE *pBuffHead, int IsStoreMedia, int nSizeBytes, OMX_PTR pBuffer);
OMX_ERRORTYPE FlushProcessingBuffers_BuffersMng(OMX_VCE_Buffers_List *pBuffersMng_List, OMX_BUFFERHEADERTYPE *pBuffHead, int IsStoreMedia);
OMX_ERRORTYPE Add_AllocateBuffer_BuffersMng(OMX_VCE_Buffers_List *pBuffersMng_List, OMX_BUFFERHEADERTYPE *pBuffHead, int IsStoreMedia, int nSizeBytes);
OMX_ERRORTYPE Free_UseBuffer_BuffersMng(OMX_VCE_Buffers_List *pBuffersMng_List, OMX_BUFFERHEADERTYPE *pBuffHead, int IsStoreMedia);
OMX_ERRORTYPE Free_AllocateBuffer_BuffersMng(OMX_VCE_Buffers_List *pBuffersMng_List, OMX_BUFFERHEADERTYPE *pBuffHead, int IsStoreMedia);
OMX_ERRORTYPE Clear_StoreMedia_BuffersMng(OMX_VCE_Buffers_List *pBuffersMng_List);
OMX_ERRORTYPE SendBuffer_BuffersMng(OMX_VCE_Buffers_List *pBuffersMng_List, OMX_BUFFERHEADERTYPE *pBuffHead, int IsStoreMedia, int Dir);
OMX_ERRORTYPE ReturnBuffer_BuffersMng(OMX_VCE_Buffers_List *pBuffersMng_List, OMX_BUFFERHEADERTYPE *pBuffHead, int IsStoreMedia);
OMX_ERRORTYPE Get_UseRingBuffer_BuffersMng(OMX_VCE_Buffers_List *pBuffersMng_List, void *bufferpool, OMX_BUFFERHEADERTYPE *pBuffHead, int ringbuf_size);
OMX_ERRORTYPE Free_UseRingBuffer_BuffersMng(OMX_VCE_Buffers_List *pBuffersMng_List, void *bufferpool, OMX_BUFFERHEADERTYPE *pBuffHead, int ringbuf_size);
void *Get_PhyAddr_BuffersMng(OMX_VCE_Buffers_List *pBuffersMng_List, OMX_BUFFERHEADERTYPE *pBuffHead, int IsStoreMedia);
void *Get_VirAddr_BuffersMng(OMX_VCE_Buffers_List *pBuffersMng_List, OMX_BUFFERHEADERTYPE *pBuffHead, int IsStoreMedia);
#ifdef enable_gralloc
OMX_ERRORTYPE UnLock_VirAddr_BuffersMng(OMX_VCE_Buffers_List *pBuffersMng_List, OMX_BUFFERHEADERTYPE *pBuffHead, int IsStoreMedia);
#endif

#endif

