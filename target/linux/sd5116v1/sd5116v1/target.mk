#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/SD5116V1
  NAME:=Huawei hisilicon SD5116 510 xPON arm chip
endef

define Profile/SD5116V1/Description
	Package set compatible with hardware any Huawei xPON device
endef

$(eval $(call Profile,SD5116V1))

