#ifndef __VCE_REG_H__
#define __VCE_REG_H__

#define VCE_BASE_ADDR   0xB0288000/*GL5207/GL5202C:0xB0278000*/
#define VCE_ID          (0)  /*0X78323634*/
#define VCE_STATUS      (4)  /*swreg1*/
#define VCE_CFG         (16) /*swreg4*/
#define VCE_PARAM0      (20) /*swreg5*/
#define VCE_PARAM1      (24)
#define VCE_STRM        (28)
#define VCE_STRM_ADDR   (32)
#define VCE_YADDR       (36)
#define VCE_LIST0_ADDR  (40)
#define VCE_LIST1_ADDR  (44)
#define VCE_ME_PARAM    (48) /*swreg12*/
#define VCE_SWIN        (52)
#define VCE_SCALE_OUT   (56)
#define VCE_RECT        (60)
#define VCE_RC_PARAM1   (64)
#define VCE_RC_PARAM2   (68)
#define VCE_RC_PARAM3   (72)
#define VCE_RC_HDBITS   (76)
#define VCE_TS_INFO     (80)
#define VCE_TS_HEADER   (84)
#define VCE_TS_BLUHD    (88)
#define VCE_REF_DHIT    (92)
#define VCE_REF_DMISS   (96)

#define VCE_OUTSTANDING_7021 (8)
#define UPS_YAS_7021         (120)
#define UPS_CBCRAS_7021      (124)
#define UPS_CRAS_7021        (128)
#define UPS_IFORMAT_7021     (132)
#define UPS_RATIO_7021       (140)
#define UPS_IFS_7021         (144)

#define UPS_BASE_7039      (VCE_BASE_ADDR + 0xc0)
#define UPS_CTL_7039       (0xc0 + 0) /*swreg48*/
#define UPS_IFS_7039       (0xc0 + 4)
#define UPS_STR_7039       (0xc0 + 8)
#define UPS_OFS_7039       (0xc0 + 12)
#define UPS_RATH_7039      (0xc0 + 16)
#define UPS_RATV_7039      (0xc0 + 20)
#define UPS_YAS_7039       (0xc0 + 24)
#define UPS_CBCRAS_7039    (0xc0 + 28)
#define UPS_CRAS_7039      (0xc0 + 32)
#define UPS_BCT_7039       (0xc0 + 36)
#define UPS_DAB_7039       (0xc0 + 40)
#define UPS_DWH_7039       (0xc0 + 44)
#define UPS_SAB0_7039      (0xc0 + 48)
#define UPS_SAB1_7039      (0xc0 + 52)
#define UPS_RGB32_SR_7039  (0xc0 + 56)  /*swreg62*/
#define UPS_BLEND_W_7039   (0xc0 + 60)  /*swreg63*/

#endif
