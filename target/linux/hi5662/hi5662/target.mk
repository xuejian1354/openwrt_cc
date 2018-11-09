#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/HI5662
  NAME:=hisilicon 5662 1G xPON arm chip
endef

define Profile/HI5662/Description
	Package set compatible with hardware any hisilicon 1G xPON device
endef

$(eval $(call Profile,HI5662))

