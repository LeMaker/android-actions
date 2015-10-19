/*
 * Copyright 2013 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef ASMARM_ARCH_POWER_H
#define ASMARM_ARCH_POWER_H

/* image of the saved processor state */
struct saved_context {
	/*
	 * Structure saved_context would be used to hold processor state
	 * except caller and callee registers, just before suspending.
	 */

	/* coprocessor 15 registers */
	/* u32 ID_code; */     /* read only reg */
	/* u32 cache_type; */  /* read only reg */
	/* u32 TCM_stat;  */  /* read only reg */
	u32 CR;
	u32 AUXCR;
	u32 DACR;
	u32 D_FSR;
	u32 I_FSR;
	u32 D_FAR;
	u32 I_FAR;
	/* u32 COR; */   /*write only reg */
	/* u32 TLBOR; */  /*write only reg */
	u32 D_CLR;
	u32 I_CLR;
	u32 D_TCMRR;
	u32 I_TCMRR;
	u32 D_TLBLR;
	u32 FCSE;
	u32 CID;
	u32 CSSR;
	u32 CACR;
	u32 TTB_0R;
	u32 TTB_1R;
	u32 TTBCR;
	u32 D_AFSR;
	u32 I_AFSR;
	u32 PAR;
	u32 PMControlR;
	u32 CESR;
	u32 CECR;
	u32 OFSR;
	u32 SIR;
	u32 PCSR;
	u32 CCR;
	u32 ESR;
	u32 PMCountR;
	u32 UER;
	u32 IESR;
	u32 IECR;
	u32 L2CLR;
	u32 I_TLBLR;
	u32 PRRR;
	u32 NRRR;
	u32 PLEUAR;
	u32 PLECNR;
	u32 PLECR;
	u32 PLEISAR;
	u32 PLEIEAR;
	u32 PLECIDR;
	u32 SNSVBAR;
	u32 URWTPID;
	u32 UROTPID;
	u32 POTPID;
} __packed;

typedef u32 (read_block_fn)(u32 lba, u32 len, void *buffer);
typedef u32 (write_block_fn)(u32 lba, u32 len, void *buffer);

extern void cpu_resume(void);
extern int cpu_suspend(unsigned long, int (*)(unsigned long));
extern void owl_pm_do_save(void);
extern void owl_pm_do_restore(void);

extern void register_swap_rw_handler(read_block_fn *read,
				write_block_fn *write);

extern unsigned long saved_cr, saved_ttb, idmap;
extern pgd_t *idmap_pgd;
extern const unsigned long __nosave_begin[], __nosave_end[];
extern void cpu_v7_reset(unsigned long addr);

extern void owl_finish_suspend(unsigned long cpu_state);
extern void owl_cpu_resume(void);

extern void fs_drop_page_caches(void);
extern int cpu_package(void);
extern int owl_pm_wakeup_flag(void);
extern int set_judge_adapter_type_handle(void* handle);

/* for PMIC operations ------------------------------------------------------ */

/* wakeup sources */
#define OWL_PMIC_WAKEUP_SRC_IR                  (1 << 0)
#define OWL_PMIC_WAKEUP_SRC_RESET               (1 << 1)
#define OWL_PMIC_WAKEUP_SRC_HDSW                (1 << 2)
#define OWL_PMIC_WAKEUP_SRC_ALARM               (1 << 3)
#define OWL_PMIC_WAKEUP_SRC_REMCON              (1 << 4)
#define OWL_PMIC_WAKEUP_SRC_TP                  (1 << 5)  /* 2603a */
#define OWL_PMIC_WAKEUP_SRC_WKIRQ               (1 << 6)  /* 2603a */
#define OWL_PMIC_WAKEUP_SRC_ONOFF_SHORT         (1 << 7)
#define OWL_PMIC_WAKEUP_SRC_ONOFF_LONG          (1 << 8)
#define OWL_PMIC_WAKEUP_SRC_WALL_IN             (1 << 9)
#define OWL_PMIC_WAKEUP_SRC_VBUS_IN             (1 << 10)
#define OWL_PMIC_WAKEUP_SRC_RESTART             (1 << 11)  /* 2603c */
#define OWL_PMIC_WAKEUP_SRC_SGPIOIRQ            (1 << 12)  /* 2603c */
#define OWL_PMIC_WAKEUP_SRC_WALL_OUT            (1 << 13)  /* 2603c */
#define OWL_PMIC_WAKEUP_SRC_VBUS_OUT            (1 << 14)  /* 2603c */
#define OWL_PMIC_WAKEUP_SRC_CNT                 (15)
#define OWL_PMIC_WAKEUP_SRC_ALL                 ((1U<<OWL_PMIC_WAKEUP_SRC_CNT)-1U)

/* reboot target */
#define OWL_PMIC_REBOOT_TGT_NORMAL              (0) /* with charger_check etc. */
#define OWL_PMIC_REBOOT_TGT_SYS                 (1) /* no charger ... */
#define OWL_PMIC_REBOOT_TGT_ADFU                (2)
#define OWL_PMIC_REBOOT_TGT_RECOVERY            (3)
#define OWL_PMIC_REBOOT_TGT_BOOTLOADER          (4)
#define OWL_PMIC_REBOOT_TGT_FASTBOOT	        (5)



struct owl_pmic_pm_ops {
	int (*set_wakeup_src)(uint wakeup_mask, uint wakeup_src);
	int (*get_wakeup_src)(void);
	int (*get_wakeup_flag)(void);       /* wakeup reason flag */

	int (*shutdown_prepare)(void);
	int (*powerdown)(uint deep_pwrdn, uint for_upgrade);
	int (*reboot)(uint tgt);

	int (*suspend_prepare)(void);
	int (*suspend_enter)(void);
	int (*suspend_wake)(void);
	int (*suspend_finish)(void);

	int (*get_bus_info)(uint *bus_num, uint *addr, uint *ic_type);
};

/* for atc260x_pm */
extern void owl_pmic_set_pm_ops(struct owl_pmic_pm_ops *ops);

/* other drivers (IR/TP/REMCON/SGPIO...) can use this API to setup their own wakeup source */
extern int owl_pmic_setup_aux_wakeup_src(uint wakeup_src, uint on);

#endif /* ASMARM_ARCH_POWER_H */
