/*	File: downmix_cz.c		$Revision:   1.0  $	*/

/****************************************************************************
 ;	File:	downmix_cz.c
 ;	by chenzhan
 ;
 ;	History:
 ;		2009/03/20		Created
 ;***************************************************************************/
#include "downmix.h"
#include <omxcore.h>
#ifdef DOWNMIX_BIT16

downmix_int32 downmix_cmixtab[4]=
{
	23170, 19484, 16384, 19484
};
downmix_int32 downmix_surmixtab[4]=
{
	23170, 16384, 0, 16384
};

#endif

#ifdef DOWNMIX_BIT24

typedef __int64 downmix_int64;

downmix_int32 downmix_cmixtab[4]=
{
	5931642, 4987896, 4194304, 4987896
};

downmix_int32 downmix_surmixtab[4]=
{
	5931642, 4194304, 0, 4194304
};

#endif

#ifdef DOWNMIX_BIT32

downmix_int32 downmix_cmixtab[4]=
{
	1518500352,1276901376,1073741824,1276901376
};
downmix_int32 downmix_surmixtab[4]=
{
	1518500352,1073741824,0,1073741824
};

#endif

downmix_int16 downmix_active;//dowmmix系数是否溢出标识,溢出需要所有系数都减少

downmix_int16 downmix_bufinu[DOWNMIX_NCHANS];//输出声道是否第一次downmix

//downmix_int32 downmix_buf[DOWNMIX_NCHANS][DOWNMIX_N]; //运行domix后存放的数据buf,所有声道完后需要搬走

downmix_int32 downmix_tab[DOWNMIX_NPCMCHANS][DOWNMIX_NPCMCHANS]; //domnmix各个声道系数表格

#ifdef DOWNMIX_GNRNG
downmix_int16 downmix_appgainrng[DOWNMIX_NCHANS];
#endif

downmix_int16 downmix_chantab[DOWNMIX_NACMOD][DOWNMIX_NPCMCHANS] = { {
		DOWNMIX_LEFT, DOWNMIX_RGHT, DOWNMIX_LFE, DOWNMIX_NONE, DOWNMIX_NONE,
		DOWNMIX_NONE }, /* 1+1 */
{ DOWNMIX_CNTR, DOWNMIX_LFE, DOWNMIX_NONE, DOWNMIX_NONE, DOWNMIX_NONE,
		DOWNMIX_NONE }, /* 1/0 */
{ DOWNMIX_LEFT, DOWNMIX_RGHT, DOWNMIX_LFE, DOWNMIX_NONE, DOWNMIX_NONE,
		DOWNMIX_NONE }, /* 2/0 */
{ DOWNMIX_LEFT, DOWNMIX_CNTR, DOWNMIX_RGHT, DOWNMIX_LFE, DOWNMIX_NONE,
		DOWNMIX_NONE }, /* 3/0 */
{ DOWNMIX_LEFT, DOWNMIX_RGHT, DOWNMIX_MSUR, DOWNMIX_LFE, DOWNMIX_NONE,
		DOWNMIX_NONE }, /* 2/1 */
{ DOWNMIX_LEFT, DOWNMIX_CNTR, DOWNMIX_RGHT, DOWNMIX_MSUR, DOWNMIX_LFE,
		DOWNMIX_NONE }, /* 3/1 */
{ DOWNMIX_LEFT, DOWNMIX_RGHT, DOWNMIX_LSUR, DOWNMIX_RSUR, DOWNMIX_LFE,
		DOWNMIX_NONE }, /* 2/2 */
{ DOWNMIX_LEFT, DOWNMIX_CNTR, DOWNMIX_RGHT, DOWNMIX_LSUR, DOWNMIX_RSUR,
		DOWNMIX_LFE }, /* AC3 5.1 3/2 */
{ DOWNMIX_LEFT, DOWNMIX_RGHT, DOWNMIX_LFE, DOWNMIX_CNTR, DOWNMIX_LSUR,
		DOWNMIX_RSUR } /* ACTIONS 5.1 3/2 */
//	{DOWNMIX_CNTR,	DOWNMIX_LEFT,	DOWNMIX_RGHT,	DOWNMIX_LSUR,	DOWNMIX_RSUR,	DOWNMIX_LFE }	    /* AAC 5.1 3/2 */
		};

downmix_int16 downmix_nfront[DOWNMIX_NACMOD] = { 2, 1, 2, 3, 2, 3, 2, 3, 3 };

downmix_int16 downmix_nrear[DOWNMIX_NACMOD] = { 0, 0, 0, 0, 1, 1, 2, 2, 2 };

downmix_uint16 downmix_chane[DOWNMIX_NACMOD] = { 0xa000, 0x4000, 0xa000,
		0xe000, 0xb000, 0xf000, 0xb800, 0xf800, 0xf800 };

downmix_int16 downmix_chanary[DOWNMIX_NACMOD] = { 2, 1, 2, 3, 3, 4, 4, 5, 5 };

downmix_state * downmix_open(void) {
	downmix_state *downmix_ac3_str = NULL;

	downmix_ac3_str
			= (downmix_state *) downmix_malloc(sizeof(*downmix_ac3_str));

	if (downmix_ac3_str == NULL) {
		printf("downmix malloc fail \n");
		return NULL;
	}

	downmix_memset(downmix_ac3_str, 0, sizeof(downmix_state));

	return downmix_ac3_str;
}

downmix_int32 downmix_set(downmix_state *downmix_ac3_str,
		downmix_int32 *downmix_buf_left, downmix_int32 *downmix_buf_right) {
	/*by clp*/
	downmix_int64 temp;
	downmix_int32 temp_s;
	downmix_int16 chan;
	downmix_int32 inchan2_L2g, outchan2_L2g;

	/*clp*/
	downmix_int32 unity_L2g, m3db_L2g, m6db_L2g;
	downmix_int64 rowsum_L2g;

	/*downmix_int32 rowsum_L2g;*/
	downmix_int32 cmixval_L2g, surmixval_L2g;

	/*	downmix_int32 *exttabptr_L2g;*/
	downmix_int16 infront_L2g, inrear_L2g, outfront_L2g, outrear_L2g;
	downmix_uint16 inchane_L2g, outchane_L2g;

	downmix_int16 acmod;
	downmix_int16 outmod;

	downmix_int16 cmixlev;
	downmix_int16 surmixlev;
	downmix_int16 lfeon;
	downmix_int16 outlfe;
	downmix_int16 dualmod;

	downmix_int16 outDOWNMIX_NCHANS;
	downmix_int16 downmix_active;

#if 1

	downmix_ac3_str->acmod = DOWNMIX_MODE32ACT; //输入编码模式0-8
	downmix_ac3_str->outmod = DOWNMIX_MODE20; //输出PCM模式0-8
	downmix_ac3_str->cmixlev = 1; //certer声道系数level
	downmix_ac3_str->surmixlev = 1; //环绕声道系数level
	downmix_ac3_str->lfeon = 1; //输入是否有低音声道
	downmix_ac3_str->outlfe = 0; //输出是否有低音声道
	downmix_ac3_str->dualmod = 0; //双声道输出模式 	dual mono downmix mode  4种模式

	downmix_ac3_str->downmix_buf[0] = downmix_buf_left;
	downmix_ac3_str->downmix_buf[1] = downmix_buf_right;
#endif

	acmod = downmix_ac3_str->acmod;
	outmod = downmix_ac3_str->outmod;

	cmixlev = downmix_ac3_str->cmixlev;
	surmixlev = downmix_ac3_str->surmixlev;
	lfeon = downmix_ac3_str->lfeon;
	outlfe = downmix_ac3_str->outlfe;
	dualmod = downmix_ac3_str->dualmod;

	outDOWNMIX_NCHANS = (downmix_int16)(downmix_chanary[outmod] + outlfe); //确定输出声道数;

	for (chan = 0; chan < DOWNMIX_NCHANS; chan++) {
		downmix_bufinu[chan] = 0;
	}

	/*clp*/
	unity_L2g = DOWNMIX_UNITY;
	m3db_L2g = DOWNMIX_M3DB;
	m6db_L2g = DOWNMIX_M6DB;

	/*Clear downmix table */
	for (outchan2_L2g = 0; outchan2_L2g < DOWNMIX_NPCMCHANS; outchan2_L2g++) {
		for (inchan2_L2g = 0; inchan2_L2g < DOWNMIX_NPCMCHANS; inchan2_L2g++) {
			downmix_tab[outchan2_L2g][inchan2_L2g] = 0;
		}
	}

	/*Set up downmix parameters */

	infront_L2g = downmix_nfront[acmod];
	inrear_L2g = downmix_nrear[acmod];
	inchane_L2g = downmix_chane[acmod];
	outfront_L2g = downmix_nfront[outmod];
	outrear_L2g = downmix_nrear[outmod];
	outchane_L2g = downmix_chane[outmod];
	cmixval_L2g = downmix_cmixtab[cmixlev];
	surmixval_L2g = downmix_surmixtab[surmixlev];

	/*If (acmod == 0), mix according to dualmod */

	if (acmod == DOWNMIX_MODE11) {
		if (outfront_L2g == 1) {
			if (dualmod == DOWNMIX_DUAL_LEFTMONO) {
				downmix_tab[DOWNMIX_CNTR][DOWNMIX_LEFT] = unity_L2g;
			} else if (dualmod == DOWNMIX_DUAL_RGHTMONO) {
				downmix_tab[DOWNMIX_CNTR][DOWNMIX_RGHT] = unity_L2g;
			} else {
				downmix_tab[DOWNMIX_CNTR][DOWNMIX_LEFT] = m6db_L2g;
				downmix_tab[DOWNMIX_CNTR][DOWNMIX_RGHT] = m6db_L2g;
			}
		} else if (outfront_L2g == 2) {
			if (dualmod == DOWNMIX_DUAL_STEREO) {
				downmix_tab[DOWNMIX_LEFT][DOWNMIX_LEFT] = unity_L2g;
				downmix_tab[DOWNMIX_RGHT][DOWNMIX_RGHT] = unity_L2g;
			} else if (dualmod == DOWNMIX_DUAL_LEFTMONO) {
				downmix_tab[DOWNMIX_LEFT][DOWNMIX_LEFT] = m3db_L2g;
				downmix_tab[DOWNMIX_RGHT][DOWNMIX_LEFT] = m3db_L2g;
			} else if (dualmod == DOWNMIX_DUAL_RGHTMONO) {
				downmix_tab[DOWNMIX_LEFT][DOWNMIX_RGHT] = m3db_L2g;
				downmix_tab[DOWNMIX_RGHT][DOWNMIX_RGHT] = m3db_L2g;
			} else {
				downmix_tab[DOWNMIX_LEFT][DOWNMIX_LEFT] = m6db_L2g;
				downmix_tab[DOWNMIX_LEFT][DOWNMIX_RGHT] = m6db_L2g;
				downmix_tab[DOWNMIX_RGHT][DOWNMIX_LEFT] = m6db_L2g;
				downmix_tab[DOWNMIX_RGHT][DOWNMIX_RGHT] = m6db_L2g;
			}
		} else {
			if (dualmod == DOWNMIX_DUAL_STEREO) {
				downmix_tab[DOWNMIX_LEFT][DOWNMIX_LEFT] = unity_L2g;
				downmix_tab[DOWNMIX_RGHT][DOWNMIX_RGHT] = unity_L2g;
			} else if (dualmod == DOWNMIX_DUAL_LEFTMONO) {
				downmix_tab[DOWNMIX_CNTR][DOWNMIX_LEFT] = unity_L2g;
			} else if (dualmod == DOWNMIX_DUAL_RGHTMONO) {
				downmix_tab[DOWNMIX_CNTR][DOWNMIX_RGHT] = unity_L2g;
			} else {
				downmix_tab[DOWNMIX_CNTR][DOWNMIX_LEFT] = m6db_L2g;
				downmix_tab[DOWNMIX_CNTR][DOWNMIX_RGHT] = m6db_L2g;
			}
		}
	}
	//非acmod=mode11模式
	else {

		/*	If input and output full bw chans exist, set diagonal term to unity_L2g */
		for (chan = 0; chan < (DOWNMIX_NPCMCHANS - 1); chan++) {
			if (inchane_L2g & outchane_L2g & 0x8000) {
				downmix_tab[chan][chan] = unity_L2g;
			}
			inchane_L2g <<= 1;
			outchane_L2g <<= 1;
		}

		/*	If (outmod == 0), mix for Dolby Surround compatibility */
		if (outmod == 0) {
			if (infront_L2g != 2) {
				downmix_tab[DOWNMIX_LEFT][DOWNMIX_CNTR] = m3db_L2g;
				downmix_tab[DOWNMIX_RGHT][DOWNMIX_CNTR] = m3db_L2g;
			}
			if (inrear_L2g == 1) {
				downmix_tab[DOWNMIX_LEFT][DOWNMIX_MSUR] = -m3db_L2g;
				downmix_tab[DOWNMIX_RGHT][DOWNMIX_MSUR] = m3db_L2g;
			} else if (inrear_L2g == 2) {
				downmix_tab[DOWNMIX_LEFT][DOWNMIX_LSUR] = -m3db_L2g;
				downmix_tab[DOWNMIX_LEFT][DOWNMIX_RSUR] = -m3db_L2g;
				downmix_tab[DOWNMIX_RGHT][DOWNMIX_LSUR] = m3db_L2g;
				downmix_tab[DOWNMIX_RGHT][DOWNMIX_RSUR] = m3db_L2g;
			}
		} else if (outmod == DOWNMIX_MODE10) {

			/*	If (outmod == 1), mix for mono compatibility */
			if (infront_L2g != 1) {
				downmix_tab[DOWNMIX_CNTR][DOWNMIX_LEFT] = m3db_L2g;
				downmix_tab[DOWNMIX_CNTR][DOWNMIX_RGHT] = m3db_L2g;
			}
			if (infront_L2g == 3) {
				temp = (downmix_int64) cmixval_L2g * (downmix_int64) m3db_L2g;

				temp_s = (downmix_int32)(temp >> (DOWNMIX_BitNum_word - 1));
				temp_s = DOWNMIX_MYLIMIT(temp_s, temp_s);
				downmix_tab[DOWNMIX_CNTR][DOWNMIX_CNTR] = temp_s;
			}
			if (inrear_L2g == 1) {
				/*clp*/
				temp = (downmix_int64) surmixval_L2g * (downmix_int64) m3db_L2g;

				temp_s = (downmix_int32)(temp >> (DOWNMIX_BitNum_word - 1));
				downmix_tab[DOWNMIX_CNTR][DOWNMIX_MSUR] = temp_s;
			} else if (inrear_L2g == 2) {
				/*clp*/
				temp = (downmix_int64) surmixval_L2g * (downmix_int64) m3db_L2g;

				temp_s = (downmix_int32)(temp >> (DOWNMIX_BitNum_word - 1));
				downmix_tab[DOWNMIX_CNTR][DOWNMIX_LSUR] = temp_s;
				downmix_tab[DOWNMIX_CNTR][DOWNMIX_RSUR] = temp_s;
			}
		} else {

			/*	If (outfront_L2g == 2) and (infront_L2g == 1), mix C => L/R with -3 dB gain */
			/*	If (outfront_L2g == 2) and (infront_L2g == 3), mix C => L/R using cmixlev */
			if (outfront_L2g == 2) {
				if (infront_L2g == 1) {
					downmix_tab[DOWNMIX_LEFT][DOWNMIX_CNTR] = m3db_L2g;
					downmix_tab[DOWNMIX_RGHT][DOWNMIX_CNTR] = m3db_L2g;
				} else if (infront_L2g == 3) {
					downmix_tab[DOWNMIX_LEFT][DOWNMIX_CNTR] = cmixval_L2g;
					downmix_tab[DOWNMIX_RGHT][DOWNMIX_CNTR] = cmixval_L2g;
				}
			}

			/*	If (inrear_L2g == 1) and (outrear_L2g == 0), mix S => L/R using surmixlev - 3 dB */
			/*	If (inrear_L2g == 1) and (outrear_L2g == 2), mix S => Ls/Rs with -3 dB gain */

			if (inrear_L2g == 1) {
				if (outrear_L2g == 0) {
					/*clp*/
					temp = (downmix_int64) surmixval_L2g
							* (downmix_int64) m3db_L2g;

					temp_s = (downmix_int32)(temp >> (DOWNMIX_BitNum_word - 1));
					downmix_tab[DOWNMIX_LEFT][DOWNMIX_MSUR] = temp_s;
					downmix_tab[DOWNMIX_RGHT][DOWNMIX_MSUR] = temp_s;
				} else if (outrear_L2g == 2) {
					downmix_tab[DOWNMIX_LSUR][DOWNMIX_MSUR] = m3db_L2g;
					downmix_tab[DOWNMIX_RSUR][DOWNMIX_MSUR] = m3db_L2g;
				}
			}

			/*	If (inrear_L2g == 2) and (outrear_L2g == 0), mix Ls => L and Rs => R using
			 surmixlev */
			/*	If (inrear_L2g == 2) and (outrear_L2g == 1), mix Ls/Rs => S with -3 dB gain */

			else if (inrear_L2g == 2) {
				if (outrear_L2g == 0) {
					downmix_tab[DOWNMIX_LEFT][DOWNMIX_LSUR] = surmixval_L2g;
					downmix_tab[DOWNMIX_RGHT][DOWNMIX_RSUR] = surmixval_L2g;
				} else if (outrear_L2g == 1) {
					downmix_tab[DOWNMIX_MSUR][DOWNMIX_LSUR] = m3db_L2g;
					downmix_tab[DOWNMIX_MSUR][DOWNMIX_RSUR] = m3db_L2g;
				}
			}
		}
	}

	if (lfeon && outlfe) {
		downmix_tab[DOWNMIX_LFE][DOWNMIX_LFE] = unity_L2g;
	}

	/*	Determine if downmixing is active */
	downmix_active = 0;
	/*clp*/
	for (outchan2_L2g = 0; outchan2_L2g < DOWNMIX_NPCMCHANS; outchan2_L2g++) {
		rowsum_L2g = 0;
		for (inchan2_L2g = 0; inchan2_L2g < DOWNMIX_NPCMCHANS; inchan2_L2g++) {
			rowsum_L2g += downmix_abs(downmix_tab[outchan2_L2g][inchan2_L2g]);
			if ((rowsum_L2g - 1) > DOWNMIX_UNITY) {
				downmix_active = 1;
			}
		}
	}

	if (downmix_active == 1) //是否有溢出，溢出需要减少次数
	{
		//以后再补充

	}

	return 0;

}

downmix_int32 downmix_run(downmix_state *dommix_str, downmix_int32 *tcbuf,
		downmix_int32 frame_len, downmix_int32 channum) //channum 输入声道标号
{
	downmix_int16 chan, count;
	downmix_int64 temp;
	downmix_int32 *tcptr_L2g, *dnmixptr_L2g, dnmixfac_L2g;
	downmix_int16 inchan2_L2g, outchan2_L2g;

	downmix_int16 acmod = dommix_str->acmod;
	downmix_int16 outmod = dommix_str->outmod;
	downmix_int16 outDOWNMIX_NCHANS = (downmix_int16)(
			downmix_chanary[dommix_str->outmod] + (dommix_str->outlfe)); //确定输出声道数;

	/*	Do downmixing */
	inchan2_L2g = downmix_chantab[acmod][channum];

	for (chan = 0; chan < outDOWNMIX_NCHANS; chan++) {
		outchan2_L2g = downmix_chantab[outmod][chan];
		/*by wang*/
#ifdef DOWNMIX_GNRNG
		dnmixfac_L2g = downmix_tab[outchan2_L2g][inchan2_L2g] >> downmix_appgainrng[channum];
#else
		dnmixfac_L2g = downmix_tab[outchan2_L2g][inchan2_L2g];
#endif

		/*by wang*/
		if (dnmixfac_L2g != 0) {
			dnmixptr_L2g = dommix_str->downmix_buf[chan];
			tcptr_L2g = tcbuf;
			if (dnmixfac_L2g == DOWNMIX_UNITY) {
				if (downmix_bufinu[chan] == 0) {
					for (count = 0; count < frame_len; count++) {
						*dnmixptr_L2g++ = *tcptr_L2g++;
					}
				} else {
					for (count = 0; count < frame_len; count++) {
#ifdef XIELIMIT
						*dnmixptr_L2g=DOWNMIX_MYLIMIT((*tcptr_L2g),+(*dnmixptr_L2g));
#else
						*dnmixptr_L2g = (*tcptr_L2g) + (*dnmixptr_L2g);
#endif
						tcptr_L2g++;
						dnmixptr_L2g++;
					}
				}
			} else {
				if (downmix_bufinu[chan] == 0) {
					for (count = 0; count < frame_len; count++) {
#if 1
						/*by wang*/
						temp = (downmix_int64)(*tcptr_L2g++)
								* (downmix_int64) dnmixfac_L2g;

						*dnmixptr_L2g++ = (downmix_int32)(
								temp >> (DOWNMIX_BitNum_word - 1));
#else
						__asm__ __volatile("mult %1,%0" : :"d" (*tcptr_L2g), "d" (dnmixfac_L2g):"memory");
						__asm__ __volatile("mfhi %0":"=r" (temp)::"memory");
						*dnmixptr_L2g=temp<<1;
						tcptr_L2g++;
						dnmixptr_L2g++;
#endif
					}
				} else {
					for (count = 0; count < frame_len; count++) {
						/*by wang*/
#if 1
						temp = (downmix_int64)(*tcptr_L2g++)
								* (downmix_int64) dnmixfac_L2g;

#ifdef XIELIMIT
						temp=(temp>>(DOWNMIX_BitNum_word-1));
						*dnmixptr_L2g=DOWNMIX_MYLIMIT(temp,(*dnmixptr_L2g));
#else
						*dnmixptr_L2g = (downmix_int32)(
								temp >> (DOWNMIX_BitNum_word - 1))
								+ (*dnmixptr_L2g);
#endif
#else
						__asm__ __volatile("mult %1,%0" : :"d" (*tcptr_L2g), "d" (dnmixfac_L2g):"memory");
						__asm__ __volatile("mfhi %0":"=r" (temp)::"memory");
						*dnmixptr_L2g=(temp<<1)+*dnmixptr_L2g;
						tcptr_L2g++;
#endif
						dnmixptr_L2g++;
					}
				}
			}
			downmix_bufinu[chan] = 1;
		}
	}

	return 0;
}

downmix_int32 downmix_close(downmix_state *dommix_str) {
	downmix_free(dommix_str);
	return 0;
}
