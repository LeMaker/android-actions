#ifndef ISPCTL_ADAPTER_H
#define ISPCTL_ADAPTER_H

#include "isp_ctl.h"

typedef struct
{
    int (*act_isp_open)(void *handle);
    int (*act_isp_cmd)(void *handle, int cmd, unsigned long mParams);
    int (*act_isp_close)(void *handle);
} ispctl_handle_t;

void *ispctl_open(void);
int ispctl_close(void *handle);

#endif
