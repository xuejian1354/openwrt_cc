#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/HI5681
  NAME:=hisilicon 5681 10G xPON arm chip
endef

define Profile/HI5681/Description
	Package set compatible with hardware any hisilicon 10G xPON device
endef

$(eval $(call Profile,HI5681))

