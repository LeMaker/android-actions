#include "flac_id3parse.h"
namespace android {
int get_metadata_block_addr(void *fp, char blocktype)
{
    unsigned int addr = 4;
    unsigned int length;
    int result;
    char endFlag = 0;
    unsigned char tmp[4];
    const unsigned char endBitMask = 0x80;
    const unsigned char typeMask = 0x7F;
    while (endFlag == 0)
    {
        result = ID3_fseek(fp, (long)addr, SEEK_SET);
        if (result < 0)
        {
            endFlag = 1;
            break;
        }
        result = (int)ID3_fread(tmp, sizeof(char), 4, fp);
        if (result != 4)
        {
            endFlag = 1;
        }
        addr += 4;
        if ((tmp[0] & typeMask) == blocktype)
        {
            break;
        }
        endFlag = (char)(tmp[0] & endBitMask);
        length = (unsigned int)( (tmp[1] * 0x10000) + (tmp[2] * 0x100) + tmp[3]);
        addr += length;
    }
    if (endFlag != 0)
    {
        addr = -1;
    }
    return addr;
}


void get_flac_item(void *fd, const char *string, int addr, id3_item_info_t* it_info)
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


void get_flactag(void *fp, id3_info_t* Tag)
{
    int blockAddr;
    blockAddr = get_metadata_block_addr(fp, FLAC_VORBIS_COMMENT);
    if (blockAddr <= 0)
    {
        return;
    }
    get_flac_item(fp, "TITLE=", blockAddr, &Tag->title);
    get_flac_item(fp, "ARTIST=", blockAddr, &Tag->author);
    get_flac_item(fp, "ALBUM=", blockAddr, &Tag->album);
    get_flac_item(fp, "GENRE=", blockAddr, &Tag->genre);
    get_flac_item(fp, "DATE=", blockAddr, &Tag->year);
    get_flac_item(fp, "COMMENT=", blockAddr, &Tag->comment);

}

void get_flac_image(void *fp, id3_image_t *imageInfo)
{
    char mineType[31];
    char *imageType;
    unsigned char *tmpBuffer;
    int offset = 0;
    int blockAddr;
    int length;
    memset(imageInfo, 0, sizeof(id3_image_t));
    memset(mineType, 0, 31);

    blockAddr = get_metadata_block_addr(fp, FLAC_PICTURE);
    if (blockAddr <= 0)
    {
        return;
    }

    if (ID3_fseek(fp, blockAddr, SEEK_SET) != 0)
    {
        return;
    }
    tmpBuffer = (unsigned char *) malloc(ID3_SECTORSIZE);
    if(NULL == tmpBuffer)
    {
        return;
    }
    if (ID3_SECTORSIZE != ID3_fread(tmpBuffer, sizeof(char), ID3_SECTORSIZE, fp))
    {
        free(tmpBuffer);
        return;
    }
    /*MIME type string */
    length = (tmpBuffer[4] * 0x1000000) + (tmpBuffer[5] * 0x10000) + (tmpBuffer[6] * 0x100) + tmpBuffer[7];
    if (length > 30)
    {
        free(tmpBuffer);
        return;
    }
    memcpy(mineType, tmpBuffer + 8, (unsigned int)length);

    /*the description string */
    offset = length + 8;
    length = (tmpBuffer[offset] * 0x1000000) + (tmpBuffer[offset + 1] * 0x10000) + (tmpBuffer[offset + 2] * 0x100)
            + tmpBuffer[offset + 3];
    offset += length + 4;

    /*The width of the picture in pixels :32bits*/
    /*The height of the picture in pixels: 32bits*/
    /*The color depth of the picture in bits-per-pixel:32bits*/
    /*For indexed-color pictures (e.g. GIF), the number of colors used, or 0 for non-indexed pictures:32bits*/
    offset += 16;

    /*The length of the picture data in bytes:32bits*/
    imageInfo->offset = blockAddr + offset + 4;
    if ((offset + 4) > ID3_SECTORSIZE)
    {
        if (ID3_fseek(fp, blockAddr + offset, SEEK_SET) != 0)
        {
            free(tmpBuffer);
            return;
        }
        if (4 != ID3_fread(tmpBuffer, sizeof(char), 4, fp))
        {
            free(tmpBuffer);
            return;
        }
        offset = 0;
    }
    imageInfo->length = (tmpBuffer[offset] * 0x1000000) + (tmpBuffer[offset + 1] * 0x10000) + 
                  (tmpBuffer[offset + 2] * 0x100)+ tmpBuffer[offset + 3];
    mineType[30] = '\0';
    imageType = strrchr(mineType, '/');
    if (imageType != NULL)
    {
        //strcpy(imageInfo->imageType, imageType + 1);
        strncpy(imageInfo->imageType, imageType + 1, 8);
    }

    free(tmpBuffer);
    return;
}

void get_flac_audio_info(void *fd, id3_info_total* info)
{
    memset(info, 0, sizeof(id3_info_total));

    get_flactag(fd, &info->tag);
    //get_ogg_extra_info(fp,&info->extra_info);
    get_flac_image(fd, &info->tag.imageInfo);
}

}