#include "id3parse.h"

namespace android {
int find_wma_guid(void *fp, char guid[16])
{
    char head_GUID[16] =
    { 
        0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 
        0xCF, 0x11, 0xA6, 0xD9, 0x00, 
        0xAA, 0x00, 0x62, 0xCE, 0x6C 
    }; //ASF_Header_Object 锟铰??UID //
    char header_object[16];
    char tmp[20];
    long object_len;
    long point;
    long object_num;
    int i;
    int content_address = 0;
    int filelength;

    if (!fp)
    {
        return 0;
    }

    filelength = ID3_getfilelength(fp);

    ID3_fseek(fp, 0, SEEK_SET);
    ID3_fread(header_object, sizeof(char), 16, fp);

    if (memcmp(header_object, head_GUID, 16) != 0)
    {
        printf("It's not a wma file!");
        return 0;
    }
    else
    {
        ID3_fseek(fp, 24, SEEK_SET);
        ID3_fread(&object_num, sizeof(char), 4, fp);

        if (object_num != 0)
        {
            point = 30;
            for (i = 0; i < object_num; i++)
            {
                ID3_fseek(fp, point, SEEK_SET);
                ID3_fread(tmp, sizeof(char), 20, fp);

                if (memcmp(tmp, guid, 16) == 0)
                {
                    content_address = point;
                    break;
                }
                else
                {
                    memcpy(&object_len, tmp + 16, 4);
                    point += object_len;
                    if (point >= filelength)
                    {
                        return 0;
                    }
                }

            }
        }
    }
    return content_address;
}
void get_wmatag(void *fp, id3_info_t* Tag)
{
    char content_GUID[16] =
    { 0x33, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C };
    int copyr_len;
    int descrip_offset;
    int content_address = 0;
    char tmp[10];
    char* unicode;
    int length;
    int filelength;

    content_address = find_wma_guid(fp, content_GUID);
    filelength = ID3_getfilelength(fp);
    if (content_address > filelength)
    {
        return;
    }

    if (content_address != 0)
    {
        //plDEBUGf("content_address:%d\n",content_address);
        /*get length*/
        ID3_fseek(fp, content_address + 24, SEEK_SET);
        ID3_fread(tmp, sizeof(char), 10, fp);
        Tag->title.length = ((unsigned char) tmp[0]) + (((unsigned char) tmp[1]) * 0x100);
        length = Tag->title.length;

        Tag->author.length = (unsigned char) tmp[2] + (((unsigned char) tmp[3]) * 0x100);
        if (length < Tag->author.length)
        {
            length = Tag->author.length;
        }

        copyr_len = (unsigned char) (tmp[4]) +((unsigned char) tmp[5] * 0x100);
        Tag->comment.length = (unsigned char) (tmp[6]) +((unsigned char) tmp[7] * 0x100);
        if (length < Tag->comment.length)
        {
            length = Tag->comment.length;
        }
        //cz_20121112 ID3信息太大引起死机
        if(length>ID3_SECTORSIZE)
        {
           Tag->title.length=0;
           Tag->comment.length=0;
           Tag->author.length=0;
           ALOGE("wma id3 too larger");
           return;            
        }
        unicode = (char*) malloc((unsigned int) (length + 2));
        if (NULL == unicode)
        {
            return;
        }

//        /*get comment*/
//        memset(unicode, 0, (unsigned int) (length + 2));
//        descrip_offset = content_address + 34 + Tag->title.length + Tag->author.length + copyr_len;
//        ID3_fseek(fp, descrip_offset, SEEK_SET);
//        ID3_fread(unicode, sizeof(char), (unsigned int) (Tag->comment.length), fp);
//            
//        Tag->comment.encoding = ENCODING_UNICODE;
//        Tag->comment.length = wstrlen(unicode, Tag->comment.length + 2);
//#if 0
//        Unicode2Char(unicode, Tag->comment.length + 2);
//        Tag->comment.length = (int)strlen(unicode);
//#endif
//        if (Tag->comment.length != 0)
//        {
//            Tag->comment.content = (char*) malloc((unsigned int) (Tag->comment.length + 2));
//            if (NULL == Tag->comment.content)
//            {
//                free(unicode);
//                return;
//            }
//            memset(Tag->comment.content, 0, (unsigned int) (Tag->comment.length + 2));
//            memcpy(Tag->comment.content, unicode, (unsigned int) Tag->comment.length);
//        }

        /*get author*/
        memset(unicode, 0, (unsigned int) (length + 2));
        ID3_fseek(fp, content_address + 34 + Tag->title.length, SEEK_SET);
        ID3_fread(unicode, sizeof(char), (unsigned int) Tag->author.length, fp);
        Tag->author.encoding = ENCODING_UNICODE;
        Tag->author.length = wstrlen((char *)unicode, Tag->author.length+2);
#if 0
        Unicode2Char(unicode, Tag->author.length + 2);
        Tag->author.length = (int)strlen(unicode);
#endif
        if (Tag->author.length != 0)
        {
            Tag->author.content = (char*) malloc((unsigned int) (Tag->author.length + 2));
            if (NULL == Tag->author.content)
            {
                free(unicode);
                return;
            }
            memset(Tag->author.content, 0, (unsigned int) (Tag->author.length + 2));
            memcpy(Tag->author.content, unicode, (unsigned int) Tag->author.length);
        }

        /*get title*/
        memset(unicode, 0, (unsigned int) (length + 2));
        ID3_fseek(fp, content_address + 34, SEEK_SET);
        ID3_fread(unicode, sizeof(char), (unsigned int) Tag->title.length, fp);
        Tag->title.encoding = ENCODING_UNICODE;
        Tag->title.length = wstrlen((char *)unicode, Tag->title.length+2);
#if 0
        Unicode2Char(unicode, Tag->title.length + 2);
        Tag->title.length = (int) strlen(unicode);
#endif
        if (Tag->title.length != 0)
        {
            Tag->title.content = (char*) malloc((unsigned int) (Tag->title.length + 2));
            if (NULL == Tag->title.content)
            {
                free(unicode);
                return;
            }
            memset(Tag->title.content, 0, (unsigned int) (Tag->title.length + 2));
            memcpy(Tag->title.content, unicode, (unsigned int) Tag->title.length);
        }
        free(unicode);
    }
}

void get_wma_image(id3_image_t *pImage, int imageArrayAddr, void *fp)
{
    char *tmp;
    short dataType = 0;
    unsigned short dataLen = 0;
    char picType;
    short MIMEType[128];
    int MIMETypeLen;
    char *pimageType;
    int offset = 0;

    if ((imageArrayAddr <= 0) || (fp == 0))
    {
        return;
    }

    tmp = (char *) malloc(ID3_SECTORSIZE);
    if (NULL == tmp)
    {
        return;
    }
    ID3_fseek(fp, imageArrayAddr, SEEK_SET);
    ID3_fread(tmp, sizeof(char), ID3_SECTORSIZE, fp);
    /*ASF_Extended_Content_Description_Object  Content Descriptor:datatype(16bits):1--byteArray*/
    memcpy(&dataType, tmp, 2);
    offset += 2;
    /*ASF_Extended_Content_Description_Object  Content Descriptor(16bits):data length*/
    memcpy(&dataLen, tmp + 2, 2);
    offset += 2;

    /*image type*/
    picType = tmp[4];
    offset++;
    /*image length*/
    memcpy(&pImage->length, tmp + offset, sizeof(int));
    offset += 4;
    /*MIMEType*/
    memcpy(MIMEType, tmp + 9, 40);
    MIMETypeLen = wstrlen((char *)MIMEType, 40);
    offset += MIMETypeLen + 2;
    Unicode2utf8((char *)MIMEType, 40);
    pimageType = strrchr((char *) MIMEType, '/');
    if (pimageType != NULL)
    {
        memcpy(pImage->imageType, pimageType + 1, 8);
    }

    /*image description*/
    memcpy(MIMEType, tmp + offset, 128);
    offset += wstrlen((char *)MIMEType, 128) + 2;
    pImage->offset = imageArrayAddr + offset;

    if ((dataType != 1) || (dataLen < pImage->length) || (offset > ID3_SECTORSIZE))
    {
        memset(pImage, 0, sizeof(id3_image_t));
    }
    free(tmp);
}

void get_wmatag1(void* fp, id3_info_t* Tag)
{
    char externed_content_GUID[16] =
    { 
        0x40, 0xA4, 0xD0, 0xD2, 0x07, 
        0xE3, 0xD2, 0x11, 0x97, 0xf0, 
        0x00, 0xA0, 0xC9, 0x5E, 0xA8, 0x50 
    }; //ASF_Extended_Content_Description_Object
    char id3count;
    unsigned int offset = 0, offset_sec = 0;
    unsigned int object_end = 0;
    int count = 0;
    int length = 0;
    int datatype = 0;
    int i = 0;
    char *tmp = NULL;
    char *title = NULL;
    id3_item_info_t *item = NULL;
    int imageArrayAddr = 0;

    offset = (unsigned int)find_wma_guid(fp, externed_content_GUID);
    if (!offset)
    {
        return;
    }
    tmp = (char *) malloc(ID3_SECTORSIZE);
    if (NULL == tmp)
    {
        return;
    }
    title = (char *) malloc(ID3_SECTORSIZE);
    if (NULL == title)
    {
        free(tmp);
        return;
    }
    offset += 16; //16 is the length of ASF_Extended_Content_Description_Object


    ID3_fseek(fp, (int)offset, SEEK_SET);
    ID3_fread(tmp, sizeof(char), ID3_SECTORSIZE, fp);
    memcpy(&object_end, tmp, 4); //object size
    object_end += offset - 16; //object end address

    memcpy(&count, tmp + 8, 2); //content number
    offset_sec = 10;
    id3count = 0;
#if __TRACK_NUM__ <= 0
    id3count++;
#endif
    while ((count > 0)&&(offset < object_end) && (id3count < 5))
    {
        /*
         WM/AlbumTitle = 0x1E, + namelen 2bytes, + flag 2bytes, + datalen 2bytes = 0x24*/
        if (offset_sec > (ID3_SECTORSIZE - 0x30))
        {
            offset += offset_sec;
            ID3_fseek(fp, (int)offset, SEEK_SET);
            ID3_fread(tmp, sizeof(char), ID3_SECTORSIZE, fp);
            offset_sec = 0;
        }
        
        memcpy(&length, tmp + offset_sec, 2);
        offset_sec += 2;
        item = NULL;
        switch (length)
        {
        case 0x10:
            memcpy(title, tmp + offset_sec, (unsigned int) length);
            //Unicode2Char(title, length);
            Unicode2utf8(title, length);
            if (strcasecmp(title, "WM/Year") == 0)
            {
                if(Tag->year.length == 0)
                {
                    item = &Tag->year;
                } 
            }
            break;

        case 0x12:
            memcpy(title, tmp + offset_sec, (unsigned int) length);
            //Unicode2Char(title, length);
            Unicode2utf8(title, length);
            if (strcasecmp(title, "WM/Genre") == 0)
            {
                if(Tag->genre.length == 0)
                {
                    item = &Tag->genre;
                }
            }
            break;

        case 0x16:
            memcpy(title, tmp + offset_sec, (unsigned int) length);
            //Unicode2Char(title, length);
            Unicode2utf8(title, length);
            if (strcasecmp(title, "WM/Picture") == 0)
            {
                if(imageArrayAddr == 0)
                {
                    imageArrayAddr = (int)(offset + offset_sec) + length;
                    id3count++;
                }
            }
            
            break;

        case 0x1c:
            memcpy(title, tmp + offset_sec, (unsigned int) length);
            //Unicode2Char(title, length);
            Unicode2utf8(title, length);
            if (strcasecmp(title, "WM/AlbumTitle") == 0)
            {
                if(Tag->album.length == 0)
                {
                    item = &Tag->album;                    
                }                 

            }
            break;

#if __TRACK_NUM__ > 0
            case 0x1e:
            memcpy(title, tmp + offset_sec, (unsigned int)length);
            //Unicode2Char(title, length);
            Unicode2utf8(title, length);
            if (strcasecmp(title, "WM/TrackNumber") == 0)
            {
                if(Tag->track.length == 0)
                {
                    item = &Tag->track;
                }
            }
            break;
#endif

        default:
            item = NULL;
            break;
        }
        count--;
        offset_sec += (unsigned int)length;

        if(offset_sec >= (ID3_SECTORSIZE -4))
        {
            //防止offset_sec超出ID3_SECTORSIZE，memcpy死机
            continue;
        }

        memcpy(&datatype, tmp + offset_sec, 2);
        offset_sec += 2;
        memcpy(&length, tmp + offset_sec, 2);
        offset_sec += 2;

        if ((item != NULL) && (length > 2))
        {
            id3count++;
            i = length;
            if (offset_sec > (unsigned int)(ID3_SECTORSIZE - length))
            {
                offset += offset_sec;
                ID3_fseek(fp, (int)offset, SEEK_SET);
                ID3_fread(tmp, sizeof(char), ID3_SECTORSIZE, fp);
                offset_sec = 0;
                i = ID3_SECTORSIZE;
            }
            memset(title, 0, ID3_SECTORSIZE);
            if(i >= ID3_SECTORSIZE)
            {
                i=ID3_SECTORSIZE-2;
            }
            memcpy(title, tmp + offset_sec, (unsigned int) i);

            item->encoding = ENCODING_UNICODE;
            item->length = wstrlen(title, i);

            if (item->length != 0)
            {
                item->content = (char*) malloc((unsigned int)(item->length + 2));
                if (NULL == item->content)
                {
                    break;
                }
                memset(item->content, 0, (unsigned int) (item->length + 2));
                memcpy(item->content, title, (unsigned int) item->length);

            }
        }
#if __TRACK_NUM__ > 0
        if ((item == &Tag->track) && (datatype == 3))
        {
            item_itoa(item);
        }
#endif
        offset_sec += (unsigned int)length;

    }

    free(tmp);
    free(title);
    if (imageArrayAddr > 0)
    {
        get_wma_image(&Tag->imageInfo, imageArrayAddr, fp);
    }

}

void get_wma_extra_info(void *fp, id3_ext_info* extra_tag)
{
    char file_Object_GUID[16] =
    { 
        0xA1, 0xDC, 0xAB, 0x8C, 0x47, 
        0xA9, 0xCF, 0x11, 0x8E, 0xE4, 
        0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65 
    };//ASF_File_Properties_Object
    char stream_Object_GUID[16] =
    { 
        0x91, 0x07, 0xDC, 0xB7, 0xB7, 
        0xA9, 0xCF, 0x11, 0x8E, 0xE6, 
        0x00, 0xC0, 0x0C, 0x20, 0x53, 
        0x65 
    };//ASF_Stream_Properties_Object
    char tmp[10];
    int file_object_addr;
    int stream_object_addr;
    char len1[8], len2[8];
    int i;
    int tmp1, tmp2;
    extra_tag->sample_rate = 0;
    extra_tag->bitrate = 0;
    extra_tag->total_time = 0;
    extra_tag->channel = 0;

    file_object_addr = find_wma_guid(fp, file_Object_GUID);
    ID3_fseek(fp, file_object_addr + 64, SEEK_SET);
    ID3_fread(len1, sizeof(char), 8, fp);

    ID3_fseek(fp, file_object_addr + 80, SEEK_SET);
    ID3_fread(len2, sizeof(char), 8, fp);

    //tmp=len1/(10000*1000)
    memset(tmp, 0, 10);
    tmp2 = 0;
    for (i = 7; i >= 0; i--)
    {
        tmp2 += len1[i];
        tmp1 = tmp2;
        tmp[i] = (char) (tmp1 / 10000000);
        tmp2 = tmp1 % 10000000;
        tmp2 = tmp2 << 8;
    }
    memcpy(len1, tmp, 8);

    //tmp=len2/(1000)
    memset(tmp, 0, 10);
    tmp2 = 0;
    for (i = 7; i >= 0; i--)
    {
        tmp2 += len1[i];
        tmp1 = tmp2;
        tmp[i] = (char) (tmp1 / 1000);
        tmp2 = tmp1 % 1000;
        tmp2 = tmp2 << 8;
    }
    memcpy(&tmp2, tmp, 4);
    memcpy(&tmp1, len1, 4);
    extra_tag->total_time = (unsigned int) (tmp1 - tmp2);
    //plDEBUGf("%d\n",extra_tag->total_time);

    stream_object_addr = find_wma_guid(fp, stream_Object_GUID);
    ID3_fseek(fp, stream_object_addr + 80, SEEK_SET);
    ID3_fread(tmp, sizeof(char), 10, fp);
    extra_tag->channel = (unsigned int) ((unsigned char) (tmp[0]) +((unsigned char) tmp[1] * 0x100));
    memcpy(&(extra_tag->sample_rate), tmp + 2, 4);
    //     printf("%d\n",extra_tag->sample_rate);
    memcpy(&(extra_tag->bitrate), tmp + 6, 4);
    //     printf("%d\n",extra_tag->bitrate);
    return;
}

void get_wma_audio_info(void *fp, id3_info_total* info)
{
    memset(info, 0, sizeof(id3_info_total));

    get_wmatag(fp, &info->tag);
    get_wmatag1(fp, &info->tag);
    //get_wma_extra_info(fp,&info->extra_info);
}
}
