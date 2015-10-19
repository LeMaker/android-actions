DM_ROOT     := $(CURDIR)/..
DM_ENGINE   := $(DM_ROOT)/engine
DM_SERVICE  := $(DM_ROOT)/DMService
DM_SETTING  := $(DM_ROOT)/setting
DM_TOOLS    := $(DM_ENGINE)/dmlib/dmt-tools
DM_ASSETS   := $(DM_SERVICE)/assets
DM_PLUGIN   := $(DM_ASSETS)/dm/plugin/
DM_GENERATE := $(DM_TOOLS)/generateDMT
DM_FSTAB    := $(DM_ROOT)/setting/fstab
DM_TREE_ZIP := $(DM_ROOT)/setting/Dmt.zip

MY_TMP      := $(DM_SETTING)/tmp2
MY_TREE     := $(MY_TMP)/Dmt
MY_OUT      := $(MY_TMP)/output

all: 
	@echo building all
	-rm -rf $(MY_TMP)
	-mkdir $(MY_TMP)
	cd $(MY_TMP); unzip $(DM_TREE_ZIP)
	cd $(DM_TOOLS); bash $(DM_GENERATE) -fstab $(DM_FSTAB) -output $(MY_OUT) -dmt $(MY_TREE)
	-rm -rf $(DM_SETTING)/dmt_data/*
	cp -r $(MY_OUT)/treedata/*    $(DM_SETTING)/dmt_data/
	cp $(MY_OUT)/treedata/fstab   $(DM_ASSETS)/dm/dmt_data/fstab
	cp $(MY_OUT)/treedata/*.wbxml $(DM_ASSETS)/dm/dmt_data/
	cp $(MY_OUT)/treedata/*.dat   $(DM_ASSETS)/dm/dmt_data/
	cp $(MY_OUT)/treedata/*.bmdf  $(DM_ASSETS)/dm/dmt_data/
	-rm -rf $(MY_TMP)

