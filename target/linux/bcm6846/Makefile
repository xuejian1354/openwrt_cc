#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=arm
BOARD:=bcm6846
BOARDNAME:=Broadcom BCM6846 (arm)
SUBTARGETS:=bcm6846
FEATURES:=squashfs
CPU_TYPE:=cortex-a9

KERNEL_PATCHVER:=4.1

include $(INCLUDE_DIR)/target.mk

define Target/Description
	Build firmware images for Broadcom based PON/routers
	currently supports BCM6846 based devices.
endef

$(eval $(call BuildTarget))
