#
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_VENDOR:=upointech
PKG_NAME:=testappv3
PKG_DBUS_VERSION:=1
PKG_VERSION:=2.0.0
PKG_BASE_NAME:=testappv3
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)

#PKG_CAPABILITIES:=kill chown sys_time syslog sys_boot sys_admin
PKG_SELF_INTERFACE:=com.upointech.testappv31.intf1 com.upointech.testappv31.intf2
PKG_NEED_ROOT:=0
PKG_ACCESS_STORAGE:=0
PKG_CPU_SHARE:=128
#size can use K,M
PKG_MEMORY_LIMIT:=1400K
PKG_USE_GWSYS_PATH:=/com/ctc/igd1/Info /com/ctc/igd1/Config /com/ctc/igd1/Storage
#size can use K,M
PKG_MAX_STORAGE_SIZE:=1M
#low medium high
PKG_PLUGIN_PRIORITY:=low

PKG_TCP_PORTS:=5663
PKG_UDP_PORTS:=3214 7712

PKG_SELF_UDS:=/uds/testappv3/testappv3.uds

#PKG_USE_INTERFACE:=


include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/$(PKG_NAME)
  SECTION:=base
  CATEGORY:=Upointech
  TITLE:= Smart gateway test application
  URL:=http://www.upointech.com/
  MAINTAINER:=Upointech
  DEPENDS:=glib2 libcapi3 libubox
endef

define Package/$(PKG_NAME)/description
   Test Application for smart gateway device
endef


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
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/testappv3 $(1)/opt/apps/$(PKG_NAME)/files/$(PKG_NAME)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
