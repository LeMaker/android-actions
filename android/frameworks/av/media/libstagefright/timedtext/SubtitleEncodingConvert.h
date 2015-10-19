#ifndef SUBTITLE_ENCODING_CONVERT_H_
#define SUBTITLE_ENCODING_CONVERT_H_
#define MAX_SUB_LINE_LEN 1024
enum Encodings { GBK, BIG5, UTF8, UTF16, ASCII, EUC_KR, EUC_JP, SJIS, OTHER, TOTAL_ENCODINGS };
int subEncodingDetect(const char* substr);
char *encodingEnumToString(int value, char *encoding);
int convertValues(const char *subText, char *local_charset, char *outbuf);
#endif