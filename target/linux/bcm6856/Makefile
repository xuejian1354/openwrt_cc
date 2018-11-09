#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=arm
BOARD:=bcm6856
BOARDNAME:=Broadcom BCM6856 (arm)
SUBTARGETS:=bcm6856
FEATURES:=squashfs
CPU_TYPE:=cortex-a9

KERNEL_PATCHVER:=3.18

include $(INCLUDE_DIR)/target.mk

define Target/Description
	Build firmware images for Broadcom based PON/routers
	currently supports BCM6856 based devices.
endef

$(eval $(call BuildTarget))
