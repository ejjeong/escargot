#!/bin/bash

function build-interp() {
  make tizen_wearable_arm.interpreter.release -j  
  mkdir -p test/bin/tizen_wearable_arm/escargot/interp
  cp ./out/arm/interpreter/release/escargot ./test/bin/tizen_wearable_arm/escargot/interp/escargot.release
}

export SDB=$TIZEN_SDK_HOME/tools/sdb

build-interp

$SDB shell mkdir -p /opt/usr/media/escar
$SDB push ./measure_for_tizen_wearable.sh /opt/usr/media/escar
$SDB push ./test/bin/memps_tizen_wearable /opt/usr/media/escar
$SDB shell mkdir -p /opt/usr/media/escar/sunspider
$SDB push ./test/SunSpiderForAndroid /opt/usr/media/escar/sunspider
$SDB shell mv /opt/usr/media/escar/sunspider/driver_for_tizen.sh /opt/usr/media/escar/sunspider/driver.sh
$SDB shell chmod 777 /opt/usr/media/escar/sunspider/driver.sh
$SDB shell mkdir -p /opt/usr/media/escar/tizen_wearable_arm/v8
$SDB shell mkdir -p /opt/usr/media/escar/tizen_wearable_arm/jsc/interp
$SDB shell mkdir -p /opt/usr/media/escar/tizen_wearable_arm/jsc/baseline
$SDB shell mkdir -p /opt/usr/media/escar/tizen_wearable_arm/escargot/interp
$SDB shell mkdir -p /opt/usr/media/escar/tizen_wearable_arm/escargot/jit
$SDB shell mkdir -p /opt/usr/media/escar/tizen_wearable_arm/duk
$SDB push test/bin/tizen_wearable_arm /opt/usr/media/escar/tizen_wearable_arm
$SDB push $TIZEN_SDK_HOME/tools/arm-linux-gnueabi-gcc-4.9/arm-linux-gnueabi/lib/libstdc++.so.6 /opt/usr/media/escar
