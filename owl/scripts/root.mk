

.PHONY: all clean
.PHONY: kernel_clean u-boot_clean bootloader_clean rootfs_clean distclean mrproper
.PHONY: kernel kernel-config modules u-boot bootloader rootfs initramfs misc firmware md5sum recovery burn_card
.PHONY: burn burn_clean

include .config

TOP_DIR=$(shell pwd)
CPUS=$$(($(shell cat /sys/devices/system/cpu/present | awk -F- '{ print $$2 }')+1))
BURN_UDISK_OFFSET=$$(($(BURN_UDISK_SIZE) + 131072 - 100))
#CPUS=1
Q=

KERNEL_SRC=$(TOP_DIR)/../kernel
UBOOT_SRC=$(TOP_DIR)/../u-boot
BURN_SRC=$(TOP_DIR)/$(IC_NAME)/burn

BOOTLOADER_SRC=$(TOP_DIR)/private/boot
SCRIPT_DIR=$(TOP_DIR)/scripts
BOARD_CONFIG_DIR=$(TOP_DIR)/$(IC_NAME)/boards/$(OS_NAME)/$(BOARD_NAME)
TOOLS_DIR=$(TOP_DIR)/tools

OUT_DIR=$(TOP_DIR)/out/$(IC_NAME)_$(OS_NAME)_$(BOARD_NAME)
BURN_DIR=$(OUT_DIR)/burn
CARD_BURN_DIR=$(OUT_DIR)/card_burn
IMAGE_DIR=$(OUT_DIR)/images
BOOTLOAD_DIR=$(OUT_DIR)/bootloader
MISC_DIR=$(OUT_DIR)/misc
CARD_BURN_MISC_DIR=$(CARD_BURN_DIR)/card_burn_misc
CARD_BURN_UDISK_DIR=$(CARD_BURN_DIR)/card_burn_udsik
CARD_BURN_IMAGE=$(CARD_BURN_DIR)/card_burn_image
KERNEL_OUT_DIR=$(OUT_DIR)/kernel
CARD_BURN_KERNEL_OUT_DIR=$(CARD_BURN_DIR)/kernel
UBOOT_OUT_DIR=$(OUT_DIR)/u-boot
K_BLD_CONFIG=$(KERNEL_OUT_DIR)/.config

CROSS_COMPILE=$(TOP_DIR)/../toolchain/arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
export PATH:=$(TOOLS_DIR)/utils:$(PATH)

DATE_STR=$(shell date +%y%m%d)
FW_NAME=$(IC_NAME)_$(OS_NAME)_$(BOARD_NAME)_$(DATE_STR)

all: burn kernel modules u-boot rootfs firmware md5sum burn_card

burn:
	$(Q)$(MAKE) -C $(BURN_SRC) CROSS_COMPILE=$(CROSS_COMPILE) all

$(K_BLD_CONFIG):
	$(Q)mkdir -p $(KERNEL_OUT_DIR)
	$(Q)$(MAKE) -C $(KERNEL_SRC) ARCH=$(ARCH) O=$(KERNEL_OUT_DIR) $(KERNEL_DEFCONFIG)

kernel: $(K_BLD_CONFIG)
	$(Q)mkdir -p $(KERNEL_OUT_DIR)
	$(Q)$(MAKE) -C $(KERNEL_SRC) CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH) O=$(KERNEL_OUT_DIR) dtbs
	$(Q)$(MAKE) -C $(KERNEL_SRC) CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH) O=$(KERNEL_OUT_DIR) -j$(CPUS) uImage

modules: $(K_BLD_CONFIG)
	$(Q)mkdir -p $(KERNEL_OUT_DIR)
	$(Q)$(MAKE) -C $(KERNEL_SRC) CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH) O=$(KERNEL_OUT_DIR) -j$(CPUS) modules

kernel-config: $(K_BLD_CONFIG)
	$(Q)$(MAKE) -C $(KERNEL_SRC) ARCH=$(ARCH) O=$(KERNEL_OUT_DIR) menuconfig

u-boot:
	$(Q)mkdir -p $(UBOOT_OUT_DIR)
	$(Q)$(MAKE) -C $(UBOOT_SRC) CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH) KBUILD_OUTPUT=$(UBOOT_OUT_DIR) $(UBOOT_DEFCONFIG)
	$(Q)$(MAKE) -C $(UBOOT_SRC) CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH) KBUILD_OUTPUT=$(UBOOT_OUT_DIR) -j$(CPUS) all u-boot-dtb.img
	$(Q)cd $(SCRIPT_DIR) && ./padbootloader $(UBOOT_OUT_DIR)/u-boot-dtb.img

bootloader:
	$(Q)mkdir -p $(BOOTLOAD_DIR)
	$(Q)cd $(TOOLS_DIR)/utils && ./bootloader_pack $(TOP_DIR)/$(IC_NAME)/bootloader/bootloader.bin $(BOARD_CONFIG_DIR)/bootloader.ini $(BOOTLOAD_DIR)/bootloader.bin

misc: initramfs
	$(Q)echo "-- Build Fat Misc image --"
	$(Q)mkdir -p $(MISC_DIR)
	$(Q)mkdir -p $(IMAGE_DIR)
	$(Q)cp -r $(BOARD_CONFIG_DIR)/misc/* $(MISC_DIR)/
	$(Q)cp $(KERNEL_OUT_DIR)/arch/$(ARCH)/boot/uImage $(MISC_DIR)
	$(Q)cp $(KERNEL_OUT_DIR)/arch/$(ARCH)/boot/dts/$(KERNEL_DTS).dtb $(MISC_DIR)/kernel.dtb
	$(Q)cp $(BOARD_CONFIG_DIR)/uenv.txt $(MISC_DIR)
	$(Q)dd if=/dev/zero of=$(IMAGE_DIR)/misc.img bs=1M count=$(MISC_IMAGE_SIZE)
	$(Q)$(TOOLS_DIR)/utils/makebootfat -o $(IMAGE_DIR)/misc.img -L misc -b $(SCRIPT_DIR)/bootsect.bin $(MISC_DIR)

$(BURN_UDISK_SIZE): burn misc firmware
	#misc
	$(Q)rm -rf $(IMAGE_DIR)/burn_image.bin
	$(Q)mkdir -p $(CARD_BURN_MISC_DIR)
	$(Q)mkdir -p $(CARD_BURN_UDISK_DIR)
	$(Q)mkdir -p $(CARD_BURN_IMAGE)
	$(Q)cp -rf $(MISC_DIR)/* $(CARD_BURN_MISC_DIR)/
	$(Q)cp $(CARD_BURN_KERNEL_OUT_DIR)/arch/$(ARCH)/boot/uImage $(CARD_BURN_MISC_DIR)
	$(Q)cp $(BOARD_CONFIG_DIR)/bootloader.ini $(CARD_BURN_UDISK_DIR)
	$(Q)cp -f ${CARD_BURN_DIR}/card_burn_upramfs.img $(CARD_BURN_MISC_DIR)/ramdisk.img
	$(Q)dd if=/dev/zero of=${CARD_BURN_IMAGE}/card_burn_misc.img bs=1M count=$(MISC_IMAGE_SIZE)
	$(Q)$(TOOLS_DIR)/utils/makebootfat -o ${CARD_BURN_IMAGE}/card_burn_misc.img -L misc -b $(SCRIPT_DIR)/bootsect.bin $(CARD_BURN_MISC_DIR)
	#bootloader
	$(Q)sed 's/.*bootdev.*=.*/bootdev=0x20/' $(BOARD_CONFIG_DIR)/bootloader.ini > $(SCRIPT_DIR)/bootloader.ini
	$(Q)cd $(TOOLS_DIR)/utils && ./bootloader_pack $(TOP_DIR)/$(IC_NAME)/bootloader/bootloader.bin $(SCRIPT_DIR)/bootloader.ini $(CARD_BURN_IMAGE)/bootloader_burn.bin
	$(Q)rm -rf $(SCRIPT_DIR)/bootloader.ini

	#copy data to udisk
	$(Q)cp $(BOOTLOAD_DIR)/bootloader.bin $(CARD_BURN_UDISK_DIR)/
	$(Q)cp $(UBOOT_OUT_DIR)/u-boot-dtb.img $(CARD_BURN_UDISK_DIR)/
	$(Q)cp $(BOOTLOAD_DIR)/bootloader.bin $(CARD_BURN_UDISK_DIR)/
	$(Q)cp $(BOARD_CONFIG_DIR)/partition.cfg $(CARD_BURN_UDISK_DIR)/
	$(Q)cp $(IMAGE_DIR)/*.img $(CARD_BURN_UDISK_DIR)/
	$(Q)dd if=/dev/zero of=${CARD_BURN_IMAGE}/card_burn_udisk.img bs=512 count=$(BURN_UDISK_SIZE)
	$(Q)$(TOOLS_DIR)/utils/makebootfat -o ${CARD_BURN_IMAGE}/card_burn_udisk.img -L udisk -b $(SCRIPT_DIR)/bootsect.bin $(CARD_BURN_UDISK_DIR)

	#make burn card image
	$(Q)dd if=$(CARD_BURN_IMAGE)/bootloader_burn.bin of=$(CARD_BURN_IMAGE)/burn_image.bin bs=512 seek=4097
	$(Q)dd if=$(UBOOT_OUT_DIR)/u-boot-dtb.img of=$(CARD_BURN_IMAGE)/burn_image.bin bs=1024 seek=3072   #3M  offset
	$(Q)dd if=${CARD_BURN_IMAGE}/card_burn_misc.img of=$(CARD_BURN_IMAGE)/burn_image.bin bs=1048576 seek=16   #16M offset
	$(Q)dd if=${CARD_BURN_IMAGE}/card_burn_udisk.img of=$(CARD_BURN_IMAGE)/burn_image.bin bs=1048576 seek=64  #64M offset

	#create partitions
	$(Q)$(TOOLS_DIR)/utils/parted -s $(CARD_BURN_IMAGE)/burn_image.bin mklabel gpt
	$(Q)$(TOOLS_DIR)/utils/parted -s $(CARD_BURN_IMAGE)/burn_image.bin unit s mkpart MISC 32768 131071     #start:16M end:64M
	$(Q)$(TOOLS_DIR)/utils/parted -s $(CARD_BURN_IMAGE)/burn_image.bin unit s mkpart UDISK 131072 $$(($(BURN_UDISK_OFFSET))) #start:64M end:64M+udisk_size
	$(Q)cp $(CARD_BURN_IMAGE)/burn_image.bin $(IMAGE_DIR)

burn_card: $(BURN_UDISK_SIZE) 
	@echo ""

firmware: bootloader misc recovery 
	$(Q)mkdir -p $(BURN_DIR)
	$(Q)cp $(BURN_SRC)/burn.bin $(BURN_DIR)/
	$(Q)cp $(UBOOT_OUT_DIR)/u-boot-dtb.img $(IMAGE_DIR)/
	$(Q)cp $(BOOTLOAD_DIR)/bootloader.bin $(IMAGE_DIR)/
	
	$(Q)cp $(BOARD_CONFIG_DIR)/partition.cfg $(SCRIPT_DIR)/partition.cfg
	$(Q)python $(SCRIPT_DIR)/partition_create.py $(SCRIPT_DIR)/partition.cfg  $(SCRIPT_DIR)/partition_tmp.cfg
	$(Q)sed -i 's/\\boardname\\/\\$(IC_NAME)_$(OS_NAME)_$(BOARD_NAME)\\/' $(SCRIPT_DIR)/partition_tmp.cfg
	
	$(Q)cp $(SCRIPT_DIR)/fwimage_linux.cfg  $(SCRIPT_DIR)/fwimage_linux_tmp.cfg
	$(Q)sed -i 's/boardname/$(IC_NAME)_$(OS_NAME)_$(BOARD_NAME)/' $(SCRIPT_DIR)/fwimage_linux_tmp.cfg
	
	$(Q)echo "--Build Firmwares.."
	$(Q)cd $(SCRIPT_DIR) && ./linux_build_fw fwimage_linux_tmp.cfg $(IMAGE_DIR) $(FW_NAME)
	$(Q)rm $(SCRIPT_DIR)/partition_tmp.cfg $(SCRIPT_DIR)/partition.cfg $(SCRIPT_DIR)/fwimage_linux_tmp.cfg

md5sum:
	@cd $(IMAGE_DIR) && md5sum *.* > image.md5


clean: burn_clean kernel_clean u-boot_clean bootloader_clean
	#$(Q)rm -rf $(TOP_DIR)/out

burn_clean:
	$(Q)$(MAKE) -C $(BURN_SRC) CROSS_COMPILE=$(CROSS_COMPILE) clean

kernel_clean:
	rm -rf $(KERNEL_OUT_DIR)

u-boot_clean:
	rm -rf $(UBOOT_OUT_DIR)

bootloader_clean:
	@echo ""

distclean:
	rm -f $(TOP_DIR)/.config
	rm -rf $(TOP_DIR)/out

mrproper:
	$(Q)$(MAKE) -C $(KERNEL_SRC) O=$(KERNEL_OUT_DIR) mrproper
	$(Q)$(MAKE) -C $(UBOOT_SRC) KBUILD_OUTPUT=$(UBOOT_OUT_DIR) mrproper

include $(BOARD_CONFIG_DIR)/os.mk
include $(TOP_DIR)/$(IC_NAME)/pcba_linux/pcba_linux.mk
