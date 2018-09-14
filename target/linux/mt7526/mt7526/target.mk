#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/MT7526
  NAME:=MTK MT7526 xPON mips chip
endef

define Profile/MT7526/Description
	Package set compatible with hardware any Mediatek xPON device
endef

$(eval $(call Profile,MT7526))

