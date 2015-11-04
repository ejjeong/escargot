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
autoreconf -vif
automake --add-missing
#./autogen.sh
#make distclean

rm -rf out

mkdir -p out/x86/release
mkdir -p out/x86/debug
mkdir -p out/x64/release
mkdir -p out/x64/debug

GCCONFFLAGS=" --disable-parallel-mark " # --enable-large-config --enable-cplusplus"

cd out/x86/release
../../../configure $GCCONFFLAGS --disable-gc-debug CFLAGS='-m32' CXXFLAGS='-m32' LDFLAGS='-m32'
make -j$NUMPROC
cd -

cd out/x86/debug
../../../configure $GCCONFFLAGS CFLAGS='-m32' CXXFLAGS='-m32' LDFLAGS='-m32'
make -j$NUMPROC
cd -

cd out/x64/release
../../../configure $GCCONFFLAGS --disable-gc-debug
make -j$NUMPROC
cd -

cd out/x64/debug
../../../configure $GCCONFFLAGS
make -j$NUMPROC
cd -
