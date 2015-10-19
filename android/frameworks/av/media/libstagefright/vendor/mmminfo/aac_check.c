#include "format_check.h"
    
int aacflag(storage_io_t *input)
{
    int rtval = 0;
    int ftyp = 0;
    int moov = 0;
    int trak = 0;
    int mdia = 0;
    int hdlr = 0;
    int vide = 0;
    int soun = 0;

    unsigned char tmpbuf[4] = {0};
    int boxsize = 0;

    int readlen = 0;
    int boxtype = 0;
    int sizemoov = 0;
    int is_audio = 0;

    ftyp = ('p'<< 24) | ('y'<< 16) | ('t'<< 8) | ('f');
    moov = ('v'<< 24) | ('o'<< 16) | ('o'<< 8) | ('m');
    trak = ('k'<< 24) | ('a'<< 16) | ('r'<< 8) | ('t');
    mdia = ('a'<< 24) | ('i'<< 16) | ('d'<< 8) | ('m');
    hdlr = ('r'<< 24) | ('l'<< 16) | ('d'<< 8) | ('h');
    vide = ('e'<< 24) | ('d'<< 16) | ('i'<< 8) | ('v');
    soun = ('n'<< 24) | ('u'<< 16) | ('o'<< 8) | ('s');

    is_audio = FALSE;

    readlen = input->read(tmpbuf, 1, 4, input);
    if (readlen != 4)
    {
        return FALSE;
    }

    boxsize = (((int)tmpbuf[0] << 24)|((int)tmpbuf[1] << 16))  \
              | (((int)tmpbuf[2] << 8) | (tmpbuf[3]));

    readlen = input->read(&boxtype, 1, 4, input);
    if (readlen != 4)
    {
        return FALSE;
    }

    if (boxtype != ftyp)
    {
        return FALSE;
    }

    boxsize -= 8;

    if (boxsize > 0)
    {
        rtval = input->seek(input, boxsize, SEEK_CUR);
        if (rtval != 0)
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    while(1)
    {
        readlen = input->read(tmpbuf, 1, 4, input);
        if (readlen != 4)
        {
            return FALSE;
        }

        boxsize = ((int)tmpbuf[0] << 24)|((int)tmpbuf[1] << 16)    \
                  | ((int)tmpbuf[2] << 8) | ((int)tmpbuf[3]);

        readlen = input->read(&boxtype, 1, 4, input);
        if (readlen != 4)
        {
            return FALSE;
        }

        if ((boxtype == moov)||(boxtype == trak)||
                (boxtype == mdia))

        {
            if (boxtype == moov)
            {
                sizemoov = boxsize;
            }

            boxsize -= 8;
            sizemoov -= 8;

            continue;
        }
        else if (boxtype == hdlr)
        {
            readlen = input->read(tmpbuf, 1, 4, input);
            if (readlen != 4)
            {
                return FALSE;
            }
            readlen = input->read(&boxtype, 1, 4, input);
            if (readlen != 4)
            {
                return FALSE;
            }

            readlen = input->read(&boxtype, 1, 4, input);
            if (readlen != 4)
            {
                return FALSE;
            }

            if (boxtype == vide)
            {
                rtval = input->seek(input, 0, SEEK_SET);
                if (rtval != 0)
                {
                    return FALSE;
                }
                return FALSE;
            }

            if (boxtype == soun)
            {
                is_audio = TRUE;
            }

            sizemoov -= boxsize;

            if (sizemoov == 0)
            {
                break;
            }

            boxsize -= 20;

            if (boxsize >= 0)
            {
                rtval = input->seek(input, boxsize, SEEK_CUR);
                if (rtval != 0)
                {
                    return FALSE;
                }
            }
            else
            {
                return FALSE;
            }
        }
        else
        {
            sizemoov -= boxsize;

            if (sizemoov == 0)
            {
                break;
            }

            boxsize -= 8;

            if (boxsize >= 0)
            {
                rtval = input->seek(input, boxsize, SEEK_CUR);
                if (rtval != 0)
                {
                    return FALSE;
                }
            }
            else
            {
                return FALSE;
            }
        }
    }

    rtval = input->seek(input, 0, SEEK_SET);
    if (rtval != 0)
    {
        return FALSE;
    }
    return is_audio;
}

#define ATOM_MOOV   0x766f6f6d
#define ATOM_TRAK   0x6b617274
#define ATOM_MDIA   0x6169646d
#define ATOM_MINF   0x666e696d
#define ATOM_STBL   0x6c627473
#define ATOM_STSD   0x64737473
#define ATOM_MP4A   0x6134706d
#define ATOM_ESDS   0x73647365
#define ATOM_ALAC   0x63616c61
#define ATOM_AMR    0x726d6173
#define ATOM_AWB    0x62776173
/*
    parse an atom, and compare with the input type
    if not equal skip current atom till equal
Note:
    align_atom只能用于同级的atom之间的对齐
*/
static int align_atom(storage_io_t *input, int type)
{
    unsigned char tmpbuf[4]; /* current atom size(bytes) */
    int size;
    int value;
    int read_count = 0;

    while (1)
    {
        input->read(&tmpbuf, 1, 4, input);
        size = (((int)tmpbuf[0] << 24)|((int)tmpbuf[1] << 16))  \
               | (((int)tmpbuf[2] << 8) | (tmpbuf[3]));
        read_count = input->read(&value, 1, 4, input);
        if (read_count == 0)
        {
            return -1;
        }

        if (value == type)
        {
            break;
        }
        if(size >= 8) {
        input->seek(input, size - 8, SEEK_CUR); /* 8 = bytes already read in current atom */
        }
        else {
            return -1;
        }
    }
    return size;
}
static int read_descr_length(storage_io_t *input, int *length)
{
    unsigned char b;
    char numBytes = 0;
    int value = 0, len = 0;
    do
    {
        len = input->read(&b, 1, 1, input);
        if(len != 1)
        {
            *length = 0;
            return -1;
        }
        numBytes++;
        value = (value << 7) | (b & 0x7F);
    }
    while ((b & 0x80) && numBytes < 4);
    *length = value;
    return numBytes;
}
int aacplusflag(storage_io_t *input)
{
    int rtval = 0, value = 0, tmp = 0, yl = 0, tmp_offset = 0;
    rtval = input->seek(input, 0, SEEK_SET);
    if (rtval != 0)
    {
        return -1;
    }
    rtval = align_atom(input, ATOM_MOOV);
    if (rtval < 0)
    {
        return -1;
    }
    rtval = align_atom(input, ATOM_TRAK);
    if (rtval < 0)
    {
        return -1;
    }
    rtval = align_atom(input, ATOM_MDIA);
    if (rtval < 0)
    {
        return -1;
    }
    rtval = align_atom(input, ATOM_MINF);
    if (rtval < 0)
    {
        return -1;
    }
    rtval = align_atom(input, ATOM_STBL);
    if (rtval < 0)
    {
        return -1;
    }
    rtval = align_atom(input, ATOM_STSD);
    if (rtval < 0)
    {
        return -1;
    }
    input->seek(input, 8, SEEK_CUR);
    tmp_offset = input->tell(input);
    rtval = align_atom(input, ATOM_MP4A);
    if (rtval >= 0)
    {
        return 2;
    }
    else
    {
        input->seek(input, tmp_offset, SEEK_SET);
        rtval = align_atom(input, ATOM_ALAC);
        if (rtval >= 0)
        {
            return 1;
        }
        else
        {
            input->seek(input, tmp_offset, SEEK_SET);
            rtval = align_atom(input, ATOM_AMR);
            if (rtval >= 0)
            {
                return 3;
            }
            else
            {
                input->seek(input, tmp_offset, SEEK_SET);
                rtval = align_atom(input, ATOM_AWB);
                if (rtval >= 0)
                { 
                return 4;
                }
            return -1;
            }
        }
    }

    input->seek(input, 28, SEEK_CUR);
    rtval = align_atom(input, ATOM_ESDS);
    if (rtval < 0)
    {
        return -1;
    }

    input->seek(input, 4, SEEK_CUR);
    yl = input->read(&value, 1, 1, input);
    if(yl != 1)
    {
        return -1;
    }
    if (value == 3)
    {
        read_descr_length(input, &tmp);
        if (tmp < 20)
        {
            return -1;
        }
        input->seek(input, 4, SEEK_CUR);
    }
    else
    {
        input->seek(input, 3, SEEK_CUR);
    }
    read_descr_length(input, &tmp);
    if (tmp < 13)
    {
        return -1;
    }
    input->seek(input, 14, SEEK_CUR);
    yl = read_descr_length(input, &tmp);
    if(yl < 0)
    {
        return -1;
    }
    else
    {
        unsigned char config, object_type;
        yl = input->read(&config, 1, 1, input);
        if(yl != 1)
        {
            return -1;
        }
        object_type = config >> 3; /* high 5 bit */
        if (object_type == 5)
        {
            return TRUE;
        }
        else
        {
            yl = input->read(&config, 1, 1, input);
            if(yl != 1)
            {
                return -1;
            }
            if (config & 0x2)
            {
                return -1;
            }
            if ((tmp * 8 - 16) > 16)
            {
                short syncExtensionType;
                yl = input->read(&tmp, 1, 2, input);
                if(yl != 2)
                {
                    return -1;
                }
                syncExtensionType = ((tmp & 0xff) << 3) | ((tmp >> 13) & 0x7);
                if (syncExtensionType == 0x2b7)
                {
                    unsigned char tmp_OTi = (tmp >> 8) & 0x1f;
                    if (tmp_OTi == 5)
                    {
                        return TRUE;
                    }
                }
                return FALSE;
            }
            else
            {
                return FALSE;
            }
        }
    }
}

int adts_aac_check(storage_io_t *input)
{
    int rtval = 0;
    unsigned char header[10];

    rtval = input->seek(input, 0, SEEK_SET);
    if (rtval != 0)
    {
        return -1;
    }

    rtval = input->read(header, 1, 10, input);
    if (rtval != 10)
    {
        return -1;
    }
    if (!memcmp(header, "ID3", 3))
    {
        int tagsize;
        /* high bit is not used */
        tagsize = (header[6] << 21) | (header[7] << 14)
                  | (header[8] <<  7) | (header[9] << 0);

        input->seek(input, tagsize, SEEK_CUR);
        /* ID3 size */
        tagsize += 10;
        /* get another 10bytes for format decision */
        rtval = input->read(header, 1, 10, input);
        if (rtval != 10)
        {
            return -1;
        }
    }

    if ((header[0] == 0xFF) && ((header[1] & 0xF6) == 0xF0))
    {
        int config, object_type, sf_index;
        int frame_length;

        config = header[2];
        object_type = (config >> 6) + 1;  // 2bit
        if (object_type > 2)
        {
            return FALSE;
        }

        sf_index = (config >> 2) & 0xf;   // 4bit
        if (sf_index >= 12)
        {
            return FALSE;
        }

        frame_length = ((((unsigned int)header[3] & 0x3)) << 11)
                       | ((unsigned int)header[4] << 3)
                       | ((unsigned int)header[5] >> 5);

        frame_length -= 10;
        if (frame_length > 0)
        {
            input->seek(input, frame_length, SEEK_CUR);
        }
        else
        {
            return FALSE;
        }

        /* one more frame verify */
        rtval = input->read(header, 1, 10, input);
        if (rtval != 10)
        {
            return -1;
        }
        if ((header[0] == 0xFF) && ((header[1] & 0xF6) == 0xF0))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return -1;
    }

    return TRUE;
}
