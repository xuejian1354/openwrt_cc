#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/BCM6856
  NAME:=BCM6856 xPON chip
endef

define Profile/BCM6856/Description
	Package set compatible with hardware any Broadcom arm xPON device
endef

$(eval $(call Profile,BCM6856))

