#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=arm
BOARD:=zx2791
BOARDNAME:=ZXIC ZX2791 xPON (ARM)
SUBTARGETS:=zx2791
FEATURES:=squashfs
CPU_TYPE:=cortex-a9

KERNEL_PATCHVER:=3.18

include $(INCLUDE_DIR)/target.mk

define Target/Description
	Build firmware images for zx2791 based routers with ARM CPU, *not* MIPS.
endef

$(eval $(call BuildTarget))
