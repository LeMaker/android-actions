/*
********************************************************************************
*                       NOYA(MAYA)---mmm_dct/mmm_mtp
*                (c) Copyright 2002-2006, Actions Co,Ld.
*                        All Right Reserved
*
* FileName: Check_format.c     Author:  yangyongdong        2008/07/15
* Description:
              1. 定义了检查文件格式的接口
* Others:         // 其它内容的说明
* History:        // 修改历史记录列表
*      <author>    <time>       <version >    <desc>
*         yyd     2008/07/15     1.0         build this file
********************************************************************************
*/
#include "format_check.h"

//char static inline  tolower(char toLower)
//{
//    if ((toLower >= 'A') && (toLower <= 'Z'))
//    {
//        return (char)(toLower + 0x20);
//    }
//    return toLower;
//}
//
//
//int static inline  strtolow(char * str)
//{
//    int i, tmp = strlen(str);
//    for(i=0;i<tmp;i++)
//    {
//        tolower(str[i]);
//    }
//    return 0;
//}

/*! \page format_check      format_check()
*
* \par  format_check()
*  - 功能描述 \n
*       格式识别函数，根据文件内容判断文件格式.               \n
*       目前支持的格式有：mp3, wma, ogg, fla(c), aac, ape, wav, mpc, aif(f), ra, aa  \n
*       视频：wmv, avi, swf, mp4, rm, flv, dat, mpg, amv, asf  \n
*       图片：jpg, gif, bmp, tif, png  \n
*  - 函数原型 \n
*       int format_check(storage_io_t* input, char* ext) \n
*  - 输入参数描述 \n
*       input ―― 中间件打开的文件句柄。 \n \n
*  - 输出参数描述 \n
*       ext ―― 文件格式，为小写的字符串(ASCII) \n
*       返回0为成功；-1为失败，失败时ext为随机值。
*  - 示例与说明 \n
* \code
*   char ext[4];
*   int pre_audio, ret;
*   int (*format_check)(storage_io_t* input, char* ext);
*   pre_audio = dlopen("pre_aud.so", RTLD_LAZY);
*   format_check = dlsym(pre_audio,"format_check");
*   ***********打开文件****************
*   ret = format_check(input, ext);
*   if(ret < 0)
*   {
*       prinrf("file format err!!\n");
*       return -1;
*   }
* \endcode
*/
int format_check(storage_io_t* input, const char* mime)
{
    int ret = 0, flag = 0, len = 5;
    unsigned int* tmp;
    char *ext = (char *)mime;
    char buf[16];    
    
    tmp = (unsigned int*)buf;
    if ((ext == NULL) || (input == NULL))
    {
        actal_error("format_check Err:buffer is empty!\n");
        return -1;
    }
    //    input->seek(input,0,SEEK_END);
    //    file_len = input->tell(input);
//    file_len = get_filesize_storage_io(input);
    input->seek(input,0,SEEK_SET);
    len = input->read(buf, 1, 16, input);
    if(len < 16)
    {
        actal_error("format_check read file header Err %x\n",len);
        return -1;
    }
    while((tmp[0] & 0xffffff) == 0x334449)
    {
        int tagsize;
        /* high bit is not used */
        if(((buf[6]|buf[7]|buf[8]|buf[9]) & 0x80) == 0x80)
        {
            actal_error("Bad ID3, skip ID3..\n");
            break;
        }
        
        tagsize =( (buf[6] << 21) | (buf[7] << 14)
                   | (buf[8] <<  7) | (buf[9] ));
        flag += (tagsize+10);
        
//        if(file_len < flag)
//        {
//            ret = -1;
//            actal_error("format check Err!(ID3) %d  ext:%s\n",__LINE__,ext);
//            goto ErrEXit;
//        }
//        
        input->seek(input, flag, SEEK_SET);
        /* get another 10bytes for format decision */
        len = input->read(buf, 1, 10, input);
        if(len != 10)
        {
            actal_error("format_check read ID3 Err\n");
            return -1;
        }
    }
    if(flag > 0)
    {
        if ((tmp[0] &0xffffff )== 0x2b504d)
        {
            strcpy(ext, PARSER_EXT_MPC);
        }
        else if (tmp[0] == 0x43614c66)
        {
            strcpy(ext, PARSER_EXT_FLAC);
        }
        else if (tmp[0] == 0x2043414d)
        {
            strcpy(ext, PARSER_EXT_APE);
        }
        else if (adts_aac_check(input) == 1)
        {
            strcpy(ext, PARSER_EXT_AAC);
        }
        else
        {
            
            input->seek(input, flag, SEEK_SET);
            flag = mp3check(input);     //0 is mp3
            if ( flag == 0)
            {
                strcpy(ext, PARSER_EXT_MP3);
                printf("format check other1 ext:%s\n",ext);
                goto ErrEXit;
            }
            else
            {
                ret = -1;
                actal_error("format check1 Err!(ID3)  ext:%s\n",ext);
                goto ErrEXit;
            }
        }
        printf("format_check ID3  ext:%s\n",ext);
        goto ErrEXit;
    }
    else if (tmp[0] == 0x75B22630)
    {
        input->seek(input,0,SEEK_SET);
        flag = wmaflag(input);
        if (flag == 0)
        {
            strcpy(ext, "wmv");
        }
        else if (flag == 1)
        {
            strcpy(ext, PARSER_EXT_WMA);
        }
        else
        {
            actal_error("format_check read ASF header Err\n");
            return -1;
        }        
        goto ErrEXit;
    }
    //"oggs"?" 0x4f,0x67,0x67,0x53"
    else if (tmp[0] == 0x5367674f)
    {
        input->seek(input,28,SEEK_SET);
        len = input->read(tmp, 1, 4,input);
        if(len != 4)
        {
            actal_error("format_check read SPX header Err\n");
            return -1;
        }
        if (tmp[0] == 0x65657053)
        {
            strcpy(ext, "SPX");
        }
        else if(tmp[0] == 0x726f7601) //0x726f7601
        {
            strcpy(ext, PARSER_EXT_OGG);
						
        }
        else 
        {
        		strcpy(ext, "ogm");
        }
        goto ErrEXit;
    }
    else if((tmp[2] & 0xffffff) == 0x564d41)
    {
//        strcpy(ext, "amv");
//        ALOGD("%s %d  ext:%s\n",__FILE__,__LINE__,ext);
        ret = -1;
        goto ErrEXit;
    }
    //"RIFF"?" 0x52,0x49,0x46,0x46"
    else if (tmp[0] == 0x46464952)
    {
        char sExt[4];
        sExt[0] = (buf[8] >= 'a') ? (buf[8] - 32) : buf[8]; // 32 = 'a' - 'A'
        sExt[1] = (buf[9] >= 'a') ? (buf[9] - 32) : buf[9];
        sExt[2] = (buf[10] >= 'a') ? (buf[10] - 32) : buf[10];
        sExt[3] = 0x20;

        if (memcmp(sExt, "AVI", 3) == 0)
        {
            strcpy(ext, "avi");
        }
        else if (strncmp(&buf[8], "CDXAfmt", 7) == 0)
        {
            strcpy(ext, "mpg");
        }
        else
        {
            flag = dts_check(input);
            if (flag == 1)
            {
                strcpy(ext, PARSER_EXT_DTS);
                actal_printf("format_check DTS 1th\n");
            }
            else
            {
                strcpy(ext, PARSER_EXT_WAV);
            }
        }
        goto ErrEXit;
    }
    //"ftyp"?" 0x66,0x74,0x79,0x70"
    else if (tmp[1] == 0x70797466)
    {
        input->seek(input,0,SEEK_SET);
        if (aacflag(input) == 0)
        {
            /* TODO: 01.m4a 音频文件被误判为视频 */
            strcpy(ext, "mp4");
            actal_printf("format_check aacflag\n");
        }
        else
        {
            int is_plus = aacplusflag(input);
            if (is_plus <= 0)
            {
                ret = -1;
                goto ErrEXit;
            }

            if (is_plus == 1 )
            {
                strcpy(ext, PARSER_EXT_ALAC);
            }
            else
            {
                strcpy(ext, PARSER_EXT_AAC);
                
            }
        }
        actal_printf("%s %d  ext:%s\n",__FILE__,__LINE__,ext);
        goto ErrEXit;
    }
    //".RMF"?" 0x2E,0x52,0x4D,0x46"
    else if (tmp[0] == 0x464d522e)
    {
        input->seek(input,0,SEEK_SET);
        flag = rm_check(input);
        if (flag == 1)
        {
            strcpy(ext, PARSER_EXT_RMA);
        }
        else
        {
            strcpy(ext, "rm");
        }
        goto ErrEXit;
    }
    else if ((tmp[0] &0xffffff )== 0x2b504d)
    {
        strcpy(ext, PARSER_EXT_MPC);
        goto ErrEXit;
    }
    else if (((tmp[0] & 0xffff) == 0x770b) ||((tmp[0] & 0xffff) == 0x0b77))
    {
        strcpy(ext, PARSER_EXT_AC3);
        goto ErrEXit;
    }
    else if (tmp[0] == 0x36759057)
    {
        strcpy(ext, PARSER_EXT_AA);
        goto ErrEXit;
    }
//    else if ((tmp[0] & 0xffffff) == 0x61722e)
//    {
//        strcpy(ext, "rm");
//        goto ErrEXit;
//    }
    else if ((tmp[0] & 0xfffffff) == 0x1564c46 || (tmp[0] & 0xffffffff) == 0x46564c58)
    {
        strcpy(ext, "flv");
        goto ErrEXit;
    }else if(tmp[0]==0x01000000 && buf[4]==0x40){
        strcpy(ext, "h265");
        goto ErrEXit;
    }
    else if ((tmp[0] == 0xba010000) || (tmp[0] == 0xb3010000))
    {
        strcpy(ext, "mpg");
        goto ErrEXit;
    }    
//    else if (tmp[0] == 0x4d412123 && buf[4] == 0x52 && buf[5] == 0x0a)
    else if (tmp[0] == 0x4d412123 )
    {
        strcpy(ext, PARSER_EXT_AMR);
        goto ErrEXit;
    }
    else if (tmp[0] == 0xa3df451a)
    {
        strcpy(ext, "mkv");
        goto ErrEXit;
    }
    /*
    //"ftyp" " 0x66,0x74,0x79,0x70"
    0x70797466
    //"moov" " 0x6d,0x6f,0x6f,0x76"
    0x766f6f6d
    //"mdat" " 0x6d,0x64,0x61,0x74"
    0x7461646d
    //"moof" " 0x6d,0x6f,0x6f,0x66"
    0x666f6f6d
    //"mfra" " 0x6d,0x66,0x72,0x61"
    0x6172666d
    //"pdin" " 0x70,0x64,0x69,0x6e"
    0x6e696470
    //"free" " 0x66,0x72,0x65,0x65"
    0x65657266
    //"skip" " 0x73,0x6b,0x69,0x70"
    0x70696b73
    //"meta" " 0x6d,0x65,0x74,0x61"
    0x6174656d
    */
    if ( (tmp[1] == 0x70797466)
        ||(tmp[1] == 0x766f6f6d)
        ||(tmp[1] == 0x7461646d)
        ||(tmp[1] == 0x666f6f6d)
        ||(tmp[1] == 0x6172666d)
        ||(tmp[1] == 0x6e696470)
        ||(tmp[1] == 0x65657266)
        ||(tmp[1] == 0x70696b73)
        ||(tmp[1] == 0x6174656d)
        ||(tmp[1] == 0x746f6e70)
        ||(tmp[1] == 0x65646977))
    {
        strcpy(ext, "mp4");
        goto ErrEXit;
    }
 
    //APE:"MAC "?" 0x4D,0x41,0x43,0x20"
    if (tmp[0] == 0x2043414d)
    {
        strcpy(ext, PARSER_EXT_APE);
        goto ErrEXit;
    }
    //fLaC"?" 0x66,0x4c,0x61,0x43"
    else if (tmp[0] == 0x43614c66)
    {
        strcpy(ext, PARSER_EXT_FLAC);
        goto ErrEXit;
    }
    else if (!(memcmp(buf, "FWS", 3)) || !(memcmp(buf, "CWS", 3)))
    {
        //strcpy(ext, "swf");
        ret = -1;
        goto ErrEXit;
    }
    else if (!(memcmp(buf, "FORM", 4)))
    {
        strcpy(ext, PARSER_EXT_AIFF);
        goto ErrEXit;
    }
//    else if((tmp[0] & 0xffff) == 0xd8ff)
//    {
//        strcpy(ext, "jpg");
//
//        printf("%s %d  ext:%s\n",__FILE__,__LINE__,ext);
//        goto ErrEXit;
//    }
//    else if((tmp[0] & 0xffffff) == 0x464947)
//    {
//        strcpy(ext, "gif");
//
//        printf("%s %d  ext:%s\n",__FILE__,__LINE__,ext);
//        goto ErrEXit;
//    }
//
//    else if((tmp[0] & 0xffff) == 0x4d42)
//    {
//        strcpy(ext, "bmp");
//
//        printf("%s %d  ext:%s\n",__FILE__,__LINE__,ext);
//        goto ErrEXit;
//    }
//    else if((tmp[0] & 0xffff == 0x4d4d) || (tmp[0] & 0xffff == 0x4141))
//    {
//        strcpy(ext, "tif");
//
//        printf("%s %d  ext:%s\n",__FILE__,__LINE__,ext);
//        goto ErrEXit;
//    }
//    else if(tmp[1] == 0x474e5089)
//    {
//        strcpy(ext, "png");
//
//        printf("%s %d  ext:%s\n",__FILE__,__LINE__,ext);
//        goto ErrEXit;
//    }
    else
    {
        input->seek(input,0,SEEK_SET);
        flag = mp3check(input);     //0 is mp3
        if ( flag == 0)
        {
            strcpy(ext, PARSER_EXT_MP3);
            actal_printf("format check other ext:%s\n",ext);
            goto ErrEXit;
        }
        input->seek(input,0,SEEK_SET);
        flag = adts_aac_check(input);     //1 is aac
        if (flag == 1)
        {
            strcpy(ext, PARSER_EXT_AAC);
            actal_printf("format check other ext:%s\n",ext);
            goto ErrEXit;
        }
		// ActionsCode(rongxing, some .ts files may contain .dts content that otherwise would
        // mistakenly lead to us identifying the entire file as a .dts file,so we must indicate
        // whether the file is a ts file firstly)
		input->seek(input,0,SEEK_SET);
        if(ts_check(input) == 1)
        {
            strcpy(ext, "ts");
            goto ErrEXit;
        }
        flag = dts_check(input);
        if (flag == 1)
        {
            strcpy(ext, PARSER_EXT_DTS);
            actal_printf("format check other ext:%s\n",ext);
            goto ErrEXit;
        }       
        ret = -1;
        actal_error("format undistinguish!!  ext:%s\n",ext);
    }
ErrEXit:
    actal_printf("format_Check  ext:%s\n",ext);
    return ret;
}
