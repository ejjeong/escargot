#!/bin/bash
sudo apt-get install libatomic-ops-dev

cd third_party/bdwgc/
./autogen.sh
make distclean

mkdir -p out/release
mkdir -p out/debug

CONFFLAGS=" --enable-cplusplus " # --enable-large-config "

cd out/release
../../configure $CONFFLAGS --disable-gc-debug
make -j
cd ../..

cd out/debug
../../configure $CONFFLAGS
make -j
cd ../..
