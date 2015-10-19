#### filelist.mk for getevent_iio ####

# headers
HEADERS += $(APP_DIR)/iio_utils.h

# sources
SOURCES := $(APP_DIR)/getevent_iio.c

INV_SOURCES += $(SOURCES)

VPATH += $(APP_DIR) $(COMMON_DIR) $(HAL_DIR)/linux
