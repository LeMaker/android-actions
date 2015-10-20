# -*- mode: makefile -*-

LOCAL_PATH := $(call my-dir)

define all-harmony-test-java-files-under
  $(patsubst ./%,%,$(shell cd $(LOCAL_PATH) && find $(2) -name "*.java" 2> /dev/null))
endef

harmony_jdwp_test_src_files := \
    $(call all-harmony-test-java-files-under,,src/test/java/)

#jdwp_test_runtime_target := dalvikvm -XXlib:libart.so
jdwp_test_runtime_target := dalvikvm -XXlib:libartd.so
#jdwp_test_runtime_host := $(ANDROID_HOST_OUT)/bin/art
jdwp_test_runtime_host := $(ANDROID_HOST_OUT)/bin/art -d

# Runtime target for CTS. We also support running with a forced abi.
cts_jdwp_test_runtime_target := dalvikvm|\#ABI\#| -XXlib:libart.so

jdwp_test_runtime_options :=
jdwp_test_runtime_options += -verbose:jdwp
cts_jdwp_test_runtime_options :=
#jdwp_test_runtime_options += -Xint
#jdwp_test_runtime_options += -verbose:threads
jdwp_test_timeout_ms := 10000 # 10s.

jdwp_test_classpath_host := $(ANDROID_HOST_OUT)/framework/apache-harmony-jdwp-tests-hostdex.jar:$(ANDROID_HOST_OUT)/framework/junit-hostdex.jar
jdwp_test_classpath_target := /data/jdwp/apache-harmony-jdwp-tests.jar:/data/junit/junit-targetdex.jar

jdwp_test_target_runtime_common_args :=  \
	-Djpda.settings.verbose=true \
	-Djpda.settings.syncPort=34016 \
	-Djpda.settings.timeout=$(jdwp_test_timeout_ms) \
	-Djpda.settings.waitingTime=$(jdwp_test_timeout_ms)

jdwp_test_target_runtime_args :=  $(jdwp_test_target_runtime_common_args)
jdwp_test_target_runtime_args += -Djpda.settings.debuggeeJavaPath='$(jdwp_test_runtime_target) $(jdwp_test_runtime_options)'

cts_jdwp_test_target_runtime_args :=  $(jdwp_test_target_runtime_common_args)
cts_jdwp_test_target_runtime_args += -Djpda.settings.debuggeeJavaPath='$(cts_jdwp_test_runtime_target) $(cts_jdwp_test_runtime_options)'

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(harmony_jdwp_test_src_files)
LOCAL_JAVA_LIBRARIES := junit-targetdex
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := CtsJdwp
LOCAL_NO_EMMA_INSTRUMENT := true
LOCAL_NO_EMMA_COMPILE := true
LOCAL_CTS_TEST_PACKAGE := android.jdwp
LOCAL_CTS_TARGET_RUNTIME_ARGS := $(cts_jdwp_test_target_runtime_args)
include $(BUILD_CTS_TARGET_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(harmony_jdwp_test_src_files)
LOCAL_JAVA_LIBRARIES := junit-targetdex
LOCAL_MODULE_TAGS := tests
LOCAL_MODULE := apache-harmony-jdwp-tests
LOCAL_NO_EMMA_INSTRUMENT := true
LOCAL_NO_EMMA_COMPILE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/jdwp
include $(BUILD_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(harmony_jdwp_test_src_files)
LOCAL_JAVA_LIBRARIES := junit
LOCAL_MODULE := apache-harmony-jdwp-tests-host
include $(BUILD_HOST_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(harmony_jdwp_test_src_files)
LOCAL_JAVA_LIBRARIES := junit-hostdex
LOCAL_MODULE := apache-harmony-jdwp-tests-hostdex
include $(BUILD_HOST_DALVIK_JAVA_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))

# Waits for device to boot completely.
define wait-for-boot-complete
$(hide) echo "Wait for boot complete ..."
$(hide) while [ `adb wait-for-device shell getprop dev.bootcomplete | grep -c 1` -eq 0 ]; \
do \
  sleep 1; \
done
$(hide) echo "Boot complete"
endef

# If this fails complaining about TestRunner, build "external/junit" manually.
.PHONY: run-jdwp-tests-target
run-jdwp-tests-target: $(TARGET_OUT_DATA)/jdwp/apache-harmony-jdwp-tests.jar $(TARGET_OUT_DATA)/junit/junit-targetdex.jar
	adb shell stop
	adb remount
	adb sync
	adb reboot
	$(call wait-for-boot-complete)
	adb shell $(jdwp_test_runtime_target) -cp $(jdwp_test_classpath_target) \
	  $(jdwp_test_target_runtime_args) \
          org.apache.harmony.jpda.tests.share.AllTests

# If this fails complaining about TestRunner, build "external/junit" manually.
.PHONY: run-jdwp-tests-host
run-jdwp-tests-host: $(HOST_OUT_EXECUTABLES)/art $(HOST_OUT_JAVA_LIBRARIES)/apache-harmony-jdwp-tests-hostdex.jar $(HOST_OUT_JAVA_LIBRARIES)/junit-hostdex.jar
	$(jdwp_test_runtime_host) -cp $(jdwp_test_classpath_host) \
          -Djpda.settings.verbose=true \
          -Djpda.settings.syncPort=34016 \
          -Djpda.settings.debuggeeJavaPath="$(jdwp_test_runtime_host) $(jdwp_test_runtime_options)" \
          -Djpda.settings.timeout=$(jdwp_test_timeout_ms) \
          -Djpda.settings.waitingTime=$(jdwp_test_timeout_ms) \
          org.apache.harmony.jpda.tests.share.AllTests

.PHONY: run-jdwp-tests-ri
run-jdwp-tests-ri: $(HOST_OUT_JAVA_LIBRARIES)/apache-harmony-jdwp-tests-host.jar $(HOST_OUT_JAVA_LIBRARIES)/junit.jar
	java -cp $(HOST_OUT_JAVA_LIBRARIES)/apache-harmony-jdwp-tests-host.jar:$(HOST_OUT_JAVA_LIBRARIES)/junit.jar \
          -Djpda.settings.verbose=true \
          -Djpda.settings.syncPort=34016 \
          -Djpda.settings.debuggeeJavaPath=java \
          org.apache.harmony.jpda.tests.share.AllTests
