#
# Copyright (C) 2006-2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=mipsel
BOARD:=mt7580
BOARDNAME:=Mediatek 7580 GPON/EPON SOC
SUBTARGETS:=mt7580
FEATURES:=squashfs
LINUX_VERSION:=3.18

include $(INCLUDE_DIR)/target.mk

define Target/Description
	Build firmware images for Mediatek based boards
endef

$(eval $(call BuildTarget))
