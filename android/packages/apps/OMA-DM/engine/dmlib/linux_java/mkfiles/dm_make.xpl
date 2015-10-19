#
# DESCRIPTION:
#   Makefile for jni shared object
#
########### Revision History ########################################
#
#  Date       Author     Reference
#  ========   ========   ================================
#  03-08-07   fdp101     initial draft
#
#####################################################################

# java home
ifndef JAVA_HOME
JAVA_HOME := /opt/jdk1.3.1
endif

GLOBAL_TARGET_SUBDIR := lib
GLOBAL_TARGET_FILENAME := libxpl.a

GLOBAL_SUBDIRS_A = portlib/lj/src \
                   linux_java/samples/portlib/src

GLOBAL_EXTRA_CC_FLAGS = \
      -I$(DIR_DM)/plugin/hdr \
      -I$(DIR_DM)/dmengine/hdr \
      -I$(DIR_DM)/dmengine/dm_util/hdr \
      -I$(DIR_DM)/dmengine/dm_persist/hdr \
      -I$(DIR_DM)/dmengine/dm_security/hdr \
      -I$(DIR_DM)/dmengine/dm_tnm/hdr \
      -I$(DIR_DM)/dmengine/dm_ssession/hdr \
      -I$(DIR_DM)/dmengine/dm_transport/hdr \
      -I$(DIR_DM)/dmengine/dm_ua/hdr \
      -I$(DIR_DM)/dmengine/oma_toolkit/sml/xpt/hdr \
      -I$(DIR_DM)/dmengine/oma_toolkit/sml/mgr/hdr \
      -I$(DIR_DM)/dmengine/oma_toolkit/sml/ghdr \
      -I$(DIR_DM)/dmengine/oma_toolkit/sml/lib/hdr \
      -I$(DIR_DM)/dmengine/oma_toolkit/sml/wsm/hdr \
      -I$(DIR_DM)/dmengine/oma_toolkit/sml/xlt/src \
      -I$(DIR_DM)/dmengine/oma_toolkit/sml/xlt/hdr \
      -I$(DIR_DM)/portlib/lj/hdr \
      -I$(DIR_DM)/portlib/generic \
      -I$(DIR_DM)/api/common \
      -I$(DIR_DM)/api/native \
      -I$(DIR_DM)/api/native/plugin \
      -I$(DIR_DM)/dmtapi/native/hdr \
      -I$(DIR_XPL)/code/portlib/hdr \
      -I$(DIR_XPL)/code/portlib/linux/hdr \
      -I$(DIR_XPL)/code/utility/hdr \
      -I$(DIR_DM)/linux_java/samples/logger/hdr \
      -I$(DIR_DM)/api/native/plugin \
      -DEZX_PORT

GLOBAL_CREATE_LIB = 1

GLOBAL_DEP_FILENAME = deps_xpl

include /vobs/linuxjava/device_mgmt/dm/core/src/linux_java/mkfiles/GenericMake

