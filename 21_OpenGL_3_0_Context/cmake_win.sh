#!/bin/bash -e

if [ ! -n "$1" ];then
	echo "--- make ---"
else
	echo "--- make clean ---"
	if [ $1="clear" ];then #输入参数 clear 时，清楚原来文件，重新编译
		rm -rf "$(pwd)/build"
	fi
fi

mkdir -p build
cd build


cmake -G "Visual Studio 16 2019" -A x64 ../

echo "output path = $(pwd)/bin/win"