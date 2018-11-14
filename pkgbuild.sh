#!/bin/bash

# ctc-pkg-build -- construct a ChinaTelecom recognized tar package from the .ipk
# Ali.Dong <szdong@upointech.com>
# based on a script by Steve Redler IV, steve@sr-tech.com 5-21-2001
# 2016-06-08 szdong@upointech.com
#   Updated to work on Familiar Pre0.7rc1, with busybox tar.
#   Note it Requires: binutils-ar (since the busybox ar can't create)
#   For UID debugging it needs a better "find".
set -e

BASEDIR=$(pwd)
version=1.0
FIND="$(which find)"
FIND="${FIND:-$(which gfind)}"
TAR="${TAR:-$(which tar)}"

ipkg_extract_value() {
	sed -e "s/^[^:]*:[[:space:]]*//"
}

required_field() {
        field=$1
        target_dir=$2

        grep "^$field:" < $target_dir/control | ipkg_extract_value

}

# ./generate_app_json [control_dir]
generate_app_json() {

        local control_dir=$1
	local vendor_name=$2

        if [ "$control_dir" = "." -o "$control_dir" = "./" ] ; then
            control_dir=$PWD
        fi

        # echo "generate_app_json: $control_dir"

        pkg=`required_field Package $control_dir`
        version=`required_field Version $control_dir | sed 's/Version://; s/^.://g;'`
        arch=`required_field Architecture $control_dir`
        appsign=com.chinatelecom.all.smartgateway.${pkg}_${arch}
        desp=`required_field Description $control_dir`
        author=`required_field Maintainer $control_dir`
        rom=`required_field Installed-Size $control_dir`
        ram=`required_field MemoryLimit $control_dir`

        if [ ! -f $control_dir/control ]; then
                echo "***Error: Control file $control_dir/control is not exist."
                return 1
        fi

        # Now generate the app.json file
        echo "{" > $control_dir/app.json

        echo -e "\t\"PlatformID\": \"OPKG\"," >> $control_dir/app.json
        echo -e "\t\"appsign\": \"${appsign}${vendor_name}\"," >> $control_dir/app.json
        echo -e "\t\"appname\": \"游戏加速器\"," >> $control_dir/app.json
        echo -e "\t\"version\": \"$version\"," >> $control_dir/app.json
        echo -e "\t\"dep_version\": \"\"," >> $control_dir/app.json
        echo -e "\t\"description\": \"$desp\"," >> $control_dir/app.json
        echo -e "\t\"author\": \"$author\"," >> $control_dir/app.json
        echo -e "\t\"URL\": \"www.example.com\"," >> $control_dir/app.json
        echo -e "\t\"email\": \"test@example.com\"," >> $control_dir/app.json
        echo -e "\t\"icon\": \"$col_file\"," >> $control_dir/app.json
        echo -e "\t\"icon1\": \"$mono_file\"," >> $control_dir/app.json
        echo -e "\t\"rom_use\": \"$rom\"," >> $control_dir/app.json
        echo -e "\t\"ram_use\": \"$ram\"," >> $control_dir/app.json
        echo -e "\t\"install_file\": \"install/${appsign}.ipk\"," >> $control_dir/app.json
        echo -e "\t\"thirdparty\": \"\"," >> $control_dir/app.json
        echo -e "\t\"package_name\": \"\"," >> $control_dir/app.json
        echo -e "\t\"activity_name\": \"\"," >> $control_dir/app.json
        echo -e "\t\"isdefaultapp\": \"0\"," >> $control_dir/app.json
        echo -e "\t\"configtype\": \"0\"," >> $control_dir/app.json
        echo -e "\t\"configurl\": \"\"," >> $control_dir/app.json
        echo -e "\t\"configurl_mobile\": \"\"" >> $control_dir/app.json
        echo "}" >> $control_dir/app.json

        return 0
}

###
# ctc-pkg-build "main"
###
ogargs="--owner=0 --group=0"
usage="Usage: $0 <ipk_path> [vendor]"


# continue on to process additional arguments
if [ $# -ne 1 ] && [ $# -ne 2 ]; then
        echo $usage  >&2
        exit 1
fi

if [ ! -f $1 ]; then
	echo "*** Error: ipk file $1 does not exist." >&2
        exit 1
fi

# parse the path string
pathstr=$1
if [ ${pathstr:0:1} = '/' ]; then
        ipk_name=$1
else
        ipk_name=${BASEDIR}/$1
fi

# Parse the path to colorful icon
pathstr="package/upointech/transvpnv3/icon/ispeed64black.png"
if [ ${pathstr:0:1} = '/' ]; then
        col_icon=$pathstr
else
        col_icon=${BASEDIR}/$pathstr
fi

col_file=${col_icon##*/}

# Parse the path to black-and-white icon
pathstr="package/upointech/transvpnv3/icon/ispeed64.png"
if [ ${pathstr:0:1} = '/' ]; then
        mono_icon=$pathstr
else
        mono_icon=${BASEDIR}/$pathstr
fi

#Parse the vendor, this is a optional parameter
if [ $# -eq 2 ]; then
	vendor=_$2
else
	vendor=""
fi

mono_file=${mono_icon##*/}

dest_dir=${ipk_name%/*}
tmp_dir=$dest_dir/CTC_PKG_BUILD.$$
mkdir $tmp_dir
mkdir $tmp_dir/tmp

( $TAR -zxf $ipk_name -C $tmp_dir/tmp && $TAR -zxf $tmp_dir/tmp/control.tar.gz -C $tmp_dir/tmp )
if [ ! -f $tmp_dir/tmp/control ]; then
	echo "*** Error: No control file found in $tmp_dir." >&2
	rm -rf $tmp_dir
	exit 1
fi

cp $tmp_dir/tmp/control $tmp_dir
rm -rf $tmp_dir/tmp
if ! generate_app_json $tmp_dir $vendor; then
	echo >&2
	echo "ctc-ipk-build: Please fix the above errors and try again." >&2
	rm -rf $tmp_dir
	exit 1
fi

rm -f $tmp_dir/control
ctc_ipk_str=com.chinatelecom.all.smartgateway.${pkg}_${arch}
#echo $ctc_ipk_str
ctc_tar_file=$dest_dir/${ctc_ipk_str}.tar.gz
rm -f $ctc_tar_file

( cd $tmp_dir && mkdir install && cp $ipk_name install/${ctc_ipk_str}.ipk && cp $col_icon ./ && cp $mono_icon ./ && $TAR $ogargs --format=gnu -czf ${ctc_tar_file} ./app.json ./install ./${col_file} ./${mono_file} )
rm -rf $tmp_dir

echo "Generated CTC package ${ctc_ipk_str}.tar.gz to $dest_dir"

find -L bin/ -name *.ipk | grep "packages/base" | grep -v "transvpn" | xargs rm
