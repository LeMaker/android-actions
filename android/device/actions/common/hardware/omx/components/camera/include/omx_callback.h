#ifndef OMX_CALLBACK_H
#define OMX_CALLBACK_H

typedef struct
{
    int (*func)(void *handle, char *info);   // 'info' is a string which contains useful debugging infomation
    void *handle;
} wdog_cb_t;


#endif
