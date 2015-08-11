#!/bin/bash

if [ ! -f /proc/cpuinfo ]; then
	echo "Is this Linux? Cannot find or read /proc/cpuinfo"
	exit 1
fi
NUMPROC=$(grep 'processor' /proc/cpuinfo | wc -l)


###########################################################
# GC build
###########################################################
cd third_party/bdwgc/
./autogen.sh
make distclean

mkdir -p out/release
mkdir -p out/debug

GCCONFFLAGS=" --disable-parallel-mark " # --enable-large-config --enable-cplusplus"

cd out/release
../../configure $GCCONFFLAGS --disable-gc-debug
make -j$NUMPROC
cd ../..

cd out/debug
../../configure $GCCONFFLAGS
make -j$NUMPROC
cd ../..

cd ../..

###########################################################
# SpiderMonkey build
###########################################################
rm -rf third_party/mozjs/build
mkdir -p third_party/mozjs/build

MOZJSFLAGS=" --disable-shared-js --disable-tests --disable-ion --disable-yarr-jit " # to make build faster

cd third_party/mozjs/build
../js/src/configure $MOZJSFLAGS
make -j$NUMPROC
cd ../../..
