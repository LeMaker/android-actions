#include <dlfcn.h>
#include "ispctl_adapter.h"
#include "log.h"

typedef struct
{
    ispctl_handle_t handle;
    void *isp_lib;
    void *(*act_isp_open)(void);
    int (*act_isp_cmd)(void *handle, int cmd, unsigned long mParams);
    int (*act_isp_close)(void *handle);
    void *isplib_handle;
} ispctl_handle_inter_t;

static int act_isp_open_plus(void *handle)
{
    ispctl_handle_inter_t *handle_inter = (ispctl_handle_inter_t *)handle;
    
    handle_inter->isplib_handle = handle_inter->act_isp_open();
    if(NULL == handle_inter->isplib_handle) return -1;

    return 0;
}

static int act_isp_cmd_plus(void *handle, int cmd, unsigned long mParams)
{
    ispctl_handle_inter_t *handle_inter = (ispctl_handle_inter_t *)handle;

    return handle_inter->act_isp_cmd(handle_inter->isplib_handle, cmd, mParams);
}

static int act_isp_close_plus(void *handle)
{
    ispctl_handle_inter_t *handle_inter = (ispctl_handle_inter_t *)handle;

    return handle_inter->act_isp_close(handle_inter->isplib_handle);
}

static int get_isp_lib(ispctl_handle_inter_t *handle)
{
    void *pCodaModule;
    char buf[50] = "libACT_ISP.so";
    int ret = 0;

    pCodaModule = dlopen(buf, RTLD_LAZY | RTLD_GLOBAL);
    if(pCodaModule == NULL)
    {
        OMXDBUG(OMXDBUG_ERR, "dlopen %s failed because %s\n", buf, dlerror());
        ret = -1;
        goto err0;
    }

    handle->act_isp_open = dlsym(pCodaModule, "act_isp_open");
    if(handle->act_isp_open == NULL)
    {
        OMXDBUG(OMXDBUG_ERR, "GetCodaLibAndHandle: dlsym failed for module %p %s\n", pCodaModule, dlerror());
        ret = -2;
        goto err1;
    }

    handle->act_isp_cmd = dlsym(pCodaModule, "act_isp_cmd");
    if(handle->act_isp_cmd == NULL)
    {
        OMXDBUG(OMXDBUG_ERR, "GetCodaLibAndHandle: dlsym failed for module %p %s\n", pCodaModule, dlerror());
        ret = -2;
        goto err1;
    }

    handle->act_isp_close = dlsym(pCodaModule, "act_isp_close");
    if(handle->act_isp_close == NULL)
    {
        OMXDBUG(OMXDBUG_ERR, "GetCodaLibAndHandle: dlsym failed for module %p %s\n", pCodaModule, dlerror());
        ret = -2;
        goto err1;
    }

    handle->isp_lib = pCodaModule;
    return ret;

err1:
    dlclose(pCodaModule);
    pCodaModule = NULL;

err0:
    handle->isp_lib = pCodaModule;
    return ret;
}

static int free_isp_lib(ispctl_handle_inter_t *handle)
{
    handle->act_isp_open = NULL;
    handle->act_isp_cmd = NULL;
    handle->act_isp_close = NULL;

    if(handle->isp_lib)
    {
        dlclose(handle->isp_lib);
        handle->isp_lib = NULL;
    }

    return 0;
}


void *ispctl_open(void)
{
    ispctl_handle_inter_t *handle_inter = (ispctl_handle_inter_t *)calloc(1, sizeof(ispctl_handle_inter_t));
    int ret = 0;

    ret = get_isp_lib(handle_inter);
    if( ret != 0)
    {
        free(handle_inter);
        return NULL;
    }

    handle_inter->handle.act_isp_open = act_isp_open_plus;
    handle_inter->handle.act_isp_cmd = act_isp_cmd_plus;
    handle_inter->handle.act_isp_close = act_isp_close_plus;

    OMXDBUG(OMXDBUG_PARAM, "isp open: 0x%x, 0x%x, 0x%x, 0x%x\n", handle_inter, handle_inter->act_isp_open, 
            handle_inter->act_isp_cmd, handle_inter->act_isp_close);

    return handle_inter;
}

int ispctl_close(void *handle)
{
    ispctl_handle_inter_t *handle_inter = (ispctl_handle_inter_t *)handle;

    if(NULL != handle_inter)
    {
        free_isp_lib(handle_inter);
        free(handle_inter);
    }

    return 0;
}


