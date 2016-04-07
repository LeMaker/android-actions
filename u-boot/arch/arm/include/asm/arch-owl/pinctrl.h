#ifndef __ASM_OWL_PINCTRL_H
#define __ASM_OWL_PINCTRL_H

/*pinctrl stuff*/
int pinctrl_init_f(void);
int pinctrl_init_r(void);

/*if you cannot parse devicetree yet, use these 3 interfaces*/
int owl_pinctrl_group_set_configs(const char *pinctrl_dev_name,
				const char *group_name,
				unsigned long *configs,
				int num_configs);
int owl_pinctrl_pin_set_configs(const char *pinctrl_dev_name,
				const char *pin_name,
				unsigned long *configs,
				int num_configs);
int owl_pinctrl_set_function(const char *pinctrl_dev_name,
				const char *group_name,
				const char *function_name);

int owl_device_fdtdec_set_pinctrl_default(int dev_offset);
int owl_fdtdec_set_pinctrl(int offset);

#endif	/* __ASM_OWL_PINCTRL_H */

