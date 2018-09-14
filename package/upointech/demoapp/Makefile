#
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_VENDOR:=upointech
PKG_NAME:=demoapp
PKG_VERSION:=2.0.0
PKG_DBUS_VERSION:=1
PKG_BASE_NAME:=demoapp
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)


#allow some system calls
PKG_CAPABILITIES:=kill chown sys_time syslog sys_boot
#interfaces provided
PKG_SELF_INTERFACE:=com.upointech.demoapp1.service com.upointech.demoapp1.mm
#interface requires
PKG_USE_INTERFACE:=com.upointech.testapp1.intf1 com.upointech.testapp1.intf2
#interface vendor requires
PKG_USE_GWSYS_PATH:=/com/ctc/igd1/Info
#interested events
PKG_SUBSCRIBE_EVENT:=com.upointech.testapp1.event1
#data socket
PKG_SELF_UDS:=/uds/demoapp/demoapp.uds
PKG_USE_UDS:=/uds/testapp/testapp.uds
#kernel modules loaded before app
PKG_KERNEL_MODULES:=demoapp.ko
#requires proc read/write permision
PKG_PROC_RW:=/proc/demoapp
PKG_NEED_ROOT:=1
#from 0 to 1024
PKG_CPU_SHARE:=128
#size can use K,M
PKG_MEMORY_LIMIT:=2M
#from 100 to 1000
#PKG_BLKIO_WEIGHT:=200
PKG_MAX_STORAGE_SIZE:=100K
PKG_PLUGIN_PRIORITY=high


include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/$(PKG_NAME)
  SECTION:=base
  CATEGORY:=Upointech
  TITLE:= Smart gateway demo application
  URL:=http://www.upointech.com/
  MAINTAINER:=Upointech
  DEPENDS:=glib2 libcapi libubox +testapp
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
/etc/config/demoapp
endef

define Build/Clean
	rm -rf $(PKG_BUILD_DIR)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/opt/apps/$(PKG_NAME)/files
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/demoapp $(1)/opt/apps/$(PKG_NAME)/files
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libdemo.so $(1)/opt/apps/$(PKG_NAME)/files
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/demoapp.config $(1)/etc/config/demoapp
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
