#
# Makefile for MMC/SD card drivers
#

obj-$(CONFIG_MMC_BLOCK)					+= mmc_block.o
mmc_block-objs							:= block.o queue.o


obj-$(CONFIG_TSD_MMC_BLOCK)   			+= tsd_mmc_block.o
tsd_mmc_block-objs						:= tsd_block.o tsd_queue.o card_proc.o

obj-$(CONFIG_CARD_TO_CARD_MMC_BLOCK)   	+= card_to_card.o
card_to_card-objs						:= tsd_block.o tsd_queue.o card_proc.o

obj-$(CONFIG_MMC_TEST)					+= mmc_test.o
obj-$(CONFIG_SDIO_UART)					+= sdio_uart.o

