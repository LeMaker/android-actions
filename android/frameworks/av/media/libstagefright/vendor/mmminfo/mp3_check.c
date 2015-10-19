/**mp3check.c*/
//#define chrisdebug

#ifdef chrisdebug
#include  < stdio.h>
#include  < stdlib.h>
#include  < string.h>
extern FILE *File_BitStream;
#include "mp3check.h"
#else
#include "format_check.h"
#endif

#define VMIPS

#ifdef VMIPS
typedef int RCHAR;
typedef int RSHORT;
typedef unsigned int RUSHORT;
typedef unsigned int RUBYTE;
typedef int DWORD;
typedef unsigned int UDWORD;
#else
typedef char RCHAR;
typedef short RSHORT;
typedef unsigned short RUSHORT;
typedef unsigned char RUBYTE;
#endif

#define BS_PAGE_SIZE 512
#define ID3_TAG (0x494433)
#define MP3_HEADORA (0xFE0C0F)
#define BS_PAGEMAX_ERRORNUM (600)
#define SYNC_HEAD_MAXCOUNT 48
#define SYNC_HEAD_MAXSAMECOUNT 3

#ifdef VMIPS
static RSHORT m_getpage(storage_io_t *input,RUBYTE *pbs)
{
    unsigned char bytebs[BS_PAGE_SIZE];
    RSHORT i;
#else
static RSHORT m_getpage(storage_io_t *input,RUBYTE *bytebs)
{
#endif
    RSHORT len;
#ifdef chrisdebug
    len = fread(bytebs,1,BS_PAGE_SIZE,File_BitStream);
#else
    len = input->read(bytebs, 1, BS_PAGE_SIZE, input);
    if(len !=  BS_PAGE_SIZE)
    {
        return -1;
    }
#endif
#ifdef VMIPS
    for (i = 0; i < len; i++)
    {
        pbs[i] = ((UDWORD)bytebs[i])&0xff;
    }
#endif
    return len;
}
static DWORD m_seekpos(storage_io_t  *input,DWORD len,DWORD original)
{
    DWORD r;
#ifdef chrisdebug
    if (original == SEEK_SET)
        original = SEEK_SET;
    else
        original = SEEK_CUR;
    r = fseek(File_BitStream,len,original);
#else
    r = input->seek(input, len, original);
#endif
    return r;
}
static RUSHORT mp3_getframesize(UDWORD curframehead)
{
    RUSHORT curframesize,t;
    RUBYTE mpeg_lsf,mpeg_ver,mpeglayer,padding;
    RUSHORT bitrate,samplerate,samplerate_table[3] = {44100,48000,32000};
    /* 1: MPEG-1, 0: MPEG-2 LSF*/
    RUSHORT table_bitrate[2][3][15]  =
    {
        {
            {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256},
            {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160},
            {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160}
        },
        {
            {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448},
            {0,32,48,56,64,80,96,112,128,160,192,224,256,320,384},
            {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320}
        }
    };
    mpeg_ver = (RUBYTE)(curframehead>>19)&0x3;
    mpeg_lsf = (mpeg_ver&0x1);
    if (mpeg_ver == 0)
    {
        mpeg_ver = 2;
    }
    else
    {
        mpeg_ver = 3-mpeg_ver;
    }
    mpeglayer = 4-((RUBYTE)(curframehead>>17)&0x3);
    t = (RUBYTE)(curframehead>>10)&0x3;
    samplerate = samplerate_table[t];
    samplerate = samplerate>>mpeg_ver;
    t = (RUBYTE)(curframehead>>12)&0xF;
    bitrate = table_bitrate[mpeg_lsf][mpeglayer-1][t];
    padding = (RUBYTE)(curframehead>>9)&0x1;
    if (mpeglayer == 3)
    {
        curframesize = (144000*(UDWORD)bitrate)/(UDWORD)samplerate;
        if (mpeg_lsf == 0)
        {
            curframesize = curframesize>>1;
        }
        curframesize = curframesize+padding;
    }
    else if (mpeglayer == 2)
    {
        curframesize = (144000*(UDWORD)bitrate)/(UDWORD)samplerate;
        curframesize = curframesize+padding;
    }
    else //lay1
    {
        curframesize = (12000*(UDWORD)bitrate)/(UDWORD)samplerate;
        curframesize = curframesize+padding;
        curframesize = curframesize << 2;
    }
    return curframesize;
}

int mp3check(storage_io_t *input)
{
    RUSHORT mp3_readpagescount = 0;
    RUBYTE ID3_TAGFlag = 0;
    UDWORD Sync_HeadValue[SYNC_HEAD_MAXCOUNT] = {0};
    RUBYTE Sync_HeadCount = 0;
    RUSHORT FrameSize[SYNC_HEAD_MAXCOUNT] = {0};
    UDWORD FramePos[SYNC_HEAD_MAXCOUNT] = {0};
    UDWORD tvalue = 0;
    RUBYTE *pbs = 0,bs[BS_PAGE_SIZE+4] = {0};
    DWORD curframepos = 0, breakpos = 0;
    RSHORT bslen = 0,readbslen = 0;
    RCHAR i = 0,Sync_HeadFindFlag = 0;
    UDWORD lastheadvalue = 0, curframehead = 0;
	UDWORD MaxSyncWordChecked = 0;
    //init
    for (i = 0; i < SYNC_HEAD_MAXCOUNT; i++)
    {
        Sync_HeadValue[i] = 0xFFFFFF;
    }
    //lastheadvalue = 0;
    breakpos = 0;
    //start
    bslen = m_getpage(input,bs);
    if (bslen <= 10)
    {
        return 1;
    }
    breakpos += (DWORD)bslen;
    pbs = bs;
    //ID3 find
//    if (ID3_TAGFlag == 0)
//    {
    tvalue = ((UDWORD)bs[0] << 16)+((UDWORD)bs[1] << 8)+(UDWORD)bs[2];
    if (ID3_TAG == tvalue)
    {
        breakpos = (((DWORD)bs[6] << 21)+((DWORD)bs[7] << 14)+((DWORD)bs[8] << 7)+(DWORD)bs[9]); //id3len
        breakpos = breakpos+10;
        curframepos = m_seekpos(input, breakpos,SEEK_SET);
        //if(curframepos! = 0)
        //    return 1;
        curframepos = (DWORD)bslen - breakpos;
        if (curframepos < 0)
        {
            bslen = -1;
        }
        else
        {
            bslen = curframepos;
        }
        pbs = bs + (UDWORD)breakpos;
    }
    ID3_TAGFlag = 1;
//    }
    while (MaxSyncWordChecked < 128*1024)
    {
        //sync head
        while (bslen>0 && (MaxSyncWordChecked < 128*1024))
        {
            if ((*pbs) != 0xFF)
            {
				MaxSyncWordChecked++;
                pbs++;
                bslen--;
                continue;
            }//if 0xff head
			MaxSyncWordChecked++;
            pbs++;
            bslen--;
            if (bslen < 3)
            {
                for (i = 0; i < bslen; i++)
                {
                    bs[i] = *pbs;
                    pbs++;
                }
                readbslen = m_getpage(input,bs+bslen);
                if (readbslen <= 0)
                {
                    return -1;
                }
                breakpos += (DWORD)readbslen;
                bslen += readbslen;
                pbs = bs;
            }
            curframehead = ((UDWORD)pbs[0] << 16)+((UDWORD)pbs[1] << 8)+(UDWORD)pbs[2]; //24bit head

            {
                if ((curframehead&0xE00000) != 0xE00000)//sync
                {
                    continue;
                }
                if ((curframehead&0x180000) == 0x080000)//ver reserved
                {
                    continue;
                }
                if ((curframehead&0x060000) == 0x000000)//layer reserved
                {
                    continue;
                }
                if ((curframehead&0x00F000) == 0x00F000)//bitrate reserved
                {
                    continue;
                }
                //if((curframehead&0x00F000) == 0x000000)//bitrate free
                //    continue;
                if ((curframehead&0x000C00) == 0x000C00)//samplerate reserved
                {
                    continue;
                }
                //if((curframehead&0x000003) == 0x000002)//emphasis reserved
                //    continue;
                tvalue = curframehead&MP3_HEADORA;
                curframepos = breakpos - (DWORD)bslen; //curframepos
                for (i = (Sync_HeadFindFlag - 1); i >= 0; i--)
                {
                    if (tvalue == Sync_HeadValue[i])
                    {
                        if (((UDWORD)curframepos - FramePos[i]) != FrameSize[i])
                        {
                            continue;
                        }
                        curframepos = (DWORD)FramePos[i];
                        Sync_HeadCount++;
                        if (Sync_HeadCount == SYNC_HEAD_MAXSAMECOUNT)
                        {
                            lastheadvalue = tvalue;
                            return 0;
                        }
                    }
                }
                //if(lastheadvalue == 0)
                {
                    Sync_HeadCount = 0;
                    if (Sync_HeadFindFlag < SYNC_HEAD_MAXCOUNT)
                    {
                        Sync_HeadValue[Sync_HeadFindFlag] = tvalue;
                        FramePos[Sync_HeadFindFlag] = (UDWORD)breakpos - (UDWORD)bslen;
                        FrameSize[Sync_HeadFindFlag] = mp3_getframesize(curframehead);
                        Sync_HeadFindFlag++;
                        continue;
                    }
                    else
                    {
                        //lastheadvalue = Sync_HeadValue[0];
                        Sync_HeadFindFlag = 0;
                        continue;
                    }
                }
            }
        }//while sync head
        bslen = m_getpage(input,bs);
        if (bslen <= 0)
        {
            return -1;
        }
        breakpos += (DWORD)bslen;
        pbs = bs;
        mp3_readpagescount++;
        if (mp3_readpagescount>BS_PAGEMAX_ERRORNUM)
        {
            return 1;
        }
    }//while(1)

    return 1;
}

