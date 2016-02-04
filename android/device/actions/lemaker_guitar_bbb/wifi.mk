#prop definition, move from system.prop
ADDITIONAL_DEFAULT_PROPERTIES += wifi.interface=wlan0

BOARD_WLAN_DEVICE :=$(R_WIFI_TYPE)

#BOARD_WLAN_DEVICE := rtl8188etv
#BOARD_WLAN_DEVICE := rtl8189es
#BOARD_WLAN_DEVICE := rtl8723bs
#BOARD_WLAN_DEVICE := rtl8723bs_vq0
#BOARD_WLAN_DEVICE := rtl8723bu
#BOARD_WLAN_DEVICE := ap6210
#BOARD_WLAN_DEVICE := ap6212
#BOARD_WLAN_DEVICE := ap6330
#BOARD_WLAN_DEVICE := ap6476

ifneq ($(strip $(BOARD_WIFI_VENDOR)),)
BOARD_HAVE_WIFI := ture
else
BOARD_HAVE_WIFI := false
endif

#################################################################################
ifeq ($(strip $(BOARD_WLAN_DEVICE)), rtl8723bs)


WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_rtl
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_rtl

BOARD_WIFI_VENDOR := realtek

# wifi.c defines
WIFI_DRIVER_FIRMWARE_PATH       := "/misc/modules"
WIFI_DRIVER_MODULE_FOLDER_PATH  := "/system/lib/modules"
WIFI_DRIVER_MODULE_PATH         := $(WIFI_DRIVER_FIRMWARE_PATH)"/wlan_8723bs.ko"
WIFI_DRIVER_MODULE_NAME 	:= "wlan_8723bs"
WIFI_DRIVER_MODULE_ARG    := "ifname=wlan0 if2name=p2p0"

#WIFI_DRIVER_MODULE_ARG    := ""
WIFI_FIRMWARE_LOADER      := "rtw_fwloader"
WIFI_DRIVER_FW_PATH_STA   := "STA"
WIFI_DRIVER_FW_PATH_AP    := "AP"
WIFI_DRIVER_FW_PATH_P2P   := "P2P"
WIFI_DRIVER_FW_PATH_PARAM := "/dev/null"

#################################################################################
else ifeq ($(strip $(BOARD_WLAN_DEVICE)), rtl8723bu)

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
#CONFIG_DRIVER_WEXT :=y
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_rtl
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_rtl

BOARD_WIFI_VENDOR := realtek

# wifi.c defines
WIFI_DRIVER_FIRMWARE_PATH       := "/misc/modules"
WIFI_DRIVER_MODULE_FOLDER_PATH  := "/system/lib/modules"
WIFI_DRIVER_MODULE_PATH         := $(WIFI_DRIVER_FIRMWARE_PATH)"/wlan_8723bu.ko"
WIFI_DRIVER_MODULE_NAME 	:= "wlan_8723bu"
WIFI_DRIVER_MODULE_ARG    := "ifname=wlan0 if2name=p2p0"

#WIFI_DRIVER_MODULE_ARG   := ""
WIFI_FIRMWARE_LOADER      := "rtw_fwloader"
WIFI_DRIVER_FW_PATH_PARAM := "/dev/null"
WIFI_DRIVER_FW_PATH_STA   := "STA"
WIFI_DRIVER_FW_PATH_P2P   := "P2P"
WIFI_DRIVER_FW_PATH_AP    := "AP"

#################################################################################
else ifeq ($(strip $(BOARD_WLAN_DEVICE)), rtl8723bs_vq0)


WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
#CONFIG_DRIVER_WEXT :=y
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_rtl
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_rtl

BOARD_WIFI_VENDOR := realtek

# wifi.c defines
WIFI_DRIVER_FIRMWARE_PATH       := "/misc/modules"
WIFI_DRIVER_MODULE_FOLDER_PATH  := "/system/lib/modules"
WIFI_DRIVER_MODULE_PATH         := $(WIFI_DRIVER_FIRMWARE_PATH)"/wlan_8723bs_vq0.ko"
WIFI_DRIVER_MODULE_NAME 	:= "wlan_8723bs_vq0"
WIFI_DRIVER_MODULE_ARG    := "ifname=wlan0 if2name=p2p0"

WIFI_FIRMWARE_LOADER      := "rtw_fwloader"
WIFI_DRIVER_FW_PATH_PARAM := "/dev/null"
WIFI_DRIVER_FW_PATH_STA   := "STA"
WIFI_DRIVER_FW_PATH_P2P   := "P2P"
WIFI_DRIVER_FW_PATH_AP    := "AP"

#################################################################################
else ifeq ($(strip $(BOARD_WLAN_DEVICE)), rtl8189es)

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
#CONFIG_DRIVER_WEXT :=y
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_rtl
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_rtl

BOARD_WIFI_VENDOR := realtek

# wifi.c defines
WIFI_DRIVER_FIRMWARE_PATH       := "/misc/modules"
WIFI_DRIVER_MODULE_FOLDER_PATH  := "/system/lib/modules"
WIFI_DRIVER_MODULE_PATH         := $(WIFI_DRIVER_FIRMWARE_PATH)"/wlan_8189es.ko"
WIFI_DRIVER_MODULE_NAME 	:= "wlan_8189es"
WIFI_DRIVER_MODULE_ARG    := "ifname=wlan0 if2name=p2p0"

#WIFI_DRIVER_MODULE_ARG   := ""
WIFI_FIRMWARE_LOADER      := ""
WIFI_DRIVER_FW_PATH_STA   := ""
WIFI_DRIVER_FW_PATH_AP    := ""
WIFI_DRIVER_FW_PATH_P2P   := ""
WIFI_DRIVER_FW_PATH_PARAM := ""

#################################################################################
else ifeq ($(strip $(BOARD_WLAN_DEVICE)), rtl8188etv)

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
#CONFIG_DRIVER_WEXT :=y
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_rtl
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_rtl

BOARD_WIFI_VENDOR := realtek

# wifi.c defines
WIFI_DRIVER_FIRMWARE_PATH       := "/misc/modules"
WIFI_DRIVER_MODULE_FOLDER_PATH  := "/system/lib/modules"
WIFI_DRIVER_MODULE_PATH         := $(WIFI_DRIVER_FIRMWARE_PATH)"/wlan_8188etv.ko"
WIFI_DRIVER_MODULE_NAME 	:= "wlan_8188etv"
WIFI_DRIVER_MODULE_ARG    := "ifname=wlan0 if2name=p2p0"

WIFI_FIRMWARE_LOADER      := "rtw_fwloader"
WIFI_DRIVER_FW_PATH_PARAM := "/dev/null"
WIFI_DRIVER_FW_PATH_STA   := "STA"
WIFI_DRIVER_FW_PATH_P2P   := "P2P"
WIFI_DRIVER_FW_PATH_AP    := "AP"

##################################################################################
else ifeq ($(strip $(BOARD_WLAN_DEVICE)), ap6330)
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
WPA_SUPPLICANT_VERSION      := VER_0_8_X
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd
BOARD_WIFI_VENDOR           := broadcom
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/wlan_bcmdhd/parameters/firmware_path"
WIFI_DRIVER_FW_PATH_STA     := "/system/etc/firmware/fw_bcmdhd.bin"
WIFI_DRIVER_FW_PATH_P2P     := "/system/etc/firmware/fw_bcmdhd_p2p.bin"
WIFI_DRIVER_FW_PATH_AP      := "/system/etc/firmware/fw_bcmdhd_apsta.bin"
WIFI_DRIVER_MODULE_PATH     := "/misc/modules/wlan_bcmdhd.ko"
WIFI_DRIVER_MODULE_NAME     := "wlan_bcmdhd"
WIFI_DRIVER_MODULE_ARG      := "iface_name=wlan firmware_path=/system/etc/firmware/fw_bcmdhd.bin nvram_path=/system/etc/firmware/nvram.txt"

##################################################################################
else ifeq ($(strip $(BOARD_WLAN_DEVICE)), ap6210)
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
WPA_SUPPLICANT_VERSION      := VER_0_8_X
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd
BOARD_WIFI_VENDOR           := broadcom
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/wlan_bcmdhd/parameters/firmware_path"
WIFI_DRIVER_FW_PATH_STA     := "/system/etc/firmware/fw_bcmdhd.bin"
WIFI_DRIVER_FW_PATH_P2P     := "/system/etc/firmware/fw_bcmdhd_p2p.bin"
WIFI_DRIVER_FW_PATH_AP      := "/system/etc/firmware/fw_bcmdhd_apsta.bin"
WIFI_DRIVER_MODULE_PATH     := "/misc/modules/wlan_bcmdhd.ko"
WIFI_DRIVER_MODULE_NAME     := "wlan_bcmdhd"
WIFI_DRIVER_MODULE_ARG      := "iface_name=wlan firmware_path=/system/etc/firmware/fw_bcmdhd.bin nvram_path=/system/etc/firmware/nvram.txt config_path=/system/etc/firmware/config.txt"

##################################################################################
else ifeq ($(strip $(BOARD_WLAN_DEVICE)), ap6212)
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
WPA_SUPPLICANT_VERSION      := VER_0_8_X
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd
BOARD_WIFI_VENDOR           := broadcom
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/wlan_bcmdhd/parameters/firmware_path"
WIFI_DRIVER_FW_PATH_STA     := "/system/etc/firmware/fw_bcmdhd.bin"
WIFI_DRIVER_FW_PATH_P2P     := "/system/etc/firmware/fw_bcmdhd_p2p.bin"
WIFI_DRIVER_FW_PATH_AP      := "/system/etc/firmware/fw_bcmdhd_apsta.bin"
WIFI_DRIVER_MODULE_PATH     := "/misc/modules/wlan_bcmdhd.ko"
WIFI_DRIVER_MODULE_NAME     := "wlan_bcmdhd"
WIFI_DRIVER_MODULE_ARG      := "iface_name=wlan firmware_path=/system/etc/firmware/fw_bcmdhd.bin nvram_path=/system/etc/firmware/nvram.txt config_path=/system/etc/firmware/config.txt"

###################################################################################
else ifeq ($(strip $(BOARD_WLAN_DEVICE)), ap6476)
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
WPA_SUPPLICANT_VERSION      := VER_0_8_X
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd
BOARD_WIFI_VENDOR           := broadcom
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/wlan_bcmdhd/parameters/firmware_path"
WIFI_DRIVER_FW_PATH_STA     := "/system/etc/firmware/fw_bcmdhd.bin"
WIFI_DRIVER_FW_PATH_P2P     := "/system/etc/firmware/fw_bcmdhd_p2p.bin"
WIFI_DRIVER_FW_PATH_AP      := "/system/etc/firmware/fw_bcmdhd_apsta.bin"
WIFI_DRIVER_MODULE_PATH     := "/misc/modules/wlan_bcmdhd.ko"
WIFI_DRIVER_MODULE_NAME     := "wlan_bcmdhd"
WIFI_DRIVER_MODULE_ARG      := "iface_name=wlan firmware_path=/system/etc/firmware/fw_bcmdhd.bin nvram_path=/system/etc/firmware/nvram.txt config_path=/system/etc/firmware/config.txt"

##################################################################################
endif  # BOARD_WIFI_VENDOR
