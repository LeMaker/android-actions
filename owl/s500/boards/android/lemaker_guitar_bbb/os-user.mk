
#candidate: userdebug, eng, user
lunch_type=user

R_OS_DIR=$(TOP_DIR)/../android
FW_RECOVERY=$(OUT_DIR)/recovery
BOOTBIN_DIR=$(R_OS_DIR)/device/actions/$(BOARD_NAME)/bootbin
PRODUCT_OUT=$(R_OS_DIR)/out/target/product/$(BOARD_NAME)


.PHONY: system recovery ota

system:
	$(Q)rm -rf $(MISC_DIR)/modules
	$(Q)mkdir -p $(MISC_DIR)/modules
	$(Q)mkdir -p $(IMAGE_DIR)
	$(Q)find $(KERNEL_OUT_DIR) -name '*.ko' | xargs cp -t $(MISC_DIR)/modules
	$(Q)$(CROSS_COMPILE)strip --strip-unneeded $(MISC_DIR)/modules/*.ko
	$(Q)cp $(MISC_DIR)/modules/atc260x-adckeypad.ko $(R_OS_DIR)/device/actions/$(BOARD_NAME)/config/recovery/
	$(Q)cd $(R_OS_DIR) && source build/envsetup.sh && lunch $(BOARD_NAME)-$(lunch_type) && $(MAKE) -j$(CPUS)
	$(Q)$(TOOLS_DIR)/utils/mkimage -n "RAMFS" -A arm -O linux -T ramdisk -C none -a 02000000 -e 02000000 -d $(PRODUCT_OUT)/ramdisk.img $(MISC_DIR)/ramdisk.img
	$(Q)cp $(PRODUCT_OUT)/system.img $(IMAGE_DIR)/
	$(Q)mkdir -p $(FW_RECOVERY)
	$(Q)$(TOOLS_DIR)/utils/mkimage -n "RAMFS" -A arm -O linux -T ramdisk -C none -a 02000000 -e 02000000 -d $(PRODUCT_OUT)/ramdisk-recovery.img $(FW_RECOVERY)/ramdisk.img

recovery:
	@echo "-- Build Fat Recovery image --"
	$(Q)mkdir -p $(FW_RECOVERY)
	$(Q)cp -r $(BOARD_CONFIG_DIR)/misc/* $(FW_RECOVERY)/
	$(Q)cp $(KERNEL_OUT_DIR)/arch/$(ARCH)/boot/uImage $(FW_RECOVERY)
	$(Q)cp $(KERNEL_OUT_DIR)/arch/$(ARCH)/boot/dts/$(KERNEL_DTS).dtb $(FW_RECOVERY)/kernel.dtb
	$(Q)cp $(KERNEL_OUT_DIR)/arch/$(ARCH)/boot/dts/lemaker_guitar_bb*.dtb $(FW_RECOVERY)/
	
	@echo "--Fix vmlinux.bin.."
	$(Q)dd if=/dev/zero of=$(IMAGE_DIR)/recovery.img bs=1M count=$(RECOVERY_IMAGE_SIZE)
	$(Q)$(TOOLS_DIR)/utils/makebootfat -o $(IMAGE_DIR)/recovery.img -L recovery -b $(SCRIPT_DIR)/bootsect.bin $(FW_RECOVERY)

ota:
	@echo "-- Build ota --"
	$(Q)mkdir -p $(BOOTBIN_DIR)
	$(Q)cp $(IMAGE_DIR)/misc.img $(BOOTBIN_DIR)
	$(Q)cp $(IMAGE_DIR)/recovery.img $(BOOTBIN_DIR)
	$(Q)cd $(R_OS_DIR) && source build/envsetup.sh && lunch $(BOARD_NAME)-$(lunch_type) && $(MAKE) otapackage -j$(CPUS)
	$(Q)cp $(PRODUCT_OUT)/*.zip $(IMAGE_DIR)/
	$(Q)cp $(PRODUCT_OUT)/obj/PACKAGING/target_files_intermediates/*.zip $(IMAGE_DIR)/

initramfs:
	echo ""

rootfs: system recovery misc ota

rootfs_clean:
	$(Q)cd $(R_OS_DIR) && source build/envsetup.sh && lunch $(BOARD_NAME)-$(lunch_type) && $(MAKE) clean
