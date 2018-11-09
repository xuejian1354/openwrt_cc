#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/CA8277
  NAME:=CA8277 10xPON chip
endef

define Profile/CA8277/Description
	Package set compatible with hardware any Realtek 10xPON device
endef

$(eval $(call Profile,CA8277))

