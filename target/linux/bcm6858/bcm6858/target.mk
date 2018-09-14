#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/BCM6858
  NAME:=BCM6858 xPON chip
endef

define Profile/BCM6858/Description
	Package set compatible with hardware any Broadcom arm xPON device
endef

$(eval $(call Profile,BCM6858))

