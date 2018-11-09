#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/BCM6848
  NAME:=BCM6848 xPON chip
endef

define Profile/BCM6848/Description
	Package set compatible with hardware any Broadcom xPON device
endef

$(eval $(call Profile,BCM6848))

