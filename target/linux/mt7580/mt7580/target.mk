#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/MT7580
  NAME:=MTK MT7580 xPON mips chip
endef

define Profile/MT7580/Description
	Package set compatible with hardware any Mediatek 7580 xPON device
endef

$(eval $(call Profile,MT7580))

