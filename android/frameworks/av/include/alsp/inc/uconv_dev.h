#ifndef __UCONV_DEV_H__
#define __UCONV_DEV_H__

#ifdef __cplusplus
extern "C" {
#endif
/*!
 * \brief
 *		利用ucnv进行转码
 * */
int convert_ucnv(char *from_charset, char *to_charset, char *inbuf, int inlen,
		char *outbuf, int outlen);
/*!
 * \brief
 *		检查是否有效的utf-8格式
 * */
int check_valid_utf8(char *utf8, int length);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif
