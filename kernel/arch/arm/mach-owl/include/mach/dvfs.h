#ifndef __ASM_ARCH_DVFS_H
#define __ASM_ARCH_DVFS_H

#define DVFSLEVEL_MAGIC    0x47739582
#define ASOC_DVFSLEVEL(ic, version, level, level_aux)   \
    (((((ic) & 0xff) << 24) | (((version) & 0xf)) << 16 | ((level^0x3f)<<10) | ((level_aux) & 0x3ff)) ^ DVFSLEVEL_MAGIC)

#define ASOC_GET_IC(dvfslevel)         ((((dvfslevel) ^ DVFSLEVEL_MAGIC) >> 24) & 0xff)
#define ASOC_GET_VERSION(dvfslevel)    ((((dvfslevel) ^ DVFSLEVEL_MAGIC) >> 16) & 0xf)
#define ASOC_GET_TYPE(dvfslevel)       ((((dvfslevel) ^ DVFSLEVEL_MAGIC) >> 10) & 0x3f)
#define ASOC_GET_TYPE_AUX(dvfslevel)	(((dvfslevel) ^ DVFSLEVEL_MAGIC) & 0x3ff)

/* version A */
#define ATM7059A_L_1              ASOC_DVFSLEVEL(0x59, 0x0, 0x0, 0x0)
#define ATM7059B_L_1              ASOC_DVFSLEVEL(0x59, 0x1, 0x0, 0x0)
#define ATM7059B_L_2              ASOC_DVFSLEVEL(0x59, 0x1, 0x0, 0x6)
#define ATM7059B_L_3              ASOC_DVFSLEVEL(0x59, 0x1, 0x0, 0x16)
#define ATM7059B_L_4              ASOC_DVFSLEVEL(0x59, 0x1, 0x0, 0x26)
#define ATM7059B_L_5              ASOC_DVFSLEVEL(0x59, 0x1, 0x0, 0x15)
#define ATM7059B_L_6              ASOC_DVFSLEVEL(0x59, 0x1, 0x0, 0x25)
#define ATM7059B_L_7              ASOC_DVFSLEVEL(0x59, 0x1, 0x0, 0x5)
#define ATM7059B_L_8              ASOC_DVFSLEVEL(0x59, 0x1, 0x0, 0x66)

#endif /* __ASM_ARCH_DVFS_H */
