#!/bin/bash

adb push ./run-Sunspider.sh /data/local/tmp
adb push ./measure_for_android.sh /data/local/tmp
adb push ./test/bin/memps_arm /data/local/tmp
adb shell mkdir /data/local/tmp/sunspider
adb push ./test/SunSpiderForAndroid /data/local/tmp/sunspider
adb shell chmod 777 /data/local/tmp/sunspider/driver.sh
adb shell mkdir /data/local/tmp/arm32
adb shell mkdir /data/local/tmp/arm32/v8
adb shell mkdir /data/local/tmp/arm32/jsc
adb shell mkdir /data/local/tmp/arm32/jsc/interp
adb shell mkdir /data/local/tmp/arm32/jsc/baseline
adb shell mkdir /data/local/tmp/arm32/escargot/interp
adb shell mkdir /data/local/tmp/arm32/escargot/jit
adb push test/bin/arm32 data/local/tmp/arm32
adb shell mkdir /data/local/tmp/arm64
adb shell mkdir /data/local/tmp/arm64/v8
adb shell mkdir /data/local/tmp/arm64/escargot/interp
adb shell mkdir /data/local/tmp/arm64/escargot/jit
adb push ./test/bin/arm64 data/local/tmp/arm64
adb shell mkdir /data/local/tmp/arm32/escargot
adb push ./android/libs/armeabi-v7a data/local/tmp/arm32/escargot
#adb shell mkdir /data/local/tmp/arm64/escargot
#adb push ./android/libs/arm64-v8a data/local/tmp/arm64/escargot

