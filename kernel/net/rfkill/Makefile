#
# Makefile for the RF switch subsystem.
#

rfkill-y			+= core.o
rfkill-$(CONFIG_RFKILL_INPUT)	+= input.o
obj-$(CONFIG_RFKILL)		+= rfkill.o
obj-$(CONFIG_RFKILL_REGULATOR)	+= rfkill-regulator.o
obj-$(CONFIG_RFKILL_GPIO)	+= rfkill-gpio.o

#
# Actions
#
ifdef CONFIG_PLATFORM_UBUNTU
EXTRA_CFLAGS += -DCONFIG_PLATFORM_UBUNTU
endif

obj-$(CONFIG_BROADCOM_WIFI_BT) += rfkill-actions_bcmdhd.o
obj-$(CONFIG_RTL8723BS_VQ0) += rfkill-actions_8723bs_vq0.o
obj-$(CONFIG_RTL8723BS) += rfkill-actions_8723bs.o

