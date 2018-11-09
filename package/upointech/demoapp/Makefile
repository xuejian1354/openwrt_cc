#
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_VENDOR:=upointech
PKG_NAME:=demoappv3
PKG_VERSION:=2.0.0
PKG_DBUS_VERSION:=1
PKG_BASE_NAME:=demoappv3
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)


#allow some system calls
#PKG_CAPABILITIES:=syslog
#interfaces provided
PKG_SELF_INTERFACE:=com.upointech.demoappv31.service com.upointech.demoappv31.mm
#interface requires
PKG_USE_INTERFACE:=com.upointech.testappv31.intf1 com.upointech.testappv31.intf2
#interface vendor requires
PKG_USE_GWSYS_PATH:=/com/ctc/igd1/Info
#interested events
PKG_SUBSCRIBE_EVENT:=com.upointech.testappv31.event1
#data socket
PKG_SELF_UDS:=/uds/demoappv3/demoappv3.uds
PKG_USE_UDS:=/uds/testappv3/testappv3.uds
#kernel modules loaded before app
PKG_KERNEL_MODULES:=demoappv3.ko
#requires proc read/write permision
PKG_PROC_RW:=/proc/demoappv3
PKG_NEED_ROOT:=0
#from 0 to 1024
PKG_CPU_SHARE:=128
#size can use K,M
PKG_MEMORY_LIMIT:=2M
PKG_MAX_STORAGE_SIZE:=100K
PKG_PLUGIN_PRIORITY=low


include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/$(PKG_NAME)
  SECTION:=base
  CATEGORY:=Upointech
  TITLE:= Smart gateway demo application
  URL:=http://www.upointech.com/
  MAINTAINER:=Upointech
  DEPENDS:=glib2 libcapi3 libubox +testappv3
endef

define Package/$(PKG_NAME)/description
  Demo Application for smart gateway device
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

define Package/$(PKG_NAME)/conffiles
/etc/config/demoappv3
endef

define Build/Clean
	rm -rf $(PKG_BUILD_DIR)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/opt/apps/$(PKG_NAME)/files
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/demoappv3 $(1)/opt/apps/$(PKG_NAME)/files/$(PKG_NAME)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libdemov3.so $(1)/opt/apps/$(PKG_NAME)/files
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/demoappv3.config $(1)/etc/config/demoappv3
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
