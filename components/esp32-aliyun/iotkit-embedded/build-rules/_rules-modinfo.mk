.PHONY: modinfo

modinfo:
	@true

MODINFO_VARS := \
    EXTRA_SRCS \
    PKG_SWITCH \
    ORIGIN \
    PKG_SOURCE \
    PKG_UPDATE \
    REF_CFLAGS \
    REF_LDFLAGS \
    LDFLAGS \
    LIBA_TARGET \
    TARGET \
    LIBSO_TARGET \

$(if $(filter modinfo,$(MAKECMDGOALS)), \
    $(if $(strip $(DEPENDS)), \
        $(info DEPENDS_$(MODULE_NAME) = $(strip $(DEPENDS))) \
    ) \
)
$(if $(filter modinfo,$(MAKECMDGOALS)), \
    $(foreach v, $(MODINFO_VARS), \
        $(if $(strip $($(v))), \
            $(info $(v)_$(MODULE_NAME) = $(strip $($(v)))) \
        ) \
    ) \
)
