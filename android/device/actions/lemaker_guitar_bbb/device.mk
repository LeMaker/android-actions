#
# Copyright (C) 2011 The Android Open-Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

ifneq ($(filter $(TARGET_PRODUCT),lemaker_guitar_bbb),)

#copy from config
R_WIFI_TYPE=rtl8723bs
R_BT_TYPE=rtl8723bs
R_BT_USE_UART=ttyS2
R_GMS_TYPE=base
R_FIRMWARE_ROOTED=true
ACTIONS_RELEASE_BUILD_FOR_CUSTOM=false
VERSION_DATE=$(shell date "+%y%m%d")

#copy to /root filesystem
PRODUCT_COPY_FILES := \
    device/actions/lemaker_guitar_bbb/kernel:kernel \
	device/actions/lemaker_guitar_bbb/ft5x06-touch.idc:system/usr/idc/ft5x06-touch.idc \
	device/actions/lemaker_guitar_bbb/goodix-ts-touch.idc:system/usr/idc/goodix-ts-touch.idc \
	device/actions/lemaker_guitar_bbb/gl5203-adckey.kl:system/usr/keylayout/gl5203-adckey.kl \
	device/actions/lemaker_guitar_bbb/vold.fstab:system/etc/vold.fstab \
	device/actions/lemaker_guitar_bbb/apns-conf.xml:system/etc/apns-conf.xml \
	device/actions/lemaker_guitar_bbb/vold.sdboot.fstab:system/etc/vold.sdboot.fstab \
	device/actions/lemaker_guitar_bbb/excluded-input-devices.xml:system/etc/excluded-input-devices.xml \
	device/actions/lemaker_guitar_bbb/packages-compat-default.xml:system/etc/packages-compat-default.xml \
	device/actions/lemaker_guitar_bbb/excluded_recovery:system/etc/excluded_recovery \
	device/actions/lemaker_guitar_bbb/excluded_dataclone:system/etc/excluded_dataclone \
	device/actions/lemaker_guitar_bbb/NOTICE.html:system/etc/NOTICE.html \
	device/actions/common/prebuilt/utils/angk/an_001:system/etc/angk/an_001 \
	device/actions/common/prebuilt/utils/angk/an_002:system/etc/angk/an_002 \
	build/target/product/security/platform.pk8:/system/etc/security/platform.pk8 \
	build/target/product/security/platform.x509.pem:/system/etc/security/platform.x509.pem \
	external/ppp/dns_conf/ip-up:/system/etc/ppp/ip-up \
	external/ppp/dns_conf/ip-down:/system/etc/ppp/ip-down \
	device/actions/lemaker_guitar_bbb/test_policy:system/etc/test_policy \
	device/actions/lemaker_guitar_bbb/init.readahead.rc:root/init.readahead.rc

# core hardware features
PRODUCT_COPY_FILES += \
	frameworks/native/data/etc/tablet_core_hardware.xml:system/etc/permissions/tablet_core_hardware.xml \
	frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
	frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
	frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml \
	frameworks/native/data/etc/android.hardware.ethernet.xml:system/etc/permissions/android.hardware.ethernet.xml \
	frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
	frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
	frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
	frameworks/native/data/etc/android.hardware.camera.autofocus.xml:system/etc/permissions/android.hardware.camera.autofocus.xml \
	frameworks/native/data/etc/android.hardware.bluetooth.xml:system/etc/permissions/android.hardware.bluetooth.xml \
	frameworks/native/data/etc/android.hardware.bluetooth_le.xml:system/etc/permissions/android.hardware.bluetooth_le.xml \
	frameworks/native/data/etc/android.hardware.bluetooth.xml:system/etc/permissions/extras/android.hardware.bluetooth.xml \
	frameworks/native/data/etc/android.hardware.bluetooth_le.xml:system/etc/permissions/extras/android.hardware.bluetooth_le.xml \
	frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/extras/android.hardware.location.gps.xml \
	frameworks/native/data/etc/android.hardware.sensor.compass.xml:/system/etc/permissions/extras/android.hardware.sensor.compass.xml \
	frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/extras/android.hardware.sensor.gyroscope.xml \
	frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/extras/android.hardware.sensor.light.xml

# core software features
PRODUCT_COPY_FILES += \
	frameworks/native/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml \
	frameworks/native/data/etc/android.software.connectionservice.xml:system/etc/permissions/android.software.connectionservice.xml \
	frameworks/native/data/etc/android.software.webview.xml:system/etc/permissions/android.software.webview.xml \
	packages/wallpapers/LivePicker/android.software.live_wallpaper.xml:system/etc/permissions/android.software.live_wallpaper.xml \
	device/actions/lemaker_guitar_bbb/readme:system/vendor/app/readme


ifeq ($(R_ANDROID_DPI),)
	R_ANDROID_DPI:=mdpi
endif

#720p(screen sizes 7 inches) hdpi
#PRODUCT_AAPT_CONFIG += normal large mdpi hdpi tvdpi xhdpi
#ywwang:  notice, we could not robustly pack all density resource, otherwise zygote wiil fail to cache resouces.... 
#songzhining: notice, the platform dosen't contain all of the bitmaps at ldpi/tvdpi, so we add hdpi res for tvdpi config, and add mdpi res for ldpi config.
ifeq ($(strip $(R_ANDROID_DPI)), tvdpi)
PRODUCT_AAPT_CONFIG += normal large $(R_ANDROID_DPI) hdpi
else ifeq ($(strip $(R_ANDROID_DPI)), ldpi)
PRODUCT_AAPT_CONFIG += normal large $(R_ANDROID_DPI) mdpi
else
PRODUCT_AAPT_CONFIG += normal large $(R_ANDROID_DPI)
endif
PRODUCT_AAPT_PREF_CONFIG := $(R_ANDROID_DPI)

PRODUCT_CHARACTERISTICS := tablet
PRODUCT_TAGS += dalvik.gc.type-precise
PRODUCT_RUNTIMES := runtime_libart_default
DEVICE_PACKAGE_OVERLAYS := device/actions/lemaker_guitar_bbb/overlay

# add for pdk
PRODUCT_PACKAGES += \
    Launcher2 \
    FusedLocation \
    InputDevices \
    Keyguard \
    LatinIME \
    Phone \
    PrintSpooler \
    Provision \
    CtsAutoSetting \
    Settings \
    SystemUI \
    TeleService \
    WAPPushManager \
    audio \
    audio_policy.default \
    audio.primary.default \
    audio.r_submix.default \
    com.android.future.usb.accessory \
    hostapd \
    librs_jni \
    libvideoeditor_core \
    libvideoeditor_jni \
    libvideoeditor_osal \
    libvideoeditorplayer \
    libvideoeditor_videofilters \
    lint \
    local_time.default \
    network \
    pand \
    power.default \
    sdptool \
    vibrator.default \
    wpa_supplicant.conf \
    libGLES_android

#actions framework
PRODUCT_PACKAGES += \
    actions \
    libperformance \
    libactions_runtime \
	charger \
	charger_res_images \
	SpeechRecorder \
	libsrec_jni \
	update   \
	ActExplore \
	ActSensorCalib \
	e2fsck \
    make_ext4fs \
    mkfs.f2fs \
    fsck.f2fs \
	dosfslabel \
	Bluetooth \
	PartnerBookmarksProvider \
	com.android.future.usb.accessory \
	libdisplay.S500 \
    lights.S500 \
    sensors.S500 \
    camera.S500 \
    libhardware_legacy

#for actions omx component audio
PRODUCT_PACKAGES += \
	libstagefrighthw \
	libOMX.Action.Video.Decoder \
	libOMX.Action.Audio.Decoder \
	libOMX.Action.Video.Decoder.Deinterlace \
	libOMX.Action.Video.Encoder \
	libOMX.Action.Video.Camera \
	libOMX_Core \
	libACT_V4L2HAL 
	

#gpu related
PRODUCT_PACKAGES += \
	gpu_config \
	libpl \
	acc_policy \
	acc_implement \
	game_r2 \
	game_r3 \
	i2cdetect \
	actionsframework
	
PRODUCT_PACKAGES += \
	libjni_mosaic 

#video codec	
PRODUCT_PACKAGES += \
	libskia_actions_opt \
	libsub \
	libbmp \
	id_jpg\
	adAPE \
	adPCM \
	apAPE \
	apWAV \
	avd_avi \
	avd_flv \
	avd_mpg \
	avd_ts \
	vd_flv1 \
	vd_h263 \
	vd_h264 \
	vd_hevc \
	vd_hevc_91 \
	vd_mjpg \
	vd_mpeg \
	vd_vp9 \
	vd_xvid \
	libACT_FD \
	libAutoFocus \
	libvde_core \
	libbacklight \
	libimg_en \
	libACT_EncAPI \
	libACT_VceResize \
	libhdri
	

	

ifeq ($(strip $(R_WIFI_TYPE)), rtl8723bu)
PRODUCT_PACKAGES += \
	dhcpcd.conf \
	init.wifi.rc \
	lib_driver_cmd_rtl \
	hostapd \
	hostapd_cli \
	wpa_cli \
	libwpa_client \
	wpa_supplicant \
	wpa_supplicant.conf \
	rtw_fwloader	\
	wpa_supplicant_overlay.conf \
	p2p_supplicant_overlay.conf
endif

ifeq ($(strip $(R_BT_TYPE)), rtl8723bu)
    BLUETOOTH_HCI_USE_RTK_HN := h4
    PRODUCT_PACKAGES += \
        libbt-vendor \
        audio.a2dp.default \
        rtl8723bu_config \
        rtl8723b_fw
endif

ifeq ($(strip $(R_WIFI_TYPE)), rtl8723bs)
PRODUCT_PACKAGES += \
        dhcpcd.conf \
        init.wifi.rc \
        lib_driver_cmd_rtl \
        hostapd \
        hostapd_cli \
        wpa_cli \
        libwpa_client \
        wpa_supplicant \
        wpa_supplicant.conf \
        rtw_fwloader    \
        wpa_supplicant_overlay.conf \
        p2p_supplicant_overlay.conf
endif

#ifeq ($(strip $(R_BT_TYPE)), rtl8723bs)
#    BLUETOOTH_HCI_USE_RTK_HN := h4
#    PRODUCT_PACKAGES += \
#        libbt-vendor-rtl8723bu \
#        audio.a2dp.default \
#        rtl8723bu_config \
#       rtl8723bu_fw\
#        libbt-vendor-rtl8723bs \
#        rtk8723_bt_config \
#        rtl8723b_fw
#endif

ifeq ($(strip $(R_BT_TYPE)), rtl8723bs)
    BLUETOOTH_HCI_USE_RTK_HN := h4
    PRODUCT_PACKAGES += \
        libbt-vendor-rtl8723bu \
        audio.a2dp.default \
        rtl8723bu_config \
        rtl8723bu_fw\
        libbt-vendor-rtl8723bs \
        rtk8723_bt_config \
        rtl8723b_fw
endif


#for network debug
PRODUCT_PACKAGES += \
	ping \
	netperf \
	netserver \
	tcpdump \
	wpa_cli \
	strace \
	rild \
	libactions-ril \
	usb_modeswitch \
	usb_modeswitch.d \
	libusb \
	libusb-compat
	
# audio config	
PRODUCT_COPY_FILES += \
    frameworks/av/media/libeffects/data/audio_effects.conf:system/etc/audio_effects.conf \
    frameworks/av/services/audiopolicy/audio_policy.conf:system/etc/audio_policy.conf \
    frameworks/av/media/libstagefright/data/media_codecs_google_audio.xml:system/etc/media_codecs_google_audio.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_telephony.xml:system/etc/media_codecs_google_telephony.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_video.xml:system/etc/media_codecs_google_video.xml
    
$(call inherit-product, frameworks/native/build/tablet-7in-hdpi-1024-dalvik-heap.mk)
$(call inherit-product, build/target/product/core_base.mk)
$(call inherit-product-if-exists, frameworks/webview/chromium/chromium.mk)
$(call inherit-product-if-exists, frameworks/base/data/keyboards/keyboards.mk)
$(call inherit-product-if-exists, frameworks/base/data/fonts/fonts.mk)
$(call inherit-product-if-exists, frameworks/base/data/sounds/AudioPackage5.mk)
$(call inherit-product-if-exists, device/actions/common/prebuilt/widevine/widevine.mk)

PRODUCT_PACKAGES += \
	libgpu_prebuilt \
	superuser_prebuilt \
	actions_prebuilt_apks \
	libutils_prebuilt \
	ActionsCamera \
	Actionslogcat \
	actionslogcat \
	PinyinIME \
	getevent_iio \
	inv_self_test-shared \
	mbrc_checksum \
	check_app \
	upgrade_app \
	pad_vm \
	libgralloc \
	libtinyxml \
	recovery \
	updater \
	Camera2 \
    boot_e2fsck \
    boot_driver \
    system_xbin \
    system_etc \
    treadahead \
	locales_list.txt


PRODUCT_COPY_FILES += \
	device/actions/lemaker_guitar_bbb/init.rc:root/init.rc \
	device/actions/lemaker_guitar_bbb/init.gs705a.rc:root/init.gs705a.rc \
	device/actions/lemaker_guitar_bbb/init.gs705a.usb.rc:root/init.gs705a.usb.rc \
	device/actions/lemaker_guitar_bbb/init.recovery.gs705a.rc:root/init.recovery.gs705a.rc \
	device/actions/lemaker_guitar_bbb/init.boot.rc:root/init.boot.rc \
	device/actions/lemaker_guitar_bbb/init.eth0.rc:root/init.eth0.rc \
	device/actions/lemaker_guitar_bbb/fstab.gs705a:root/fstab.gs705a \
	device/actions/lemaker_guitar_bbb/fstab.sd2:root/fstab.sd2 \
	device/actions/lemaker_guitar_bbb/fstab.sd0:root/fstab.sd0 \
	device/actions/lemaker_guitar_bbb/fstab.sdboot.gs705a:root/fstab.sdboot.gs705a \
	device/actions/lemaker_guitar_bbb/ueventd.rc:root/ueventd.rc \
	device/actions/lemaker_guitar_bbb/ueventd.gs705a.rc:root/ueventd.gs705a.rc \
	device/actions/lemaker_guitar_bbb/config/root/init.modules.rc:root/init.modules.rc \
	device/actions/lemaker_guitar_bbb/config/root/init.storage.nand.rc:root/init.storage.nand.rc \
	device/actions/lemaker_guitar_bbb/config/root/init.storage.sd.rc:root/init.storage.sd.rc \
	device/actions/lemaker_guitar_bbb/config/root/init.extra_modules.rc:root/init.extra_modules.rc \
	device/actions/lemaker_guitar_bbb/config/root/cp_vendor_app.sh:root/cp_vendor_app.sh \
	device/actions/lemaker_guitar_bbb/config/root/insmod_camera.sh:root/insmod_camera.sh \
	device/actions/lemaker_guitar_bbb/config/root/insmod_ctp.sh:root/insmod_ctp.sh \
	device/actions/lemaker_guitar_bbb/config/root/LMKNetwork.sh:root/LMKNetwork.sh \
	device/actions/lemaker_guitar_bbb/config/root/insmod_gsensor.sh:root/insmod_gsensor.sh \
	device/actions/lemaker_guitar_bbb/config/root/LMKBoardSelect.sh:root/LMKBoardSelect.sh \
	device/actions/lemaker_guitar_bbb/config/root/usbmond.sh:root/usbmond.sh

PRODUCT_COPY_FILES += \
	device/actions/lemaker_guitar_bbb/config/system/media_profiles.xml:system/etc/media_profiles.xml \
	device/actions/lemaker_guitar_bbb/config/system/android.hardware.bluetooth.xml:system/etc/permissions/android.hardware.bluetooth.xml \
	device/actions/lemaker_guitar_bbb/config/system/default_wallpaper.png:system/etc/wallpaper/default_wallpaper.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_00.png:system/etc/wallpaper/wallpaper_00.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_01.png:system/etc/wallpaper/wallpaper_01.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_02.png:system/etc/wallpaper/wallpaper_02.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_03.png:system/etc/wallpaper/wallpaper_03.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_04.png:system/etc/wallpaper/wallpaper_04.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_05.png:system/etc/wallpaper/wallpaper_05.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_06.png:system/etc/wallpaper/wallpaper_06.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_07.png:system/etc/wallpaper/wallpaper_07.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_08.png:system/etc/wallpaper/wallpaper_08.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_09.png:system/etc/wallpaper/wallpaper_09.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_010.png:system/etc/wallpaper/wallpaper_010.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_011.png:system/etc/wallpaper/wallpaper_011.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_012.png:system/etc/wallpaper/wallpaper_012.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_00_small.png:system/etc/wallpaper/wallpaper_00_small.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_01_small.png:system/etc/wallpaper/wallpaper_01_small.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_02_small.png:system/etc/wallpaper/wallpaper_02_small.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_03_small.png:system/etc/wallpaper/wallpaper_03_small.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_04_small.png:system/etc/wallpaper/wallpaper_04_small.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_05_small.png:system/etc/wallpaper/wallpaper_05_small.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_06_small.png:system/etc/wallpaper/wallpaper_06_small.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_07_small.png:system/etc/wallpaper/wallpaper_07_small.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_08_small.png:system/etc/wallpaper/wallpaper_08_small.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_09_small.png:system/etc/wallpaper/wallpaper_09_small.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_010_small.png:system/etc/wallpaper/wallpaper_010_small.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_011_small.png:system/etc/wallpaper/wallpaper_011_small.png \
	device/actions/lemaker_guitar_bbb/config/system/wallpaper_012_small.png:system/etc/wallpaper/wallpaper_012_small.png \
	device/actions/lemaker_guitar_bbb/config/root/build_bba.prop:system/build_bba.prop \
	device/actions/lemaker_guitar_bbb/config/root/build_bbb.prop:system/build_bbb.prop \
	device/actions/lemaker_guitar_bbb/config/root/build_bbb_plus.prop:system/build_bbb_plus.prop \
	device/actions/lemaker_guitar_bbb/config/root/build_bbc.prop:system/build_bbc.prop \
	device/actions/lemaker_guitar_bbb/config/root/build_bbd.prop:system/build_bbd.prop \
	device/actions/lemaker_guitar_bbb/config/system/gslX680.idc:system/usr/idc/gslX680.idc

ADDITIONAL_BUILD_PROPERTIES += \
	persist.demo.hdmirotationlock=true \
	ro.camerahal.configorientation=90 \
	ro.camerahal.single_vsize=0 \
	ro.camerahal.prevres0=SVGA,HD \
	ro.camerahal.imageres0=VGA,SVGA,2M \
	ro.camerahal.prevresdft0=SVGA \
	ro.camerahal.imageresdft0=2M \
	ro.camerahal.fpsdft0=30 \
	ro.camerahal.prevres1=QVGA,VGA \
	ro.camerahal.imageres1=QVGA,VGA \
	ro.camerahal.prevresdft1=VGA \
	ro.camerahal.imageresdft1=VGA \
	ro.camerahal.fpsdft1=30 \
	camcorder.settings.xml=/data/camera/camcorder_profiles.xml \
	ro.camerahal.uvc_replacemode=1 \
	ro.camerahal.hdr0=1 \
	ro.camerahal.hdr1=1 \
	ro.bootmusic.enable=0 \
	ro.hdmi.onoffmode=auto \
	ro.customer.boot.music.rec=false \
	dalvik.vm.heapstartsize=8m \
	dalvik.vm.heapsize=384m \
	dalvik.vm.heaptargetutilization=0.75 \
	dalvik.vm.heapminfree=2m \
	dalvik.vm.heapmaxfree=8m \
	ro.sf.lcd_density=160 \
	ro.settings.support.bluetooth=true \
	system.ctl.recoverywhencrash=4 \
	system.ctl.poweroffwhencrash=2 \
	ro.customer.3glist=0 \
	ro.customer.show.reboot.dlg=true \
	ro.ota.server=http://ota.actions-semi.net/AD500A/ \
	persist.sys.extra_features=1 \
	ro.change_property.enable=true \
	persist.service.adb.enable=1 \
	ro.settings.datausage=true \
	ro.settings.hotspot=true \
	ro.settings.mobilenetworks=true \
	ro.settings.phonestatus=true \
	ro.g3.display=true \
	ro.support.eth0=false \
        ro.support.eth1=false \
	ro.airplanemode.display=true \
	ro.settings.support.ethernet=true \
	ro.settings.compatibility=false \
	ro.settings.support.gps=false \
	ro.camerahal.hangle0=100.0 \
	ro.camerahal.hangle1=100.0 \
	ro.sf.hwrotation=270 \
	ro.sf.default_rotation=1 \
	ro.net.config=0 \
	persist.sys.shutok=init \
	ro.adb.secure=0 \
	ro.config.hdmi_secure_check=0 \
	ro.browser.search_engin=baidu \
	ro.skia.img.decode.standard=ACTIONS \
	ro.settings.eth0=true \
        ro.settings.eth1=false \
	ro.phone.mode=PHONE 
    

PRODUCT_PROPERTY_OVERRIDES += \
	ro.device.model=S500 \
	ro.product.model=guitar \
	ro.product.brand=Actions \
	ro.product.name=Demo \
	ro.product.device=guitar \
	ro.product.board=lemaker_guitar_bbb \
	ro.product.manufacturer=Actions \
	ro.build.display.id=TAG_AD500A_5110_$(VERSION_DATE) \
    ro.carrier=unknown \
    ro.com.android.dateformat=MM-dd-yyyy \
    ro.config.ringtone=Ring_Synth_04.ogg \
    ro.config.notification_sound=pixiedust.ogg \
	ro.product.locale.language=en \
	ro.product.locale.region=US \
	dalvik.vm.dexopt-flags=m=y \
	dalvik.vm.checkjni=false \
	dalvik.vm.heapgrowthlimit=128m \
	hwui.render_dirty_regions=false


PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
  persist.sys.usb.config=mass_storage
ADDITIONAL_DEFAULT_PROPERTIES+= \
	ro.debuggable=1 \
	ro.sf.hwrotation=270 \
	ro.allow.mock.location=0 \
	ro.sf.default_rotation=1 \
	ro.setupwizard.mode=DISABLE \
	persist.sys.timezone=Asia/Shanghai \
	persist.sys.strictmode.disable=true \


ifeq ($(R_GMS_TYPE),)
    R_GMS_TYPE:=base
endif

ifneq ($(R_GMS_TYPE),full)
#OwlBrowser, GMS don't need flashplayer
PRODUCT_PACKAGES += owlbrowser_package
endif

ifeq ($(R_GMS_TYPE),core)
$(call inherit-product-if-exists,device/actions/common/prebuilt/apk/google/products/gms_core.mk)
else ifeq ($(R_GMS_TYPE),base)
$(call inherit-product-if-exists,device/actions/common/prebuilt/apk/google/products/gms_base.mk)
else ifeq ($(R_GMS_TYPE),advance)
$(call inherit-product-if-exists,device/actions/common/prebuilt/apk/google/products/gms_advance.mk)
else ifeq ($(R_GMS_TYPE),full)
$(call inherit-product-if-exists,device/actions/common/prebuilt/apk/google/products/gms.mk)
else
$(call inherit-product-if-exists,device/actions/common/prebuilt/apk/google/products/gms_base.mk)
endif

endif

