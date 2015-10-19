#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>
#include <stdlib.h>
#include "xpl_Lib.h"
#include <string.h>

#ifdef __cplusplus  
extern "C" {
#endif

static XPL_DL_HANDLE_T dllhandle_browser = NULL;
static XPL_DL_HANDLE_T dllhandle_session = NULL;


XPL_DL_HANDLE_T XPL_DL_Load(CPCHAR dllib_name)
{
  //!!! DP needs attention; ignore for now...
    if ( 0 && !strcmp(dllib_name,"libdmssession.so") )
    {   
        // load browser stack lib
        dllhandle_browser = dlopen("libdmportlib.so", RTLD_NOW | RTLD_GLOBAL );
        if (  dllhandle_browser ) 
            dllhandle_session = dlopen("libdmssession.so", RTLD_NOW );
        if ( dllhandle_session == NULL )
            dlclose(dllhandle_browser);
        else
            return dllhandle_session;
    }
    else // plugins;
    {
        void * handle=dlopen(dllib_name, RTLD_LAZY );            
        return handle;
    }    

}

/* Gets function pointer */
void * XPL_DL_GetFunction (XPL_DL_HANDLE_T lib_handle, CPCHAR name)
{
    void * pFunc=NULL;
    pFunc=dlsym(lib_handle, name); 
    return pFunc;

}    

/* Nnloads shared objects */
void XPL_DL_Unload(XPL_DL_HANDLE_T lib_handle)
{
     if ( lib_handle == dllhandle_session && dllhandle_browser != NULL )
     {
        dlclose(dllhandle_browser);
        dlclose(dllhandle_session);
        dllhandle_browser = NULL;
        dllhandle_session = NULL;
     }   
     else
        dlclose(lib_handle);
     return;
}    

#ifdef __cplusplus  
}
#endif

