#!/bin/bash

if [ -z "$ANDROID_NDK" ]; then
    echo "Need to set ANDROID_NDK"
    exit 1
fi

echo "ANDROID_NDK env is ..."
echo $ANDROID_NDK

cd android

TOOL_CHAIN_VERSION=4.8 REACT_NATIVE=1 V=1 BUILD_OBJECT=so $ANDROID_NDK/ndk-build -j -B
