
EXTRA_CFLAGS += -DDEF_LINUX 

ifeq ($(CONFIG_MII),y)
EXTRA_CFLAGS += -DMII
endif
obj-$(CONFIG_NET_VENDOR_SG8201G)	+= sg8201g_ethernet.o
sg8201g_ethernet-objs := ec_ethtool.o ec_phy.o ethctrl.o

