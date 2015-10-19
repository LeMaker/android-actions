/*******************************************************************************
 *                                GL5003
 *                            Module: getinfo
 *                 Copyright(c) 2003-2008 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>    <time>           <version >             <desc>
 *       yyd     2010-02-21 10:00     1.0             build this file
*******************************************************************************/
/*!
 * \file     getinfo.c
 * \brief    获取媒体文件信息
 * \author   yyd
 * \par      GENERAL DESCRIPTION:
 *               通过格式识别函数，加载对应的parser库，获取媒体文件信息
 * \par      EXTERNALIZED FUNCTIONS:
 *               格式parser库, libalc.so,
 * \version 1.0
 * \date  2010/2/21
*******************************************************************************/
#include "getinfo.h"
#include "mmm_getinfo.h"
static void *(* func_handle)(void);
static void *plib_handle;//插件库句柄


int my_strrchr(const char *p, char tmp)
{
    int i = 0,len = strlen(p);
    for (i = len; len > 0; i--)
    {
        if (p[i] == tmp)
        {
            return i;
        }
    }
    return -1;
}

int mmm_info_getfileinfo(char *filename, mmm_info_file_info_t *info)
{
    int ret = 0;
    int file_extension;
    char ext[8];
    char plib_name[20] = {""};
    void *parser_handle;//插件库句柄
    stream_input_t *stream_input = NULL;
    storage_io_t *storage_io = NULL;
    int t1 = actal_get_ts();
    actal_printf("WARNNING: mmm_info_getfileinfo is slow!!!\n");
    if (stream_input == NULL)
    {
        stream_input = stream_input_open(READ_FLAG, 1 << 16);
        if (stream_input == NULL)
        {
            return -1;
        }

        storage_io = stream_input_get_storageio(stream_input);
        if (storage_io == NULL)
        {
            ret = -1;
            goto err_exit1;
        }
    }

    actal_memset(info, 0, sizeof(mmm_info_file_info_t));

    ret = (int)stream_input_init(stream_input,filename);
    if (ret <= 0)
    {
        actal_error("\n%s %d  %s ret: %x;\n",__func__,__LINE__,filename,ret);
        ret = -1;
        goto err_exit1;
    }
    file_extension = my_strrchr(filename, '.') + 1;
    memcpy(ext,&filename[file_extension],8);
    ret = format_check(storage_io,ext);
    if (ret < 0)
    {
        actal_printf("format_check Err: %s %s \n",filename,ext);
        goto err_exit2;
    }
    /*format_check 返回小写为视频，大写为音频*/
    if (ext[0] > 0x5a)
    {
        demux_plugin_t *demux_info;//插件库信息：插件函数入口
        media_info_t media_info;

        strcat(plib_name, "avd_");
        strcat(plib_name,ext);
        strcat(plib_name, LIBPOSTFIX);
        plib_handle = dlopen(plib_name, RTLD_LAZY);
        if (plib_handle == NULL)
        {
            actal_error("%s dlopen %s err\n",__FILE__,plib_name);
            ret = -1;
            goto err_exit2;
        }
        func_handle = dlsym(plib_handle,"get_plugin_info");
        if (func_handle == NULL)
        {
            ret = -1;
            goto err_exit3;
        }
        demux_info = (demux_plugin_t *)func_handle();
        if (demux_info == NULL)
        {
            actal_error("%s dlsym %s err\n",__FILE__,plib_name);
            ret = -1;
            goto err_exit3;
        }
        parser_handle = (void *)demux_info->open(stream_input,&media_info);
        if (parser_handle == NULL)
        {
            ret = -1;
            goto err_exit3;
        }
        demux_info->dispose(parser_handle);
        
        info->total_time = media_info.total_time;
        actal_memcpy(&info->format,ext,8);
        info->is_video = 1;
        info->audio_bitrate = media_info.parser_audio[0].audio_bitrate;
        actal_memcpy(info->audio_format, &media_info.parser_audio[0],16);
        actal_memcpy(info->video_format, &media_info.parser_video,24);
        info->video_bitrate = media_info.parser_video.video_bitrate;
        info->drm_flag = media_info.parser_drm.drm_flag;
        actal_printf("******Get Media Info.*******\n");
        actal_printf("   filename     : %s use: %dms\n",filename, actal_get_ts()-t1);
        actal_printf("   total time   : %dms \n",info->total_time);
        actal_printf("   file format  : %s \n",info->format);
        actal_printf("   is_video     : %d \n",info->is_video);
        actal_printf("   audio format : %s \n",info->audio_format);
        actal_printf("   sample_rate  : %dhz\n",info->sample_rate);
        actal_printf("   channels     : %d \n",info->channels);
        actal_printf("   audiobitrate : %dkbps\n",info->audio_bitrate);
        actal_printf("   video_format : %s \n",info->video_format);
        // actal_printf("   file w: %d h:%d\n",info->width, info->height);
        // actal_printf("   frame_rate   : %d \n",info->frame_rate);
        // actal_printf("   video_bitrate: %dbps \n",info->video_bitrate);
    }
    else
    {
        music_parser_plugin_t *plugin_info;
        music_info_t music_info;
        strcat(plib_name, "ap");
        strcat(plib_name, ext);
        strcat(plib_name, LIBPOSTFIX);
        plib_handle = dlopen(plib_name, RTLD_LAZY);
        if (plib_handle == NULL)
        {
            actal_error("%s: %s\n", __FILE__, dlerror());
            ret = -1;
            goto err_exit2;
        }

        func_handle = dlsym(plib_handle, "get_plugin_info");
        if (func_handle == NULL)
        {
            actal_error("%s: %s\n", __FILE__, dlerror());
            ret = -1;
            goto err_exit3;
        }

        plugin_info = (music_parser_plugin_t *)func_handle();
        if (plugin_info == NULL)
        {
            ret = -1;
            goto err_exit3;
        }
        parser_handle = plugin_info->open(storage_io);
        if (parser_handle == NULL)
        {
            ret = -1;
            goto err_exit3;
        }
        plugin_info->parser_header(parser_handle, &music_info);
        plugin_info->close(parser_handle);

        info->total_time = music_info.total_time*1000;
        actal_memcpy(&info->format, ext, 8);
        info->is_video = 0;
        actal_memcpy(info->audio_format, music_info.extension,8);
        info->audio_bitrate = music_info.avg_bitrate;
        info->sample_rate = music_info.sample_rate;
        info->channels = music_info.channels;
        //  info->drm_flag = music_info.drm_type;
        actal_printf("******Get Media Info.*******\n");
        actal_printf("   filename     : %s use: %dms\n",filename, actal_get_ts()-t1);
        actal_printf("   total time   : %dms \n",info->total_time);
        actal_printf("   file format  : %s \n",info->format);
        actal_printf("   is_video     : %d \n",info->is_video);
        actal_printf("   audio format : %s \n",info->audio_format);
        actal_printf("   sample_rate  : %dhz\n",info->sample_rate);
        actal_printf("   channels     : %d \n",info->channels);
        actal_printf("   audiobitrate : %dbps\n",info->audio_bitrate);
        // actal_printf("   video_format : %s \n",info->video_format);
        // actal_printf("   file w: %d h:%d\n",info->width, info->height);
        // actal_printf("   frame_rate   : %d \n",info->frame_rate);
        // actal_printf("   video_bitrate: %dbps \n",info->video_bitrate);
    }

err_exit3:
    dlclose(plib_handle);
    plib_handle = NULL;
err_exit2:
    stream_input_fini(stream_input);
err_exit1:
    if (stream_input != NULL)
    {
        stream_input_dispose(stream_input);
        stream_input = NULL;
    }
    return ret;
}

DL_EXPORT_SYMBOL(mmm_info_getfileinfo);
DL_EXPORT_SYMBOL(format_check);
int __attribute__((constructor)) so_init(void)
{
    return 0;
}

int __attribute__((destructor)) so_exit(void)
{

    return 0;
}
