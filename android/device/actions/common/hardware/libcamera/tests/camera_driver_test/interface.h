

#ifndef _INTERFACE_H
#define _INTERFACE_H

extern void camera_open(void);
extern void camera_get_ctrls(void);
extern void camera_set_ctrls_all_slow(void);
extern void camera_set_ctrls_default_slow(void);
extern void camera_set_ctrls_all_fast(void);
extern void camera_set_ctrls_default_fast(void);
extern void camera_get_framesize(void);
extern void camera_get_framerate(void);
extern void camera_set_format(void);
extern void camera_set_frame_rate(void);
extern void camera_start(void);

extern void report_capture_time(void);
extern void report_frame_rate(void);

extern void camera_get_param_brightness(void);
extern void camera_get_param_scene_exposure(void);
extern void camera_get_param_white_balance(void);
extern void camera_get_param_colorfx(void);

extern void camera_set_param_brightness(void);
extern void camera_set_param_scene_exposure(void);
extern void camera_set_param_white_balance(void);
extern void camera_set_param_colorfx(void);
extern void load_config(void);
extern void report_capture_time(void);
extern void report_frame_rate(void);

extern void camera_front_in_use(void);
extern void camera_back_in_use(void);

extern void camera_stop(void);
extern void camera_close(void);





#endif //_INTERFACE_H