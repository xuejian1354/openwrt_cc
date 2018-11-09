#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=arm
BOARD:=hi5681
BOARDNAME:=Hisilicon HI5681 10G GPON/EPON (ARM)
SUBTARGETS:=hi5681
FEATURES:=squashfs
CPU_TYPE:=cortex-a9

KERNEL_PATCHVER:=3.18

include $(INCLUDE_DIR)/target.mk

define Target/Description
	Build firmware images for hisilicon based routers with ARM CPU, *not* MIPS.
endef


$(eval $(call BuildTarget))
