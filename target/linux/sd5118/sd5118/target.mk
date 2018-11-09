#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/SD5118
  NAME:=Huawei hisilicon SD5118 xPON arm chip
endef

define Profile/SD5115/Description
	Package set compatible with hardware any Huawei 10G xPON device
endef

$(eval $(call Profile,SD5118))

