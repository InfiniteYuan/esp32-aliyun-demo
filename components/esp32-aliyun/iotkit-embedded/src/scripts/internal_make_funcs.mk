define CompLib_Map
$(eval \
    COMP_LIB_COMPONENTS += \
        $(if \
            $(filter y,$(FEATURE_$(strip $(1)))),$(strip $(2)) \
        ) \
)
endef

define Post_Distro
    @find $(FINAL_DIR) -name "*.[ch]" -exec chmod a-x {} \;
    @mkdir -p $(FINAL_DIR)/src
    $(if $(filter y,$(FEATURE_MQTT_ID2_AUTH)),
        @cp -f $(OUTPUT_DIR)/src/tfs/$(LIBA_TARGET_src/tfs) $(FINAL_DIR)/lib
        @cat doc/export.sdk.demo/head_id2.mk > $(FINAL_DIR)/src/Makefile,
        @cat doc/export.sdk.demo/head.mk > $(FINAL_DIR)/src/Makefile
        @rm -f $(FINAL_DIR)/lib/libtfs*.a)
    $(if $(filter y,$(FEATURE_MQTT_COMM_ENABLED)),
        @cp -f sample/mqtt/mqtt-example.c $(FINAL_DIR)/src/mqtt-example.c
        @cat doc/export.sdk.demo/mqtt.mk >> $(FINAL_DIR)/src/Makefile)
    $(if $(filter y,$(FEATURE_COAP_COMM_ENABLED)),
        @cp -f sample/coap/coap-example.c $(FINAL_DIR)/src/coap-example.c
        @cat doc/export.sdk.demo/coap.mk >> $(FINAL_DIR)/src/Makefile)
    $(if $(filter y,$(FEATURE_HTTP_COMM_ENABLED)),
        @cp -f sample/http/http-example.c $(FINAL_DIR)/src/http-example.c
        @cat doc/export.sdk.demo/http.mk >> $(FINAL_DIR)/src/Makefile)
    @chmod a-x $(FINAL_DIR)/src/*

    @echo ""
    @echo "========================================================================="
    @echo "o BUILD COMPLETE WITH FOLLOWING SWITCHES:"
    @echo "----"
    @( \
    $(foreach V,$(SWITCH_VARS), \
        $(if $(findstring FEATURE_,$(V)), \
            printf "%-32s : %-s\n" "    $(V)" "$($(V))"; \
        ) \
    ) )
    @echo ""
    @echo "o RELEASE PACKAGE LAYOUT:"
    @echo "----"
    @cd $(FINAL_DIR) && echo -n "    " && pwd && echo "" && \
     find . -not -path "./include/mbedtls/*" -print | awk '!/\.$$/ { \
        for (i = 1; i < NF-1; i++) { \
            printf("|   "); \
        } \
        print "+-- "$$NF}' FS='/' | sed 's!\(.*\)!    &!g'
    @echo ""
    @echo "o BINARY FOOTPRINT CONSIST:"
    @echo "----"
    @STAGED=$(LIBOBJ_TMPDIR) STRIP=$(strip $(STRIP)) $(SCRIPT_DIR)/stats_static_lib.sh $(FINAL_DIR)/lib/$(COMP_LIB)
    @echo "========================================================================="
    @echo ""
endef

