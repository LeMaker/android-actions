
EXTRA_CFLAGS += -DDEF_LINUX 

ifeq ($(CONFIG_MII),y)
EXTRA_CFLAGS += -DMII
endif
obj-$(CONFIG_NET_VENDOR_ACTS)	+= ethernet.o
ethernet-objs := ec_ethtool.o ec_phy.o des.o ethctrl.o

