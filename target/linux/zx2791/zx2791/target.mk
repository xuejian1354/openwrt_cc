#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/ZX2791
  NAME:=ZX2791 xPON chip
endef

define Profile/ZX2791/Description
	Package set compatible with hardware any zxic xPON device
endef

$(eval $(call Profile,ZX2791))

