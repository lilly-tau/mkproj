#!/usr/bin/env bash

CFLAGS="-std=c90 -Wpedantic"
DEBUG=none
TEST=false
INSTALL=false

while getopts "ditv" OPT; do
	case $OPT in
	d) DEBUG=gdb;;
	i) INSTALL=true;;
	t) TEST=true;;
	v) DEBUG=valgrind;;
	esac
done

if [[ "$DEBUG" = none ]]; then
	CFLAGS="$CFLAGS -ffunction-sections -fdata-sections -Wl,--gc-sections"
else
	CFLAGS="$CFLAGS -g"
fi

rm -rf ./build
mkdir ./build
chmod +w ./build
cc $CFLAGS -o build/mkproj src/*.c || exit 1
chmod +x build/mkproj

if [[ "$TEST" = true ]]; then
	if [[ "$DEBUG" = gdb ]]; then
		gdb -ex "set args $TEST_ARGS" build/mkproj
	elif [[ "$DEBUG" = valgrind ]]; then
		valgrind build/mkproj $TEST_ARGS
	else
		build/mkproj -ctest.mkproj -thelp
		build/mkproj -ctest.mkproj -tC -Dbuild=bash
		build/mkproj -ctest.mkproj -tC -Dbuild=make
	fi
fi

if [[ "$INSTALL" = true ]]; then
	cp build/mkproj /usr/bin/mkproj
fi
