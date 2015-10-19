
#include <dlfcn.h>

#include "libparser.h"
void * mLib_handle;
void * mPlugin_handle;
storage_io_t *mInput;
music_parser_plugin_t *mPlugin_info; /* 解码插件信息 */
typedef int(*FuncPtr)(void);

int parser_open (char *filename, char *format, music_info_t* info)
{
    char libname[32] = "ap";
    int tmp = 0;
    int ret = 0;  
    FuncPtr func_handle;
    mInput = create_storage_io(MODE_DISK,1<<12);
    if(mInput == NULL)
    {
        printf("ActAudioExtractor create_storage_io Err;\n");
        goto EXIT;
    }
    init_storage_io(mInput, filename,RWMODE_READ);
    strcat(libname, (const char*)format);
    strcat(libname, ".so");
    mLib_handle = dlopen(libname, RTLD_NOW);
    //    LOGD("ActAudioExtractor::init-mime:%s-libname: %s; handle:%x\n",mime, libname, (int)mLib_handle);
    if (mLib_handle == NULL)
    {
        printf("ActAudioExtractor dlopen Err: libname: %s;\n", libname);
        goto EXIT;
    }
    func_handle = (FuncPtr) dlsym(mLib_handle, "get_plugin_info");
    //LOGD("ActAudioExtractor::dlsym func_handle:%x\n", (int)func_handle);
    if (func_handle == NULL)
    {
        printf("ActAudioExtractor dlsym get_plugin_info Err;\n");
        dlclose(mLib_handle);
        mLib_handle = NULL;
        goto EXIT;
    }
    tmp = func_handle();
    mPlugin_info = (music_parser_plugin_t *)(tmp);
    if (mPlugin_info == NULL)
    {
        printf("ActAudioExtractor get_plugin_info Err;\n");
        dlclose(mLib_handle);
        mLib_handle = NULL;
        goto EXIT;
    }

    mPlugin_handle = mPlugin_info->open(mInput);
    if (mPlugin_handle == NULL)
    {
        printf("%s: open parser fail\n", __FILE__);
        dlclose(mLib_handle);
        mLib_handle = NULL;
        goto EXIT;
    }
    
    

    tmp = mPlugin_info->parser_header(mPlugin_handle, info);
    if(tmp!=0)
    {
        printf("%s: parser_header fail\n", __FILE__);
        mPlugin_info->close(mPlugin_handle);
        dlclose(mLib_handle);
        mLib_handle = NULL;
        mPlugin_handle = NULL;
        goto EXIT;
    }

    return info;
EXIT:
    return -1;
}

int parser_chunk(char *buf, int *len)
{
//    if(mLib_handle == NULL)
//    parser_open("/data/test.wav","WAV");

    return mPlugin_info->get_chunk(mPlugin_handle, buf, len);
}

int parser_seek(int time_offset, int whence, int *chunk_start_time)
{
//    if(mLib_handle == NULL)
//    parser_open("/data/test.wav","WAV");

    return mPlugin_info->seek_time(mPlugin_handle, time_offset, whence,chunk_start_time);
}
int parser_dispose(void)
{
    printf("________parser_dispose__________ %d %x %x",__LINE__,mPlugin_handle,mLib_handle);
     if(mPlugin_handle != NULL)
    {
        mPlugin_info->close(mPlugin_handle);
        dlclose(mLib_handle);        
        mLib_handle = NULL;
        mPlugin_handle = NULL;
    }

    return 0;
}
