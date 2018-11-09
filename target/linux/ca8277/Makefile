#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=aarch64
BOARD:=ca8277
BOARDNAME:=Realtek CA8277 (aarch64)
SUBTARGETS:=ca8277
FEATURES:=squashfs
CPU_TYPE:=cortex-a5

KERNEL_PATCHVER:=3.18

include $(INCLUDE_DIR)/target.mk

define Target/Description
	Build firmware images for Realtek based 10 PON/routers
	currently supports ca8277 based devices.
endef

$(eval $(call BuildTarget))
