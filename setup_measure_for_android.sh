#!/bin/bash

function build-interp() {
  ./build_android.sh armeabi-v7a.32bit.interpreter.release
  mkdir -p test/bin/arm32/escargot/interp
  mkdir -p test/bin/arm64/escargot/interp
  cp ./android/libs/armeabi-v7a/escargot ./test/bin/arm32/escargot/interp/escargot.release
  ./build_android.sh arm64-v8a.64bit.interpreter.release
  cp ./android/libs/arm64-v8a/escargot ./test/bin/arm64/escargot/interp/escargot.release
}

function build-jit() {
  ./build_android.sh armeabi-v7a.32bit.jit.release
  mkdir -p test/bin/arm32/escargot/jit
  cp ./android/libs/armeabi-v7a/escargot ./test/bin/arm32/escargot/jit/escargot.release
  ./build_android.sh armeabi-v7a.32bit.jit.debug
  cp ./android/libs/armeabi-v7a/escargot ./test/bin/arm32/escargot/jit/escargot.debug
}

if [[ "$1" = "build-all" ]]; then
  build-interp
  build-jit
elif [[ "$1" = "build-jit" ]]; then
  build-jit
elif [[ "$1" = "build-interp" ]]; then
  build-interp
fi

adb push ./run-Sunspider.sh /data/local/tmp
adb push ./measure_for_android.sh /data/local/tmp
adb push ./test/bin/memps_arm /data/local/tmp
adb shell mkdir -p /data/local/tmp/sunspider
adb push ./test/SunSpiderForAndroid /data/local/tmp/sunspider
adb shell chmod 777 /data/local/tmp/sunspider/driver.sh
adb shell mkdir -p /data/local/tmp/arm32/v8
adb shell mkdir -p /data/local/tmp/arm32/jsc/interp
adb shell mkdir -p /data/local/tmp/arm32/jsc/baseline
adb shell mkdir -p /data/local/tmp/arm32/escargot/interp
adb shell mkdir -p /data/local/tmp/arm32/escargot/jit
adb shell mkdir -p /data/local/tmp/arm32/duk
adb push test/bin/arm32 data/local/tmp/arm32
adb shell mkdir -p /data/local/tmp/arm64/v8
adb shell mkdir -p /data/local/tmp/arm64/escargot/interp
adb shell mkdir -p /data/local/tmp/arm64/escargot/jit
adb shell mkdir -p /data/local/tmp/arm64/duk
adb push ./test/bin/arm64 data/local/tmp/arm64
adb push ./android/libs/armeabi-v7a data/local/tmp/arm32/escargot
#adb shell mkdir /data/local/tmp/arm64/escargot
#adb push ./android/libs/arm64-v8a data/local/tmp/arm64/escargot
