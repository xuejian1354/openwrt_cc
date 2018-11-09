#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=mips
BOARD:=bcm6848
BOARDNAME:=Broadcom BCM6848 (mips)
SUBTARGETS:=bcm6848
FEATURES:=squashfs
KERNEL_PATCHVER:=4.1

include $(INCLUDE_DIR)/target.mk

define Target/Description
	Build firmware images for Broadcom based PON/routers
	currently supports BCM6848 based devices.
endef

$(eval $(call BuildTarget))
