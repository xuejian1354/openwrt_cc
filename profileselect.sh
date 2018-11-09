#!/bin/bash

BASEDIR=$(pwd)

set `ls *.config`

if [ $# -le 0 ]
then
	echo "No profile found!"
	exit 0
fi

echo "Profile to choose: "
i=0
for confile in $(ls *.config)
do
	let i+=1
	echo -e "\t$i) $confile"
done

profile="InvalidFile"
while true
do
	echo ""
	read -p "Please choose a profile [ 1 ~ $# ]: " index
	if [[ -z "$index" ]]; then
		printf '%s\n' "No profile selected."
		exit 1
	fi	
	if [ $index -gt $# ] || [ $index -lt 1 ]
	then
		echo "Invalid profile index $index, please choose again."
	else
		profile=`eval echo '$'{$index}`
	fi

	if [ $profile != "InvalidFile" ]
	then
		break
	fi
done

if [ -f ".config" ]
then
	unlink .config
fi

ln -sf $profile .config

echo ""
echo "You choose profile $profile, the soft link is established."
echo "Please execute 'make prepare' before compilling you application."

exit 0
