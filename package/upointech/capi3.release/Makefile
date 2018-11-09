#
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=libcapi3
PKG_VERSION:=1
PKG_FLAGS:=essential
PKG_BASE_NAME:=libcapi3
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
SVN_REVISION:=R2623

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
	$(CP) ./libcapi3.so.rtl9600 $(PKG_BUILD_DIR)/libcapi3.so
endif
ifneq ($(CONFIG_TARGET_rtl9600c),)
	$(CP) ./libcapi3.so.rtl9600c $(PKG_BUILD_DIR)/libcapi3.so
endif
ifneq ($(CONFIG_TARGET_bcm6838),)
	$(CP) ./libcapi3.so.brcm6838 $(PKG_BUILD_DIR)/libcapi3.so
endif
ifneq ($(CONFIG_TARGET_bcm6848),)
	$(CP) ./libcapi3.so.brcm6848 $(PKG_BUILD_DIR)/libcapi3.so
endif
ifneq ($(CONFIG_TARGET_bcm6846),)
	$(CP) ./libcapi3.so.brcm6858 $(PKG_BUILD_DIR)/libcapi3.so
endif
ifneq ($(CONFIG_TARGET_bcm6858),)
	$(CP) ./libcapi3.so.brcm6858 $(PKG_BUILD_DIR)/libcapi3.so
endif
ifneq ($(CONFIG_TARGET_bcm6856),)
	$(CP) ./libcapi3.so.brcm6856 $(PKG_BUILD_DIR)/libcapi3.so
endif
ifneq ($(CONFIG_TARGET_sd5117),)
	$(CP) ./libcapi3.so.sd5117 $(PKG_BUILD_DIR)/libcapi3.so
endif
ifneq ($(CONFIG_TARGET_mt7526),)
	$(CP) ./libcapi3.so.mt7526 $(PKG_BUILD_DIR)/libcapi3.so
endif
ifneq ($(CONFIG_TARGET_mt7580),)
	$(CP) ./libcapi3.so.mt7580 $(PKG_BUILD_DIR)/libcapi3.so
endif
ifneq ($(CONFIG_TARGET_zx279127),)
	$(CP) ./libcapi3.so.zx27912X32 $(PKG_BUILD_DIR)/libcapi3.so
endif
ifneq ($(CONFIG_TARGET_zx279128),)
	$(CP) ./libcapi3.so.zx27912X32 $(PKG_BUILD_DIR)/libcapi3.so
endif
ifneq ($(CONFIG_TARGET_zx279131),)
	$(CP) ./libcapi3.so.zx279131 $(PKG_BUILD_DIR)/libcapi3.so
endif
ifneq ($(CONFIG_TARGET_zxic127),)
	$(CP) ./libcapi3.so.zx27912X64 $(PKG_BUILD_DIR)/libcapi3.so
endif
ifneq ($(CONFIG_TARGET_zxic127v1),)
	$(CP) ./libcapi3.so.zx27912XV164 $(PKG_BUILD_DIR)/libcapi3.so
endif
ifneq ($(CONFIG_TARGET_zxic128),)
	$(CP) ./libcapi3.so.zx27912X64 $(PKG_BUILD_DIR)/libcapi3.so
endif
ifneq ($(CONFIG_TARGET_zxic128v1),)
	$(CP) ./libcapi3.so.zx27912XV164 $(PKG_BUILD_DIR)/libcapi3.so
endif
ifneq ($(CONFIG_TARGET_hi5681),)
	$(CP) ./libcapi3.so.hi5681 $(PKG_BUILD_DIR)/libcapi3.so
endif
ifneq ($(CONFIG_TARGET_hi5662),)
	$(CP) ./libcapi3.so.hi5662 $(PKG_BUILD_DIR)/libcapi3.so
endif
ifneq ($(CONFIG_TARGET_ca8277),)
	$(CP) ./libcapi3.so.ca8277 $(PKG_BUILD_DIR)/libcapi3.so
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
	$(INSTALL_DIR) $(STAGING_DIR)/usr/include/$(PKG_NAME)
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/libcapi3.so $(1)/usr/lib/	

	$(CP) $(PKG_BUILD_DIR)/include/capi.h $(STAGING_DIR)/usr/include/$(PKG_NAME)/
	$(CP) $(PKG_BUILD_DIR)/include/capi-variable-type.h $(STAGING_DIR)/usr/include/$(PKG_NAME)/
	$(CP) $(PKG_BUILD_DIR)/include/capi-dbus-service.h $(STAGING_DIR)/usr/include/$(PKG_NAME)/
	$(CP) $(PKG_BUILD_DIR)/include/capi-sys-service.h $(STAGING_DIR)/usr/include/$(PKG_NAME)/
	$(CP) $(PKG_BUILD_DIR)/include/capi-app-mgr.h $(STAGING_DIR)/usr/include/$(PKG_NAME)/
	$(CP) $(PKG_BUILD_DIR)/include/capi-util.h $(STAGING_DIR)/usr/include/$(PKG_NAME)/
	$(CP) $(PKG_BUILD_DIR)/include/ctsgw.h $(STAGING_DIR)/usr/include/$(PKG_NAME)/
	$(CP) $(PKG_BUILD_DIR)/libcapi3.so $(STAGING_DIR)/usr/lib
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
