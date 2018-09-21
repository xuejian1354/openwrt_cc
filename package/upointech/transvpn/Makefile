#
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_VENDOR:=upointech
PKG_NAME:=transvpn
PKG_DBUS_VERSION:=1
PKG_VERSION:=2.0.0
PKG_BASE_NAME:=transvpn
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)

PKG_CAPABILITIES:=chown dac_override dac_read_search fowner fsetid kill setgid setuid setpcap linux_immutable net_bind_service net_broadcast net_admin net_raw ipc_lock ipc_owner sys_module sys_rawio sys_chroot sys_ptrace sys_pacct sys_admin sys_boot sys_nice sys_resource sys_time sys_tty_config mknod lease audit_write audit_control setfcap


PKG_SELF_INTERFACE:=com.upointech.transvpn1.intf1 com.upointech.transvpn1.intf2
PKG_NEED_ROOT:=1
PKG_ACCESS_STORAGE:=1
PKG_CPU_SHARE:=128
#size can use K,M
PKG_MEMORY_LIMIT:=2800K
PKG_USE_GWSYS_PATH:=/com/ctc/igd1/Info /com/ctc/igd1/Config /com/ctc/igd1/Storage /com/ctc/igd1/Network
#size can use K,M
PKG_MAX_STORAGE_SIZE:=6M
#low medium high
PKG_PLUGIN_PRIORITY:=high

#PKG_TCP_PORTS:=5663
PKG_UDP_PORTS:=1701

PKG_SELF_UDS:=/uds/transvpn/transvpn.uds

#PKG_USE_INTERFACE:=
#PKG_SUBSCRIBE_EVENT:=com.upointech.test2.event1


include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/$(PKG_NAME)
  SECTION:=base
  CATEGORY:=Upointech
  TITLE:= Smart gateway test application
  URL:=http://www.upointech.com/
  MAINTAINER:=Upointech
  DEPENDS:=glib2 libcapi libubox +librt
endef
#DEPENDS:=glib2 libcapi libubox +librt +libip4tc libip6tc +libxtables +kmod-ipt-core +kmod-ipt-conntrack +kmod-ipt-nat

define Package/$(PKG_NAME)/description
   Test Application for smart gateway device
endef

#define Package/$(PKG_NAME)/config
#	config TESTAPP_WITH_ARCH
#		string "Configuration arch"
#		default ""
#		depends on PACKAGE_$(PKG_NAME)
#endef

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
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/transvpn $(1)/opt/apps/$(PKG_NAME)/files
	tar -xf ./files/transite-$(CONFIG_TESTAPP_WITH_ARCH).tar.gz -C $(1)/opt/apps/$(PKG_NAME)/files
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
