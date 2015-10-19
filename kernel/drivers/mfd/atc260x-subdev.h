/* This is ugly,
 * but we need to move these long list away from the core,
 * to make code clean and readable. */

#ifdef __MFD_ATC260X__NEED_SUB_DEV_DEFINE__

#ifndef __MFD_ATC260X_SUB_DEV_DEF_H__
#define __MFD_ATC260X_SUB_DEV_DEF_H__


/* --------------  ATC2603A ------------------------------------------------- */
static const struct resource atc2603a_onoff_resources[] = {
	{
		.start = ATC2603A_IRQ_ONOFF,
		.end   = ATC2603A_IRQ_ONOFF,
		.flags = IORESOURCE_IRQ,
	},
};
static const struct resource atc2603a_irkey_resources[] = {
        {
                .start = ATC2603A_IRQ_IR,
                .end   = ATC2603A_IRQ_IR,
                .flags = IORESOURCE_IRQ,
        },
};
static const struct resource atc2603a_ethernet_resources[] = {
	{
		.start = ATC2603A_IRQ_ETHERNET,
		.end   = ATC2603A_IRQ_ETHERNET,
		.flags = IORESOURCE_IRQ,
	},
};
static const struct resource atc2603a_rtc_resources[] = {
	{
		.start = ATC2603A_IRQ_ALARM,
		.end   = ATC2603A_IRQ_ALARM,
		.flags = IORESOURCE_IRQ,
	},
};
static const struct resource atc2603a_audio_resources[] = {
	{
		.start = ATC2603A_IRQ_AUDIO,
		.end   = ATC2603A_IRQ_AUDIO,
		.flags = IORESOURCE_IRQ,
	},
};

/* remember to update the declaration in the core.c !!! */
static const struct mfd_cell sc_atc2603a_cells[] = {
	/* DCDC */
	{
		.id = 1,
		.name = "atc2603a-dcdc1",
		.of_compatible = "actions,atc2603a-dcdc1",
		/* 注意这里的of_compatible是去match DTS的(不是match driver中的那个list),
		 * 找到的device_node 将赋值到dev->of_node中.
		 * 驱动probe的时候的of_match还是dev->of_node和driver中的那个of_compatible list
		 * 两者进行match. */
	},
	{
		.id = 2,
		.name = "atc2603a-dcdc2",
		.of_compatible = "actions,atc2603a-dcdc2",
	},
	{
		.id = 3,
		.name = "atc2603a-dcdc3",
		.of_compatible = "actions,atc2603a-dcdc3",
	},
	{
		.id = 4,
		.name = "atc2603a-dcdc4",
		.of_compatible = "actions,atc2603a-dcdc4",
	},

	/* LDO */
	{
		.id = 1,
		.name = "atc2603a-ldo1",
		.of_compatible = "actions,atc2603a-ldo1",
	},
	{
		.id = 2,
		.name = "atc2603a-ldo2",
		.of_compatible = "actions,atc2603a-ldo2",
	},
	{
		.id = 3,
		.name = "atc2603a-ldo3",
		.of_compatible = "actions,atc2603a-ldo3",
	},
	{
		.id = 4,
		.name = "atc2603a-ldo4",
		.of_compatible = "actions,atc2603a-ldo4",
	},
	{
		.id = 5,
		.name = "atc2603a-ldo5",
		.of_compatible = "actions,atc2603a-ldo5",
	},
	{
		.id = 6,
		.name = "atc2603a-ldo6",
		.of_compatible = "actions,atc2603a-ldo6",
	},
	{
		.id = 7,
		.name = "atc2603a-ldo7",
		.of_compatible = "actions,atc2603a-ldo7",
	},
	{
		.id = 8,
		.name = "atc2603a-ldo8",
		.of_compatible = "actions,atc2603a-ldo8",
	},
	{
		.id = 9,
		.name = "atc2603a-ldo9",
		.of_compatible = "actions,atc2603a-ldo9",
	},
	{
		.id = 10,
		.name = "atc2603a-ldo10",
		.of_compatible = "actions,atc2603a-ldo10",
	},
	{
		.id = 11,
		.name = "atc2603a-ldo11",
		.of_compatible = "actions,atc2603a-ldo11",
	},

	/* switches-ldo */
	{
		.id = 1,
		.name = "atc2603a-switch1",
		.of_compatible = "actions,atc2603a-switch1",
	},
	{
		.id = 2,
		.name = "atc2603a-switch2",
		.of_compatible = "actions,atc2603a-switch2",
	},

	/* External PWM emulated DC-DC */
	{
		.id = 1,
		.name = "atc2603a-ext-pwm-dcdc1",
		.of_compatible = "actions,atc2603a-ext-pwm-dcdc1",
	},
	{
		.id = 2,
		.name = "atc2603a-ext-pwm-dcdc2",
		.of_compatible = "actions,atc2603a-ext-pwm-dcdc2",
	},

	/* On/Off key */
	{
		.name = "atc2603a-onoff",
		.of_compatible = "actions,atc2603a-onoff",
		.num_resources = ARRAY_SIZE(atc2603a_onoff_resources),
		.resources = atc2603a_onoff_resources,
	},

	/* Ethernet phy */
	{
		.name = "atc2603a-ethernet",
		.of_compatible = "actions,atc2603a-ethernet",
		.num_resources = ARRAY_SIZE(atc2603a_ethernet_resources),
		.resources = atc2603a_ethernet_resources,
	},

	/* RTC */
	{
		.name = "atc2603a-rtc",
		.of_compatible = "actions,atc2603a-rtc",
		.num_resources = ARRAY_SIZE(atc2603a_rtc_resources),
		.resources = atc2603a_rtc_resources,
	},

	/* GPIO */
	{
		.name = "atc2603a-gpio",
		.of_compatible = "actions,atc2603a-gpio",
	},

	/* ADC keypad */
	{
		.name = "atc2603a-adckeypad",
		.of_compatible = "actions,atc2603a-adckeypad",
	},

    /* IR keypad */
    {
        .name = "atc2603a-irkeypad",
        .of_compatible = "actions,atc2603a-irkeypad",
        .num_resources = ARRAY_SIZE(atc2603a_irkey_resources),
        .resources = atc2603a_irkey_resources,
    },
    
  /* pwm */
	{
		.name = "atc2603a-pwm",
		.of_compatible = "actions,atc2603a-pwm",
	},

	/* Audio */
	{
		.name = "atc2603a-audio",
		.of_compatible = "actions,atc2603a-audio",
		.num_resources = ARRAY_SIZE(atc2603a_audio_resources),
		.resources = atc2603a_audio_resources,
	},

	/* Hardware monitor */
	{
		.name = "atc2603a-hwmon",
		.of_compatible = "actions,atc2603a-hwmon",
	},

	/* Power */
	{
		.name = "atc2603a-power",
		.of_compatible = "actions,atc2603a-power",
	},

	/* Power Cap Gauge */
	{
		.name = "atc2603a-cap-gauge",
		.of_compatible = "actions,atc2603a-cap-gauge",
	},

	/* Suspend / Wakeup */
	{
		.name = "atc2603a-pm",
		.of_compatible = "actions,atc2603a-pm",
	},
};


/* --------------  ATC2603C ------------------------------------------------- */
static const struct resource atc2603c_onoff_resources[] = {
	{
		.start = ATC2603C_IRQ_ONOFF,
		.end   = ATC2603C_IRQ_ONOFF,
		.flags = IORESOURCE_IRQ,
	},
};
static const struct resource atc2603c_irkey_resources[] = {
    {
        .start = ATC2603C_IRQ_IR,
        .end   = ATC2603C_IRQ_IR,
        .flags = IORESOURCE_IRQ,
    },
};
static const struct resource atc2603c_rtc_resources[] = {
	{
		.start = ATC2603C_IRQ_ALARM,
		.end   = ATC2603C_IRQ_ALARM,
		.flags = IORESOURCE_IRQ,
	},
};
static const struct resource atc2603c_sgpio_resources[] = {
	{
		.start = ATC2603C_IRQ_SGPIO,
		.end   = ATC2603C_IRQ_SGPIO,
		.flags = IORESOURCE_IRQ,
	},
};
static const struct resource atc2603c_audio_resources[] = {
	{
		.start = ATC2603C_IRQ_AUDIO,
		.end   = ATC2603C_IRQ_AUDIO,
		.flags = IORESOURCE_IRQ,
	},
};

static const struct mfd_cell sc_atc2603c_cells[] = {
	/* DCDC */
	{
		.id = 1,
		.name = "atc2603c-dcdc1",
		.of_compatible = "actions,atc2603c-dcdc1",
	},
	{
		.id = 2,
		.name = "atc2603c-dcdc2",
		.of_compatible = "actions,atc2603c-dcdc2",
	},
	{
		.id = 3,
		.name = "atc2603c-dcdc3",
		.of_compatible = "actions,atc2603c-dcdc3",
	},

	/* LDO */
	{
		.id = 1,
		.name = "atc2603c-ldo1",
		.of_compatible = "actions,atc2603c-ldo1",
	},
	{
		.id = 2,
		.name = "atc2603c-ldo2",
		.of_compatible = "actions,atc2603c-ldo2",
	},
	{
		.id = 3,
		.name = "atc2603c-ldo3",
		.of_compatible = "actions,atc2603c-ldo3",
	},
	{
		.id = 5,
		.name = "atc2603c-ldo5",
		.of_compatible = "actions,atc2603c-ldo5",
	},
	{
		.id = 6,
		.name = "atc2603c-ldo6",
		.of_compatible = "actions,atc2603c-ldo6",
	},
	{
		.id = 7,
		.name = "atc2603c-ldo7",
		.of_compatible = "actions,atc2603c-ldo7",
	},
	{
		.id = 8,
		.name = "atc2603c-ldo8",
		.of_compatible = "actions,atc2603c-ldo8",
	},
	{
		.id = 11,
		.name = "atc2603c-ldo11",
		.of_compatible = "actions,atc2603c-ldo11",
	},

	/* switches-ldo */
	{
		.id = 1,
		.name = "atc2603c-switch1",
		.of_compatible = "actions,atc2603c-switch1",
	},

	/* External PWM emulated DC-DC */
	{
		.id = 1,
		.name = "atc2603c-ext-pwm-dcdc1",
		.of_compatible = "actions,atc2603c-ext-pwm-dcdc1",
	},
	{
		.id = 2,
		.name = "atc2603c-ext-pwm-dcdc2",
		.of_compatible = "actions,atc2603c-ext-pwm-dcdc2",
	},

	/* On/Off key */
	{
		.name = "atc2603c-onoff",
		.of_compatible = "actions,atc2603c-onoff",
		.num_resources = ARRAY_SIZE(atc2603c_onoff_resources),
		.resources = atc2603c_onoff_resources,
	},

	/* RTC */
	{
		.name = "atc2603c-rtc",
		.of_compatible = "actions,atc2603c-rtc",
		.num_resources = ARRAY_SIZE(atc2603c_rtc_resources),
		.resources = atc2603c_rtc_resources,
	},

	/* GPIO */
	{
		.name = "atc2603c-gpio",
		.of_compatible = "actions,atc2603c-gpio",
	},

	/* SGPIO */
	{
		.name = "atc2603c-sgpio",
		.of_compatible = "actions,atc2603c-sgpio",
		.num_resources = ARRAY_SIZE(atc2603c_sgpio_resources),
		.resources = atc2603c_sgpio_resources,
	},

	/* ADC keypad */
	{
		.name = "atc2603c-adckeypad",
		.of_compatible = "actions,atc2603c-adckeypad",
	},

    /* IR keypad */
    {
        .name = "atc2603c-irkeypad",
        .of_compatible = "actions,atc2603c-irkeypad",
        .num_resources = ARRAY_SIZE(atc2603c_irkey_resources),
        .resources = atc2603c_irkey_resources,
    },
    
  /* pwm */
	{
		.name = "atc2603c-pwm",
		.of_compatible = "actions,atc2603c-pwm",
	},

	/* Audio */
	{
		.name = "atc2603c-audio",
		.of_compatible = "actions,atc2603c-audio",
		.num_resources = ARRAY_SIZE(atc2603c_audio_resources),
		.resources = atc2603c_audio_resources,
	},

	/* Hardware monitor */
	{
		.name = "atc2603c-hwmon",
		.of_compatible = "actions,atc2603c-hwmon",
	},

	/* Power */
	{
		.name = "atc2603c-power",
		.of_compatible = "actions,atc2603c-power",
	},

	/* Power Cap Gauge */
	{
		.name = "atc2603c-cap-gauge",
		.of_compatible = "actions,atc2603c-cap-gauge",
	},

	/* Suspend / Wakeup */
	{
		.name = "atc2603c-pm",
		.of_compatible = "actions,atc2603c-pm",
	},
};





/* --------------  ATC2609A ------------------------------------------------- */
static const struct resource atc2609a_onoff_resources[] = {
	{
		.start = ATC2609A_IRQ_ONOFF,
		.end   = ATC2609A_IRQ_ONOFF,
		.flags = IORESOURCE_IRQ,
	},
};
static const struct resource atc2609a_irkey_resources[] = {
    {
        .start = ATC2609A_IRQ_IR,
        .end   = ATC2609A_IRQ_IR,
        .flags = IORESOURCE_IRQ,
    },
};
static const struct resource atc2609a_rtc_resources[] = {
	{
		.start = ATC2609A_IRQ_ALARM,
		.end   = ATC2609A_IRQ_ALARM,
		.flags = IORESOURCE_IRQ,
	},
};
static const struct resource atc2609a_audio_resources[] = {
	{
		.start = ATC2609A_IRQ_AUDIO,
		.end   = ATC2609A_IRQ_AUDIO,
		.flags = IORESOURCE_IRQ,
	},
};

static const struct mfd_cell sc_atc2609a_cells[] = {
	/* DCDC */
	{
		.id = 0,
		.name = "atc2609a-dcdc0",
		.of_compatible = "actions,atc2609a-dcdc0",
	},
	{
		.id = 1,
		.name = "atc2609a-dcdc1",
		.of_compatible = "actions,atc2609a-dcdc1",
	},
	{
		.id = 2,
		.name = "atc2609a-dcdc2",
		.of_compatible = "actions,atc2609a-dcdc2",
	},
	{
		.id = 3,
		.name = "atc2609a-dcdc3",
		.of_compatible = "actions,atc2609a-dcdc3",
	},
	{
		.id = 4,
		.name = "atc2609a-dcdc4",
		.of_compatible = "actions,atc2609a-dcdc4",
	},

	/* LDO */
	{
		.id = 0,
		.name = "atc2609a-ldo0",
		.of_compatible = "actions,atc2609a-ldo0",
	},
	{
		.id = 1,
		.name = "atc2609a-ldo1",
		.of_compatible = "actions,atc2609a-ldo1",
	},
	{
		.id = 2,
		.name = "atc2609a-ldo2",
		.of_compatible = "actions,atc2609a-ldo2",
	},
	{
		.id = 3,
		.name = "atc2609a-ldo3",
		.of_compatible = "actions,atc2609a-ldo3",
	},
	{
		.id = 4,
		.name = "atc2609a-ldo4",
		.of_compatible = "actions,atc2609a-ldo4",
	},
	{
		.id = 5,
		.name = "atc2609a-ldo5",
		.of_compatible = "actions,atc2609a-ldo5",
	},
	{
		.id = 6,
		.name = "atc2609a-ldo6",
		.of_compatible = "actions,atc2609a-ldo6",
	},
	{
		.id = 7,
		.name = "atc2609a-ldo7",
		.of_compatible = "actions,atc2609a-ldo7",
	},
	{
		.id = 8,
		.name = "atc2609a-ldo8",
		.of_compatible = "actions,atc2609a-ldo8",
	},
	{
		.id = 9,
		.name = "atc2609a-ldo9",
		.of_compatible = "actions,atc2609a-ldo9",
	},

	/* 2609a has no swtich-ldo */

	/* External PWM emulated DC-DC */
	{
		.id = 1,
		.name = "atc2609a-ext-pwm-dcdc1",
		.of_compatible = "actions,atc2609a-ext-pwm-dcdc1",
	},
	{
		.id = 2,
		.name = "atc2609a-ext-pwm-dcdc2",
		.of_compatible = "actions,atc2609a-ext-pwm-dcdc2",
	},

	/* On/Off key */
	{
		.name = "atc2609a-onoff",
		.of_compatible = "actions,atc2609a-onoff",
		.num_resources = ARRAY_SIZE(atc2609a_onoff_resources),
		.resources = atc2609a_onoff_resources,
	},

	/* RTC */
	{
		.name = "atc2609a-rtc",
		.of_compatible = "actions,atc2609a-rtc",
		.num_resources = ARRAY_SIZE(atc2609a_rtc_resources),
		.resources = atc2609a_rtc_resources,
	},

	/* GPIO */
	{
		.name = "atc2609a-gpio",
		.of_compatible = "actions,atc2609a-gpio",
	},

	/* ADC keypad */
	{
		.name = "atc2609a-adckeypad",
		.of_compatible = "actions,atc2609a-adckeypad",
	},

    /* IR keypad */
    {
        .name = "atc2609a-irkeypad",
        .of_compatible = "actions,atc2609a-irkeypad",
        .num_resources = ARRAY_SIZE(atc2609a_irkey_resources),
        .resources = atc2609a_irkey_resources,
    },
    
  /* pwm */
	{
		.name = "atc2609a-pwm",
		.of_compatible = "actions,atc2609a-pwm",
	},

	/* Audio */
	{
		.name = "atc2609a-audio",
		.of_compatible = "actions,atc2609a-audio",
		.num_resources = ARRAY_SIZE(atc2609a_audio_resources),
		.resources = atc2609a_audio_resources,
	},

	/* Hardware monitor */
	{
		.name = "atc2609a-hwmon",
		.of_compatible = "actions,atc2609a-hwmon",
	},

	/* Power */
	{
		.name = "atc2609a-power",
		.of_compatible = "actions,atc2609a-power",
	},

	/* Power Cap Gauge */
	{
		.name = "atc2609a-cap-gauge",
		.of_compatible = "actions,atc2609a-cap-gauge",
	},

	/* Suspend / Wakeup */
	{
		.name = "atc2609a-pm",
		.of_compatible = "actions,atc2609a-pm",
	},
};




static const struct mfd_cell * const sc_atc260x_mfd_cell_def_tbl[ATC260X_ICTYPE_CNT] = {
	[ATC260X_ICTYPE_2603A] = sc_atc2603a_cells,
	[ATC260X_ICTYPE_2603C] = sc_atc2603c_cells,
	[ATC260X_ICTYPE_2609A] = sc_atc2609a_cells,
};

static const u16 sc_atc260x_mfd_cell_cnt_tbl[ATC260X_ICTYPE_CNT] = {
	[ATC260X_ICTYPE_2603A] = ARRAY_SIZE(sc_atc2603a_cells),
	[ATC260X_ICTYPE_2603C] = ARRAY_SIZE(sc_atc2603c_cells),
	[ATC260X_ICTYPE_2609A] = ARRAY_SIZE(sc_atc2609a_cells),
};


#endif /* __MFD_ATC260X_SUB_DEV_DEF_H__ */

#endif
