#!/bin/bash

if [ -z "$ANDROID_NDK" ]; then
    echo "Need to set ANDROID_NDK"
    exit 1
fi

echo "ANDROID_NDK env is ..."
echo $ANDROID_NDK

cd android

BUILD_TYPE=
BUILD_MODE=

PS3='Please enter your choice: '
options=("armeabi-v7a.32bit.interpreter.debug" "armeabi-v7a.32bit.interpreter.release" "armeabi-v7a.32bit.jit.debug" "armeabi-v7a.32bit.jit.release" "all")
select opt in "${options[@]}"
do
    case $opt in
        "armeabi-v7a.32bit.interpreter.debug")
            BUILD_TYPE=interpreter
            BUILD_ARCH=armeabi-v7a-hard
            BUILD_MODE=debug
            echo "you chose choice 1"
            break
            ;;
        "armeabi-v7a.32bit.interpreter.release")
            BUILD_TYPE=interpreter
            BUILD_ARCH=armeabi-v7a-hard
            BUILD_MODE=release
            echo "you chose choice 2"
            break
            ;;
        "armeabi-v7a.32bit.jit.debug")
            BUILD_TYPE=jit
            BUILD_ARCH=armeabi-v7a-hard
            BUILD_MODE=debug
            echo "you chose choice 3"
            break
            ;;
        "armeabi-v7a.32bit.jit.release")
            BUILD_TYPE=jit
            BUILD_ARCH=armeabi-v7a-hard
            BUILD_MODE=release
            echo "you chose choice 4"
            break
            ;;
        "all")
            BUILD_TYPE=interpreter
            BUILD_ARCH=
            BUILD_MODE=
            echo "you chose choice 5"
            break
            ;;
        *)
            echo invalid option;;
    esac
done

#echo BUILD_TYPE
#echo $BUILD_TYPE
#echo BUILD_MODE
#echo $BUILD_MODE
V=1 BUILD_ARCH=$BUILD_ARCH BUILD_TYPE=$BUILD_TYPE BUILD_MODE=$BUILD_MODE $ANDROID_NDK/ndk-build clean
V=1 BUILD_ARCH=$BUILD_ARCH BUILD_TYPE=$BUILD_TYPE BUILD_MODE=$BUILD_MODE $ANDROID_NDK/ndk-build -j
