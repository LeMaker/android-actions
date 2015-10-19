#author huixu

ifeq ($(strip $(R_FIRMWARE_ROOTED)), true)
	_ACT_APK_PREBUILT_EXCLUSION :=
	_ACT_APK_PREBUILT_EXCLUSION_DIR :=
else
	_ACT_APK_PREBUILT_EXCLUSION := 
	_ACT_APK_PREBUILT_EXCLUSION_DIR := thirdparty/superuser
endif
	
