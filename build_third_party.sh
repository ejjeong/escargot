#!/bin/bash

###########################################################
# GC build
###########################################################
cd third_party/bdwgc/
./autogen.sh
make distclean

mkdir -p out/release
mkdir -p out/debug

CONFFLAGS=" " # --enable-large-config --enable-cplusplus"

cd out/release
../../configure $CONFFLAGS --disable-gc-debug
make -j
cd ../..

cd out/debug
../../configure $CONFFLAGS
make -j
cd ../..

cd ../..

###########################################################
# SpiderMonkey build
###########################################################
rm -rf third_party/mozjs/build
mkdir -p third_party/mozjs/build
cd third_party/mozjs/build
../js/src/configure
make -j 24
cd ../../..

