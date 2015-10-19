#include "id3parse.h"
#include "music_parser_lib_dev.h"
#include "unicode/ucnv.h"
#include "unicode/ustring.h"

//#define LOG_NDEBUG 0
#define LOG_TAG "ID3parser"
#include <utils/Log.h>

namespace android
{

#define EXT_LEN   10
void Item_UTF8_To_UTF16(id3_item_info_t *pItemInfo);
void change_ext_standard(char *min, char *cap);
static void ensure_info(id3_item_info_t *info);




/*
 * out:     0         ----    转换失败
 *            not 0    ----    实际转换的字节数
 */


size_t codec_convert( char *from_charset,
                    char *to_charset,
                    char *inbuf,
                    size_t inlen,
                    char *outbuf,
                    size_t outlen )
{

    size_t length = outlen;
    char *pin = inbuf;
    char *pout = outbuf;
    UErrorCode status = U_ZERO_ERROR;

    UConverter *conv = ucnv_open(from_charset, &status);
    if (U_FAILURE(status)) {
        ALOGE("could not create UConverter for %s\n", from_charset);
        return -1;
    }
    UConverter *destConv = ucnv_open(to_charset, &status);
    if (U_FAILURE(status)) {
        ALOGE("could not create UConverter for  for %s\n", to_charset);
        ucnv_close(conv);
        return -1;
    }
    ucnv_convertEx(destConv, conv, &pout, pout + outlen,
            (const char **)&pin, (const char *)pin + inlen, NULL, NULL, NULL, NULL, TRUE, TRUE, &status);
    if (U_FAILURE(status)) {
        ALOGE("ucnv_convertEx failed: %d\n", status);
    } else {
    // zero terminate
        *pout = 0;
    }
    ucnv_close(conv);
    ucnv_close(destConv);
    return 0;
}

void getgenre(char c, id3_item_info_t* temp)
{
//    const char* genre_tab[] =
//        {
//            "Blues", "ClassicRock", "Country", "Dance", "Disco", "Funk", "Grunge", "Hip-Hop", "Jazz", "Metal", "NewAge",
//            "Oldies", "Other", "Pop", "R&B", "Rap", "Reggae", "Rock", "Techno", "Industrial", "Alternative", "Ska",
//            "DeathMetal", "Pranks", "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk", "Fusion",
//            "Trance", "Classical", "Instrumental", "Acid", "House", "Game", "SoundClip", "Gospel", "Noise",
//            "AlternRock", "Bass", "Soul", "Punk", "Space", "Meditative", "InstrumentalPop", "InstrumentalRock",
//            "Ethnic", "Gothic", "Darkwave", "Techno-Industrial", "Electronic", "Pop-Folk", "Eurodance", "Dream",
//            "SouthernRock", "Comedy", "Cult", "Gangsta", "Top40", "ChristianRap", "Pop/Funk", "Jungle",
//            "NativeAmerican", "Cabaret", "NewWave", "Psychadelic", "Rave", "Showtunes", "Trailer", "Lo-Fi", "Tribal",
//            "AcidPunk", "AcidJazz", "Polka", "Retro", "Musical", "Rock&Roll", "HardRock", "Folk", "Folk-Rock",
//            "NationalFolk", "Swing", "FastFusion", "Bebob", "Latin", "Revival", "Celtic", "Bluegrass", "Avantgarde",
//            "GothicRock", "ProgessiveRock", "PsychedelicRock", "SymphonicRock", "SlowRock", "BigBand", "Chorus",
//            "EasyListening", "Acoustic", "Humour", "Speech", "Chanson", "Opera", "ChamberMusic", "Sonata", "Symphony",
//            "BootyBass", "Primus", "PornGroove", "Satire", "SlowJam", "Club", "Tango", "Samba", "Folklore", "Ballad",
//            "PowerBallad", "RhythmicSoul", "Freestyle", "Duet", "PunkRock", "DrumSolo", "Acapella", "Euro-House",
//            "DanceHall", "Goa", "Drum&Bass"
//            "Club-House", "Hardcore", "Terror", "Indie", "BritPop", "Negerpunk", "PolskPunk", "Beat",
//            "ChristianGangstaRap", "HeavyMetal", "BlackMetal", "Crossover", "ContemporaryChristian", "ChristianRock",
//            "Merengue", "Salsa", "TrashMetal", "Anime", "JPop", "Synthpop"
//        };

    int i;
    i = c & 0xff;
//    if ( i >= (int)(sizeof(genre_tab) / sizeof(genre_tab[0])))
//    {
//        temp->length = 0;
//        temp->content = NULL;
//            if(NULL == temp->content)
//          	{
//          	    return;
//          	}
//            strcpy(temp->content, "unknown");
//        return;
//    }

//    temp->length = (int) strlen(genre_tab[i]);
    temp->content = (char*) malloc(16);
//    if (NULL == temp->content)
//    {
//        return;
//    }
    sprintf(temp->content, "%d", i);
    temp->length = 16;
    //strncpy(temp->content, genre_tab[i], (unsigned int)(temp->length+1));
//("%s\n", temp->content);
}

void freeItem(id3_item_info_t *item)
{
    if (item->content != NULL)
    {
        free(item->content);
    }
    memset(item, 0, sizeof(id3_item_info_t));
}

void freeallmemory(id3_info_total *info)
{
    freeItem(&info->tag.author);
    freeItem(&info->tag.composer);
    freeItem(&info->tag.album);
    freeItem(&info->tag.genre);
    freeItem(&info->tag.year);
    freeItem(&info->tag.title);
    freeItem(&info->tag.comment);
    freeItem(&info->tag.autoLoop);
#if __TRACK_NUM__ > 0

    freeItem(&info->tag.track);
#endif
}

/*!
 * 转换大端数据为小端数据
 *
 * \par  Description:
 *    以2byte为长度转换buffer中的大端数据为小端数据
 * \param[in]   char *  待转换的数据
 * \param[in]   int  buffer长度
 * \ingroup     IRVPlaylist
 *
 */
void TransToLittleEnd(char *buffer, int bufferLen)
{
    int i;
    char tmp;
    if (buffer == NULL)
    {
        return;
    }
    for (i = 0; i < bufferLen; i += 2)
    {
        tmp = buffer[i];
        buffer[i] = buffer[i + 1];
        buffer[i + 1] = tmp;
    }
    return;
}

/******************************************************************************/
/*!
 * \par  Description:
 *	  将item中的信息由整型转换为字符串.
 * \param[in]   pitem:  待转化的item
 * \param[out]  pitem: 转化后的item
 * \return      void
 *******************************************************************************/
void item_itoa(id3_item_info_t *pitem)
{
    int num;
    char buffer[10];
    int offset;
    if (pitem == NULL)
    {
        return;
    }
    if (pitem->length != 4)
    {
        return;
    }

    memcpy(&num, pitem->content, 4);
    num = num % 10000;
    if (num > 1000)
    {
        offset = 0;
    }
    else if (num > 100)
    {
        offset = 1;
    }
    else if (num > 10)
    {
        offset = 2;
    }
    else
    {
        offset = 3;
    }

    memset(buffer, 0, 10);
    buffer[0] = (char) ((num / 1000) + 0x30);
    num = num % 1000;
    buffer[1] = (char) ((num / 100) + 0x30);
    num = num % 100;
    buffer[2] = (char) ((num / 10) + 0x30);
    buffer[3] = (char) ((num % 10) + 0x30);

    /*去除前面无效的0*/
    buffer[0] = buffer[offset];
    buffer[1] = buffer[offset + 1];
    buffer[2] = buffer[offset + 2];
    buffer[3] = buffer[offset + 3];

    freeItem(pitem);
    assert(strlen(buffer) < 10);
    pitem->length = (int) strlen(buffer);
    pitem->encoding = ENCODING_NORMAL;
    pitem->content = (char *)malloc((unsigned int) (pitem->length + 1));
    if (NULL == pitem->content)
    {
        return;
    }
    memset(pitem->content, 0, (unsigned int) (pitem->length + 1));
    memcpy(pitem->content, buffer, (unsigned int) pitem->length);

}
/*    将unicode字符串转化为内码字符串(根据参数指定的语言的codepage) */
/*    只在wma 时 才会调用 Unicode2Char  */
//int mlang_unicode_to_mbcs(unsigned char *src_ucs, unsigned char *dst_mbcs,
//                          int src_len, int *dst_len, int nation_id)
//{
//    int ret = 0, i;
//#if 1
//    codec_convert("UCS-2LE", "GBK", (char *)src_ucs, src_len, (char *)dst_mbcs, *dst_len );
//#else   
//    for( i = 0; i < src_len; i++ )
//        dst_mbcs[i] = (unsigned char)(src_ucs[i] &0xff);
//    dst_mbcs[i] = 0;
//
//    *dst_len = src_len;
//#endif
//    return ret;
//}

/*  内码字符串转换到utf8编码字符串 */
int mlang_unicode_to_utf8 (unsigned short *unicode, int unicode_len, char *utf8, int *putf8_len)
{
    int i = 0;
    int outsize = 0;
    int charscount = 0;
    char *tmp = utf8;
#if 0
    codec_convert("unicode", "UTF-8", (char *)unicode, unicode_len, (char *)utf8, *putf8_len );
#else 
    charscount = unicode_len / sizeof(uint16_t);
    unsigned short unicode_tmp;
    for (i = 0; i < charscount; i++)
    {
        unicode_tmp = unicode[i];
        if (unicode_tmp >= 0x0000 && unicode_tmp <= 0x007f)
        {
            *tmp = (uint8_t)unicode_tmp;
            tmp += 1;
            outsize += 1;
        } 
        else if (unicode_tmp >= 0x0080 && unicode_tmp <= 0x07ff)
        {
            *tmp = 0xc0 | (unicode_tmp >> 6);
            tmp += 1;
            *tmp = 0x80 | (unicode_tmp & 0x003f);
            tmp += 1;
            outsize += 2;
        }
        else if (unicode_tmp >= 0x0800 && unicode_tmp <= 0xffff)
        {
            *tmp = 0xe0 | (unicode_tmp >> 12);
            tmp += 1;
            *tmp = 0x80 | ((unicode_tmp >> 6) & 0x003f);
            tmp += 1;
            *tmp = 0x80 | (unicode_tmp & 0x003f);
            tmp += 1;
            outsize += 3;
        }

    }

    *tmp = '\0';
    *putf8_len = outsize;
#endif    
    return 0;
}

//
//int mlang_mbcs_to_utf8 (unsigned char *mbcs, int mbcs_len, char *utf8, int *putf8_len, int language)
//{
//    int ret = 0;
//#if 1
//    codec_convert( "GBK", "UTF-8",  (char *)mbcs, mbcs_len,  (char *)utf8, *putf8_len );
//#else
//    int utf8len = 0;
//    for (size_t i = 0; i < mbcs_len; ++i) {
//        if (mbcs[i] == '\0') {
//            mbcs_len = i;
//            break;
//        } else if (mbcs[i] < 0x80) {
//            ++utf8len;
//        } else {
//            utf8len += 2;
//        }
//    }
//    *putf8_len = utf8len;
//    if (utf8len == mbcs_len) {
//        // Only ASCII characters present.
//        memcpy(utf8, mbcs, mbcs_len);
//        return ret;
//    }
//
//    char *ptr = utf8;
//    for (size_t i = 0; i < mbcs_len; ++i) {
//        if (mbcs[i] == '\0') {
//            break;
//        } else if (mbcs[i] < 0x80) {
//            *ptr++ = mbcs[i];
//        } else if (mbcs[i] < 0xc0) {
//            *ptr++ = 0xc2;
//            *ptr++ = mbcs[i];
//        } else {
//            *ptr++ = 0xc3;
//            *ptr++ = mbcs[i] - 64;
//        }
//    }
//#endif
//
//    return ret;
//}
///*
//    first we need to untangle the utf8 and convert it back to the original bytes
//    since we are reducing the length of the string, we can do this in place*/
//int mlang_mbcs_to_mbcs (unsigned char *mbcs, int mbcs_len, char *dest, int *pdest_len, int language)
//{
//    int ret = 0;
//    ALOGD("mlang_mbcs_to_mbcs %s",mbcs);
//    int len = mbcs_len;
//    uint8_t uch;
//    while ((uch = *mbcs++)) {
//    if (uch & 0x80)
//       *dest++ = ((uch << 6) & 0xC0) | (*mbcs++ & 0x3F);
//    else
//       *dest++ = uch;
//    }
//    *dest = 0;
//    *pdest_len = mbcs_len;
//    return ret;
//}
/********************************************************************************
 函数说明 :获取以00 00 为结束标志的字符串长度(unicode)
 
 输入参数:
 buffer:存放buffer
 bufferLen: buffer长度
 
 返回值:
 长度
 
 注意:
 ********************************************************************************/
int wstrlen(char *buffer, int bufferLen)
{
    int retVal;
    short *wchar;
    int i;
    int len = bufferLen / 2;
    wchar = (short *) buffer;
    for (i = 0; i < len; i++)
    {
        if (*wchar == 0)
        {
            break;
        }
        wchar++;
    }
    if (i <= len)
    {
        retVal = i * 2;
    }
    else
    {
        retVal = 0;
    }
    return retVal;
}

int Unicode2utf8(char *str, int length)
{
    char * temp;
    int stringlen;
    int dst_len;
    int retVal;

    if ((str == NULL) || (length < 0))
    {
        return -1;
    }

    temp = (char *) malloc(512);
    if (NULL == temp)
    {
        return -1;
    }
    dst_len = 512;
    retVal = mlang_unicode_to_utf8((unsigned short *)str, length, temp, &dst_len);
    if (retVal < 0)
    {
        free(temp);
        return -1;
    }

    memcpy(str, (const char*)temp, (size_t)(dst_len+1));
    free(temp);
    return 0;
}
//        } 
//        */              
//    }
//    pdst[num] = 0;
//    return num;
//}

/*
 * test modifiy utf8
 * phchen.2011.10.26
 */
int is_modifiy_utf8 (char *utf8, int length)
{
    int i = 0;
    unsigned char chr;
    char *pt = utf8;

    while ( i < length ) 
    {
        chr = *(pt++);
        i++;
        // Switch on the high four bits.
        switch (chr >> 4) 
        {
            case 0x00:
            case 0x01:
            case 0x02:
            case 0x03:
            case 0x04:
            case 0x05:
            case 0x06:
            case 0x07: 
            {
                // Bit pattern 0xxx. No need for any extra bytes.
                break;
            }
            case 0x08:
            case 0x09:
            case 0x0a:
            case 0x0b:
            case 0x0f:
            {
                /*
                 * Bit pattern 10xx or 1111, which are illegal start bytes.
                 * 
                 */
                 return -1;
            }

            case 0x0e: 
            {
                // Bit pattern 1110, so there are two additional bytes.
                chr = *(pt++);
                i++;
                if ((chr & 0xc0) != 0x80) 
                {
                    return -1;
                }
            }
            case 0x0c:
            case 0x0d: 
            {
                // Bit pattern 110x, so there is one additional byte.
                chr = *(pt++);
                i++;
                if ((chr & 0xc0) != 0x80) 
                {
                    return -1;
                }
                break;
            }
        }
    }
    return 0;
}

//所有编码目标编码为utf8
/*
 * actions_code(author:lishiyuan,change_code)
 */
void transItemEncoding(id3_item_info_t *targetItem, id3_item_info_t *sourceItem, char encoding)
{
    int length;
    int ret = 0;

    if ((NULL == sourceItem) || (NULL == targetItem) || (sourceItem->length <= 0))
    {
        return;
    }
    memset(targetItem, 0, sizeof(id3_item_info_t));

    length = ID3_SECTORSIZE * 3;
    targetItem->content = (char *) malloc((unsigned int) length);
    if (NULL == targetItem->content)
    {
        return;
    }
    memset(targetItem->content, 0, (unsigned int) length);

    if (ENCODING_UTF8 == sourceItem->encoding)
    {
        /*
        if(sourceItem->length > length)
        {
            sourceItem->length = length-1;
        }
        memcpy(targetItem->content, sourceItem->content, (unsigned int) sourceItem->length);
        targetItem->length = sourceItem->length;
        */
        if (sourceItem->content == NULL)
        {
            targetItem->length = 0;
            free(targetItem->content);
            targetItem->content = NULL;
            return;
        }
 
        ret = is_modifiy_utf8((char *)sourceItem->content, sourceItem->length);
        if (ret != 0)
        {
            char *pt =  targetItem->content;
            targetItem->length = sourceItem->length + 2;
            *pt    = 0xcf;
            pt++;
            *pt    = 0xcf;
            pt++;			
            memcpy(pt, sourceItem->content, (unsigned int) sourceItem->length);
            pt[sourceItem->length] = 0;
        }
        else
        {
            memcpy(targetItem->content, sourceItem->content, (unsigned int) sourceItem->length);
            targetItem->length = sourceItem->length;
        }    
           /*     
        ret = mlang_utf8_to_android_utf8((char *)sourceItem->content, sourceItem->length, targetItem->content, &length);
        if (ret <= 0)
        {
            targetItem->length = 0;
            free(targetItem->content);
            targetItem->content = NULL;
            ALOGV("target content NULL, ENCODING_UTF8\n");
            return;
        }
        targetItem->length = ret;
        */
        ALOGV("ENCODING_UTF8 (%s)(%s)\n", sourceItem->content, targetItem->content);
        
    }
    else if (ENCODING_NORMAL == sourceItem->encoding)
    {
        /*
        ret = mlang_mbcs_to_utf8((unsigned char *)sourceItem->content, sourceItem->length, targetItem->content, &length, -1);
        if ((ret < 0) || (length < 0))
        {
            targetItem->length = 0;
            free(targetItem->content);
            targetItem->content = NULL;
            return;
        }
        targetItem->length = (int) strlen(targetItem->content);
        */
        
        // make something flag to point that it is mbcs,
        // phchen 2011.10.26
        char *pt = sourceItem->content;
        ALOGV("ENCODING_NORMAL begin (%s)\n", sourceItem->content);
        ret = 0;
        for (int i = 0; i < sourceItem->length; i++,pt++)
        {
            // Is the string all assic,
            if ((*pt >= 0x80) || (*pt < 0))
            {
                ret = -1;
                break;
            }
        } 
        pt =  targetItem->content;
        targetItem->length = sourceItem->length;		
        if (ret < 0)
        {
            // these are not only inclue ASSIC.
            targetItem->length = sourceItem->length + 2;			
            *pt    = 0xcf;
            pt++;
            *pt    = 0xcf;
            pt++;		
        }
        memcpy(pt, sourceItem->content, (unsigned int) sourceItem->length);
        pt[sourceItem->length] = 0;
        ALOGV("ENCODING_NORMAL end (%s)\n", pt);
    }
    else if (ENCODING_UNICODE == sourceItem->encoding)
    {
        ret = mlang_unicode_to_utf8((unsigned short *)sourceItem->content, sourceItem->length, targetItem->content, &targetItem->length);
        if ((ret < 0) || (length < 0))
        {
            targetItem->length = 0;
            free(targetItem->content);
            targetItem->content = NULL;
            ALOGV("target content NULL, ENCODING_UNICODE\n");
            return;
        }
        ALOGV("ENCODING_UNICODE (%s)(%s)\n", sourceItem->content, targetItem->content);
    }
    else
    {
        ALOGD("unknown ENCODING\n");
        return;
    }

}

void transItem(id3_item_info_t *pitem, char encoding)
{
    id3_item_info_t tmpItem;

    memset(&tmpItem, 0, sizeof(tmpItem));
    /*
    if (pitem->encoding == encoding)
    {
        return;
    }
    */
    transItemEncoding(&tmpItem, pitem, encoding);
    freeItem(pitem);
    memcpy(pitem, &tmpItem, sizeof(tmpItem));
}

/*获取音乐文件的id3信息*/
//int get_audio_id3_info(const char* fileinfo, id3_info_total* info)
int get_audio_id3_info(ID3file_t* fp, char *fileinfo, id3_info_total* info)
{
    id3_item_info_t *id3tag;
    int i;
    int value = 0;
    int length;
    void * source = (void *)fp;

    if((strcmp(fileinfo, PARSER_EXT_MP3) == 0) ||(strcmp(fileinfo, "audio/mpeg") == 0))
    {
        get_mp3_audio_info(source, info);
    }
    else if(( strcmp(fileinfo, PARSER_EXT_WMA) == 0) ||(strcmp(fileinfo, "audio/x-ms-wma") == 0))
    {
        get_wma_audio_info(source, info);
    }
    else if(( strcmp(fileinfo, PARSER_EXT_OGG) == 0)||(strcmp(fileinfo, "audio/ogg") == 0))
    {
        get_ogg_audio_info(source, info);
    }    
    else if(( strcmp(fileinfo, PARSER_EXT_FLAC) == 0)||(strcmp(fileinfo, "audio/x-flac") == 0))
    {
        get_flac_audio_info(source, info);
    }
    else
    {
        get_ape_audio_info(source, info);
    }

    id3tag = &info->tag.author;
    
/*   由于字符串转换没有实现，可能会导致死机，暂时注释掉 by yyd 2011-5-25 15:58*/ 
#if 1
    for (i = 0; i < 8; i++)
    {
    //将所有id3信息转换为utf8格式
        ensure_info(id3tag);
        transItem(id3tag, ENCODING_UTF8);
        id3tag++;
    }
#endif    
    return 0;
}

/******************************************************************************/
/*!
 * \par  Description:
 *	将文件后缀转为大写方式
 * \param[in]    char *min 待转换字符
 * \param[out]   char *cap 转换后的字符
 * \return
 * \retval
 * \ingroup      drv fileselector api
 **\attention
 *******************************************************************************/
void change_ext_standard(char *min, char *cap)
{
    unsigned int i;
    char c;

    if ((NULL == min) || (NULL == cap))
    {
        return;
    }

    for (i = 0, c = *min; (c != 0)&&(i < (EXT_LEN-1)); min++, c = *min)
    {
        if ((c > 0x60) && (c < 0x7b))
        {
            cap[i] = c - 32;
        }
        else
        {
            cap[i] = c;
        }
        i++;
    }
    cap[i] = '\0';
}

/*!
 * 确保item内容
 *
 * \par  Description:
 *   确保item内容，去除多余空格并添加结束符
 * \param[in]    item_info_t *info : 需要确认的item
 * \return   void
 */
static void ensure_info(id3_item_info_t *info)
{
    char * tmp;
    int i;

    if (info == NULL)
    {
        return;
    }
    if (info->content == NULL)
    {
        memset(info, 0, sizeof(id3_item_info_t));
        return;
    }

    /*去除空格*/
    if ((info->encoding == ENCODING_NORMAL) || (info->encoding == ENCODING_UTF8))
    {
        info->length = (int)strlen(info->content);
        i = info->length - 1;
        while (i >= 0)
        {
            if (info->content[i] != 0x20)
            {
                break;
            }
            i--;
        }
        if (i < 0)
        {
            freeItem(info);
            return;
        }
        info->content[i + 1] = '\0';
        info->length = (int)strlen(info->content);
    }

    return;
}

} // namespace android
