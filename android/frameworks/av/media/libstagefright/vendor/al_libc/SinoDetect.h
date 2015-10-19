#ifndef _SINODETECT_H
#define _SINODETECT_H

/*
 SinoDetect
 C++ class to detect encoding of an unsigned char* array
 Works for
 Chinese:  GB, GBK, HZ, Big5, Big5+, EUC-TW, ISO 2022-CN
 Korean:   EUC-KR, Cp949, ISO 2022-KR
 Japanese: EUC-JP, SJIS, ISO 2022-JP
 General:  UTF-8, UTF-16
 Written by Erik Peterson, erik@mandarintools.com
 Last update July, 2006
 License fee required for commercial use
 */
class SinoDetect {

public:
	/*  enum Encodings { GB2312, GBK, GB18030, HZ,
	 BIG5, BIG5PLUS, EUC_TW, ISO_2022_CN,
	 EUC_KR, CP949, ISO_2022_KR, JOHAB,
	 EUC_JP, SJIS, ISO_2022_JP,
	 UTF8, UTF16,
	 ASCII, OTHER, TOTAL_ENCODINGS };
	 */
	enum Encodings {
		GBK, BIG5, EUC_KR, EUC_JP, SJIS, OTHER, TOTAL_ENCODINGS
	};

	SinoDetect();
	~SinoDetect();

//	int detect_encoding(FILE *srcfile);
	int detect_encoding(unsigned char* rawtext);

private:

	// Frequency tables to hold the GB, Big5, and EUC-TW character
	// frequencies
	int GBFreq[94][94];
	int GBKFreq[126][191];
	int Big5Freq[94][158];
	//  int Big5PFreq[126][191];
	//  int EUC_TWFreq[94][94];
	int KRFreq[94][94];
	int JPFreq[94][94];

	//int UnicodeFreq[94][128];

	void initialize_frequencies();

	//  int gb2312_probability(unsigned char* rawtext);
	int gbk_probability(unsigned char* rawtext);
	//  int gb18030_probability(unsigned char* rawtext);
	//  int hz_probability(unsigned char* rawtext);
	int big5_probability(unsigned char* rawtext);
	//  int big5plus_probability(unsigned char* rawtext);
	//  int euc_tw_probability(unsigned char* rawtext);
	//  int iso_2022_cn_probability(unsigned char* rawtext);
	//  int utf8_probability(unsigned char* rawtext);
	//  int utf16_probability(unsigned char* rawtext);
	//  int ascii_probability(unsigned char* rawtext);
	int euc_kr_probability(unsigned char* rawtext);
	//  int cp949_probability(unsigned char* rawtext);
	//  int iso_2022_kr_probability(unsigned char* rawtext);
	int sjis_probability(unsigned char* rawtext);
	int euc_jp_probability(unsigned char* rawtext);
	//  int iso_2022_jp_probability(unsigned char* rawtext);

};

#endif
