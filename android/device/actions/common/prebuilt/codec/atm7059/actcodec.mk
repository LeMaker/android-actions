
#ifneq ($(filter $(TARGET_PRODUCT),full_gs705a),)

PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/media_profiles.xml:system/etc/media_profiles.xml \
	$(LOCAL_PATH)/media_codecs.xml:system/etc/media_codecs.xml \
	$(LOCAL_PATH)/omx_codec.xml:system/etc/omx_codec.xml \
	$(LOCAL_PATH)/actions_product_test.xml:system/etc/actions_product_test.xml \
	$(LOCAL_PATH)/actions_pcba_android.xml:system/etc/actions_pcba_android.xml

#endif

