#!/bin/bash
set -x

if [ ! -f /proc/cpuinfo ]; then
	echo "Is this Linux? Cannot find or read /proc/cpuinfo"
	exit 1
fi
NUMPROC=$(grep 'processor' /proc/cpuinfo | wc -l)

TIZEN="1"
if [ -z "$TIZEN_SDK_HOME" ]; then
	TIZEN="0"
fi

if [ $TIZEN == "1" ]
then
	echo "TIZEN_SDK_HOME env is ..."
	echo $TIZEN_SDK_HOME
	#TIZEN_SYS_ROOT=$TIZEN_SDK_HOME/platforms/mobile-2.3/rootstraps/mobile-2.3-device.core/
	TIZEN_SYS_ROOT=$TIZEN_SDK_HOME/platforms/tizen-2.4/mobile/rootstraps/mobile-2.4-device.core/
	TIZEN_WEARABLE_SYS_ROOT=$TIZEN_SDK_HOME/platforms/tizen-2.3.1/wearable/rootstraps/wearable-2.3.1-device.core/
	TIZEN_WEARABLE_EMULATOR_SYS_ROOT=$TIZEN_SDK_HOME//platforms/tizen-2.3.1/wearable/rootstraps/wearable-2.3.1-emulator.core/
	TIZEN_TOOLCHAIN=$TIZEN_SDK_HOME/tools/arm-linux-gnueabi-gcc-4.9/
	TIZEN_EMULATOR_TOOLCHAIN=$TIZEN_SDK_HOME/tools/i386-linux-gnueabi-gcc-4.9/
fi

###########################################################
# GC build
###########################################################
cd third_party/bdwgc/
autoreconf -vif
automake --add-missing
#./autogen.sh
#make distclean

rm -rf out

mkdir -p out/linux/x86/release
mkdir -p out/linux/x86/debug
mkdir -p out/linux/x64/release
mkdir -p out/linux/x64/debug
mkdir -p out/linux/x64/release.shared
mkdir -p out/linux/x64/debug.shared
if [ $TIZEN == "1" ]
then
	mkdir -p out/tizen_arm/arm/release.shared
	mkdir -p out/tizen_arm/arm/release
	mkdir -p out/tizen_wearable_arm/arm/debug.shared
	mkdir -p out/tizen_wearable_arm/arm/release.shared
	mkdir -p out/tizen_wearable_arm/arm/release
	mkdir -p out/tizen_wearable_arm/arm/debug
#	mkdir -p out/tizen_wearable_emulator/x86/release
	mkdir -p out/tizen_wearable_emulator/x86/debug.shared
	mkdir -p out/tizen_wearable_emulator/x86/release.shared
fi
GCCONFFLAGS=" --disable-parallel-mark " # --enable-large-config --enable-cplusplus"

cd out/linux/x86/release
../../../../configure $GCCONFFLAGS --disable-gc-debug CFLAGS='-m32' CXXFLAGS='-m32' LDFLAGS='-m32'
make -j$NUMPROC
cd -

cd out/linux/x86/debug
../../../../configure $GCCONFFLAGS CFLAGS='-m32 -g3' CXXFLAGS='-m32' LDFLAGS='-m32' --enable-debug
make -j$NUMPROC
cd -

cd out/linux/x64/release
../../../../configure $GCCONFFLAGS --disable-gc-debug
make -j$NUMPROC
cd -

cd out/linux/x64/debug
../../../../configure $GCCONFFLAGS --enable-debug CFLAGS='-g3'
make -j$NUMPROC
cd -

cd out/linux/x64/release.shared
../../../../configure $GCCONFFLAGS --disable-gc-debug CFLAGS='-fPIC'
make -j$NUMPROC
cd -

cd out/linux/x64/debug.shared
../../../../configure $GCCONFFLAGS --enable-debug CFLAGS='-g3 -fPIC'
make -j$NUMPROC
cd -


#cd out/arm/release
#../../../configure $GCCONFFLAGS --disable-gc-debug --with-sysroot=$TIZEN_SYS_ROOT --host=arm-linux-gnueabi CFLAGS='-march=armv7-a'
#make -j$NUMPROC
#cd -
if [ $TIZEN == "1" ]
then
	cd out/tizen_arm/arm/release.shared
	TCF="-fPIC -march=armv7-a -O2 -mthumb -finline-limit=64 --sysroot="${TIZEN_SYS_ROOT}
	../../../../configure $GCCONFFLAGS --disable-gc-debug --with-sysroot=$TIZEN_SYS_ROOT --with-cross-host=arm-linux-gnueabi CFLAGS="$TCF" CC=$TIZEN_TOOLCHAIN/bin/arm-linux-gnueabi-gcc CXX=$TIZEN_TOOLCHAIN/bin/arm-linux-gnueabi-g++ LD=$TIZEN_TOOLCHAIN/bin/arm-linux-gnueabi-ld
	make -j$NUMPROC
	cd -

	cd out/tizen_wearable_arm/arm/release.shared
	TWCF="-DTIZEN_WEARABLE -fPIC -march=armv7-a -Os -mthumb -finline-limit=64 --sysroot="${TIZEN_WEARABLE_SYS_ROOT}
	../../../../configure $GCCONFFLAGS --disable-gc-debug --with-sysroot=$TIZEN_WEARABLE_SYS_ROOT --host=arm-linux-gnueabi CFLAGS="$TWCF" CC=$TIZEN_TOOLCHAIN/bin/arm-linux-gnueabi-gcc CXX=$TIZEN_TOOLCHAIN/bin/arm-linux-gnueabi-g++ LD=$TIZEN_TOOLCHAIN/bin/arm-linux-gnueabi-ld
	make -j$NUMPROC
	cd -

	cd out/tizen_wearable_arm/arm/debug.shared
	TWCF="-DTIZEN_WEARABLE -fPIC -march=armv7-a -Os -g3 -mthumb -finline-limit=64 --sysroot="${TIZEN_WEARABLE_SYS_ROOT}
	../../../../configure $GCCONFFLAGS --enable-gc-debug --with-sysroot=$TIZEN_WEARABLE_SYS_ROOT --host=arm-linux-gnueabi CFLAGS="$TWCF" CC=$TIZEN_TOOLCHAIN/bin/arm-linux-gnueabi-gcc CXX=$TIZEN_TOOLCHAIN/bin/arm-linux-gnueabi-g++ LD=$TIZEN_TOOLCHAIN/bin/arm-linux-gnueabi-ld
	make -j$NUMPROC
	cd -

	cd out/tizen_wearable_emulator/x86/release.shared
	TWCF="-DTIZEN_WEARABLE -fPIC -m32 -Os -g0 -finline-limit=64 --sysroot="${TIZEN_WEARABLE_EMULATOR_SYS_ROOT}
	../../../../configure $GCCONFFLAGS --enable-gc-debug --with-sysroot=$TIZEN_WEARABLE_EMULATOR_SYS_ROOT --host=i386-linux-gnueabi CFLAGS="$TWCF" CC=$TIZEN_EMULATOR_TOOLCHAIN/bin/i386-linux-gnueabi-gcc CXX=$TIZEN_EMULATOR_TOOLCHAIN/bin/i386-gnueabi-g++ LD=$TIZEN_EMULATOR_TOOLCHAIN/bin/i386-linux-gnueabi-ld
	make -j$NUMPROC
	cd -

	cd out/tizen_wearable_emulator/x86/debug.shared
	TWCF="-DTIZEN_WEARABLE -fPIC -m32 -Os -g3 -finline-limit=64 --sysroot="${TIZEN_WEARABLE_EMULATOR_SYS_ROOT}
	../../../../configure $GCCONFFLAGS --enable-gc-debug --with-sysroot=$TIZEN_WEARABLE_EMULATOR_SYS_ROOT --host=i386-linux-gnueabi CFLAGS="$TWCF" CC=$TIZEN_EMULATOR_TOOLCHAIN/bin/i386-linux-gnueabi-gcc CXX=$TIZEN_EMULATOR_TOOLCHAIN/bin/i386-gnueabi-g++ LD=$TIZEN_EMULATOR_TOOLCHAIN/bin/i386-linux-gnueabi-ld
	make -j$NUMPROC
	cd -

	cd out/tizen_arm/arm/release
	TCF="-march=armv7-a -O2 -mthumb -finline-limit=64 --sysroot="${TIZEN_SYS_ROOT}
	../../../../configure $GCCONFFLAGS --disable-gc-debug --with-sysroot=$TIZEN_SYS_ROOT --with-cross-host=arm-linux-gnueabi CFLAGS="$TCF" CC=$TIZEN_TOOLCHAIN/bin/arm-linux-gnueabi-gcc CXX=$TIZEN_TOOLCHAIN/bin/arm-linux-gnueabi-g++ LD=$TIZEN_TOOLCHAIN/bin/arm-linux-gnueabi-ld
	make -j$NUMPROC
	cd -

	cd out/tizen_wearable_arm/arm/release
	TWCF="-DTIZEN_WEARABLE -march=armv7-a -Os -mthumb -finline-limit=64 --sysroot="${TIZEN_WEARABLE_SYS_ROOT}
	../../../../configure $GCCONFFLAGS --disable-gc-debug --with-sysroot=$TIZEN_WEARABLE_SYS_ROOT --host=arm-linux-gnueabi CFLAGS="$TWCF" CC=$TIZEN_TOOLCHAIN/bin/arm-linux-gnueabi-gcc CXX=$TIZEN_TOOLCHAIN/bin/arm-linux-gnueabi-g++ LD=$TIZEN_TOOLCHAIN/bin/arm-linux-gnueabi-ld
	make -j$NUMPROC
	cd -

  cd out/tizen_wearable_arm/arm/debug
	TWCF="-DTIZEN_WEARABLE -march=armv7-a -Os -g3 -mthumb -finline-limit=64 --sysroot="${TIZEN_WEARABLE_SYS_ROOT}
	../../../../configure $GCCONFFLAGS --enable-gc-debug --with-sysroot=$TIZEN_WEARABLE_SYS_ROOT --host=arm-linux-gnueabi CFLAGS="$TWCF" CC=$TIZEN_TOOLCHAIN/bin/arm-linux-gnueabi-gcc CXX=$TIZEN_TOOLCHAIN/bin/arm-linux-gnueabi-g++ LD=$TIZEN_TOOLCHAIN/bin/arm-linux-gnueabi-ld
	make -j$NUMPROC
	cd -

fi

#cd out/arm/debug
#../../../configure $GCCONFFLAGS --enable-debug --with-sysroot=$TIZEN_SYS_ROOT --host=arm-linux-gnueabi CFLAGS='-g3 -march=armv7-a'
#make -j$NUMPROC
#cd -
