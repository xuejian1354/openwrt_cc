#!/bin/sh

SAF_PATH=/opt/upt/framework/saf/rootfs
SAF_APP_PATH=/opt/upt/apps/apps/

showver() {
  if [ -f /usr/lib/opkg/info/$1.control ]; then
    echo -en "Package:" $1 '\t' $2
    echo `grep Version /usr/lib/opkg/info/$1.control`
  fi

  if [ -f $SAF_APP_PATH/usr/lib/opkg/info/$1.control ]; then
    echo -en "Package:" $1 '\t' $2
    echo `grep Version $SAF_APP_PATH/usr/lib/opkg/info/$1.control`	
  elif [ -f $SAF_PATH/usr/lib/opkg/info/$1.control ]; then
    echo -en "Package:" $1 '\t' $2
    echo `grep Version $SAF_PATH/usr/lib/opkg/info/$1.control`	  
  fi
}

if [ -f /etc/openwrt_release ]; then
    cat /etc/openwrt_release
fi
if [ -f $SAF_PATH/etc/openwrt_release ]; then
    cat $SAF_PATH/etc/openwrt_release
fi
echo -e "\n-----------------------------------------------------"
showver cloudclient '\t'
showver appmgr '\t'
showver libcapi '\t'
showver libcapi-lua '\t'
showver luci-base '\t'
showver luci-mod-admin-full