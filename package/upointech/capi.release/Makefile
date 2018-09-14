#
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=libcapi
PKG_VERSION:=1
PKG_FLAGS:=essential
PKG_BASE_NAME:=libcapi
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
SVN_REVISION:=R1524

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=base
  CATEGORY:=Upointech
  TITLE:= DBUS C-API for smart gateway
  URL:=http://www.upointech.com/
  MAINTAINER:=Upointech
  DEPENDS:=+glib2
  VERSION:=$(SVN_REVISION)
endef

define Package/$(PKG_NAME)/description
   dbus c-api for smart gateway device
endef


define Build/Prepare	
	export BUILD_DIR
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./include $(PKG_BUILD_DIR)	
ifneq ($(CONFIG_TARGET_rtl9600),)
	$(CP) ./libcapi.so.rtl9600 $(PKG_BUILD_DIR)/libcapi.so
endif
ifneq ($(CONFIG_TARGET_rtl9600c),)
	$(CP) ./libcapi.so.rtl9600c $(PKG_BUILD_DIR)/libcapi.so
endif
ifneq ($(CONFIG_TARGET_bcm6838),)
	$(CP) ./libcapi.so.brcm6838 $(PKG_BUILD_DIR)/libcapi.so
endif
ifneq ($(CONFIG_TARGET_BRCM6858),)
	$(CP) ./libcapi.so.brcm6858 $(PKG_BUILD_DIR)/libcapi.so
endif
ifneq ($(CONFIG_TARGET_sd5116),)
	$(CP) ./libcapi.so.sd5116 $(PKG_BUILD_DIR)/libcapi.so
endif
ifneq ($(CONFIG_TARGET_sd5116v1),)
	$(CP) ./libcapi.so.sd5116v1 $(PKG_BUILD_DIR)/libcapi.so
endif
ifneq ($(CONFIG_TARGET_mt7526),)
	$(CP) ./libcapi.so.mt7526 $(PKG_BUILD_DIR)/libcapi.so
endif
ifneq ($(CONFIG_TARGET_zx279100),)
	$(CP) ./libcapi.so.zte279100 $(PKG_BUILD_DIR)/libcapi.so
endif
ifneq ($(CONFIG_TARGET_ZTE279121),)
	$(CP) ./libcapi.so.zx27912X32 $(PKG_BUILD_DIR)/libcapi.so
endif
ifneq ($(CONFIG_TARGET_ZXIC2791),)
	$(CP) ./libcapi.so.zx27912X64 $(PKG_BUILD_DIR)/libcapi.so
endif
ifneq ($(CONFIG_TARGET_hi5681),)
	$(CP) ./libcapi.so.hi5681 $(PKG_BUILD_DIR)/libcapi.so
endif
endef

define Build/Compile	
	echo "use binary do nothing"
endef

define Build/Clean
	rm -rf $(PKG_BUILD_DIR)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/libcapi.so $(1)/usr/lib/	

	$(CP) $(PKG_BUILD_DIR)/include/capi.h $(STAGING_DIR)/usr/include
	$(CP) $(PKG_BUILD_DIR)/include/capi-variable-type.h $(STAGING_DIR)/usr/include
	$(CP) $(PKG_BUILD_DIR)/include/capi-dbus-service.h $(STAGING_DIR)/usr/include
	$(CP) $(PKG_BUILD_DIR)/include/capi-sys-service.h $(STAGING_DIR)/usr/include
	$(CP) $(PKG_BUILD_DIR)/include/capi-app-mgr.h $(STAGING_DIR)/usr/include
	$(CP) $(PKG_BUILD_DIR)/include/capi-util.h $(STAGING_DIR)/usr/include
	$(CP) $(PKG_BUILD_DIR)/include/ctsgw.h $(STAGING_DIR)/usr/include
	$(CP) $(PKG_BUILD_DIR)/libcapi.so $(STAGING_DIR)/usr/lib
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
