#
# Copyright (C) 2006-2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=mips
BOARD:=mt7526
BOARDNAME:=Mediatek 7526 GPON/EPON SOC
SUBTARGETS:=mt7526
FEATURES:=squashfs
LINUX_VERSION:=3.18

include $(INCLUDE_DIR)/target.mk

#DEFAULT_PACKAGES += kmod-gpio-button-hotplug

define Target/Description
	Build firmware images for Mediatek based boards
endef

$(eval $(call BuildTarget))
