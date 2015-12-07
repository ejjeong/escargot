#!/bin/bash

if [ ! -f /proc/cpuinfo ]; then
	echo "Is this Linux? Cannot find or read /proc/cpuinfo"
	exit 1
fi
NUMPROC=$(grep 'processor' /proc/cpuinfo | wc -l)


if [ -z "$TIZEN_SDK_HOME" ]; then
  echo "Need to set TIZEN_SDK_HOME"
  exit 1
fi

echo "TIZEN_SDK_HOME env is ..."
echo $TIZEN_SDK_HOME
# TIZEN_SYS_ROOT=$TIZEN_SDK_HOME/platforms/mobile-2.3/rootstraps/mobile-2.3-device.core
TIZEN_SYS_ROOT=$TIZEN_SDK_HOME/platforms/tizen-2.4/mobile/rootstraps/mobile-2.4-device.core

###########################################################
# GC build
###########################################################
cd third_party/bdwgc/
autoreconf -vif
automake --add-missing
#./autogen.sh
#make distclean

rm -rf out

mkdir -p out/x86/release
mkdir -p out/x86/debug
mkdir -p out/x64/release
mkdir -p out/x64/debug
mkdir -p out/arm/release
mkdir -p out/arm/release.shared
mkdir -p out/arm/debug

GCCONFFLAGS=" --disable-parallel-mark " # --enable-large-config --enable-cplusplus"

cd out/x86/release
../../../configure $GCCONFFLAGS --disable-gc-debug CFLAGS='-m32' CXXFLAGS='-m32' LDFLAGS='-m32'
make -j$NUMPROC
cd -

cd out/x86/debug
../../../configure $GCCONFFLAGS CFLAGS='-m32 -g3' CXXFLAGS='-m32' LDFLAGS='-m32' --enable-debug
make -j$NUMPROC
cd -

cd out/x64/release
../../../configure $GCCONFFLAGS --disable-gc-debug
make -j$NUMPROC
cd -

cd out/x64/debug
../../../configure $GCCONFFLAGS --enable-debug CFLAGS='-g3'
make -j$NUMPROC
cd -

cd out/arm/release
../../../configure $GCCONFFLAGS --disable-gc-debug --with-sysroot=$TIZEN_SYS_ROOT --host=arm-linux-gnueabi CFLAGS='-march=armv7-a'
make -j$NUMPROC
cd -

cd out/arm/release.shared
../../../configure $GCCONFFLAGS --disable-gc-debug --with-sysroot=$TIZEN_SYS_ROOT --host=arm-linux-gnueabi CFLAGS='-fPIC -march=armv7-a'
make -j$NUMPROC
cd -

cd out/arm/debug
../../../configure $GCCONFFLAGS --enable-debug --with-sysroot=$TIZEN_SYS_ROOT --host=arm-linux-gnueabi CFLAGS='-g3 -march=armv7-a'
make -j$NUMPROC
cd -
