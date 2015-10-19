#include "id3parse.h"

//***************************************************************************
// Description :在buffer中找出oggTabInfo,如果找到返回true及用vorbis_address记录oggTabInfo位置;
//              否则返回false *
// Arguments   :buffer--待查找数组；buffer_len--buffer的长度；                                        *
// Returns     :为真时返回true;否则返回false                                                         *
// Notes       :                                                                                  *
//*****************************************************************************

namespace android {

//int get_total_time(FILE *fd, unsigned int sample_rate);
int find_ogg_Tag(void *fp, char * ogg_TagInfo, int length)
{
    int i;
    int j = 0;
    int k = 0;
    int vorbis_address = 0;
    int len;
    char * tmp = (char *) malloc(ID3_SECTORSIZE);
    if (NULL == tmp)
    {
        return -1;
    }
    for (i = 0; i < 8; i++)
    {
        ID3_fseek(fp, i * ID3_SECTORSIZE, SEEK_SET);
        ID3_fread(tmp, sizeof(char), ID3_SECTORSIZE, fp);
        for (j = 0; j < ID3_SECTORSIZE; j++)
        {
            if (tmp[j] != ogg_TagInfo[k])
            {
                k = 0;
            }
            else
            {
                k++;
                if (k >= length)
                {
                    vorbis_address = (i * ID3_SECTORSIZE) + j + 1; //find ogg_TagInfo,record the vorbis_address;
                    free(tmp);
                    return vorbis_address;
                }
            }
        }
    }
    free(tmp);
    return vorbis_address;
}

void get_ogg_item(void *fd, const char *string, int addr, id3_item_info_t* it_info)
{
    int i;
    int j;
    int k = 0;
    int len;
    char * tmp;
    int tempval;

    if ((addr <= 0) || (string == NULL))
    {
        return;
    }
    len = (int) strlen(string);
    tmp = (char *) malloc(ID3_SECTORSIZE);
    if (tmp == NULL)
    {
        return;
    }
    for (i = 0; i < (8 - (addr / ID3_SECTORSIZE)); i++)
    {
        ID3_fseek(fd, addr + (i * ID3_SECTORSIZE), SEEK_SET);
        ID3_fread(tmp, sizeof(char), ID3_SECTORSIZE, fd);

        for (j = 0; j < ID3_SECTORSIZE; j++)
        {
            if (UPPER(tmp[j]) != string[k])
            {
                k = 0;
            }
            else
            {
                k++;
                if (k >= len)
                {
                    tempval = j - len - 4;
                    if (tempval >= 0)
                    {
                        memcpy(&(it_info->length), tmp + tempval + 1, 4);
                    }
                    else
                    {
                        ID3_fseek(fd, addr + (i * ID3_SECTORSIZE) + (j - len - 4) + 1, SEEK_SET);
                        ID3_fread(&(it_info->length), sizeof(char), 4, fd);
                    }
                    it_info->length -= len;
                    if ((it_info->length <= 0) || (it_info->length > 0xC000))
                    {
                        k = 0;
                        it_info->length = 0;
                        continue;
                    }
                    it_info->content = (char*) malloc((unsigned int) (it_info->length + 1));
                    if (NULL == it_info->content)
                    {
                        free(tmp);
                        return;
                    }

                    if (j < (ID3_SECTORSIZE - it_info->length))
                    {
                        memcpy(it_info->content, tmp + j + 1, (unsigned int) it_info->length);
                    }
                    else
                    {
                        ID3_fseek(fd, addr + (i * ID3_SECTORSIZE) + j + 1, SEEK_SET);
                        ID3_fread(it_info->content, sizeof(char), (unsigned int) it_info->length, fd);
                    }
                    it_info->content[it_info->length] = '\0';
                    it_info->encoding = ENCODING_UTF8;
                    free(tmp);
                    return;
                }
            }
        }
    }
    free(tmp);
    return;
}

//**********************************************************************
// Description :获取fileinfo中ogg_tab[i]各标签的内容，并存于䶿1¤7777符䶿1¤7     *
// Arguments   :fileinfo--待查找文件的文件路径ﺿ1¤7777                         *
// Returns     :类型：QString；内容：懿1¤7777TAB[i]中的标签内容，各个之长1¤7777    *
//               璿1¤7777"隔开                                                     *
// Notes       :循环调用 get_ogg_item()                                           *
//**********************************************************************

void get_oggtag(void *fd, id3_info_t* Tag)
{
    int addr;
    int tmp;
    char ogg_TagInfo[] =
    { 0x03, 0x76, 0x6f, 0x72, 0x62, 0x69, 0x73 };

    addr = find_ogg_Tag(fd, ogg_TagInfo, 7);
    if (addr <= 0)
    {
        return;
    }
    ID3_fseek(fd, addr, SEEK_SET);
    ID3_fread(&tmp, sizeof(char), 4, fd);
    addr += tmp + 8;
    get_ogg_item(fd, "TITLE=", addr, &Tag->title);
    get_ogg_item(fd, "ARTIST=", addr, &Tag->author);
    get_ogg_item(fd, "ALBUM=", addr, &Tag->album);
    get_ogg_item(fd, "GENRE=", addr, &Tag->genre);
    get_ogg_item(fd, "DATE=", addr, &Tag->year);
    get_ogg_item(fd, "COMMENT=", addr, &Tag->comment);
    get_ogg_item(fd, "ANDROID_LOOP=", addr, &Tag->autoLoop);

    /*Item_UTF8_To_UTF16(&Tag->title);
    Item_UTF8_To_UTF16(&Tag->author);
    Item_UTF8_To_UTF16(&Tag->album);
    Item_UTF8_To_UTF16(&Tag->genre);
    Item_UTF8_To_UTF16(&Tag->year);
    Item_UTF8_To_UTF16(&Tag->comment);*/
#if __TRACK_NUM__ > 0
    get_ogg_item(fd, "TRACKNUMBER=", addr, &Tag->track);
   // Item_UTF8_To_UTF16(&Tag->trackNumber);
#endif
}
/*
void get_ogg_extra_info(void *fd, id3_ext_info* extra_tag)
{
    char ogg_TagInfo[] =
    { 0x01, 0x76, 0x6f, 0x72, 0x62, 0x69, 0x73 };
    char buffer[16];
    char size[4];
    int i;
    int j = 0;
    int addr = 0;
    int k = 0;
    addr = find_ogg_Tag(fd, ogg_TagInfo, 7);
    if (addr > 0)
    {
        ID3_fseek(fd, addr + 4, SEEK_SET);
        ID3_fread(buffer, sizeof(char), 16, fd);
        extra_tag->channel = (unsigned int) buffer[0];
        memcpy(&(extra_tag->sample_rate), buffer + 1, 4);
        memcpy(&(extra_tag->bitrate), buffer + 9, 4);
        extra_tag->total_time = (unsigned int) get_total_time(fd, extra_tag->sample_rate);
    }
    else
    {
        extra_tag->sample_rate = 0;
        extra_tag->bitrate = 0;
        extra_tag->channel = 0;
        extra_tag->total_time = 0;
    }

}

int get_total_time(void *fd, unsigned int sample_rate)
{
    char sample_flag[] =
    { 0x4f, 0x67, 0x67, 0x53 }; //"OggS"
    char tmp[ID3_SECTORSIZE];
    int totaltime = 0;
    int i = 0, j = 0, k = 0;
    int addr = 0;
    char len[8];
    char time[4];
    unsigned int tmp1, tmp2;

    memset(time, 0, 4);
    if (sample_rate == 0)
    {
        return 0;
    }

    memset(time, 0, 4);
    memset(len, 0, 8);

    //for (i = 12; i > 0; i--)
    i = 12;
    while (i > 0)
    {
        ID3_fseek(fd, -(i * ID3_SECTORSIZE), SEEK_END);
        ID3_fread(tmp, sizeof(char), ID3_SECTORSIZE, fd);
        for (j = 0; j < ID3_SECTORSIZE; j++)
        {
            if (tmp[j] != sample_flag[k])
            {
                k = 0;
            }
            else
            {
                k++;
                if (k >= 4)
                {
                    if (j < (ID3_SECTORSIZE - 10))
                    {
                        memcpy(len, tmp[j + 3], 8);
                    }
                    else
                    {
                        addr = (i * ID3_SECTORSIZE) - j - 1; //find ogg_TagInfo,record the vorbis_address;
                        addr -= 2;
                        ID3_fseek(fd, -addr, SEEK_END);
                        ID3_fread(len, sizeof(char), 8, fd);
                    }

                    memcpy(&tmp2, len + 3, 4);
                    for (i = 3; i > 0; i--)
                    {
                        tmp1 = tmp2;
                        time[i] = (char) (tmp1 / sample_rate);
                        tmp2 = tmp1 % sample_rate;
                        tmp2 = tmp2 << 8;
                        tmp2 += (unsigned int) len[i - 1] & 0xff;
                    }
                    memcpy(&totaltime, time, 4);
                    return totaltime;

                }
            }
        }
        i--;
    }

    return totaltime;
}
*/
void get_ogg_audio_info(void *fd, id3_info_total* info)
{
    memset(info, 0, sizeof(id3_info_total));
    get_oggtag(fd, &info->tag);
    //get_ogg_extra_info(fd,&info->extra_info);
}
}
