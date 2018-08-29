
final-out: sub-mods
	$(TOP_Q) \
	if  [ ! -f $(SYSROOT_LIB)/lib$(COMP_LIB_NAME).a ] && \
	    [ ! -f $(SYSROOT_LIB)/lib$(COMP_LIB_NAME).so ]; then \
	    $(call Build_CompLib, FORCE) \
	fi; \

	$(TOP_Q) \
	if [ -f $(STAMP_PRJ_CFG) ]; then true; else \
	    rm -rf $(DIST_DIR); \
	    mkdir -p $(DIST_DIR) $(FINAL_DIR); \
	    for i in bin lib include; do \
	        if [ -d $(OUTPUT_DIR)/usr/$${i} ]; then \
	            cp -rf $(OUTPUT_DIR)/usr/$${i} $(FINAL_DIR); \
	        fi; \
	    done; \
	    VDR_NAME=$$(grep -m 1 "VENDOR *:" $(CONFIG_TPL) 2>/dev/null|awk '{ print $$NF }'); \
	    if [ -d $(IMPORT_DIR)/$${VDR_NAME}/libs ]; then \
	        cp -f $(IMPORT_DIR)/$${VDR_NAME}/libs/lib* $(FINAL_DIR)/lib; \
	    fi; \
	fi

	$(TOP_Q)$(STRIP) $(FINAL_DIR)/bin/* 2>/dev/null || true
	$(TOP_Q)$(STRIP) --strip-debug $(FINAL_DIR)/lib/* 2>/dev/null || true

	$(TOP_Q)$(call $(POST_FINAL_OUT_HOOK))
