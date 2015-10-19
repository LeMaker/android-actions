#include "id3parse.h"
namespace android {
int check_ape_item(char *buffer, int point, int version, int item_len)
{
    int check;

    if (buffer[point + item_len] == 0)
    {

        if (version == 1000)
        {
            memcpy(&check, buffer + (point - 4), 4);
            //printf("%s %d __1.0_check:%d_\n", __FILE__, __LINE__, check);
            if (check == 0)
            {
                return 0;
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return -1;
    }
}


void parse_Ape(void *fp, char *buffer, id3_info_t* Tag)
{
    char *buffer1;
    int i, j;
    unsigned int tag_len, value_len;
    int number;
    id3_item_info_t *ptmpItem;
    int version;
    int check;
    int offset, data;

    //ape version   1000 = Version 1.000 (old);  2000 = Version 2.000 (new)
    memcpy(&version, buffer + 104, 4);

    memcpy(&tag_len, buffer + 108, 4);
    ID3_fseek(fp, -((int)tag_len), SEEK_END);
    tag_len = tag_len - 32;

    buffer1 = (char *) malloc((unsigned int) tag_len);
    if (NULL == buffer1)
    {
        return;
    }
    ID3_fread(buffer1, sizeof(char), (unsigned int) tag_len, fp);

    i = 0;
    number = 0;
    while (((unsigned int)i < tag_len) && (number < 5))
    {
        j = 0;
        ptmpItem = NULL;
        //不获取comment字段，应用中一直未用
        if ((UPPER(buffer1[i]) == 'A') || (UPPER(buffer1[i]) == 'T') || (UPPER(buffer1[i]) == 'G') 
            || (UPPER(buffer1[i]) == 'Y'))
        {
            if ((strncmp(buffer1 + i, "AlbumArtist", 11) == 0) || (strncmp(buffer1 + i, "ArtistAlbum", 11) == 0))
            {
                i = i + 11;
                continue;
            }

            offset = i + 1;
            data = (int)UPPER(buffer1[i]);
            data = (data << 8) + UPPER(buffer1[offset]);
            offset++;
            data = (data << 8) + UPPER(buffer1[offset]);
            offset++;
            data = (data << 8) + UPPER(buffer1[offset]);
            //printf("APE data:0x%x\n", data);
            switch (data)
            {
            case 0x41525449://ARTIST
                if ((UPPER(buffer1[offset + 1]) != 'S') || (UPPER(buffer1[offset + 2]) != 'T'))
                {
                    break;
                }
                if ((check_ape_item(buffer1, i, version, 6) == 0) &&(Tag->author.length == 0))
                {
                    ptmpItem = &Tag->author;
                    j = 6;
                }
                break;
            case 0x414C4255://ALBUM
                if ((UPPER(buffer1[offset + 1]) != 'M'))
                {
                    break;
                }
                if ((check_ape_item(buffer1, i, version, 5) == 0)&&(Tag->album.length == 0))
                {
                    ptmpItem = &Tag->album;
                    j = 5;
                }
                break;
            case 0x5449544C://TITLE
                if ((UPPER(buffer1[offset + 1]) != 'E'))
                {
                    break;
                }
                if ((check_ape_item(buffer1, i, version, 5) == 0) && (0 == Tag->title.length))
                {
                    ptmpItem = &Tag->title;
                    j = 5;
                }
                break;
            case 0x47454E52://GENRE
                if ((UPPER(buffer1[offset + 1]) != 'E'))
                {
                    break;
                }
                if ((check_ape_item(buffer1, i, version, 5) == 0) && (0 == Tag->genre.length))
                {
                    ptmpItem = &Tag->genre;
                    j = 5;
                }
                break;
            case 0x59454152: //YEAR
                if ((check_ape_item(buffer1, i, version, 4) == 0) &&(0 == Tag->year.length))
                {
                    ptmpItem = &Tag->year;
                    j = 4;
                }
                break;
            default:
                ptmpItem = NULL;
                break;
            }

            if (ptmpItem != NULL)
            {
                memcpy(&value_len, (buffer1 + i) - 8, 4);
                //确保获取的id3长度小于整个tag记录的长度
                if (value_len < tag_len)
                {
                    ptmpItem->length = (int)value_len;
                    ptmpItem->content = (char *) malloc((unsigned int) (value_len + 1));
                    if (ptmpItem->content == NULL)
                    {
                        ptmpItem->length = 0;
                        free(buffer1);
                        return;
                    }
                    ptmpItem->encoding = ENCODING_UTF8;
                    if (ptmpItem->content != NULL)
                    {
                        memcpy(ptmpItem->content, buffer1 + i + j + 1, (unsigned int)value_len);
                        ptmpItem->content[value_len] = '\0';
                        i = i + (int)value_len + j;
                        number++;
                    }
                }
            }
        }

        i++;
    }
    free(buffer1);
}

//***********************************************************************
//description:鑾峰彇鏂囦欢鐨則itle,artist,album,genre,year,comment鍐呭,鍚勫睘鎬у唴瀹圭敤0x09闅斿紑
//argument : fileinfo--鏂囦欢鍚嶆垨缁濆璺緞
//return    : title,artist,album,genre,year,comment鍐呭
//notes    : 璋冪敤get_ape_item()
//***********************************************************************

void get_apetag(void *fp, id3_info_total* info)
{

    char buffer[128];
    ID3_fseek(fp, -128, SEEK_END);
    ID3_fread(buffer, sizeof(char), 128, fp);

    //printf("enter get_apetag:name \n");
    if ((buffer[0] == 'T') && (buffer[1] == 'A') && (buffer[2] == 'G'))
    {
        parse_id3V1(fp, info);
    }
    else if (memcmp(buffer + 96, "APETAGEX", 8) == 0)
    {
        parse_Ape(fp, buffer, &info->tag);
    }
    else
    {
        ID3_fseek(fp, 0, SEEK_SET);
        ID3_fread(buffer, sizeof(char), 4, fp);
        if ((buffer[0] == 'I') && (buffer[1] == 'D') && (buffer[2] == '3')) {
            parse_id3V2_3(fp, info);
        } else {
        	memset(&info->tag, 0, sizeof(id3_info_t));
        }
    }

    //printf("left get_apetag:name \n");
    /*
     Item_UTF8_To_UTF16(&Tag->title);
     Item_UTF8_To_UTF16(&Tag->author);
     Item_UTF8_To_UTF16(&Tag->album);
     Item_UTF8_To_UTF16(&Tag->genre);
     Item_UTF8_To_UTF16(&Tag->year);
     */
    //Item_UTF8_To_UTF16(&Tag->comment);
}
#if 0
void get_ape_extra_info(void *fp, id3_ext_info* extra_tag)
{
    char head[4];
    char version[2];
    char buffer[128];
    int ver;
    int total_frames = 0;
    int final_frame_block = 0;
    int total_samples = 0;
    int blocks_per_frame = 0;
    unsigned int file_len = 0;
    int descrip_len = 0;
    extra_tag->bitrate = 0;
    extra_tag->sample_rate = 0;
    extra_tag->total_time = 0;
    extra_tag->channel = 0;
    ID3_fseek(fp, 0, SEEK_SET);
    ID3_fread(buffer, sizeof(char), 128, fp);
    if ((buffer[0] == 'M') && (buffer[1] == 'A') && (buffer[2] == 'C') && (buffer[3] == ' '))
    {
        ver = ((unsigned char) buffer[4]) + (((unsigned char) buffer[5]) * 0x100); //version
        if (ver >= 3980)
        {
            //             printf("\nthis a new version for ape file");
            memcpy(&descrip_len, buffer + 8, 4);
            if (descrip_len > 50)
            {
                memcpy(&blocks_per_frame, buffer + 56, 4);
                memcpy(&final_frame_block, buffer + 60, 4);
                memcpy(&total_frames, buffer + 64, 4);
                memcpy(&extra_tag->sample_rate, buffer + 72, 4);
                total_samples = (total_frames * blocks_per_frame) + final_frame_block;
                extra_tag->total_time = (unsigned int) ((unsigned int) total_samples / extra_tag->sample_rate);
                ID3_fseek(fp, 0, SEEK_END);
                file_len = (unsigned int) ftell(fp);
                extra_tag->bitrate = file_len / extra_tag->total_time;

                extra_tag->channel = (unsigned int) (((unsigned char) buffer[70]) + (((unsigned char) buffer[71])
                        * 0x100));

            }

        }
        else if (ver >= 3950)
        {
            //             printf("\nthis a old version fou ape file");
            memcpy(&extra_tag->sample_rate, buffer + 12, 4);
            memcpy(&total_frames, buffer + 24, 4);
            memcpy(&final_frame_block, buffer + 28, 4);
            total_samples = (total_frames * 0x48000) + final_frame_block;
            extra_tag->total_time = (unsigned int) ((unsigned int) total_samples / extra_tag->sample_rate);
            ID3_fseek(fp, 0, SEEK_END);
            file_len = (unsigned int) ftell(fp);
            extra_tag->bitrate = file_len / extra_tag->total_time;

            extra_tag->channel = (unsigned int) ((unsigned char) buffer[10] + (((unsigned char) buffer[11]) * 0x100));

        }
        else
        {
            printf("\nthis version of ape file have not supported with us!");
        }
    }
}
#endif

void get_ape_audio_info(void *fp, id3_info_total* info)
{
    memset(info, 0, sizeof(id3_info_total));
    get_apetag(fp, info);
    //get_ape_extra_info(fp,&info->extra_info);
}

}
    