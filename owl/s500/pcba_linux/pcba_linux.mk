PCBA_LINUX_OUT_DIR=$(OUT_DIR)/pcba_linux
PCBA_LINUX_PREBUILT_DIR=$(TOP_DIR)/$(IC_NAME)/pcba_linux
INITRAMFS_PCBA_LINUX=$(PCBA_LINUX_OUT_DIR)/initramfs
FW_MISC_PCBA_LINUX=$(PCBA_LINUX_OUT_DIR)/fwmisc
IMAGE_PCBA_LINUX=$(PCBA_LINUX_OUT_DIR)/image_pcba

MISC_PCBA_LINUX_IMAGE_SIZE=80

TOOLCHAIN=/opt/arm-2011.09
TOOLCHAIN_LIB=${TOOLCHAIN}/arm-none-linux-gnueabi/libc

install_toolchain_libc=	cp -a $(1)/lib/libc* $(2)/lib/; \
	cp -a $(1)/lib/libm* $(2)/lib/; \
	cp -a $(1)/lib/libdl* $(2)/lib/; \
	cp -a $(1)/lib/libpthread* $(2)/lib/; \
    cp -a $(1)/lib/libgcc* $(2)/lib/; \
    cp -a $(1)/usr/lib/libstdc++.so* $(2)/usr/lib/; \
    cp -a $(1)/lib/librt* $(2)/lib/; \
    cp -a $(1)/usr/lib/librt* $(2)/usr/lib/; \
	cp -a $(1)/lib/ld* $(2)/lib/; \
	rm -fr $(2)/lib/*.py&& rm -fr $(2)/usr/lib/*.py; \
	cd $(2)/lib;find . -name "*.so*" -type f|xargs ${STRIP}; \
	cd $(2)/usr/lib;find . -name "*.so*" -type f|xargs ${STRIP}; 

.PHONY: pcba_linux_clean pcba_linux_prepare

###pcba ####
pcba_linux_clean:
	@echo "-- PCBA Linux: clean -- "
	$(Q)rm -rf $(INITRAMFS_PCBA_LINUX)
	$(Q)rm -rf $(FW_MISC_PCBA_LINUX)
	
pcba_linux_prepare:
	@echo "-- PCBA Linux: prepare -- "
	$(Q)mkdir -p $(INITRAMFS_PCBA_LINUX)
	$(Q)mkdir -p $(FW_MISC_PCBA_LINUX)
	$(Q)mkdir -p $(IMAGE_PCBA_LINUX)
	
	$(Q)$(SCRIPT_DIR)/populate_dir $(INITRAMFS_PCBA_LINUX)
	$(Q)cp -r $(UPRAMFS_ROOTFS)/* $(INITRAMFS_PCBA_LINUX)
	$(Q)cp -r $(MISC_DIR)/* $(FW_MISC_PCBA_LINUX)
	$(Q)cp -r $(PCBA_LINUX_PREBUILT_DIR)/initramfs $(INITRAMFS_PCBA_LINUX)/..
	$(Q)cp -r $(PCBA_LINUX_PREBUILT_DIR)/fwmisc/*  $(FW_MISC_PCBA_LINUX)
	$(Q)$(call install_toolchain_libc,$(TOOLCHAIN_LIB),$(INITRAMFS_PCBA_LINUX))
	##build pcba test bin
	$(Q)echo "--Build pcba.."
	$(Q)cd $(PCBA_LINUX_PREBUILT_DIR)/pcba && make
	##make ramdisk.img
	$(Q)echo "--Build ramdisk.img.."
	$(Q)$(SCRIPT_DIR)/gen_initramfs_list.sh -u 0 -g 0 $(INITRAMFS_PCBA_LINUX) > $(PCBA_LINUX_OUT_DIR)/initramfs_pcba_linux.list
	$(Q)${SCRIPT_DIR}/gen_init_cpio $(PCBA_LINUX_OUT_DIR)/initramfs_pcba_linux.list > ${PCBA_LINUX_OUT_DIR}/ramdisk.img.tmp
	$(Q)$(TOOLS_DIR)/utils/mkimage -n "RAMFS" -A arm -O linux -T ramdisk -C none -a 02000000 -e 02000000 -d \
		${PCBA_LINUX_OUT_DIR}/ramdisk.img.tmp ${FW_MISC_PCBA_LINUX}/ramdisk.img
	$(Q)rm ${PCBA_LINUX_OUT_DIR}/initramfs_pcba_linux.list
	$(Q)rm ${PCBA_LINUX_OUT_DIR}/ramdisk.img.tmp
	##make misc.img
	$(Q)echo "--Build misc.img.."
	$(Q)dd if=/dev/zero of=$(IMAGE_PCBA_LINUX)/misc.img bs=1M count=$(MISC_PCBA_LINUX_IMAGE_SIZE)
	$(Q)$(TOOLS_DIR)/utils/makebootfat -o $(IMAGE_PCBA_LINUX)/misc.img -L misc -b $(SCRIPT_DIR)/bootsect.bin $(FW_MISC_PCBA_LINUX)
	##make fw
	$(Q)cp $(UBOOT_OUT_DIR)/u-boot-dtb.img $(IMAGE_PCBA_LINUX)/
	$(Q)cp $(BOOTLOAD_DIR)/bootloader.bin $(IMAGE_PCBA_LINUX)/
	
	$(Q)cp $(PCBA_LINUX_PREBUILT_DIR)/partition.cfg $(SCRIPT_DIR)/partition.cfg
	$(Q)python $(SCRIPT_DIR)/partition_create.py $(SCRIPT_DIR)/partition.cfg  $(SCRIPT_DIR)/partition_tmp.cfg
	$(Q)sed -i 's/\\boardname\\/\\$(IC_NAME)_$(OS_NAME)_$(BOARD_NAME)\\/' $(SCRIPT_DIR)/partition_tmp.cfg
	$(Q)cp $(SCRIPT_DIR)/fwimage_linux.cfg  $(SCRIPT_DIR)/fwimage_linux_tmp.cfg
	$(Q)sed -i 's/boardname/$(IC_NAME)_$(OS_NAME)_$(BOARD_NAME)/' $(SCRIPT_DIR)/fwimage_linux_tmp.cfg
	
	$(Q)echo "--Build Firmwares.."
	$(Q)cd $(SCRIPT_DIR) && ./linux_build_fw fwimage_linux_tmp.cfg $(IMAGE_PCBA_LINUX) $(FW_NAME)
	$(Q)rm $(SCRIPT_DIR)/partition_tmp.cfg $(SCRIPT_DIR)/partition.cfg $(SCRIPT_DIR)/fwimage_linux_tmp.cfg
	@cd $(IMAGE_PCBA_LINUX) && md5sum *.* > image.md5

pcba_linux: pcba_linux_clean pcba_linux_prepare
