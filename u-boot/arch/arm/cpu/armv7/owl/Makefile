#
# (C) Copyright 2000-2003
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# (C) Copyright 2008
# Guennadi Liakhovetki, DENX Software Engineering, <lg@denx.de>
#
# SPDX-License-Identifier:	GPL-2.0+
#


obj-y	+= pmu.o
obj-y   += atc260x_pstore.o
obj-y	+= soc.o
obj-$(CONFIG_USE_IRQ) += irq.o
obj-$(CONFIG_OWL_PINCTRL) += pinctrl.o
ifdef CONFIG_ATM7059A
obj-$(CONFIG_OWL_PINCTRL) += pinctrl_data-atm7059.o
endif
obj-$(CONFIG_OWL_PWM) += pwm.o
obj-y	+= owl_timer.o owl_reset.o owl_clk.o

