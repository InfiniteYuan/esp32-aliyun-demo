.PHONY: detect config reconfig toolchain sub-mods final-out env

all: detect config toolchain sub-mods final-out
	$(TOP_Q) \
	if [ -f $(STAMP_PRJ_CFG) ]; then \
	    $(RECURSIVE_MAKE) toolchain; \
	    rm -f $(STAMP_PRJ_CFG); \
	fi

RESET_ENV_VARS := \
    CROSS_PREFIX \
    CFLAGS \
    HOST \
    LDFLAGS \

detect:
	@if [ -d .git ]; then \
	    mkdir -p .git/hooks; \
	    for i in $(RULE_DIR)/hooks/*; do \
	        cp -f $$i .git/hooks && chmod a+x .git/hooks/$$(basename $$i); \
	    done; \
	fi

	@for i in $$(grep "^ *include" $(TOP_DIR)/$(TOP_MAKEFILE)|awk '{ print $$NF }'|sed '/^\$$/d'); do \
	    if [ $$i -nt $(CONFIG_TPL) ]; then \
	        echo "Re-configure project since '$${i}' updated"|grep --color ".*"; \
	        $(RECURSIVE_MAKE) reconfig; \
	    fi; \
	done

	@if [ ! -d $(OUTPUT_DIR) ]; then \
	    echo "Re-configure project since '$(OUTPUT_DIR)' non-exist!"|grep --color ".*"; \
	    $(RECURSIVE_MAKE) reconfig; \
	fi

config:
	@mkdir -p $(OUTPUT_DIR) $(INSTALL_DIR)
	@mkdir -p $(SYSROOT_BIN) $(SYSROOT_INC) $(SYSROOT_LIB)

	$(TOP_Q) \
	if [ -f $(STAMP_BLD_VAR) ]; then \
	    if [ "$$(sed '/[-_/a-zA-Z0-9]* = ..*/d' $(STAMP_BLD_VAR)|wc -l)" != "0" ]; then \
	        rm -f $(STAMP_BLD_VAR); \
	    fi \
	fi

	$(TOP_Q)+( \
	if [ -f $(CONFIG_TPL) ]; then \
	    if [ "$(filter comp-lib,$(MAKECMDGOALS))" = "" ]; then \
	        printf "BUILDING WITH EXISTING CONFIGURATION:\n\n"; \
	        command grep -m 1 "VENDOR *:" $(CONFIG_TPL)|cut -c 3-; \
	        command grep -m 1 "MODEL *:" $(CONFIG_TPL)|cut -c 3-; \
	        echo ""; \
	    fi \
	else \
	    if [ "$(BUILD_CONFIG)" != "" ] && [ -f $(BUILD_CONFIG) ]; then \
	        printf "# Automatically Generated Section End\n\n" >> $(CONFIG_TPL); \
	        printf "# %-10s %s\n" "VENDOR :" $$(basename $(BUILD_CONFIG)|cut -d. -f2) >> $(CONFIG_TPL); \
	        printf "# %-10s %s\n" "MODEL  :" $$(basename $(BUILD_CONFIG)|cut -d. -f3) >> $(CONFIG_TPL); \
	        cat $(BUILD_CONFIG) >> $(CONFIG_TPL); \
	    else \
	        printf "SELECT A CONFIGURATION:\n\n"; \
	        LIST=$$(for i in $(CONFIG_DIR)/config.*.*; do basename $${i}; done|sort); \
	        select V in $${LIST}; do \
	            echo ""; \
	            printf "# Automatically Generated Section End\n\n" >> $(CONFIG_TPL); \
	            printf "# %-10s %s\n" "VENDOR :" $$(echo $${V}|cut -d. -f2) >> $(CONFIG_TPL); \
	            printf "# %-10s %s\n" "MODEL  :" $$(echo $${V}|cut -d. -f3) >> $(CONFIG_TPL); \
	            cp -f -P $(IMPORT_DIR)/$$(echo $${V}|cut -d. -f2)/$(PREBUILT_LIBDIR)/*.so* $(SYSROOT_LIB) 2>/dev/null; \
	            cat $(CONFIG_DIR)/$${V} >> $(CONFIG_TPL); \
	            break; \
	        done; \
	    fi && \
	    printf "SELECTED CONFIGURATION:\n\n" && \
	    command grep -m 1 "VENDOR *:" $(CONFIG_TPL)|cut -c 3- && \
	    command grep -m 1 "MODEL *:" $(CONFIG_TPL)|cut -c 3- && \
	    echo ""; \
	    if [ "$(MAKECMDGOALS)" = "config" ]; then true; else \
	        if [ "$(BUILD_CONFIG)" = "" ]; then \
	            touch $(STAMP_PRJ_CFG); \
	        fi; \
	    fi; \
	    for i in $(RESET_ENV_VARS); do unset $${i}; done; \
	    $(MAKE) --no-print-directory -f $(TOP_MAKEFILE) $(STAMP_BLD_VAR); \
	fi)

toolchain: VSP_TARBALL = $(OUTPUT_DIR)/$(shell $(SHELL_DBG) basename $(CONFIG_TOOLCHAIN_RPATH))
toolchain: config
ifneq ($(CONFIG_TOOLCHAIN_NAME),)
	$(TOP_Q) \
	if [ -e $(OUTPUT_DIR)/$(CONFIG_TOOLCHAIN_NAME) ]; then \
	    true; \
	else \
	    if [ "$(CONFIG_CACHED_TOOLCHAIN)" != "" ] && [ -d $(CONFIG_CACHED_TOOLCHAIN) ]; then \
	        ln -sf $(CONFIG_CACHED_TOOLCHAIN) $(OUTPUT_DIR)/$(CONFIG_TOOLCHAIN_NAME); \
	    else \
	        echo "Downloading ToolChain ..." && \
	        wget -O $(VSP_TARBALL) $(CONFIG_VSP_WEBSITE)/$(CONFIG_TOOLCHAIN_RPATH) && \
	        echo "De-compressing ToolChain ..." && \
	        tar xf $(VSP_TARBALL) -C $(OUTPUT_DIR); \
	    fi \
	fi
endif

reconfig: distclean
	$(TOP_Q)+$(RECURSIVE_MAKE) config
	$(TOP_Q)rm -f $(STAMP_PRJ_CFG)

clean:
	$(TOP_Q) \
	for i in $(SUBDIRS) $(COMP_LIB_COMPONENTS); do \
	    if [ -d $(OUTPUT_DIR)/$${i} ]; then \
	        $(MAKE) --no-print-directory -C $(OUTPUT_DIR)/$${i} clean; \
	    fi \
	done

	$(TOP_Q) \
	rm -rf \
	        $(LIBOBJ_TMPDIR) \
	        $(COMPILE_LOG) \
	        $(DIST_DIR)/* \
	        $(SYSROOT_INC)/* $(SYSROOT_LIB)/* $(SYSROOT_LIB)/../bin/* \
	        $(shell $(SHELL_DBG) find $(OUTPUT_DIR) -name "$(COMPILE_LOG)" \
	                             -o -name "$(WARNING_LOG)" \
	                             -o -name "$(STAMP_BUILD)" \
	                             -o -name "$(STAMP_INSTALL)" \
	                             -o -name "$(STAMP_POSTINS)" \
	        2>/dev/null)

distclean:
	$(TOP_Q) \
	rm -rf \
	    $(CONFIG_TPL) $(COMPILE_LOG) \
	    $(STAMP_PRJ_CFG) $(STAMP_BLD_ENV) $(STAMP_BLD_VAR) $(STAMP_POST_RULE) \
	    $(DIST_DIR) \

	$(TOP_Q) \
	if [ -d $(OUTPUT_DIR) ]; then \
	    cd $(OUTPUT_DIR); \
	    if [ "$(CONFIG_TOOLCHAIN_NAME)" = "" ]; then \
	        rm -rf *; \
	    else \
	        rm -rf $$(ls -I $(CONFIG_TOOLCHAIN_NAME)); \
	    fi \
	fi

