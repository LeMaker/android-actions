

.PHONY: all clean kernel_clean u-boot_clean
.PHONY: kernel u-boot upramfs

CUR_DIR=$(shell pwd)
TOP_DIR=$(CUR_DIR)/../..
CPUS=$$(($(shell cat /sys/devices/system/cpu/present | awk -F- '{ print $$2 }')+1))
#CPUS=1
Q=

include $(TOP_DIR)/.config
include $(CUR_DIR)/boards/$(BOARD_NAME)/config


KERNEL_SRC=$(TOP_DIR)/../kernel
UBOOT_SRC=$(TOP_DIR)/../u-boot
SCRIPT_DIR=$(TOP_DIR)/scripts
TOOLS_DIR=$(TOP_DIR)/tools
BURN_SCRIPT_DIR=$(CUR_DIR)/scripts

OUT_DIR=$(TOP_DIR)/out/$(IC_NAME)_$(OS_NAME)_$(BOARD_NAME)
BURN_DIR=$(OUT_DIR)/burn
CARD_BURN_DIR=$(OUT_DIR)/card_burn
UPRAMFS_ROOTFS=$(BURN_DIR)/upramfs
CARD_BURN_UPRAMFS_ROOTFS=$(CARD_BURN_DIR)/card_burn_upramfs
KERNEL_OUT_DIR=$(BURN_DIR)/kernel
CARD_BURN_KERNEL_OUT_DIR=$(CARD_BURN_DIR)/kernel
UBOOT_OUT_DIR=$(BURN_DIR)/u-boot

CROSS_COMPILE=$(TOP_DIR)/../toolchain/arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
export PATH:=$(TOOLS_DIR)/utils:$(PATH)

all: kernel burn_card_kernel u-boot upramfs card_burn_upramfs
	$(Q)mkdir -p $(BURN_DIR)
	$(Q)cp $(KERNEL_OUT_DIR)/arch/$(ARCH)/boot/uImage $(BURN_DIR)
	$(Q)cp $(KERNEL_OUT_DIR)/arch/$(ARCH)/boot/dts/$(KERNEL_BURN_DTS).dtb $(BURN_DIR)/kernel.dtb
	$(Q)cp $(UBOOT_OUT_DIR)/u-boot-dtb.img $(BURN_DIR)/u-boot-dtb-burn.img
	
	$(Q)cp $(CUR_DIR)/adfudec/adfudec.bin $(BURN_DIR)/
	
	$(Q)cp $(BURN_SCRIPT_DIR)/burn_sys.cfg  $(BURN_SCRIPT_DIR)/burn_sys_tmp.cfg
	$(Q)sed -i 's/boardname/$(IC_NAME)_$(OS_NAME)_$(BOARD_NAME)/' $(BURN_SCRIPT_DIR)/burn_sys_tmp.cfg
	
	$(Q)cd $(BURN_SCRIPT_DIR) && ./build_burn_bin.sh ./burn_sys_tmp.cfg $(CUR_DIR)/burn.bin
	$(Q)rm $(BURN_SCRIPT_DIR)/burn_sys_tmp.cfg

kernel:
	$(Q)mkdir -p $(KERNEL_OUT_DIR)
	$(Q)$(MAKE) -C $(KERNEL_SRC) ARCH=$(ARCH) O=$(KERNEL_OUT_DIR) $(KERNEL_BURN_DEFCONFIG)
	$(Q)$(MAKE) -C $(KERNEL_SRC) CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH) O=$(KERNEL_OUT_DIR) dtbs
	$(Q)$(MAKE) -C $(KERNEL_SRC) CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH) O=$(KERNEL_OUT_DIR) -j$(CPUS) uImage

burn_card_kernel:
	$(Q)mkdir -p $(CARD_BURN_KERNEL_OUT_DIR)
	$(Q)$(MAKE) -C $(KERNEL_SRC) ARCH=$(ARCH) O=$(CARD_BURN_KERNEL_OUT_DIR) $(KERNEL_CARD_BURN_DEFCONFIG)
	$(Q)$(MAKE) -C $(KERNEL_SRC) CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH) O=$(CARD_BURN_KERNEL_OUT_DIR) dtbs
	$(Q)$(MAKE) -C $(KERNEL_SRC) CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH) O=$(CARD_BURN_KERNEL_OUT_DIR) -j$(CPUS) uImage

u-boot:
	$(Q)mkdir -p $(UBOOT_OUT_DIR)
	$(Q)$(MAKE) -C $(UBOOT_SRC) CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH) KBUILD_OUTPUT=$(UBOOT_OUT_DIR) $(UBOOT_BURN_DEFCONFIG)
	$(Q)$(MAKE) -C $(UBOOT_SRC) CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH) KBUILD_OUTPUT=$(UBOOT_OUT_DIR) -j$(CPUS) all u-boot-dtb.img
	$(Q)cd $(SCRIPT_DIR) && ./padbootloader $(UBOOT_OUT_DIR)/u-boot-dtb.img

upramfs:
	$(Q)rm -rf $(UPRAMFS_ROOTFS)
	$(Q)mkdir -p $(UPRAMFS_ROOTFS)
	$(Q)$(SCRIPT_DIR)/populate_dir $(UPRAMFS_ROOTFS)
	$(Q)cp -rf $(CUR_DIR)/initramfs/* $(UPRAMFS_ROOTFS)
	$(Q)$(CROSS_COMPILE)strip --strip-unneeded $(UPRAMFS_ROOTFS)/lib/modules/*.ko
	
	$(Q)$(SCRIPT_DIR)/gen_initramfs_list.sh -u 0 -g 0 $(UPRAMFS_ROOTFS) > $(BURN_DIR)/upramfs.list
	$(Q)${SCRIPT_DIR}/gen_init_cpio $(BURN_DIR)/upramfs.list > ${BURN_DIR}/upramfs.img.tmp
	$(Q)$(TOOLS_DIR)/utils/mkimage -n "RAMFS" -A arm -O linux -T ramdisk -C none -a 02000000 -e 02000000 -d ${BURN_DIR}/upramfs.img.tmp ${BURN_DIR}/upramfs.img
	$(Q)rm ${BURN_DIR}/upramfs.img.tmp
	$(Q)rm ${BURN_DIR}/upramfs.list

card_burn_upramfs:
	$(Q)rm -rf $(CARD_BURN_UPRAMFS_ROOTFS)
	$(Q)mkdir -p $(CARD_BURN_UPRAMFS_ROOTFS)
	$(Q)$(SCRIPT_DIR)/populate_dir $(CARD_BURN_UPRAMFS_ROOTFS)
	$(Q)cp -rf $(CUR_DIR)/card_burn_initramfs/* $(CARD_BURN_UPRAMFS_ROOTFS)
	$(Q)$(CROSS_COMPILE)strip --strip-unneeded $(CARD_BURN_UPRAMFS_ROOTFS)/lib/modules/*.ko
	
	$(Q)$(SCRIPT_DIR)/gen_initramfs_list.sh -u 0 -g 0 $(CARD_BURN_UPRAMFS_ROOTFS) > $(CARD_BURN_DIR)/upramfs.list
	$(Q)${SCRIPT_DIR}/gen_init_cpio $(CARD_BURN_DIR)/upramfs.list > ${CARD_BURN_DIR}/upramfs.img.tmp
	$(Q)$(TOOLS_DIR)/utils/mkimage -n "RAMFS" -A arm -O linux -T ramdisk -C none -a 02000000 -e 02000000 -d ${CARD_BURN_DIR}/upramfs.img.tmp ${CARD_BURN_DIR}/card_burn_upramfs.img
	$(Q)rm ${CARD_BURN_DIR}/upramfs.img.tmp
	$(Q)rm ${CARD_BURN_DIR}/upramfs.list

clean: kernel_clean u-boot_clean
	#$(Q)rm -rf $(TOP_DIR)/out

kernel_clean:
	rm -rf $(KERNEL_OUT_DIR)

u-boot_clean:
	rm -rf $(UBOOT_OUT_DIR)