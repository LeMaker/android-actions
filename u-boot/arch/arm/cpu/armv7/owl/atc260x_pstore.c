#include <common.h>
#include <asm/io.h>
#include <asm/arch/pmu.h>

/* persistent storage
 * 本模块负责管理260x中几个不掉电/不复位的寄存器.
 * 因为要考虑N颗相似IC的兼容, 使用了比较繁琐的数据结构. */


#ifndef __ffs /* 0-based, same as the one in kernel */
#define __ffs(val) __builtin_ctzl(val) /* provided by GCC */
#endif

struct atc260x_pstore_fmt {
	u16     regs[4];   /* destination register of each slice */
	u16     reg_bm[4]; /* register bitmap of each slice */
	u8      src_bp[4]; /* source start bit position of each slice */
};

static const struct atc260x_pstore_fmt sc_atc2603a_pstore_fmt_tbl[] = {
	[ATC260X_PSTORE_TAG_REBOOT_ADFU] = {
		.regs   = {ATC2603A_PMU_UV_STATUS },
		.reg_bm = {(1U << 1)              },
		.src_bp = {0                      },
	},

	[ATC260X_PSTORE_TAG_REBOOT_RECOVERY] = {
		.regs   = {ATC2603A_PMU_SYS_CTL2 },
		.reg_bm = {(3U << 1)             },
		.src_bp = {0                     },
	},

	[ATC260X_PSTORE_TAG_FW_S2] = {
		.regs   = {ATC2603A_PMU_SYS_CTL3 },
		.reg_bm = {(1U << 4)             },
		.src_bp = {0                     },
	},

	[ATC260X_PSTORE_TAG_DIS_MCHRG] = {
		.regs   = {ATC2603A_PMU_UV_INT_EN },
		.reg_bm = {(1U << 0)              },
		.src_bp = {0                      },
	},

	[ATC260X_PSTORE_TAG_RTC_MSALM] = {
		.regs   = {ATC2603A_PMU_SYS_CTL3, ATC2603A_PMU_SYS_CTL9},
		.reg_bm = {(0xfU << 6),           (0xff << 0)          },
		.src_bp = {8,                     0                    },
	},

	[ATC260X_PSTORE_TAG_RTC_HALM] = {
		.regs   = {ATC2603A_PMU_VBUS_CTL1 },
		.reg_bm = {(0x1fU << 0)           },
		.src_bp = {0                      },
	},

	[ATC260X_PSTORE_TAG_RTC_YMDALM] = {
		.regs   = {ATC2603A_PMU_SYS_CTL8 },
		.reg_bm = {(0xffffU << 0)        },
		.src_bp = {0                     },
	},

	[ATC260X_PSTORE_TAG_GAUGE_CAP] = {
		.regs   = {ATC2603A_PMU_SYS_CTL9 },
		.reg_bm = {(0xffU << 8)          },
		.src_bp = {0                     },
	},

	[ATC260X_PSTORE_TAG_GAUGE_BAT_RES] = {
		.regs   = {ATC2603A_PMU_OC_INT_EN, ATC2603A_PMU_OC_INT_EN, ATC2603A_PMU_OC_INT_EN, ATC2603A_PMU_SYS_PENDING },
		.reg_bm = {(0x3fU << 0),           (1U << 10),             (7U<<13),               (0x3fU << 1)             },
		.src_bp = {0,                      6,                      7,                      10                       },
	},

	[ATC260X_PSTORE_TAG_GAUGE_ICM_EXIST] = {
		.regs   = {ATC2603A_PMU_SYS_CTL3 },
		.reg_bm = {(1U << 5)             },
		.src_bp = {0                     },
	},

	[ATC260X_PSTORE_TAG_GAUGE_SHDWN_TIME] = {
		.regs   = {0, }, /* not need for 2603a */
		.reg_bm = {0, },
		.src_bp = {0, },
	},

	[ATC260X_PSTORE_TAG_GAUGE_S2_CONSUMP] = {
		.regs   = {0, }, /* not need for 2603a */
		.reg_bm = {0, },
		.src_bp = {0, },
	},

	[ATC260X_PSTORE_TAG_GAUGE_CLMT_RESET] = {
		.regs   = {0, }, /* not need for 2603a */
		.reg_bm = {0, },
		.src_bp = {0, },
	},

	[ATC260X_PSTORE_TAG_RESUME_ADDR] = {
		.regs   = {ATC2603A_PMU_OC_STATUS, ATC2603A_PMU_SYS_CTL3, ATC2603A_PMU_SYS_CTL9, ATC2603A_PMU_SYS_CTL8},
		.reg_bm = {(0xfU << 2),            (0xfU << 6),           (0xff << 0),           (0xffffU << 0)       },
		.src_bp = {28,                     24,                     16,                   0                    },
	},
};

static const struct atc260x_pstore_fmt sc_atc2603c_pstore_fmt_tbl[] = {
	[ATC260X_PSTORE_TAG_REBOOT_ADFU] = {
		.regs   = {ATC2603C_PMU_UV_STATUS },
		.reg_bm = {(1U << 1)              },
		.src_bp = {0                      },
	},

	[ATC260X_PSTORE_TAG_REBOOT_RECOVERY] = {
		.regs   = {ATC2603C_PMU_OV_INT_EN },
		.reg_bm = {(3U << 0)              },
		.src_bp = {0                      },
	},

	[ATC260X_PSTORE_TAG_FW_S2] = {
		.regs   = {ATC2603C_PMU_SYS_CTL3 },
		.reg_bm = {(1U << 4)             },
		.src_bp = {0                     },
	},

	[ATC260X_PSTORE_TAG_DIS_MCHRG] = {
		.regs   = {ATC2603C_PMU_UV_INT_EN },
		.reg_bm = {(1U << 0)              },
		.src_bp = {0                      },
	},

	[ATC260X_PSTORE_TAG_RTC_MSALM] = {
		.regs   = {ATC2603C_PMU_FW_USE0 },
		.reg_bm = {(0xfffU << 0)        },
		.src_bp = {0                    },
	},

	[ATC260X_PSTORE_TAG_RTC_HALM] = {
		.regs   = {ATC2603C_PMU_VBUS_CTL1 },
		.reg_bm = {(0x1fU << 0)           },
		.src_bp = {0                      },
	},

	[ATC260X_PSTORE_TAG_RTC_YMDALM] = {
		.regs   = {ATC2603C_PMU_SYS_CTL8 },
		.reg_bm = {(0xffffU << 0)        },
		.src_bp = {0                     },
	},

	[ATC260X_PSTORE_TAG_GAUGE_CAP] = {
		.regs   = {ATC2603C_PMU_SYS_CTL9 },
		.reg_bm = {(0xffU << 8)          },
		.src_bp = {0                     },
	},

	[ATC260X_PSTORE_TAG_GAUGE_BAT_RES] = {
		.regs   = {ATC2603C_PMU_OC_INT_EN, ATC2603C_PMU_OC_INT_EN, ATC2603C_PMU_OC_INT_EN, ATC2603A_PMU_SYS_CTL9 },
		.reg_bm = {(0x3fU << 0),           (1U << 10),             (7U<<13),               (0x3fU << 2)          },
		.src_bp = {0,                      6,                      7,                      10                    },
	},

	[ATC260X_PSTORE_TAG_GAUGE_ICM_EXIST] = {
		.regs   = {ATC2603C_PMU_SYS_CTL3 },
		.reg_bm = {(1U << 5)             },
		.src_bp = {0                     },
	},

	[ATC260X_PSTORE_TAG_GAUGE_SHDWN_TIME] = {
		.regs   = {0, }, /* not need for 2603c */
		.reg_bm = {0, },
		.src_bp = {0, },
	},

	[ATC260X_PSTORE_TAG_GAUGE_S2_CONSUMP] = {
		.regs   = {0, }, /* not need for 2603c */
		.reg_bm = {0, },
		.src_bp = {0, },
	},

	[ATC260X_PSTORE_TAG_GAUGE_CLMT_RESET] = {
		.regs   = {0, }, /* not need for 2603c */
		.reg_bm = {0, },
		.src_bp = {0, },
	},

	[ATC260X_PSTORE_TAG_RESUME_ADDR] = {
		.regs   = {ATC2603C_PMU_FW_USE0, ATC2603C_PMU_SYS_CTL8},
		.reg_bm = {(0xffffU << 0),       (0xffffU << 0)       },
		.src_bp = {16,                   0                    },
	},
};

static const struct atc260x_pstore_fmt sc_atc2609a_pstore_fmt_tbl[] = {
	[ATC260X_PSTORE_TAG_REBOOT_ADFU] = {
		.regs   = {ATC2609A_PMU_UV_STATUS },
		.reg_bm = {(1U << 1)              },
		.src_bp = {0                      },
	},

	[ATC260X_PSTORE_TAG_REBOOT_RECOVERY] = {
		.regs   = {ATC2609A_PMU_OV_INT_EN },
		.reg_bm = {(3U << 0)              },
		.src_bp = {0                      },
	},

	[ATC260X_PSTORE_TAG_FW_S2] = {
		.regs   = {ATC2609A_PMU_SYS_CTL3 },
		.reg_bm = {(1U << 4)             },
		.src_bp = {0                     },
	},

	[ATC260X_PSTORE_TAG_DIS_MCHRG] = {
		.regs   = {ATC2609A_PMU_UV_INT_EN },
		.reg_bm = {(1U << 0)              },
		.src_bp = {0                      },
	},

	[ATC260X_PSTORE_TAG_RTC_MSALM] = {
		.regs   = {ATC2609A_PMU_SYS_CTL7 },
		.reg_bm = {(0xfffU << 0)         },
		.src_bp = {0                     },
	},

	[ATC260X_PSTORE_TAG_RTC_HALM] = {
		.regs   = {ATC2609A_PMU_VBUS_CTL1 },
		.reg_bm = {(0x1fU << 0)           },
		.src_bp = {0                      },
	},

	[ATC260X_PSTORE_TAG_RTC_YMDALM] = {
		.regs   = {ATC2609A_PMU_SYS_CTL8 },
		.reg_bm = {(0xffffU << 0)        },
		.src_bp = {0                     },
	},

	[ATC260X_PSTORE_TAG_GAUGE_CAP] = {
		.regs   = {ATC2609A_PMU_SYS_CTL9 },
		.reg_bm = {(0xffU << 8)          },
		.src_bp = {0                     },
	},

	[ATC260X_PSTORE_TAG_GAUGE_BAT_RES] = {
		.regs   = {0, }, /* not need for 2609a */
		.reg_bm = {0, },
		.src_bp = {0, },
	},

	[ATC260X_PSTORE_TAG_GAUGE_ICM_EXIST] = {
		.regs   = {0, }, /* not need for 2609a */
		.reg_bm = {0, },
		.src_bp = {0, },
	},

	[ATC260X_PSTORE_TAG_GAUGE_SHDWN_TIME] = {
		.regs   = {ATC2609A_PMU_SYS_CTL9, ATC2609A_PMU_BAT_CTL0, ATC2609A_PMU_BAT_CTL1, ATC2609A_PMU_WALL_CTL1 },
		.reg_bm = {(0xffU << 0),          (0x3fU << 0),          (0x1ffU << 0),         (0xffU << 0)           },
		.src_bp = {0,                     8,                     14,                    23                     },
	},

	[ATC260X_PSTORE_TAG_GAUGE_S2_CONSUMP] = {
		.regs   = {ATC2609A_PMU_VBUS_CTL0, },
		.reg_bm = {(0x3fU << 0),           },
		.src_bp = {0,                      },
	},

	[ATC260X_PSTORE_TAG_GAUGE_CLMT_RESET] = {
		.regs   = {ATC2609A_PMU_OV_STATUS, },
		.reg_bm = {(1U << 1),              },
		.src_bp = {0,                      },
	},

	[ATC260X_PSTORE_TAG_RESUME_ADDR] = {
		.regs   = {ATC2609A_PMU_SYS_CTL7, ATC2609A_PMU_SYS_CTL8},
		.reg_bm = {(0xffffU << 0),        (0xffffU << 0)       },
		.src_bp = {16,                    0                    },
	},
};

static const struct atc260x_pstore_fmt *_atc260x_pstore_get_fmt_desc(uint tag)
{
	/* 这样写而不是直接查表, 是为了size的优化,
	 * 当 OWL_PMU_ID 是常量时就有差别了. */
	switch (OWL_PMU_ID) {
	case OWL_PMU_ID_ATC2603A:
		return &sc_atc2603a_pstore_fmt_tbl[tag];
	case OWL_PMU_ID_ATC2603C:
		return &sc_atc2603c_pstore_fmt_tbl[tag];
	case OWL_PMU_ID_ATC2603B: /* OWL_PMU_ID_ATC2609A */
		return &sc_atc2609a_pstore_fmt_tbl[tag];
	}
	return NULL;
}

int atc260x_pstore_set(uint tag, u32 value)
{
	const struct atc260x_pstore_fmt *fmt;
	uint i, reg_shift;
	u32 src_bm, reg_bm, reg_val;
	int ret;

	if (tag >= ATC260X_PSTORE_TAG_NUM)
		return -1;

	fmt = _atc260x_pstore_get_fmt_desc(tag);

	if (fmt->reg_bm[0] == 0 && value != 0) {
		printf("pstore tag %u not available (ic_type=%u)\n",
			tag, OWL_PMU_ID);
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(fmt->regs); i++) {
		if (fmt->reg_bm[i] == 0)
			break;

		reg_bm = fmt->reg_bm[i];
		reg_shift = __ffs(reg_bm);
		src_bm = (reg_bm >> reg_shift) << fmt->src_bp[i];
		reg_val = ((value & src_bm) >> fmt->src_bp[i]) << reg_shift;

		ret = atc260x_set_bits(fmt->regs[i], reg_bm, reg_val);
		if (ret) {
			printf("pstore io failed, ret=%d", ret);
			return ret;
		}
	}

	return 0;
}

int atc260x_pstore_get(uint tag, u32 *p_value)
{
	const struct atc260x_pstore_fmt *fmt;
	uint i, reg_shift;
	u32 result, reg_bm;
	int ret;

	if (tag >= ATC260X_PSTORE_TAG_NUM)
		return -1;

	fmt = _atc260x_pstore_get_fmt_desc(tag);

	result = 0;
	for (i = 0; i < ARRAY_SIZE(fmt->regs); i++) {
		if (fmt->reg_bm[i] == 0)
			break;

		reg_bm = fmt->reg_bm[i];
		reg_shift = __ffs(reg_bm);

		ret = atc260x_reg_read(fmt->regs[i]);
		if (ret < 0) {
			printf("pstore io failed, ret=%d", ret);
			return ret;
		}
		result |= (((u32)ret & reg_bm) >> reg_shift) << fmt->src_bp[i];
	}

	*p_value = result;
	return 0;
}

ulong atc260x_pstore_get_noerr(uint tag)
{
	u32 tmp;
	while (atc260x_pstore_get(tag, &tmp) != 0);
	return tmp;
}
