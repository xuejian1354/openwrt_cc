# Makefile for OpenWrt
#
# Copyright (C) 2007 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

TOPDIR:=${CURDIR}
LC_ALL:=C
LANG:=C
export TOPDIR LC_ALL LANG

empty:=
space:= $(empty) $(empty)
$(if $(findstring $(space),$(TOPDIR)),$(error ERROR: The path to the OpenWrt directory must not include any spaces))

rootfs:

include $(TOPDIR)/include/host.mk

ifneq ($(OPENWRT_BUILD),1)
  _SINGLE=export MAKEFLAGS=$(space);

  override OPENWRT_BUILD=1
  export OPENWRT_BUILD
  GREP_OPTIONS=
  export GREP_OPTIONS
  include $(TOPDIR)/include/debug.mk
  include $(TOPDIR)/include/depends.mk
  include $(TOPDIR)/include/toplevel.mk
else
  include rules.mk
  include $(INCLUDE_DIR)/depends.mk
  include $(INCLUDE_DIR)/subdir.mk
#  include target/Makefile
  include package/Makefile
  include tools/Makefile
  include toolchain/Makefile

$(toolchain/stamp-install): $(tools/stamp-install)
$(target/stamp-compile): $(toolchain/stamp-install) $(tools/stamp-install) $(BUILD_DIR)/.prepared
$(package/stamp-compile): $(target/stamp-compile) $(package/stamp-cleanup)
$(package/stamp-install): $(package/stamp-compile)
$(target/stamp-install): $(package/stamp-compile) $(package/stamp-install)

printdb:
	@true

prepare: $(target/stamp-compile)

clean: FORCE
	rm -rf $(BUILD_DIR) $(STAGING_DIR) $(BIN_DIR) $(BUILD_LOG_DIR)

dirclean: clean
	rm -rf $(STAGING_DIR_HOST) $(TOOLCHAIN_DIR) $(BUILD_DIR_HOST) $(BUILD_DIR_TOOLCHAIN)
	rm -rf $(TMP_DIR)

ifndef DUMP_TARGET_DB
$(BUILD_DIR)/.prepared: Makefile
	@mkdir -p $$(dirname $@)
	@touch $@

tmp/.prereq_packages: .config
	unset ERROR; \
	for package in $(sort $(prereq-y) $(prereq-m)); do \
		$(_SINGLE)$(NO_TRACE_MAKE) -s -r -C package/$$package prereq || ERROR=1; \
	done; \
	if [ -n "$$ERROR" ]; then \
		echo "Package prerequisite check failed."; \
		false; \
	fi
	touch $@
endif

# check prerequisites before starting to build
prereq: $(target/stamp-prereq) tmp/.prereq_packages
	@if [ ! -f "$(INCLUDE_DIR)/site/$(ARCH)" ]; then \
		echo 'ERROR: Missing site config for architecture "$(ARCH)" !'; \
		echo '       The missing file will cause configure scripts to fail during compilation.'; \
		echo '       Please provide a "$(INCLUDE_DIR)/site/$(ARCH)" file and restart the build.'; \
		exit 1; \
	fi

prepare: .config $(tools/stamp-install) $(toolchain/stamp-install)
world: prepare $(package/stamp-compile) $(package/stamp-install) FORCE
	$(_SINGLE)$(SUBMAKE) -r package/index


CHIPTYPE:=$(shell echo $(BOARD) | tr a-z A-Z)
VENDORNAME:=$(shell echo $(CONFIG_VERSION_MANUFACTURER) | tr a-z A-Z)
IMG_PREFIX:=FRAMEWORK$(if $(VENDORNAME),_$(VENDORNAME))_$(CHIPTYPE)
	
rootfs: world
ifneq ($(or $(CONFIG_TARGET_REALTEK),$(CONFIG_TARGET_BRCM),$(CONFIG_TARGET_RTL960XC)),)
#   ld-uClibc-0.9.33.so -> ld.so.1
#   ld-uClibc-0.9.33.so -> ld-uClibc.so.0 -> ld-uClibc.so
	cp $(CONFIG_TOOLCHAIN_ROOT)/lib/libuClibc-0.9.33.2.so $(TARGET_DIR)/lib
	cd $(TARGET_DIR)/lib; ln -sf ld-uClibc-0.9.33.2.so ld.so.1; ln -sf ld-uClibc.so.0 ld-uClibc.so; ln -sf ld-uClibc-0.9.33.2.so ld-uClibc.so.0;
	cd $(TARGET_DIR)/lib; ln -sf libgcc_s.so.1 libgcc.so; ln -sf libgcc_s.so.1 libgcc_s.so
endif
ifneq ($(CONFIG_TARGET_BRCM6858),)
	cp $(CONFIG_TOOLCHAIN_ROOT)/usr/arm-buildroot-linux-gnueabi/sysroot/lib/libc-2.24.so $(TARGET_DIR)/lib
	cd $(TARGET_DIR)/lib;ln -sf libc-2.24.so libc.so.0; ln -sf ld-2.24.so ld.so.1; ln -sf ld-2.24.so ld.so.0; ln -sf ld.so.0 ld.so;
endif
ifneq ($(or $(CONFIG_TARGET_ZTE),$(CONFIG_TARGET_ZXIC),$(CONFIG_TARGET_ZXIC2791)),)
	cp $(CONFIG_TOOLCHAIN_ROOT)/arm-linux-uclibcgnueabi/lib/libuClibc-0.9.33.2.so $(TARGET_DIR)/lib
	cd $(TARGET_DIR)/lib;ln -sf libuClibc-0.9.33.2.so libc.so.0; ln -sf ld-uClibc-0.9.33.2.so ld.so.1; ln -sf ld-uClibc-0.9.33.2.so ld-uClibc.so.0; ln -sf ld-uClibc.so.0 ld-uClibc.so;
endif
ifneq ($(CONFIG_TARGET_ZTE279121),)
	cp $(CONFIG_TOOLCHAIN_ROOT)/arm-buildroot-linux-uclibcgnueabi/sysroot/lib/libuClibc-0.9.33.2.so $(TARGET_DIR)/lib
	cd $(TARGET_DIR)/lib;ln -sf libuClibc-0.9.33.2.so libc.so.0; ln -sf ld-uClibc-0.9.33.2.so ld.so.1; ln -sf ld-uClibc-0.9.33.2.so ld-uClibc.so.0; ln -sf ld-uClibc.so.0 ld-uClibc.so;
endif
ifneq ($(CONFIG_TARGET_HUAWEI),)
	cp $(CONFIG_TOOLCHAIN_ROOT)/sdk/lib/libuClibc-0.9.34-git.so $(TARGET_DIR)/lib
	cd $(TARGET_DIR)/lib;ln -sf libuClibc-0.9.34-git.so libc.so.0; ln -sf ld-uClibc-0.9.34-git.so ld.so.1; ln -sf ld-uClibc-0.9.34-git.so ld-uClibc.so.0; ln -sf ld-uClibc.so.0 ld-uClibc.so;
endif
ifneq ($(CONFIG_TARGET_HAISI),)
	cp $(CONFIG_TOOLCHAIN_ROOT)/lib/libuClibc-0.9.33.2.so $(TARGET_DIR)/lib
	cd $(TARGET_DIR)/lib;ln -sf libuClibc-0.9.33.2.so libc.so.0; ln -sf ld-uClibc-0.9.33.2.so ld.so.1; ln -sf ld-uClibc-0.9.33.2.so ld-uClibc.so.0; ln -sf ld-uClibc.so.0 ld-uClibc.so;
endif
ifneq ($(CONFIG_TARGET_MTK),)
	cp $(CONFIG_TOOLCHAIN_ROOT)/mips-buildroot-linux-uclibc/sysroot/lib/libuClibc-0.9.33.2.so $(TARGET_DIR)/lib
	cd $(TARGET_DIR)/lib;ln -sf libuClibc-0.9.33.2.so libc.so.0; ln -sf ld-uClibc-0.9.33.2.so ld.so.1; ln -sf ld-uClibc-0.9.33.2.so ld-uClibc.so.0; ln -sf ld-uClibc.so.0 ld-uClibc.so;
endif
#fixme
	chmod +x $(TARGET_DIR)/etc/profile
	chmod a+w $(TARGET_DIR)/usr/lib/opkg -R
	chmod a+w $(TARGET_DIR)/etc/config
	$(TAR) -czpf $(BIN_DIR)/$(IMG_PREFIX)_rootfs.tar.gz --same-owner -C $(TARGET_DIR)/ .

SQUASHFSOPT := -b $(CONFIG_TARGET_SQUASHFS_BLOCK_SIZE)k
#LZMA_XZ_OPTIONS := -Xpreset 9 -Xe -Xlc 0 -Xlp 2 -Xpb 2
UBIFS_OPTS = -c 2560 -m 512 -e 15360 -U
FRAMEWORK_DIR = $(BIN_DIR)/framework.dir
SAF_PATH = $(FRAMEWORK_DIR)/saf
SAF_ROOTFS_PATH = $(SAF_PATH)/rootfs
SAF_DEV_PATH = $(SAF_ROOTFS_PATH)/dev
IPKS_FOLDER = $(TOPDIR)/ipks/$(shell echo $(CONFIG_VERSION_MANUFACTURER))

.PHONY: clean dirclean prereq prepare world package/symlinks package/symlinks-install package/symlinks-clean

endif
