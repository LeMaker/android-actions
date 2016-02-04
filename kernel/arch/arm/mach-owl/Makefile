#
# Makefile for the linux kernel.
#
# Common support
obj-y := common-owl.o cpu-owl.o timer-owl.o serial-owl.o i2c-owl.o sirq-owl.o powergate-owl.o debug-owl.o secure_storage-owl.o \
		bootdev-owl.o  dump_stack-owl.o clocktree-atm7059.o clocktree-owl.o \
	    user_dump_stack.o
#obj-y += version-owl.o
obj-$(CONFIG_MACH_OWL) += board-owl.o
obj-$(CONFIG_GPIO_OWL) += gpio-owl.o
obj-$(CONFIG_PINCTRL_OWL) += pinctrl-owl.o pinctrl_common-owl.o pinctrl_data-atm7059.o
obj-$(CONFIG_SMP) += platsmp-owl.o headsmp-owl.o hotplug_base-owl.o 
obj-$(CONFIG_ACTIONS_SOC_CAMERA) += isp_pwd-owl.o
obj-$(CONFIG_HIBERNATION) += swsusp-owl.o processor-owl.o
obj-$(CONFIG_PM) += pm-owl.o sleep-owl.o
obj-$(CONFIG_HOTPLUG_OWL) += hotplug-owl.o

#obj-y += libcidop 

