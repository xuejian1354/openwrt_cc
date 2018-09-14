#
# Copyright (C) 2006-2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=mips
BOARD:=rtl9600
BOARDNAME:=Realtek rtl9600 SOC
SUBTARGETS:=rtl9600
FEATURES:=squashfs
LINUX_VERSION:=3.10.49

include $(INCLUDE_DIR)/target.mk

#DEFAULT_PACKAGES += kmod-gpio-button-hotplug

define Target/Description
	Build firmware images for Realtek based boards
endef

$(eval $(call BuildTarget))
