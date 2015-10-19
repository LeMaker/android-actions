#include "id3parse.h"

namespace android {
#define ID3_BUFFER_SIZE  4096
/******************************************************************************/
/*!
 * \par  Description:
 *	  解析mp3文件id3v1格式标签的id3信息
 * \param[in]   pfile : mp3文件文件句柄
 * \param[out]  pid3Info: mp3文件的id3信息
 * \return      the result
 * \retval      >=0  sucess
 * \retval      <0  文件无id3v1格式标签
 *******************************************************************************/
int parse_id3V1(void *fp, id3_info_total* pid3Info)
{
    char tmpBuffer[128];
    char genre;

    memset(pid3Info, 0, sizeof(id3_info_total));
    memset(tmpBuffer, 0, 128);
    ID3_fseek(fp, -128, SEEK_END);
    ID3_fread(tmpBuffer, sizeof(char), 128, fp);
    if (memcmp(tmpBuffer, "TAG", 3) != 0)
    {
        return -1;
    }

    /*title*/
    pid3Info->tag.title.content = (char *) malloc(31);
    if (NULL == pid3Info->tag.title.content)
    {
        return -1;
    }
    pid3Info->tag.title.length = 31;
    pid3Info->tag.title.encoding = ENCODING_NORMAL;
    memset(pid3Info->tag.title.content, 0, 31);
    memcpy(pid3Info->tag.title.content, tmpBuffer + 3, 30);

    /*auther*/
    pid3Info->tag.author.content = (char *) malloc(31);
    if (NULL == pid3Info->tag.author.content)
    {
        return -1;
    }
    pid3Info->tag.author.length = 31;
    pid3Info->tag.author.encoding = ENCODING_NORMAL;
    memset(pid3Info->tag.author.content, 0, 31);
    memcpy(pid3Info->tag.author.content, tmpBuffer + 33, 30);

    /*album*/
    pid3Info->tag.album.content = (char *) malloc(31);
    if (NULL == pid3Info->tag.album.content)
    {
        return -1;
    }
    pid3Info->tag.album.length = 31;
    pid3Info->tag.album.encoding = ENCODING_NORMAL;
    memset(pid3Info->tag.album.content, 0, 31);
    memcpy(pid3Info->tag.album.content, tmpBuffer + 63, 30);

    /*year*/
    pid3Info->tag.year.content = (char *) malloc(5);
    if (NULL == pid3Info->tag.year.content)
    {
        return -1;
    }
    pid3Info->tag.year.length = 5;
    pid3Info->tag.year.encoding = ENCODING_NORMAL;
    memset(pid3Info->tag.year.content, 0, 5);
    memcpy(pid3Info->tag.year.content, tmpBuffer + 93, 4);

    /*COMM*/
    pid3Info->tag.comment.content = (char *) malloc(31);
    if (NULL == pid3Info->tag.comment.content)
    {
        return -1;
    }
    pid3Info->tag.comment.length = 31;
    pid3Info->tag.comment.encoding = ENCODING_NORMAL;
    memset(pid3Info->tag.comment.content, 0, 31);
    memcpy(pid3Info->tag.comment.content, tmpBuffer + 97, 30);

    /*genre*/
    genre = tmpBuffer[127];
    getgenre(genre, &pid3Info->tag.genre);

    return 1;
}

/******************************************************************************/
/*!
 * \par  Description:
 *	  解析mp3文件id3v2格式APIC frame
 * \param[in]   pfile : mp3文件文件句柄
 * \param[in]   picAddr: APIC frame地址
 * \param[in]   picSize : APIC frame大小
 * \param[out]  imageInfo: APIC frame信息
 * \return      void
 *******************************************************************************/
void get_mp3_image(void *pFile, int picAddr, int picSize, id3_image_t *imageInfo, int version)
{
    char mineType[31];
    char *imageType;
    char *tmpBuffer;
    short data;
    char textEncoding;
    char endStringTimes = 0;
    int picDataOffset = 0;
    int i = 0, j, ret;

    memset(mineType, 0, 31);
    memset(imageInfo, 0, sizeof(id3_image_t));
    if ((pFile == 0) || (picSize <= 0))
    {
        return;
    }
    tmpBuffer = (char *) malloc(ID3_BUFFER_SIZE);
    if (NULL == tmpBuffer)
    {
        return;
    }
    memset(tmpBuffer, 0, ID3_BUFFER_SIZE);
    ID3_fseek(pFile, picAddr, SEEK_SET);
    ret = (int) ID3_fread(tmpBuffer, sizeof(char), ID3_BUFFER_SIZE, pFile);

    textEncoding = tmpBuffer[0];
    
    if (version == 2)
    {
        memcpy(imageInfo->imageType, tmpBuffer + 1, 3);
        imageInfo->imageType[3] = '\0';

        i = 5;
        while (i < (ret - 1))
        {
            /*找到一个字符串结束符*/
            if (tmpBuffer[i] == 0)
            {
                /*unicode编码为双字节0表示结束*/
                if (textEncoding == 1)
                {
                    if (tmpBuffer[i + 1] != 0)
                    {
                        continue;
                    }
                    i++;
                }
                break;
            }
            if (i >= (ret - 2))
            {
                picDataOffset += ID3_BUFFER_SIZE - 1;
                ID3_fseek(pFile, picAddr + picDataOffset, SEEK_SET);
                ret = (int) ID3_fread(tmpBuffer, sizeof(char), ID3_BUFFER_SIZE, pFile);
                if (ret <= 0)
                {
                    free(tmpBuffer);
                    return;
                }
                i = 0;
            }
        }

        picDataOffset += i + 1;
        imageInfo->offset = picAddr + picDataOffset;
        imageInfo->length = picSize - picDataOffset;
        free(tmpBuffer);
        return;

    }
    i = 1;
    do
    {
        if (tmpBuffer[i] == 0)
        {
            memcpy(mineType, tmpBuffer + 1, 30);
            i += 2;
            break;
        }
        if (i >= (ID3_BUFFER_SIZE - 2))
        {
            picDataOffset += ID3_BUFFER_SIZE - 1;
            ID3_fseek(pFile, picAddr + picDataOffset, SEEK_SET);
            ID3_fread(tmpBuffer, sizeof(char), ID3_BUFFER_SIZE, pFile);
            i = 0;
        }
        i++;
    } while (1);
    do
    {
        if (i >= (ID3_BUFFER_SIZE - 2))
        {
            picDataOffset += ID3_BUFFER_SIZE - 1;
            ID3_fseek(pFile, picAddr + picDataOffset, SEEK_SET);
            ID3_fread(tmpBuffer, sizeof(char), ID3_BUFFER_SIZE, pFile);
            i = 0;
        }
        /*找到一个字符串结束符*/
        if (textEncoding == 0)
        {
            if (tmpBuffer[i] == 0)
            {
                break;
            }
        }
        else /*unicode编码为双字节0表示结束*/
        {
            //pbuf =(short *)(tmpBuffer+i);
            memcpy(&data, tmpBuffer + i, sizeof(short));
            i++;/*unicode是双字节*/
            if (data == 0)
            {
                break;
            }
        }
        i++;
    } while (1);
    picDataOffset += i + 1;
    mineType[30] = '\0';
    imageType = strrchr(mineType, '/');
    if (imageType != NULL)
    {
        strncpy(imageInfo->imageType, imageType + 1, 8);
        imageInfo->offset = picAddr + picDataOffset;
        imageInfo->length = picSize - picDataOffset;
    }
    exit: free(tmpBuffer);
    return;
}

/******************************************************************************/
/*!
 * \par  Description:
 *	  解析mp3文件id3v2 frame
 * \param[in]   fp : mp3文件文件句柄
 * \param[in]   buffer : buffer
 * \param[in]   bufferLen : buffer 长度
 * \param[in]   pfileOffset : frame在文件中的offset
 * \param[in]   pbufferOffset : frame在buffser 中的offset
 * \param[out]  pframeInfo: frame信息
 * \return      void
 *******************************************************************************/
void parse_id3V2_frame(void *fp, char *buffer, int bufferLen, int *pfileOffset, int *pbufferOffset,
        id3_item_info_t *pframeInfo, int vesion)
{
    int frameLen;
    int bufferOffset;
    int fileOffset;

    memset(pframeInfo, 0, sizeof(id3_item_info_t));
    bufferOffset = *pbufferOffset;
    fileOffset = *pfileOffset;
    if (vesion >= 3)
    {
        frameLen = ((unsigned char) buffer[bufferOffset] * 0x1000000) + ((unsigned char) buffer[bufferOffset + 1]
                * 0x10000) + ((unsigned char) buffer[bufferOffset + 2] * 0x100) + (unsigned char) buffer[bufferOffset
                + 3];
        bufferOffset += 6; /*frame length 4bytes + flag 2bytes*/
        fileOffset += 6;
    }
    else if (vesion == 2)
    {
        frameLen = ((unsigned char) buffer[bufferOffset] * 0x10000)
                + ((unsigned char) buffer[bufferOffset + 1] * 0x100) + ((unsigned char) buffer[bufferOffset + 2]);
        bufferOffset += 3; /*frame length 4bytes + flag 2bytes*/
        fileOffset += 3;
    }
    else
    {
        return;
    }
    if (frameLen > ID3_SECTORSIZE)
    {
        frameLen = ID3_SECTORSIZE;
    }

    if ((bufferOffset + frameLen) > bufferLen)
    {
        ID3_fseek(fp, fileOffset, SEEK_SET);
        ID3_fread(buffer, sizeof(char), (unsigned int) bufferLen, fp);
        bufferOffset = 0;
    }

    /*编码方式:0, 内码; 1:unicode*/
    pframeInfo->encoding = buffer[bufferOffset];

    //printf("%s__%d__pframeInfo->encoding:%d\n", __FILE__,__LINE__, pframeInfo->encoding);
    bufferOffset++;
    fileOffset++;
    switch (pframeInfo->encoding)
    {
    case 0:
        pframeInfo->length = frameLen - 1;
        pframeInfo->encoding = ENCODING_NORMAL;
        break;
    case 1:
        pframeInfo->length = frameLen - 1;
        if ((pframeInfo->length % 2) != 0)
        {
            pframeInfo->length--;
        }
        /*unicode 签名(BOM):为0xFF FE 或者0xFE FF */
        if ((buffer[bufferOffset] == (char) 0x0fe) && (buffer[bufferOffset + 1] == (char) 0x0ff))
        {
            pframeInfo->length -= 2;
            TransToLittleEnd(buffer + bufferOffset + 2, pframeInfo->length);
            bufferOffset += 2;
            fileOffset += 2;
        }
        else if ((buffer[bufferOffset] == (char) 0x0ff) && (buffer[bufferOffset + 1] == (char) 0x0fe))
        {
            pframeInfo->length -= 2;
            bufferOffset += 2;
            fileOffset += 2;
        }
        else
        {
            ;//可能不包含头字节，此处容错处理 //
        }
        //dump(buffer + bufferOffset + 2, pframeInfo->length);

        pframeInfo->encoding = ENCODING_UNICODE;
        break;
    case 2:
        pframeInfo->encoding = ENCODING_UNICODE;
        pframeInfo->length = frameLen - 1;
        break;
    case 3:
        pframeInfo->encoding = ENCODING_UTF8;
        pframeInfo->length = frameLen - 1;
        break;
    default:
        return ;
    }

    if (pframeInfo->length > 0)
    {
        pframeInfo->content = (char*) malloc((unsigned int) (pframeInfo->length + 2));
        if (NULL != pframeInfo->content)
        {
            memset(pframeInfo->content, 0, (unsigned int) (pframeInfo->length + 2));
            memcpy(pframeInfo->content, buffer + bufferOffset, (unsigned int) pframeInfo->length);
        }
    }

    bufferOffset += pframeInfo->length;
    fileOffset += pframeInfo->length;

    *pbufferOffset = bufferOffset;
    *pfileOffset = fileOffset;
}

int parse_id3V2_2(void *fp, int headsize, char * buffer, id3_info_total* pid3Info)
{
    int keysize = 0;
    int fileOffset;
    int bufferOffset;
    int number;
    id3_item_info_t *ptmpItem;
    int picAddr;
    int picSize;
    int data, offset, j = 0;
    unsigned int ret;
    unsigned int bufoff_tmp = 0, fileoff_tmp = 0;

    number = 0;
#if __TRACK_NUM__ <= 0
    number++;
#endif
    picAddr = 0;
    picSize = 0;
    fileOffset = 10;
    bufferOffset = 10;
    while ((fileOffset < headsize) && (number < 7) && (j < 20))
    {
        j++;
        if (bufferOffset >= (ID3_BUFFER_SIZE - 8))
        {
            ID3_fseek(fp, fileOffset, SEEK_SET);
            ret = ID3_fread(buffer, sizeof(char), ID3_BUFFER_SIZE, fp);
            if (ret <= 0)
            {
                return -1;
            }
            bufferOffset = 0;
        }

        if ((UPPER(buffer[bufferOffset]) == 'T'))
        {
            offset = bufferOffset + 1;
            data = (int) UPPER(buffer[bufferOffset]);
            data = (data << 8) + UPPER(buffer[offset]);
            offset++;
            data = (data << 8) + UPPER(buffer[offset]);
            ptmpItem = NULL;
            switch (data)
            {
                case 0x545432: // TT2
                {
                    if (pid3Info->tag.title.length == 0)
                    {
                        ptmpItem = &pid3Info->tag.title;
                    }
                    break;
                }
    
                case 0x545031: // TP1
                {
                    if (pid3Info->tag.author.length == 0)
                    {
                        ptmpItem = &pid3Info->tag.author;
                    }
                    break;
                }
    
                case 0x54414c: // TAL
                {
                    if (pid3Info->tag.album.length == 0)
                    {
                        ptmpItem = &pid3Info->tag.album;
                    }
                    break;
                }
    
                case 0x545945: // TYE
                {
                    if (pid3Info->tag.year.length == 0)
                    {
                        ptmpItem = &pid3Info->tag.year;
                    }
                    break;
                }
    
                case 0x54434f: // TCO
                {
                    if (pid3Info->tag.genre.length == 0)
                    {
                        ptmpItem = &pid3Info->tag.genre;
                    }
                    break;
                }
    
#if __TRACK_NUM__ > 0
                case 0x54524b: // TRK
                {
                    if(pid3Info->tag.track.length == 0)
                    {
                        ptmpItem = &pid3Info->tag.track;
                    }
                    break;
                }
#endif
                default:
                {
                    ptmpItem = NULL;
                    break;
                }
            }

            if (ptmpItem != NULL)
            {
                bufoff_tmp = (unsigned int) (bufferOffset + 3);
                fileoff_tmp = (unsigned int) (fileOffset + 3);
                parse_id3V2_frame(fp, buffer, ID3_BUFFER_SIZE, (int*)&fileoff_tmp, (int*)&bufoff_tmp, ptmpItem, 2);
                number++;
            }
        }

        //else
        {
            keysize = ((unsigned char) buffer[bufferOffset + 3] * 0x10000) + ((unsigned char) buffer[bufferOffset + 4]
                    * 0x100) + ((unsigned char) buffer[bufferOffset + 5]);
            if (strncmp(buffer + bufferOffset, "PIC", 3) == 0)
            {
                if ((picAddr == 0) && (keysize > 128))
                {
                    picAddr = fileOffset + 6;
                    picSize = keysize;
                    number++;
                }
            }
            if (keysize == 0)
            {
                j++;
            }
            else
            {
                j = 0;
            }

            bufferOffset += 6 + keysize;
            fileOffset += 6 + keysize;
        }
    }
    if (picSize > 0)
    {
        get_mp3_image(fp, picAddr, picSize, &(pid3Info->tag.imageInfo), 2);
    }
    return number;

}

/******************************************************************************/
/*!
 * \par  Description:
 *	  解析mp3文件id3v2格式标签的id3信息
 * \param[in]   pfile : mp3文件文件句柄
 * \param[out]  pid3Info: mp3文件的id3信息
 * \return      the result
 * \retval      >=0  sucess
 * \retval      <0  文件无id3v1格式标签
 *******************************************************************************/
int parse_id3V2_3(void *fp, id3_info_total* pid3Info)
{
    char tmpBuffer[ID3_BUFFER_SIZE];
    unsigned int fileOffset, tagSize;
    unsigned int bufferOffset;
    unsigned int bufoff_tmp = 0, fileoff_tmp = 0;
    int number;
    id3_item_info_t *ptmpItem;
    int picAddr;
    int picSize;
    int data, offset, j = 0, keysize = 0;
    unsigned int ret;
    int version;

    memset(tmpBuffer, 0, ID3_BUFFER_SIZE);
    memset(pid3Info, 0, sizeof(id3_info_total));
    ID3_fseek(fp, 0, SEEK_SET);
    ret = ID3_fread(tmpBuffer, sizeof(char), ID3_BUFFER_SIZE, fp);
    if (ret <= 0)
    {
        return -1;
    }
    if ((memcmp(tmpBuffer, "ID3", 3) == 0) && (tmpBuffer[3] >= 2))
    {
        tagSize = (unsigned int) ((tmpBuffer[6] & 0x7F) * 0x200000 + ((tmpBuffer[7] & 0x7F) * 0x4000) + ((tmpBuffer[8]
                & 0x7F) * 0x80) + (tmpBuffer[9] & 0x7F));
    }
    else
    {
        return -1;
    }
    /* 版本说明 version 值：2，mp3 2.0；3/4，mp3 2.3，2.4 */
    version = (int) tmpBuffer[3];

    bufferOffset = 10;
    number = 0;
#if __TRACK_NUM__ <= 0
    number++;
#endif
    picAddr = 0;
    picSize = 0;

    if (version == 2)//2.2版本
    {
        return parse_id3V2_2(fp, (int) tagSize, tmpBuffer, pid3Info);
    }
    //for (fileOffset = 10; (fileOffset < tagSize) && (number < 7); fileOffset++)
    fileOffset = 10;
    while ((fileOffset < tagSize) && (number < 7) && (j < 20))
    {
        j++;
        if (bufferOffset >= (ID3_BUFFER_SIZE - 8))
        {
            ID3_fseek(fp, (int) fileOffset, SEEK_SET);
            ret = ID3_fread(tmpBuffer, sizeof(char), ID3_BUFFER_SIZE, fp);
            if (ret <= 0)
            {
                return -1;
            }
            bufferOffset = 0;
        }
        if ((UPPER(tmpBuffer[bufferOffset]) == 'T'))
        {
            offset = (int) (bufferOffset + 1);
            data = (int) UPPER(tmpBuffer[bufferOffset]);
            data = (data << 8) + UPPER(tmpBuffer[offset]);
            offset++;
            data = (data << 8) + UPPER(tmpBuffer[offset]);
            offset++;
            data = (data << 8) + UPPER(tmpBuffer[offset]);
            ptmpItem = NULL;
            switch (data)
            {
                case 0x54495432: // TIT2
                {
                    if (pid3Info->tag.title.length == 0)
                    {
                        ptmpItem = &pid3Info->tag.title;
                    }
                    break;
                }
    
                case 0x54504531: // TPE1
                {
                    if (pid3Info->tag.author.length == 0)
                    {
                        ptmpItem = &pid3Info->tag.author;
                    }
                    break;
                }
    
                case 0x54414c42: // TALB
                {
                    if (pid3Info->tag.album.length == 0)
                    {
                        ptmpItem = &pid3Info->tag.album;
                    }
                    break;
                }
    
                case 0x54594552: // TYER
                {
                    if (pid3Info->tag.year.length == 0)
                    {
                        ptmpItem = &pid3Info->tag.year;
                    }
                    break;
                }
    
                case 0x54434f4e: // TCON
                {
                    if (pid3Info->tag.genre.length == 0)
                    {
                        ptmpItem = &pid3Info->tag.genre;
                    }
                    break;
                }
    
#if __TRACK_NUM__ > 0
                case 0x5452434b: // TRCK

                {
                    if(pid3Info->tag.track.length == 0)
                    {
                        ptmpItem = &pid3Info->tag.track;
                    }
                    break;
                }
#endif
                default:
                {
                    ptmpItem = NULL;
                    break;
                }
            }

            //不在parse_id3V2_frame中计算关键字跳转，避免大于512被裁断，跳转失效 //
            if (ptmpItem != NULL)
            {
                bufoff_tmp = bufferOffset + 4;
                fileoff_tmp = fileOffset + 4;
                parse_id3V2_frame(fp, tmpBuffer, ID3_BUFFER_SIZE, (int*)&fileoff_tmp, (int*)&bufoff_tmp, ptmpItem, version);
                number++;
                //continue;
            }
        }
        //else
        {
            keysize = ((unsigned char) tmpBuffer[bufferOffset + 4] * 0x1000000)
                    + ((unsigned char) tmpBuffer[bufferOffset + 5] * 0x10000) + ((unsigned char) tmpBuffer[bufferOffset
                    + 6] * 0x100) + (unsigned char) tmpBuffer[bufferOffset + 7];
            if (strncmp(tmpBuffer + bufferOffset, "APIC", 4) == 0)
            {
                if ((picAddr == 0) && (keysize > 128))
                {
                    picAddr = (int) (fileOffset + 10);
                    picSize = keysize;
                    number++;
                }
            }
            if (keysize == 0)
            {
                j++;
            }
            else
            {
                j = 0;
            }

            bufferOffset += 10 + (unsigned int) keysize;
            fileOffset += 10 + (unsigned int) keysize;
        }

    }
    if (picSize > 0)
    {
        get_mp3_image(fp, picAddr, picSize, &(pid3Info->tag.imageInfo), version);
    }
    return number;
}

#if 0
void get_mp3_extra_info(void *fp,id3_info_total* info)
{
    plDEBUG("calling");
    char vor_flag[4] =
    {   0x58,0x69,0x6E,0x67};
    char vori_flag[4] =
    {   0x56,0x42,0x52,0x49};
    char Head[10];
    int i;
    int id3v2_len=0;
    int frame_start_addr = 0;
    id3_ext_info* extra_tag = &info->extra_info;
    char *buffer=(char *)malloc(2*ID3_SECTORSIZE);
    ID3_fseek(fp,0,SEEK_SET);
    ID3_fread(Head,sizeof(char),10,fp);
    if( Head[0] == 'I' && Head[1] == 'D' && Head[2] == '3' )
    {
        id3v2_len =((int)(Head[6]&0x7F))*0x200000+((int)(Head[7]&0x7F))*0x4000+(int)((Head[8]&0x7F))*0x80+(int)(Head[9]&0x7F);
        frame_start_addr = id3v2_len;
    }
    ID3_fseek(fp,frame_start_addr,SEEK_SET);
    S_FS_FRead(buffer,sizeof(char),1024,fp);
    for( i = 0; i < 1024; i++)
    {
        if( (buffer[i] == vor_flag[0]) && (buffer[i+1] == vor_flag[1]) && (buffer[i+2] == vor_flag[2]) && (buffer[i+3] == vor_flag[3]) )
        {
            plDEBUG("this is a vbr file!\n");
            int addr = i+4;
            get_vor_extra_info(buffer,addr,extra_tag);
            break;
        }
        else if( (buffer[i] == vori_flag[0]) && (buffer[i+1] == vori_flag[1]) && (buffer[i+2] == vori_flag[2]) && (buffer[i+3] == vori_flag[3]) )
        {
            plDEBUG("this is a vbri file!\n");
            int addr = i+4;
            get_vori_extra_info(buffer,addr,extra_tag);
            break;
        }
        else if( i == 1021)
        {
            plDEBUG("this file is a noamal mp3 file!\n");
            int addr = 0;
            get_normal_extra_info(buffer,addr,info);
            if(extra_tag->bitrate != 0)
            {
                int file_len;
                int data_len;

                ID3_fseek(fp, 0, SEEK_END);
                file_len=ftell(fp);
                data_len = file_len - id3v2_len;
                //printf("%s__%s__%d:data_len=%d\n",__FILE__,__FUNCTION__,__LINE__,data_len);
                extra_tag->total_time = data_len*8/(extra_tag->bitrate);
                //printf("%s__%s__%d:extra_tag->total_time=%d\n",__FILE__,__FUNCTION__,__LINE__,extra_tag->total_time);
            }
            break;
        }
    }
    //    fsel_free(buffer);
    //    free(buffer1);
    free(buffer);
}

void get_vor_extra_info(char* buffer,int addr,id3_ext_info* extra_tag)
{
    plDEBUG("calling");
    int i;
    int sample_rate_flag;
    char total_frame[4];
    int total_frame_num;
    extra_tag->sample_rate = 0;
    extra_tag->bitrate = 0;
    extra_tag->channel = 0;
    extra_tag->total_time = 0;
    for ( i = 0; i < 1024; i++)
    {
        if( ((buffer[i]&0xff) == 0xff) && (((buffer[i+1]>>5)&0x07)) == 0x07 )
        {
            if( (((buffer[i+3]>>6)&0x03) == 0x00) ||(((buffer[i+3]>>6)&0x03) == 0x01) || ( ((buffer[i+3]>>6)&0x03) == 0x02))
            extra_tag->channel = 2;
            else if( ((buffer[i+3]>>6)&0x03) == 0x03)
            extra_tag->channel = 1;

            if( (((buffer[i+1]<<3)>>6)&0x03) == 0x03 )
            {
                sample_rate_flag = ((buffer[i+2]<<4)>>6)&0x03;
                if ( sample_rate_flag == 0x00 )
                {
                    extra_tag->sample_rate = 44100;
                }
                else if ( sample_rate_flag == 0x01 )
                {
                    extra_tag->sample_rate = 48000;
                }
                else if ( sample_rate_flag == 0x02 )
                {
                    extra_tag->sample_rate = 32000;
                }
                break;

            }
            else if( (((buffer[i+1]<<3)>>6)&0x03) == 0x02 )
            {
                sample_rate_flag = ((buffer[i+2]<<4)>>6)&0x03;
                if ( sample_rate_flag == 0x00 )
                {
                    extra_tag->sample_rate = 22050;
                }
                else if ( sample_rate_flag == 0x01 )
                {
                    extra_tag->sample_rate = 24000;
                }
                else if ( sample_rate_flag == 0x02 )
                {
                    extra_tag->sample_rate = 16000;
                }
                break;
            }
        }
    }
    total_frame[0] = buffer[addr+4];
    total_frame[1] = buffer[addr+5];
    total_frame[2] = buffer[addr+6];
    total_frame[3] = buffer[addr+7];
    total_frame_num = (unsigned char)(total_frame[0])*0x1000000 + (unsigned char)total_frame[1]*0x10000 + (unsigned char)total_frame[2]*0x100 + (unsigned char)total_frame[3];
    if ( extra_tag->sample_rate >= 32000)
    extra_tag->total_time = total_frame_num * 1152 /extra_tag->sample_rate;
    else if(extra_tag->sample_rate> 0)
    extra_tag->total_time = total_frame_num * 576 /extra_tag->sample_rate;
}

void get_vori_extra_info(char* buffer,int addr,id3_ext_info* extra_tag)
{
    int i;
    int sample_rate_flag;
    char total_frame[4];
    int total_frame_num;
    extra_tag->sample_rate = 0;
    extra_tag->bitrate = 0;
    extra_tag->channel = 0;
    extra_tag->total_time = 0;
    for ( i = 0; i < 1024; i++)
    {
        if( ((buffer[i]&0xff) == 0xff) && (((buffer[i+1]>>5)&0x07)) == 0x07 )
        {
            if( (((buffer[i+3]>>6)&0x03) == 0x00) ||(((buffer[i+3]>>6)&0x03) == 0x01) || ( ((buffer[i+3]>>6)&0x03) == 0x02))
            extra_tag->channel = 2;
            else if( ((buffer[i+3]>>6)&0x03) == 0x03)
            extra_tag->channel = 1;

            if( (((buffer[i+1]<<3)>>6)&0x03) == 0x03 )
            {
                sample_rate_flag = ((buffer[i+2]<<4)>>6)&0x03;
                if ( sample_rate_flag == 0x00 )
                {
                    extra_tag->sample_rate = 44100;
                }
                else if ( sample_rate_flag == 0x01 )
                {
                    extra_tag->sample_rate = 48000;
                }
                else if ( sample_rate_flag == 0x02 )
                {
                    extra_tag->sample_rate = 32000;
                }
                break;

            }
            else if( (((buffer[i+1]<<3)>>6)&0x03) == 0x02 )
            {
                sample_rate_flag = ((buffer[i+2]<<4)>>6)&0x03;
                if ( sample_rate_flag == 0x00 )
                {
                    extra_tag->sample_rate = 22050;
                }
                else if ( sample_rate_flag == 0x01 )
                {
                    extra_tag->sample_rate = 24000;
                }
                else if ( sample_rate_flag == 0x02 )
                {
                    extra_tag->sample_rate = 16000;
                }
                break;
            }
        }
    }
    total_frame[0] = buffer[addr+10];
    total_frame[1] = buffer[addr+11];
    total_frame[2] = buffer[addr+12];
    total_frame[3] = buffer[addr+13];
    total_frame_num = ((unsigned char)total_frame[0])*0x1000000 + ((unsigned char)total_frame[1])*0x10000 + ((unsigned char)total_frame[2])*0x100 + ((unsigned char)total_frame[3]);
    if ( extra_tag->sample_rate >= 32000)
    extra_tag->total_time = total_frame_num * 1152 /extra_tag->sample_rate;
    else if(extra_tag->sample_rate> 0)
    extra_tag->total_time = total_frame_num * 576 /extra_tag->sample_rate;

}

void get_normal_extra_info(char* buffer,int addr,id3_info_total* info)
{
    int i;
    int sample_rate_flag;
    int bitrate_flag;
    id3_ext_info* extra_tag = &info->extra_info;
    extra_tag->sample_rate = 0;
    extra_tag->bitrate = 0;
    extra_tag->channel = 0;
    extra_tag->total_time = 0;
    for ( i = 0; i < 1024; i++)
    {
        if( ((buffer[i]&0xff) == 0xff) && (((buffer[i+1]>>5)&0x07)) == 0x07 )
        {
            if( (((buffer[i+3]>>6)&0x03) == 0x00) ||(((buffer[i+3]>>6)&0x03) == 0x01) || ( ((buffer[i+3]>>6)&0x03) == 0x02))
            extra_tag->channel = 2;
            else if( ((buffer[i+3]>>6)&0x03) == 0x03)
            extra_tag->channel = 1;

            if( (((buffer[i+1]<<3)>>6)&0x03) == 0x03 )
            {
                sample_rate_flag = ((buffer[i+2]<<4)>>6)&0x03;
                if ( sample_rate_flag == 0x00 )
                {
                    extra_tag->sample_rate = 44100;
                }
                else if ( sample_rate_flag == 0x01 )
                {
                    extra_tag->sample_rate = 48000;
                }
                else if ( sample_rate_flag == 0x02 )
                {
                    extra_tag->sample_rate = 32000;
                }
                bitrate_flag = (buffer[i+2]>>4)&0x0f;
                if(bitrate_flag == 0x01)
                extra_tag->bitrate = 32000;
                else if(bitrate_flag == 0x02)
                extra_tag->bitrate = 40000;
                else if(bitrate_flag == 0x03)
                extra_tag->bitrate = 48000;
                else if(bitrate_flag == 0x04)
                extra_tag->bitrate = 56000;
                else if(bitrate_flag == 0x05)
                extra_tag->bitrate = 64000;
                else if(bitrate_flag == 0x06)
                extra_tag->bitrate = 80000;
                else if(bitrate_flag == 0x07)
                extra_tag->bitrate = 96000;
                else if(bitrate_flag == 0x08)
                extra_tag->bitrate = 112000;
                else if(bitrate_flag == 0x09)
                extra_tag->bitrate = 128000;
                else if(bitrate_flag == 0x0a)
                extra_tag->bitrate = 160000;
                else if(bitrate_flag == 0x0b)
                extra_tag->bitrate = 192000;
                else if(bitrate_flag == 0x0c)
                extra_tag->bitrate = 224000;
                else if(bitrate_flag == 0x0d)
                extra_tag->bitrate = 256000;
                else if(bitrate_flag == 0x0e)
                extra_tag->bitrate = 320000;
                //printf("%s__%s__%d:bitrate=%d\n",__FILE__,__FUNCTION__,__LINE__,extra_tag->bitrate);
                break;
            }
            else if( (((buffer[i+1]<<3)>>6)&0x03) == 0x02 )
            {
                sample_rate_flag = ((buffer[i+2]<<4)>>6)&0x03;
                if ( sample_rate_flag == 0x00 )
                {
                    extra_tag->sample_rate = 22050;
                }
                else if ( sample_rate_flag == 0x01 )
                {
                    extra_tag->sample_rate = 24000;
                }
                else if ( sample_rate_flag == 0x02 )
                {
                    extra_tag->sample_rate = 16000;
                }
                //printf("%s__%s__%d:sample_rate=%d\n",__FILE__,__FUNCTION__,__LINE__,extra_tag->sample_rate);
                bitrate_flag = (buffer[i+2]>>4)&0x0f;
                if(bitrate_flag == 0x01)
                extra_tag->bitrate = 32000;
                else if(bitrate_flag == 0x02)
                extra_tag->bitrate = 40000;
                else if(bitrate_flag == 0x03)
                extra_tag->bitrate = 48000;
                else if(bitrate_flag == 0x04)
                extra_tag->bitrate = 56000;
                else if(bitrate_flag == 0x05)
                extra_tag->bitrate = 64000;
                else if(bitrate_flag == 0x06)
                extra_tag->bitrate = 80000;
                else if(bitrate_flag == 0x07)
                extra_tag->bitrate = 96000;
                else if(bitrate_flag == 0x08)
                extra_tag->bitrate = 112000;
                else if(bitrate_flag == 0x09)
                extra_tag->bitrate = 128000;
                else if(bitrate_flag == 0x0a)
                extra_tag->bitrate = 160000;
                else if(bitrate_flag == 0x0b)
                extra_tag->bitrate = 192000;
                else if(bitrate_flag == 0x0c)
                extra_tag->bitrate = 224000;
                else if(bitrate_flag == 0x0d)
                extra_tag->bitrate = 256000;
                else if(bitrate_flag == 0x0e)
                extra_tag->bitrate = 320000;
                //printf("%s__%s__%d:bitrate=%d\n",__FILE__,__FUNCTION__,__LINE__,extra_tag->bitrate);
                break;
            }
        }
    }

}
#endif

/******************************************************************************/
/*!
 * \par  Description:
 *	  解析mp3格式文件id3信息
 * \param[in]   pfile : mp3文件文件句柄
 * \param[out]  pid3Info: mp3文件的id3信息
 * \return     void
 *******************************************************************************/
void get_mp3_audio_info(void *pfile, id3_info_total* pid3Info)
{
    int err;
    char i;
    char j = 0;
    unsigned short * pstring = NULL;

    memset(pid3Info, 0, sizeof(id3_info_total));
    err = parse_id3V2_3(pfile, pid3Info);
    if (err <= 0)
    {

        parse_id3V1(pfile, pid3Info);
        return;
    }

    if ((pid3Info->tag.genre.content == NULL) || (pid3Info->tag.genre.length <= 0))
    {
        return;
    }
    /*防止unicode编码时，数字未转化成对应流派*/
    if (pid3Info->tag.genre.encoding == ENCODING_UNICODE)
    {
        pstring = (unsigned short *) pid3Info->tag.genre.content;
        if (pstring[0] != '(')
        {
            return;
        }

        i = 1;
        while ((pstring[i] >= '0') && (pstring[i] <= '9'))
        {
            j = j * 10;
            j += (char)(pstring[i] - '0');
            i++;
        }

        if (pid3Info->tag.genre.content[i] != ')')
        {
            return;
        }
        freeItem(&pid3Info->tag.genre);
        getgenre(j, &pid3Info->tag.genre);
        return;

    }

    if (pid3Info->tag.genre.content[0] != '(')
    {
        return;
    }

    i = 1;
    while ((pid3Info->tag.genre.content[i] >= '0') && (pid3Info->tag.genre.content[i] <= '9'))
    {
        j = j * 10;
        j += pid3Info->tag.genre.content[i] - '0';
        i++;
    }
    if (pid3Info->tag.genre.content[i] != ')')
    {
        return;
    }
    freeItem(&pid3Info->tag.genre);
    getgenre(j, &pid3Info->tag.genre);
}
}

