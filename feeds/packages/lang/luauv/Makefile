#
# Copyright (C) 2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=luauv
PKG_VERSION:=2016-12-16
PKG_RELEASE:=$(PKG_SOURCE_VERSION)

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/luvit/luv.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=1.9.1-1
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

CMAKE_OPTIONS += -DWITH_LUA_ENGINE=Lua \
	-DCMAKE_C_FLAGS="$(CFLAGS)" \
	-DCMAKE_MODULE_LINKER_FLAGS="$(LDFLAGS)" \
	-DLUA_LIBDIR="$(STAGING_DIR)/usr/lib" \
	-DLUA_INCDIR="$(STAGING_DIR)/usr/include" \
	-DLUA="$(STAGING_DIR)/usr/bin/lua" \
	-DLIBDIR="/usr/lib/lua" \
	-DLUADIR="/usr/bin"

define Package/luauv
  SUBMENU:=Lua
  SECTION:=lang
  CATEGORY:=Languages
  TITLE:=Bare libuv bindings for lua
  URL:=https://github.com/luvit/luv.git
  DEPENDS:=+lua +libpthread +librt
endef

define Package/luauv/install
	$(INSTALL_DIR) $(1)/usr/lib/lua
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lua/luv.so $(1)/usr/lib/lua
endef

$(eval $(call BuildPackage,luauv))
