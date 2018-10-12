#
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_VENDOR:=jarvis
PKG_NAME:=testvpn
PKG_DBUS_VERSION:=1
PKG_VERSION:=1.0.0
PKG_BASE_NAME:=testvpn
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)

PKG_CAPABILITIES:=kill net_bind_service net_admin net_raw syslog sys_admin sys_time


PKG_SELF_INTERFACE:=com.jarvis.testvpn1.intf1 com.jarvis.testvpn1.intf2
PKG_NEED_ROOT:=1
PKG_ACCESS_STORAGE:=1
PKG_CPU_SHARE:=128
#size can use K,M
PKG_MEMORY_LIMIT:=8M
PKG_USE_GWSYS_PATH:=/com/ctc/igd1/Info /com/ctc/igd1/Config /com/ctc/igd1/Storage /com/ctc/igd1/Network
#size can use K,M
PKG_MAX_STORAGE_SIZE:=6M
#low medium high
PKG_PLUGIN_PRIORITY:=high

#PKG_TCP_PORTS:=5663
PKG_UDP_PORTS:=1701

PKG_SELF_UDS:=/uds/testvpn/testvpn.uds


include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/$(PKG_NAME)
  SECTION:=base
  CATEGORY:=Upointech
  TITLE:= Test VPN service
  URL:=http://www.chinatelecom.com.cn/
  MAINTAINER:=Jarvis
  DEPENDS:=glib2 libcapi libubox
endef

ifeq ($(CONFIG_TARGET_rtl9600_rtl9600),y)
CONFIG_TESTAPP_WITH_ARCH:=mips-rsdk
else ifeq ($(CONFIG_TARGET_sd5116_sd5116),y)
CONFIG_TESTAPP_WITH_ARCH:=arm-a9le
endif

# make the submake silent:
ifeq ($(QUIET),1)
MAKE_FLAGS+=-s
endif
ifneq (,$(findstring 99,$(V)))
MAKE_FLAGS+=-s
endif

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)
endef


define Build/Clean
	rm -rf $(PKG_BUILD_DIR)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/opt/apps/$(PKG_NAME)/files
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/testvpn $(1)/opt/apps/$(PKG_NAME)/files
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
