# Don't build the library unless forced to.
ifeq (true,$(FORCE_BUILD_LLVM_COMPONENTS))
# Don't build the library in unbundled branches.
ifeq (,$(TARGET_BUILD_APPS))

LOCAL_PATH:= $(call my-dir)

LOCAL_IS_HOST_MODULE := true

LOCAL_MODULE:= libclang

LOCAL_MODULE_TAGS := optional

LOCAL_WHOLE_STATIC_LIBRARIES := \
	libclangDriver \
	libclangParse \
	libclangSema \
	libclangAnalysis \
	libclangCodeGen \
	libclangAST \
	libclangEdit \
	libclangLex \
	libclangFrontend \
	libclangBasic \
	libclangRewriteFrontend \
	libclangRewriteCore \
	libclangSerialization


ifeq ($(HOST_OS),windows)
  LOCAL_SHARED_LIBRARIES := libLLVM
  LOCAL_LDLIBS := -limagehlp -lpsapi
else
  LOCAL_SHARED_LIBRARIES := libLLVM libc++
  LOCAL_LDLIBS := -ldl -lpthread
endif

include $(CLANG_HOST_BUILD_MK)
include $(BUILD_HOST_SHARED_LIBRARY)

endif # don't build in unbundled branches
endif # don't build unless forced to
