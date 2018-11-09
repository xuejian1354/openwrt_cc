#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=mips
BOARD:=bcm6838
BOARDNAME:=Broadcom BCM6838 (mips)
SUBTARGETS:=bcm6838
FEATURES:=squashfs
KERNEL_PATCHVER:=3.18

include $(INCLUDE_DIR)/target.mk

define Target/Description
	Build firmware images for Broadcom based PON/routers
	currently supports BCM6838 based devices.
	(e.g. Inventel Livebox, Siemens SE515, Neufbox 4)
endef

$(eval $(call BuildTarget))
