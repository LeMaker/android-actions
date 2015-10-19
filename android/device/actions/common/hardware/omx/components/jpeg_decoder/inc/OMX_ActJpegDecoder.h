#ifdef __cplusplus
extern "C" {
#endif
#include "omx_jpegdec_component.h"
#include "mmm_plugin_img.h"

typedef int  (*rw_ptr)(struct mmm_stream_input_s *,unsigned char *,unsigned int);
extern void jpeg_set_read_fn(struct mmm_stream_input_s *pInput, rw_ptr read_data_fn);
extern void OMX_ActImageDecoder_Close(omx_jpegdec_component_PrivateType *p);
extern int OMX_ActImageDecoder_Open(omx_jpegdec_component_PrivateType *p);
extern int OMX_ActImageDecoder_Decode(omx_jpegdec_component_PrivateType *p);

#ifdef __cplusplus
}
#endif
