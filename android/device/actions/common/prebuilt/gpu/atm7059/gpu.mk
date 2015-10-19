
#ifneq ($(filter $(TARGET_PRODUCT),full_gs705a),)

PRODUCT_COPY_FILES += \
$(LOCAL_PATH)/egl.cfg:system/lib/egl/egl.cfg

#endif

