#author huixu
LOCAL_PATH:= $(call my-dir)
_PREBUILT_LOCAL_PATH:= $(LOCAL_PATH)


define _act-apk-find-subdir-files
$(call find-subdir-files,$(1) -name "*" -and -not -name "*x86*" -and -not -name "*.mk" -and -not -name ".git" -and -not -path "*x86*" -and -not -type d)
endef

define _act-apk-find-subdir-lib
$(if $(shell cd $(_PREBUILT_LOCAL_PATH); if [ -d $(1)/lib ];  then  echo "exist"; else echo ""; fi), \
	$(call find-subdir-files,$(1)/lib -name "*.so" -and -not -name "*x86*" -and -not -name ".git" -and -not -path "*x86*" -and -not -type d),\
	$(warning "not exist $(1)/lib") \
)
endef

define _act-apk-find-subdir-app
$(if $(shell cd $(_PREBUILT_LOCAL_PATH); if [ -d $(1)/app ];  then  echo "exist"; else echo ""; fi), \
	$(call find-subdir-files,$(1)/app -name "*.apk" -and -not -name "*x86*" -and -not -name ".git" -and -not -path "*x86*" -and -not -type d),\
	$(warning "not exist $(1)/app") \
)
endef

define _act-apk-find-subdir-priv-app
$(if $(shell cd $(_PREBUILT_LOCAL_PATH); if [ -d $(1)/priv-app ];  then  echo "exist"; else echo ""; fi), \
	$(call find-subdir-files,$(1)/priv-app -name "*.apk" -and -not -name "*x86*" -and -not -name ".git" -and -not -path "*x86*" -and -not -type d),\
	$(warning "not exist $(1)/priv-app") \
)
endef

define _act-apk-find-subdir-xbin
$(if $(shell cd $(_PREBUILT_LOCAL_PATH); if [ -d $(1)/xbin ];  then  echo "exist"; else echo ""; fi), \
	$(call find-subdir-files,$(1)/xbin -name "*" -and -not -name "*x86*" -and -not -name ".git" -and -not -path "*x86*" -and -not -type d),\
	$(warning "not exist $(1)/xbin") \
)
endef

define _act-apk-exclusion-dir
$(filter-out $(1),$(2))
endef

define _act-apk-find-subdir-dir
$(filter-out $(1),$(call find-subdir-files,$(1) -maxdepth 1 -type d))
endef




#install actions and thirdparty apks
define _act-apk-add-prebuilt
include $$(CLEAR_VARS)
LOCAL_MODULE := prebuild_apk_$(patsubst %.apk,%,$(notdir $(1)))
LOCAL_MODULE_STEM := $(notdir $(1))
_act_apk_prebuilt_modules += $$(LOCAL_MODULE)
LOCAL_SRC_FILES := $1
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := $(2)
LOCAL_CERTIFICATE := $(3)
LOCAL_MODULE_PATH := $(4)
LOCAL_PRIVILEGED_MODULE := $(5)
include $$(BUILD_PREBUILT)
endef

define _act-apk-add-prebuilt-apk
$(if $(findstring GoogleBackupTransport.apk,$(1)),				\
	$(eval $(call _act-apk-add-prebuilt,$(_apk),APPS,platform,,true)),	\
	$(eval $(call _act-apk-add-prebuilt,$(_apk),APPS,PRESIGNED,,$(2))))
endef

define _act-apk-add-prebuilt-so
$(eval $(call _act-apk-add-prebuilt,$(1),SHARED_LIBRARIES,,))
endef

define _act-apk-add-prebuilt-other
$(eval $(call _act-apk-add-prebuilt,$(1),ETC,,$(2)))
endef

define _act-apk-add-prebuilt-all-so
$(foreach _so, $(call _act-apk-find-subdir-lib, $(1)), \
  	$(eval $(call _act-apk-add-prebuilt-so,$(_so))))  
endef

define _act-apk-add-prebuilt-all-apk
$(foreach _apk, $(call _act-apk-find-subdir-app, $(1)), \
  	$(eval $(call _act-apk-add-prebuilt-apk,$(_apk),false))) \
$(foreach _apk, $(call _act-apk-find-subdir-priv-app, $(1)), \
  	$(eval $(call _act-apk-add-prebuilt-apk,$(_apk),true)))  
endef

define _act-apk-add-prebuilt-all-xbin
$(foreach _other, $(call _act-apk-find-subdir-xbin, $(1)), \
  	$(eval $(call _act-apk-add-prebuilt-other,$(_other),$(TARGET_OUT)/xbin)))  
endef


include $(CLEAR_VARS)

include $(_PREBUILT_LOCAL_PATH)/apk_config.mk

_act_apk_prebuilt_modules :=

_ACT_APK_SUBDIRS := $(call _act-apk-find-subdir-dir,actions) 
_ACT_APK_SUBDIRS += $(call _act-apk-find-subdir-dir,thirdparty) 
_ACT_APK_SUBDIRS := $(shell echo $(_ACT_APK_SUBDIRS))

_ACT_APK_SUBDIRS := $(call _act-apk-exclusion-dir,$(_ACT_APK_PREBUILT_EXCLUSION_DIR),$(_ACT_APK_SUBDIRS))

_ACT_APK_NOT_SCAN_SUBDIRS := 
$(foreach _dir,$(_ACT_APK_SUBDIRS),		\
	$(eval _not_scan_dir:=$(strip $(_dir)))                         \
	$(if $(shell cd $(_PREBUILT_LOCAL_PATH); if [ -f $(_dir)/Android.mk ] ; then echo "found"; else echo ""; fi),  \
		$(eval _ACT_APK_NOT_SCAN_SUBDIRS+=$(_dir)),))                                   \


_ACT_APK_SUBDIRS := $(call _act-apk-exclusion-dir,$(_ACT_APK_NOT_SCAN_SUBDIRS),$(_ACT_APK_SUBDIRS))



#install apks in actions thirdparty
$(foreach _dir,$(_ACT_APK_SUBDIRS),		\
	$(eval _add_dir:=$(strip $(_dir)))                         \
	$(if $(findstring $(_add_dir),$(_ACT_APK_PREBUILT_EXCLUSION)),  \
		$(eval _add_dir:=),)                                   \
	$(if $(_add_dir),                                           \
		$(call _act-apk-add-prebuilt-all-apk,$(_add_dir))		\
		$(call _act-apk-add-prebuilt-all-so,$(_add_dir))		\
		$(call _act-apk-add-prebuilt-all-xbin,$(_add_dir))		\
		,))


#just include mk in subdirs
$(foreach _dir,$(_ACT_APK_NOT_SCAN_SUBDIRS),		\
	$(eval include $(_PREBUILT_LOCAL_PATH)/$(_dir)/Android.mk)  \
	)


_act_apk_prebuilt_modules += superuser_prebuilt iWnnIME
_act_apk_prebuilt_modules += OWLPlayer AdobeFlashPlayer webkit42_xml webkit42_jar libwebcore libactmd5 webcore_ttf

-include $(_PREBUILT_LOCAL_PATH)/preferred-apps/Android.mk 
_act_apk_prebuilt_modules += preferred_apps_xml

include $(CLEAR_VARS)
LOCAL_MODULE := actions_prebuilt_apks
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(_act_apk_prebuilt_modules)
include $(BUILD_PHONY_PACKAGE)


#gms
include $(CLEAR_VARS)
$(warning "_PREBUILT_LOCAL_PATH=$(_PREBUILT_LOCAL_PATH)")
include $(_PREBUILT_LOCAL_PATH)/google/Android.mk
