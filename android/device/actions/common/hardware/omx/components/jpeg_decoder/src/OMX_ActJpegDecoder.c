#include "mmm_plugin_img.h"
#include "OMX_ActJpegDecoder.h"
#include <utils/Log.h>

#define BUF_STREAM 0

static int read_data(struct mmm_stream_input_s *pInput,unsigned int *buf,unsigned int len)
{
	buf_stream_info_t *buf_info;

	buf_info = ((actImage_caller_t*)pInput)->buf_info;

    if(buf == 0 || pInput == 0 || len == 0) {
	    return 0;
    } 
	if (buf_info->cur_len + len <= buf_info->buf_len){
		memcpy((void *)buf, (void *)(buf_info->buf_start + buf_info->cur_len), len);
	} else {
		len = buf_info->buf_len - buf_info->cur_len;
		memcpy((void *)buf, (void *)(buf_info->buf_start + buf_info->cur_len), len);
	}

	buf_info->cur_len  += len;
	return len;

}


static int seek_data(struct mmm_stream_input_s *pInput,int offset,int original)
{
	buf_stream_info_t *buf_info;
	buf_info = ((actImage_caller_t*)pInput)->buf_info;

	if(original == MMM_DSEEK_SET) {
		buf_info->cur_len = offset;
	} else if (original == MMM_DSEEK_END) {
		buf_info->cur_len = buf_info->buf_len - offset;
	} else if (original == MMM_DSEEK_CUR) {
		buf_info->cur_len += offset;
	} else {
	}

	if(buf_info->cur_len > buf_info->buf_len) {
		buf_info->cur_len = buf_info->buf_len;
	}

	return 0;
}

static int tell_data(struct mmm_stream_input_s *pInput)
{
	buf_stream_info_t *buf_info;
	buf_info = ((actImage_caller_t*)pInput)->buf_info;

	return buf_info->cur_len;
}

static int get_data_size(struct mmm_stream_input_s *pInput)
{
	buf_stream_info_t *buf_info;
	buf_info = ((actImage_caller_t*)pInput)->buf_info;

	return buf_info->buf_len;
}
static int dispose_data(struct mmm_stream_input_s *pInput)
{
	return 0;
}


int OMX_ActImageDecoder_set_buf(omx_jpegdec_component_PrivateType *p, unsigned char *buf, unsigned int len)
{
	actImage_caller_t *s = (actImage_caller_t *)p->p_imgCaller;
	s->buf_info->buf_start = (unsigned char *)buf;
	s->buf_info->buf_len   = len;
	s->buf_info->cur_len   = 0;

	return 0;
}

void OMX_ActImageDecoder_Close(omx_jpegdec_component_PrivateType *p)
{
	actImage_caller_t *s= (actImage_caller_t *)p->p_imgCaller;

	if(s->p_imgInterface != NULL) {
		s->p_imgInterface->dispose(s->p_imgInterface);
		s->p_imgInterface = NULL;
	}

	if(p->source_img != NULL) {
		free(p->source_img);
		p->source_img=NULL;
	}

	s->input.dispose(&(s->input));

	if(s->buf_info != NULL) {
		free(s->buf_info);
		s->buf_info=NULL;
	}

	if(s != NULL)
	{
		free(s);
		s = NULL;
	}
}


int OMX_ActImageDecoder_Open(omx_jpegdec_component_PrivateType *p)
{
	actImage_caller_t *s;
	s = (actImage_caller_t *)malloc(sizeof(actImage_caller_t));
	if(s == NULL) {
		goto ERROR;
	}
	memset(s, 0, sizeof(actImage_caller_t));
//	s->input.read = read_data;
	s->input.seek = seek_data;
	s->input.tell = tell_data;
	s->input.get_data_size = get_data_size;
	s->input.dispose = dispose_data;
	s->buf_info = (buf_stream_info_t*)malloc(sizeof(buf_stream_info_t));
	if(s->buf_info == NULL) {
		goto ERROR;
	}

	memset(s->buf_info, 0, sizeof(buf_stream_info_t));

	p->p_imgCaller = (void*)s;

	p->source_img = (void*)malloc(sizeof(mmm_image_info_t));
	if(p->source_img == NULL) {
		goto ERROR;
	}
	memset(p->source_img, 0, sizeof(mmm_image_info_t));
	return 0;

ERROR:
	OMX_ActImageDecoder_Close(p);
	return -1;
}



int OMX_ActImageDecoder_Decode(omx_jpegdec_component_PrivateType *p)
{
	actImage_caller_t *s= (actImage_caller_t *)p->p_imgCaller;
	mmm_image_file_info_t image_info;
	mmm_imagedec_plugio_t plugio;
	plugio.input = &(s->input);
	int result;
	OMX_ActImageDecoder_set_buf(p, p->cur_headbuf->pBuffer, p->cur_headbuf->nFilledLen);
//	((mmm_plugin_info_t*)p->p_imgPluginInfo)->get_file_info(&(s->input), &image_info);
	s->p_imgInterface = ((mmm_plugin_info_t *) p->p_imgPluginInfo)->open((void *) &plugio);
	if(s->p_imgInterface == NULL) {
		return -1;
	}
//	result = s->p_imgInterface->init(s->p_imgInterface);
//	if (result == -1) {
//		return -1;
//	}
	((mmm_image_info_t*)p->source_img)->width  = (p->image_width/p->scalefactor);
	((mmm_image_info_t*)p->source_img)->height = (p->image_height/p->scalefactor);
	((mmm_image_info_t*)p->source_img)->xpos   = 0;
	((mmm_image_info_t*)p->source_img)->ypos   = 0;
	((mmm_image_info_t*)p->source_img)->phyaddr	= p->source_phy;
	((mmm_image_info_t*)p->source_img)->viraddr = p->cur_headbuf->pBuffer;
	((mmm_image_info_t*)p->source_img)->yuv_info.y_buf = p->p_imgOutputBuf;	
	((mmm_image_info_t*)p->source_img)->yuv_info.y_buf_len = p->imgOutputBufLen;
	((mmm_image_info_t*)p->source_img)->format = p->out_config;
	((mmm_image_info_t*)p->source_img)->doEnhance = p->doEnhance;
	result = s->p_imgInterface->decode_img(s->p_imgInterface, ((mmm_image_info_t*)p->source_img));
	if(result != 0) {
		return -1;
	}
	return 0;
}
