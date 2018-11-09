#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/MT7525
  NAME:=MTK MT7525 xPON mips chip
endef

define Profile/MT7525/Description
	Package set compatible with hardware any Mediatek 7525 xPON device
endef

$(eval $(call Profile,MT7525))

