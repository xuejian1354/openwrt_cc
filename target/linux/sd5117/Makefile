#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=arm
BOARD:=sd5117
BOARDNAME:=Huawei SD5117 GPON/EPON (ARM)
SUBTARGETS:=sd5117
FEATURES:=squashfs
CPU_TYPE:=cortex-a9

KERNEL_PATCHVER:=3.10.53

include $(INCLUDE_DIR)/target.mk

define Target/Description
	Build firmware images for hisilicon based routers with ARM CPU, *not* MIPS.
endef


$(eval $(call BuildTarget))
