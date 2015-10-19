#include "format_check.h"

typedef unsigned char BYTE;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef int INT32;
typedef INT32 HX_RESULT;

void rm_pack32(UINT32 ulValue, BYTE** ppBuf, UINT32* pulLen) {
	if (ppBuf && pulLen && *pulLen >= 4) {
		BYTE* pBuf = *ppBuf;
		pBuf[0] = (BYTE) ((ulValue & 0xFF000000) >> 24);
		pBuf[1] = (BYTE) ((ulValue & 0x00FF0000) >> 16);
		pBuf[2] = (BYTE) ((ulValue & 0x0000FF00) >> 8);
		pBuf[3] = (BYTE) (ulValue & 0x000000FF);
		*ppBuf += 4;
		*pulLen -= 4;
	}
}

void rm_pack16(UINT16 usValue, BYTE** ppBuf, UINT32* pulLen) {
	if (ppBuf && pulLen && *pulLen >= 2) {
		BYTE* pBuf = *ppBuf;
		pBuf[0] = (BYTE) ((usValue & 0x0000FF00) >> 8);
		pBuf[1] = (BYTE) (usValue & 0x000000FF);
		*ppBuf += 2;
		*pulLen -= 2;
	}
}

void rm_pack8(BYTE ucValue, BYTE** ppBuf, UINT32* pulLen) {
	if (ppBuf && pulLen && *pulLen > 0) {
		BYTE* pBuf = *ppBuf;
		pBuf[0] = (BYTE) ucValue;
		*ppBuf += 1;
		*pulLen -= 1;
	}
}

UINT32 rm_unpack32(BYTE** ppBuf, UINT32* pulLen) {
	UINT32 ulRet = 0;

	if (ppBuf && pulLen && *pulLen >= 4) {
		BYTE* pBuf = *ppBuf;
		ulRet = (pBuf[0] << 24) | (pBuf[1] << 16) | (pBuf[2] << 8) | pBuf[3];
		*ppBuf += 4;
		*pulLen -= 4;
	}

	return ulRet;
}

UINT16 rm_unpack16(BYTE** ppBuf, UINT32* pulLen) {
	UINT16 usRet = 0;

	if (ppBuf && pulLen && *pulLen >= 2) {
		BYTE* pBuf = *ppBuf;
		usRet = (pBuf[0] << 8) | pBuf[1];
		*ppBuf += 2;
		*pulLen -= 2;
	}

	return usRet;
}

BYTE rm_unpack8(BYTE** ppBuf, UINT32* pulLen) {
	BYTE ucRet = 0;

	if (ppBuf && pulLen && *pulLen > 0) {
		BYTE* pBuf = *ppBuf;
		ucRet = pBuf[0];
		*ppBuf += 1;
		*pulLen -= 1;
	}

	return ucRet;
}

/* 区分RM文件中是否含有视频，如果包含视频返回0，单纯音频则返回1*/
int rm_check(storage_io_t *input) {
	int rt = 0;
	char buf[2048];
	int size;
	unsigned char *pBuf;
	unsigned int ulLen;
	int stream_name_sz;
	int mime_type_sz;
	int type_spec_sz;
	int audio_tag = 0;
	int video_tag = 0;
	char mime_type = 0;
	int num_headers = 0;
	int i;
	int bDone = 0;
	//is .RMF
	rt = input->read(buf, 1, 4, input);
	//rt = fread(buf,1,4,fp);
	if (rt != 4) {
		goto end;
	}
	if (*(unsigned int *) buf != 0x464d522e) {
		goto end;
	}
	rt = input->read(buf, 1, 4, input);
	//rt = fread(buf,1,4,fp);//size
	if (rt != 4) {
		goto end;
	}
	pBuf = buf;
	ulLen = 4;
	size = rm_unpack32(&pBuf, &ulLen) - 8;
	rt = input->read(buf, 1, size, input);
	//rt = fread(buf,1,size,fp);
	if (rt != size) {
		goto end;
	}
	pBuf = buf + 6;
	ulLen = size - 6;
	num_headers = rm_unpack32(&pBuf, &ulLen);

	for (i = 0; (i < num_headers) && (!bDone); i++) {
		pkt_start: rt = input->read(buf, 1, 4, input);
		//rt = fread(buf,1,4,fp);
		if (rt != 4) {
			break;
		}
		if (*(unsigned int *) buf == 0x504f5250)//is PROP
		{
			rt = input->read(buf, 1, 4, input);
			//rt = fread(buf,1,4,fp);//size
			if (rt != 4) {
				break;
			}
			pBuf = buf;
			ulLen = 4;
			size = rm_unpack32(&pBuf, &ulLen) - 8;
			rt = input->read(buf, 1, size, input);
			//rt = fread(buf,1,size,fp);
			if (rt != size) {
				break;
			}
		} else if (*(unsigned int *) buf == 0x5250444d)//is MDPR
		{
			rt = input->read(buf, 1, 4, input);
			//rt = fread(buf,1,4,fp);
			if (rt != 4) {
				break;
			}
			pBuf = buf;
			ulLen = 4;
			size = rm_unpack32(&pBuf, &ulLen) - 8;
			rt = input->read(buf, 1, size, input);
			// rt = fread(buf,1,size,fp);
			if (rt != size) {
				break;
			}
			pBuf = buf;
			ulLen = size;

			if (ulLen >= 33) {
				pBuf += 32;
				ulLen -= 32;
				stream_name_sz = rm_unpack8(&pBuf, &ulLen);
				if (ulLen >= (UINT32) stream_name_sz + 1) {
					pBuf += stream_name_sz;
					ulLen -= stream_name_sz;
					mime_type_sz = rm_unpack8(&pBuf, &ulLen);
					if (ulLen >= (UINT32) mime_type_sz + 4) {
						mime_type = rm_unpack8(&pBuf, &ulLen);
						pBuf += (mime_type_sz - 1);
						ulLen -= (mime_type_sz - 1);
						type_spec_sz = rm_unpack32(&pBuf, &ulLen);
						if (ulLen >= (UINT32) type_spec_sz) {
							if (mime_type == 97) {
								audio_tag = 1;
								goto pkt_start;
							} else if (mime_type == 118) {
								video_tag = 1;
								goto pkt_start;
							}
						}
					}
				}
			}

		} else if (*(unsigned int *) buf == 0x544e4f43)//CONT
		{
			rt = input->read(buf, 1, 4, input);
			//rt = fread(buf,1,4,fp);//size
			if (rt != 4) {
				break;
			}
			pBuf = buf;
			ulLen = 4;
			size = rm_unpack32(&pBuf, &ulLen) - 8;
			rt = input->read(buf, 1, size, input);
			//rt = fread(buf,1,size,fp);
			if (rt != size) {
				break;
			}
		} else if (*(unsigned int *) buf == 0x41544144)//DATA
		{
			bDone = 1;
		} else {
			rt = input->read(buf, 1, 4, input);
			//rt = fread(buf,1,4,fp);//size
			if (rt != 4) {
				break;
			}
			pBuf = buf;
			ulLen = 4;
			size = rm_unpack32(&pBuf, &ulLen) - 8;
			rt = input->read(buf, 1, size, input);
			//rt = fread(buf,1,size,fp);
			if (rt != size) {
				break;
			}
		}

	}

	if (audio_tag == 1) {
		if (video_tag == 1) {
			//printf("is rm\n");
			return 0;
		} else {
			//printf("is ra\n");
			return 1;
		}
	} else {
		if (video_tag == 1) {
			return 0;
		} else {
			return -1;
		}
	}

	return 0;
	end: return -1;
}
