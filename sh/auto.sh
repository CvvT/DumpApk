#! /bin/sh

help(){
	echo "./auto.sh [push|pull]"
	echo "push: push the libdump into dir /system/lib/"
	echo "pull: when you has dumped the dex, pull it from device"
}

if [ $1 ]
then
	if [ $1 == 'push' ]
	then
		adb push libdump.so /data/local/tmp/
		adb shell < instr.txt
	elif [ $1 == 'pull' ]
	then
		adb shell < pull.txt
		adb pull /data/local/tmp/whole.dex
	elif [ $1 == 'clean' ]
	then
		adb shell < clean.txt
	fi
else
	help
fi
