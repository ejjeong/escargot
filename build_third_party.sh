#!/bin/bash
sudo apt-get install libatomic-ops-dev
cd third_party/bdwgc/
./autogen.sh
./configure
make -j
