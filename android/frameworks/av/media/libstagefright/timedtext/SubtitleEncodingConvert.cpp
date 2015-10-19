//#define LOG_NDEBUG 0
#define LOG_TAG "Subtitle-JNI"
#include "utils/Log.h"
#include <stdio.h>
#include <unicode/ucnv.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include "utils/Errors.h"  // for status_t
#include "utils/String8.h"


#include "SinoDetect.h"
#include "SubtitleEncodingConvert.h"

using namespace android;//class SinoDetect declare

int convertValues(const char *subText, char *local_charset, char *outbuf) {
	int rt = 0;	
	if (!subText) {
		return -1;
	}
	
	if (strcmp(local_charset, "UTF-8") != 0) {
        UErrorCode ErrorCode = U_ZERO_ERROR;
        rt = ucnv_convert((char *)"UTF-8",
  						local_charset,
  						outbuf,     
  						MAX_SUB_LINE_LEN,
  						subText,  
  						strlen(subText),
  						&ErrorCode);
	} else {
        strcpy(outbuf, subText);
	}

	return 0;
}

char *encodingEnumToString(int value, char *encoding) {
	switch(value) {
	    case GBK:
	    	strcpy(encoding, "gbk");
	        break;
	    case BIG5:
	        strcpy(encoding, "Big5");
	        break;
	    case UTF8:
	        strcpy(encoding, "UTF-8");
	        break;
	    case UTF16:
	        strcpy(encoding, "UTF-16");
	        break;
	    case ASCII:
	        strcpy(encoding, "ASCII");
	        break;
	    case EUC_KR:
	        strcpy(encoding, "EUC-KR");
	        break;
	    case EUC_JP:
	        strcpy(encoding, "EUC-JP");
	        break;
	    case SJIS:
	        strcpy(encoding, "shift-jis");
	        break;
	    default:
	    	strcpy(encoding, "gbk");
	        break;
	}

	return encoding;
}

int subEncodingDetect(const char* substr) {
    SinoDetect mSinoDetect;

    int subEncoding = 0;
    
    if (substr != NULL) {
        subEncoding = mSinoDetect.detect_encoding((unsigned char*)substr);
    }
    
    return subEncoding;
}
