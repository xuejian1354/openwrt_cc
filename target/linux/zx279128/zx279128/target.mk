#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/ZX279128
  NAME:=ZX279128 10G xPON chip
endef

define Profile/ZX279128/Description
	Package set compatible with hardware any zte xPON device
endef

$(eval $(call Profile,ZX279128))

