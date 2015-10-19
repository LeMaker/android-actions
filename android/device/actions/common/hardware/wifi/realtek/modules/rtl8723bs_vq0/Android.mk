

#LOCAL_WIFI_MODULES :=
ifeq ($(BOARD_WLAN_DEVICE), rtl8723bs_vq0)
	include $(call all-subdir-makefiles)
endif	
