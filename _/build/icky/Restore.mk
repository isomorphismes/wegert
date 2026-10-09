ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
ICK_INTERFACE ?= $(ROOT)/../ai-ci-ick/ick-android/Makefile
ICK_ARTIFACTS ?= $(ROOT)/ick-packages
ICK_ROOT ?= $(ROOT)/ick-stages
ABIS := armeabi-v7a arm64-v8a x86_64
RESTORED := $(addsuffix /.restored,$(addprefix $(ICK_ROOT)/,$(ABIS)))

.PHONY: restore
restore: $(RESTORED)

$(ICK_ROOT)/%/.restored: $(ICK_ARTIFACTS)/wegert-ick-%.tar.gz
	$(MAKE) -f "$(ICK_INTERFACE)" restore-stage ABI="$*" ICK_STAGE="$(@D)" ICK_ARCHIVE="$<"
	touch "$@"
