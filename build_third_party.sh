#!/bin/bash
sudo apt-get install libatomic-ops-dev

cd third_party/bdwgc/
./autogen.sh
make distclean

mkdir -p out/release
mkdir -p out/debug

cd out/release
../../configure --enable-cplusplus --disable-gc-debug
make -j
cd ../..

cd out/debug
../../configure --enable-cplusplus
make -j
cd ../..
