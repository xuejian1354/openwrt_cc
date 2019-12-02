#
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_VENDOR:=xbsafe
PKG_NAME:=nets
PKG_DBUS_VERSION:=1
PKG_VERSION:=1.1.7
PKG_BASE_NAME:=nets
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)

PKG_CAPABILITIES:=net_admin net_raw
PKG_SELF_INTERFACE:=com.xbsafe.nets1.intf1 com.xbsafe.nets1.intf2
PKG_SAME_NTWK_NS:=1
PKG_NEED_ROOT:=1
PKG_ACCESS_STORAGE:=0
PKG_CPU_SHARE:=1000
#size can use K,M
PKG_MEMORY_LIMIT:=20M
PKG_USE_GWSYS_PATH:=/com/ctc/igd1/Info /com/ctc/igd1/Config /com/ctc/igd1/Storage
#size can use K,M
PKG_MAX_STORAGE_SIZE:=5M
#low medium high
PKG_PLUGIN_PRIORITY:=low

#PKG_TCP_PORTS:=5663
#PKG_UDP_PORTS:=3214 7712

PKG_SELF_UDS:=/uds/nets/nets.uds

#PKG_USE_INTERFACE:=
#PKG_SUBSCRIBE_EVENT:=com.xbsafe.test2.event1


include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/$(PKG_NAME)
  SECTION:=base
  CATEGORY:=xbsafe
  TITLE:= Smart gateway speed test
  URL:=http://www.xbsafe.com/
  MAINTAINER:=xbsafe
  DEPENDS:=glib2 libcapi3 libubox +librt
endef

#define Package/$(PKG_NAME)/extra_provides
#	echo 'libstdc++.so.6' 
#endef

define Package/$(PKG_NAME)/description
   Speed Test for smart gateway device
endef


# make the submake silent:
ifeq ($(QUIET),1)
MAKE_FLAGS+=-s
endif
ifneq (,$(findstring 99,$(V)))
MAKE_FLAGS+=-s
endif

CMAKE_OPTIONS += -DTOPDIR=$(TOPDIR)

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)
endef


define Build/Clean
	rm -rf $(PKG_BUILD_DIR)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/opt/apps/$(PKG_NAME)/files
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/nets $(1)/opt/apps/$(PKG_NAME)/files
	$(INSTALL_BIN) ./files/xtables-multi_$(CONFIG_TARGET_ARCH_PACKAGES) $(1)/opt/apps/$(PKG_NAME)/files/xtables-multi
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/default_xbnets.ini $(1)/etc/config/default_xbnets.ini	
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
