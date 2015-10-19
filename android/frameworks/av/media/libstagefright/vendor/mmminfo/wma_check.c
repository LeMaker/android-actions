
#include "format_check.h"

int wmaflag(storage_io_t *input)
{
	int rtval = 0, count = 0;
	int is_audio;
	unsigned int *guidp1;
	unsigned int *guidp2;
#ifdef WIN32
	__int64 headerobjectsize;
	__int64 objectsize;
#else
	long long headerobjectsize;
	long long objectsize;
#endif
	int readlen;
	unsigned char guidbuf[16];
	unsigned char headerobject[16] = {0x30,0x26,0xb2,0x75,0x8e,0x66,0xcf,0x11,0xa6,0xd9,0x00,0xaa,0x00,0x62,0xce,0x6c};
	unsigned char streamobject[16] = {0x91,0x07,0xdc,0xb7,0xb7,0xa9,0xcf,0x11,0x8e,0xe6,0x00,0xc0,0x0c,0x20,0x53,0x65};
	unsigned char audioobject[16]  = {0x40,0x9e,0x69,0xf8,0x4d,0x5b,0xcf,0x11,0xa8,0xfd,0x00,0x80,0x5f,0x5c,0x44,0x2b};
	int streamcounter;

	is_audio = 0;
	readlen = input->read(&guidbuf[0], 1, 16,input);
	if (readlen != 16)
	{
		return -1;
	}

	guidp2 = (unsigned int *)&guidbuf[0];
	guidp1 = (unsigned int *)&headerobject[0];

	if (*guidp1 != *guidp2)
	{
		return -1;
	}
	if (*(guidp1 + 1) != *(guidp2 + 1))
	{
		return -1;
	}
	if (*(guidp1 + 2) != *(guidp2 + 2))
	{
		return -1;
	}
	if (*(guidp1 + 3) != *(guidp2 + 3))
	{
		return -1;
	}

	readlen = input->read(&headerobjectsize, 1, 8,input);
	if (readlen != 8)
	{
		return -1;
	}

	if (headerobjectsize < 30)
	{
		return -1;
	}

	rtval = input->seek(input, 6, SEEK_CUR);
	if (rtval != 0)
	{
		return -1;
	}

	headerobjectsize -= 30;

	streamcounter = 0;

	while(headerobjectsize > 0)
	{
	    if(count++ > 100)
	        break;
		readlen = input->read(&guidbuf[0], 1, 16,input);
		if (readlen != 16)
		{
			return -1;
		}

		readlen = input->read(&objectsize, 1, 8,input);
		if ((readlen != 8) || (objectsize < 0))
		{
			return -1;
		}

		guidp2 = (unsigned int *)&guidbuf[0];
		guidp1 = (unsigned int *)&streamobject[0];

		if ((*guidp1 == *guidp2)&&(*(guidp1 + 1) == *(guidp2 + 1))               \
		    &&(*(guidp1 + 2) == *(guidp2 + 2))&&(*(guidp1 + 3) == *(guidp2 + 3)))
		{
			streamcounter++;
			readlen = input->read(&guidbuf[0], 1, 16,input);
			if (readlen != 16)
			{
				return -1;
			}
			guidp2 = (unsigned int *)&guidbuf[0];
			guidp1 = (unsigned int *)&audioobject[0];
			if ((*guidp1 == *guidp2)&&(*(guidp1 + 1) == *(guidp2 + 1))              \
			    &&(*(guidp1 + 2) == *(guidp2 + 2))&&(*(guidp1 + 3) == *(guidp2 + 3)))
			{
//			    int audio_tag = 0;
				is_audio = 1;
//				rtval = input->seek(input, 38, SEEK_CUR);
//            	if (rtval != 0)
//            	{
//            		return 0;
//            	}
//            	
//            	readlen = input->read(&audio_tag, 1, 2,input);
//            	if (readlen != 2)
//    			{
//    				return 0;
//    			}
//            	if(audio_tag == 0x163)
//            	{
//            	    is_audio = 2;
//            	}
//            	else if(audio_tag == 0x162)
//            	{
//            	    is_audio = 3;
//            	}
//            	rtval = input->seek(input, -40, SEEK_CUR);
            	//return is_audio;
			}
			headerobjectsize -= 16;
			objectsize -= 16;
		}

		rtval = input->seek(input, (int)objectsize - 24, SEEK_CUR);
		if (rtval != 0)
		{
		    printf("wma_check seek err!!\n");
			return -1;
		}
		headerobjectsize -= objectsize;
	}
    if(streamcounter == 0)
    {
        return -1;
    }
	else if (streamcounter == 1)
	{
		return is_audio;
	}
	else
	{
		return 0;
	}
}

