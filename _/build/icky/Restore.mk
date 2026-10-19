ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
ICK_INTERFACE ?= $(ROOT)/../ai-ci-ick/ick-android/Makefile
ICK_ARTIFACTS ?= $(ROOT)/ick-packages
ICK_ROOT ?= $(ROOT)/ick-stages
ABIS := armeabi-v7a arm64-v8a x86_64
RESTORED := $(addsuffix /.restored,$(addprefix $(ICK_ROOT)/,$(ABIS)))
ICK_SHARED := $(abspath $(dir $(ICK_INTERFACE))/..)
ICK_SHARED_REVISION := f6d825d15cd3c0c34ae1b43240a463ff5d343090
ICK_SOURCE_REVISION := fbe86e23d55cfec2000c08e61deea2a407fd7175
TARGET_armeabi-v7a := arm-linux-gnueabi
TARGET_arm64-v8a := aarch64-linux-gnu
TARGET_x86_64 := x86_64-linux-gnu

.PHONY: restore verify verify-shared $(addprefix verify-,$(ABIS))
restore: $(RESTORED)

# The two git-archive application builds consume these explicit immutable
# dependencies. Recheck every archived file and the entire member inventory
# before each build; only restore-stage's marker may be additional.
verify: verify-shared $(addprefix verify-,$(ABIS))

verify-shared:
	test "$$(git -C "$(ICK_SHARED)" rev-parse HEAD)" = "$(ICK_SHARED_REVISION)"
	git -C "$(ICK_SHARED)" diff --exit-code HEAD -- ick-android
	test -z "$$(git -C "$(ICK_SHARED)" ls-files --others -- ick-android)"

$(addprefix verify-,$(ABIS)): verify-%: verify-shared
	test -f "$(ICK_ARTIFACTS)/wegert-ick-$*.tar.gz"
	tar --compare -zf "$(ICK_ARTIFACTS)/wegert-ick-$*.tar.gz" -C "$(ICK_ROOT)/$*"
	@set -eu; evidence=$$(mktemp -d); trap 'rm -rf "$$evidence"' EXIT; \
	 tar -tzf "$(ICK_ARTIFACTS)/wegert-ick-$*.tar.gz" | sed -e 's#^\./##' -e 's#/$$##' -e '/^$$/d' | LC_ALL=C sort > "$$evidence/archive"; \
	 (cd "$(ICK_ROOT)/$*" && find . -mindepth 1 ! -path './.restored' -printf '%P\n') | LC_ALL=C sort > "$$evidence/stage"; \
	 cmp "$$evidence/archive" "$$evidence/stage"
	grep -Fx 'source	$(ICK_SOURCE_REVISION)' "$(ICK_ROOT)/$*/qualification/receipt.tsv"
	grep -Fx 'abi	$*' "$(ICK_ROOT)/$*/qualification/receipt.tsv"
	grep -Fx 'observed_target	$(TARGET_$*)' "$(ICK_ROOT)/$*/qualification/receipt.tsv"
	grep -Fx 'api	26' "$(ICK_ROOT)/$*/qualification/receipt.tsv"
	grep -Fx 'fortify_source	2' "$(ICK_ROOT)/$*/qualification/receipt.tsv"
	test "$$("$(ICK_ROOT)/$*/bin/$(TARGET_$*)-gcc" -dumpmachine)" = "$(TARGET_$*)"
	test "$$(wc -l < "$(ICK_ROOT)/$*/qualification/compiler.sha256")" = 2
	printf '%s  %s\n' "$$(sed -n '1s/ .*//p' "$(ICK_ROOT)/$*/qualification/compiler.sha256")" "$(ICK_ROOT)/$*/bin/$(TARGET_$*)-gcc" | sha256sum -c -
	printf '%s  %s\n' "$$(sed -n '2s/ .*//p' "$(ICK_ROOT)/$*/qualification/compiler.sha256")" "$(ICK_ROOT)/$*/libexec/gcc/$(TARGET_$*)/17.0.0/cc1" | sha256sum -c -

$(ICK_ROOT)/%/.restored: $(ICK_ARTIFACTS)/wegert-ick-%.tar.gz
	$(MAKE) -f "$(ICK_INTERFACE)" restore-stage ABI="$*" ICK_STAGE="$(@D)" ICK_ARCHIVE="$<"
	touch "$@"
