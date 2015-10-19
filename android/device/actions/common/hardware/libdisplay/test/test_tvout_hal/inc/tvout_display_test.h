#ifndef __TVOUT_DISPLAY_TEST_H__
#define __TVOUT_DISPLAY_TEST_H__

namespace android {
int actions_server_DisplayService_init(void);
void  actions_server_DisplayService_getHdmiCap(void);
void actions_server_DisplayService_cable_monitor(void);
int actions_server_DisplayService_getDisplayerCount(void);
char * actions_server_DisplayService_getDisplayerInfo(int id);
bool actions_server_DisplayService_setOutputDisplayer(int id);
bool actions_server_DisplayService_setOutputDisplayer1(char *id);
bool actions_server_DisplayService_enableOutput(bool enable);
bool actions_server_DisplayService_setDisplayerParam(const char * params);
bool actions_server_DisplayService_setHdmiParam(int vid);
bool actions_server_DisplayService_setCvbsParam(const char *params);
char * actions_server_DisplayService_getDisplayerParam(void);
void actions_server_DisplayService_setDisplaySingleMode(int mode);
void actions_server_DisplayService_setDisplayMode(int mode);
int actions_server_DisplayService_getCableState(void);
void actions_server_DisplayService_cable_monitor(void);
void actions_server_DisplayService_setTvScale(int scaleXParam, int scaleYPparam);
};
#endif
