#define LOG_TAG             "AL_DETECT"
#include <utils/Log.h>


#include <actal_posix_dev.h>
#include "SinoDetect.h"

/*
 * return 0 if no illegal bytes, else -1
 * */
int actal_encode_detect(const char *src, char *encoding) {
	int sd_encoding = SinoDetect::OTHER;
	SinoDetect sd;

	if ((src == NULL) || (strlen(src) == 0)) {
		return -1;
	}

	sd_encoding = sd.detect_encoding((unsigned char*) src);
	switch (sd_encoding) {
	case SinoDetect::BIG5:
		strcpy(encoding, "Big5");
		break;
	case SinoDetect::EUC_KR:
		strcpy(encoding, "EUC-KR");
		break;
	case SinoDetect::EUC_JP:
		strcpy(encoding, "EUC-JP");
		break;
	case SinoDetect::SJIS:
		strcpy(encoding, "shift-jis");
		break;
	case SinoDetect::GBK:
	default:
		strcpy(encoding, "gbk");
		break;
	}

	return 0;
}