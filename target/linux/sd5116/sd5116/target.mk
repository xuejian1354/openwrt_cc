#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/SD5116
  NAME:=Huawei hisilicon SD5116 380 xPON arm chip
endef

define Profile/SD5116/Description
	Package set compatible with hardware any Huawei xPON device
endef

$(eval $(call Profile,SD5116))

