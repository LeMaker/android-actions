#ifndef __PINCTRL_DATA_OWL_H__ 
#define __PINCTRL_DATA_OWL_H__

#define PINCTRL_GROUP_NAME_LEN 24
#define PINCTRL_MFPCTLREGS  4
#define PINCTRL_GPIOBANKS   5
#define PINCTRL_STREGS	    2

enum owl_pinconf_param {
	OWL_PINCONF_PARAM_PULL,
	OWL_PINCONF_PARAM_PADDRV,
	OWL_PINCONF_PARAM_SCHMITT,
};

enum owl_pinconf_pull {
	OWL_PINCONF_PULL_NONE,
	OWL_PINCONF_PULL_DOWN,
	OWL_PINCONF_PULL_UP,
};

#define OWL_PINCONF_PACK(_param_, _arg_) ((_param_) << 16 | ((_arg_) & 0xffff))
#define OWL_PINCONF_UNPACK_PARAM(_conf_) ((_conf_) >> 16)
#define OWL_PINCONF_UNPACK_ARG(_conf_) ((_conf_) & 0xffff)



/**
 * struct owl_pinmux_group - describes a Actions SOC pin group
 * @name: the name of this specific pin group
 * @pads: an array of discrete physical pins, ie, named pads in ic spec,
 *      used in this group, defined in driver-local pin enumeration space
 * @padcnt: the number of pins in this group array, i.e. the number of
 *	elements in .pads so we can iterate over that array
 * @mfpctl: fragment of mfp code
 * @gpiosw: gpio code for switch off pads
 * @schimtt: schmitt trigger setup code for this module
 */
struct owl_group {
	const char *name;
	unsigned int *pads;
	unsigned int padcnt;
	unsigned int *funcs;
	unsigned int nfuncs;

	int mfpctl_regnum;
	unsigned int mfpctl_shift;
	unsigned int mfpctl_width;

	int paddrv_regnum;
	unsigned int paddrv_shift;
	unsigned int paddrv_width;

//	u32 gpiosw[PINCTRL_GPIOBANKS];
//	u32 schimtt[PINCTRL_STREGS];
};

/**
 * struct owl_pinmux_func - Actions SOC pinctrl mux functions
 * @name: The name of the function, exported to pinctrl core.
 * @groups: An array of pin groups that may select this function.
 * @ngroups: The number of entries in @groups.
 */
struct owl_pinmux_func {
	const char *name;
	const char * const *groups;
	unsigned ngroups;
};

/**
 * struct owl_pinconf_reg_pull - Actions SOC pinctrl pull up/down regs
 * @reg: The index of PAD_PULLCTL regs.
 * @mask: The bit mask of PAD_PULLCTL fragment.
 * @pullup: The pullup value of PAD_PULLCTL fragment.
 * @pulldown: The pulldown value of PAD_PULLCTL fragment.
 */
struct owl_pinconf_reg_pull {
	int reg_num;
	unsigned int shift;
	unsigned int width;
	unsigned int pullup;
	unsigned int pulldown;
};

/**
 * struct owl_pinconf_schimtt - Actions SOC pinctrl PAD_ST regs
 * @reg: The index of PAD_ST regs.
 * @mask: The bit mask of PAD_ST fragment.
 */
struct owl_pinconf_schimtt {
	unsigned int *schimtt_funcs;
	unsigned int num_schimtt_funcs;
	int reg_num;
	unsigned int shift;
};


/**
 * struct owl_pinconf_pad_info - Actions SOC pinctrl pad info
 * @pad: The pin, in soc, the pad code of the silicon.
 * @gpio: The gpio number of the pad.
 * @pull: pull up/down reg, mask, and value.
 * @paddrv: pad drive strength info.
 * @schimtt: schimtt triger info.
 */
struct owl_pinconf_pad_info {
	int  pad;
	int  gpio;
	struct owl_pinconf_reg_pull *pull;
	struct owl_pinconf_schimtt *schimtt;
};

/**
 * this struct is identical to pinctrl_pin_desc.
 * struct pinctrl_pin_desc - boards/machines provide information on their
 * pins, pads or other muxable units in this struct
 * @number: unique pin number from the global pin number space
 * @name: a name for this pin
 */
struct owl_pinctrl_pin_desc {
	unsigned number;
	const char *name;
};

struct owl_gpio_pad{
	unsigned int gpio;
	unsigned int reg;
	unsigned int mask;
	unsigned int bit;
	int ref_count;
};

struct owl_gpio_pad_data {
	struct owl_gpio_pad *gpio_pads;
	int size;
};

extern const struct owl_pinctrl_pin_desc atm7059_pads[];
extern unsigned int atm7059_num_pads;
extern const struct owl_group atm7059_groups[];
extern int atm7059_num_groups;
extern const struct owl_pinmux_func atm7059_functions[];
extern int atm7059_num_functions;
extern struct owl_pinconf_pad_info atm7059_pad_tab[];
extern struct owl_gpio_pad_data atm7059_gpio_pad_data;

#endif /* __PINCTRL_DATA_OWL_H__ */

