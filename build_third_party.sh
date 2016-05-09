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

TIZEN3="1"
if [ -z "$TIZEN3_SDK_HOME" ]; then
	TIZEN3="0"
fi

function set_variables() {
	if [ $TIZEN == "1" ]
	then
		echo "TIZEN_SDK_HOME env is ...""$TIZEN_SDK_HOME"
		TIZEN_SYS_ROOT=$TIZEN_SDK_HOME/platforms/tizen-2.4/mobile/rootstraps/mobile-2.4-device.core/
		TIZEN_WEARABLE_SYS_ROOT=$TIZEN_SDK_HOME/platforms/tizen-2.3.1/wearable/rootstraps/wearable-2.3.1-device.core/
		TIZEN_WEARABLE_EMULATOR_SYS_ROOT=$TIZEN_SDK_HOME//platforms/tizen-2.3.1/wearable/rootstraps/wearable-2.3.1-emulator.core/
		TIZEN_TOOLCHAIN=$TIZEN_SDK_HOME/tools/arm-linux-gnueabi-gcc-4.9/
		TIZEN_EMULATOR_TOOLCHAIN=$TIZEN_SDK_HOME/tools/i386-linux-gnueabi-gcc-4.9/
	fi
	if [ $TIZEN3 == "1" ]
	then
		echo "TIZEN_SDK3_HOME env is ...""$TIZEN_SDK3_HOME"
		TIZEN3_SYS_ROOT=$TIZEN3_SDK_HOME/platforms/tizen-3.0/mobile/rootstraps/mobile-3.0-device.core/
		TIZEN3_WEARABLE_SYS_ROOT=$TIZEN3_SDK_HOME/platforms/tizen-3.0/wearable/rootstraps/wearable-3.0-device.core/
		TIZEN3_WEARABLE_EMULATOR_SYS_ROOT=$TIZEN3_SDK_HOME//platforms/tizen-3.0/wearable/rootstraps/wearable-3.0-emulator.core/
		TIZEN3_TOOLCHAIN=$TIZEN3_SDK_HOME/tools/arm-linux-gnueabi-gcc-4.9/
		TIZEN3_EMULATOR_TOOLCHAIN=$TIZEN3_SDK_HOME/tools/i386-linux-gnueabi-gcc-4.9/
	fi
}

set_variables

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

function make_tizen_direcotry_with_version() {
	local version=$1
	local TIZEN_ARM_DIR="tizen""$version""_arm"
	local TIZEN_WEARABLE_ARM_DIR="tizen""$version""_wearable_arm"
	local TIZEN_WEARABLE_EMULATOR_DIR="tizen""$version""_wearable_emulator"

	mkdir -p out/$TIZEN_ARM_DIR/arm/release.shared
	mkdir -p out/$TIZEN_ARM_DIR/arm/release
	mkdir -p out/$TIZEN_WEARABLE_ARM_DIR/arm/debug.shared
	mkdir -p out/$TIZEN_WEARABLE_ARM_DIR/arm/release.shared
	mkdir -p out/$TIZEN_WEARABLE_ARM_DIR/arm/release
	mkdir -p out/$TIZEN_WEARABLE_ARM_DIR/arm/debug
#	mkdir -p out/$TIZEN_WEARABLE_EMULATOR_DIR/x86/release
	mkdir -p out/$TIZEN_WEARABLE_EMULATOR_DIR/x86/debug.shared
	mkdir -p out/$TIZEN_WEARABLE_EMULATOR_DIR/x86/release.shared


}

function reset_tizen_directories() {
	if [ $TIZEN == "1" ]
	then
		make_tizen_direcotry_with_version
	fi
	if [ $TIZEN3 == "1" ]
	then
		make_tizen_direcotry_with_version 3
	fi
}

reset_tizen_directories

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


function build_gc_for_tizen_with_version() {
	local version=$1
	local TIZEN_ARM_DIR="tizen""$version""_arm"
	local TIZEN_WEARABLE_ARM_DIR="tizen""$version""_wearable_arm"
	local TIZEN_WEARABLE_EMULATOR_DIR="tizen""$version""_wearable_emulator"
	if [ $version == "3" ]
	then
		local TIZEN_SYS_ROOT_WITH_VERSION="$TIZEN3_SYS_ROOT"
		local TIZEN_WEARABLE_SYS_ROOT_WITH_VERSION="$TIZEN3_WEARABLE_SYS_ROOT"
		local TIZEN_WEARABLE_EMULATOR_SYS_ROOT_WITH_VERSION="$TIZEN3_WEARABLE_EMULATOR_SYS_ROOT"
		local TIZEN_TOOLCHAIN_WITH_VERSION="$TIZEN3_TOOLCHAIN"
		local TIZEN_EMULATOR_TOOLCHAIN_WITH_VERSION="$TIZEN3_EMULATOR_TOOLCHAIN"
	else
		local TIZEN_SYS_ROOT_WITH_VERSION="$TIZEN_SYS_ROOT"
		local TIZEN_WEARABLE_SYS_ROOT_WITH_VERSION="$TIZEN_WEARABLE_SYS_ROOT"
		local TIZEN_WEARABLE_EMULATOR_SYS_ROOT_WITH_VERSION="$TIZEN_WEARABLE_EMULATOR_SYS_ROOT"
		local TIZEN_TOOLCHAIN_WITH_VERSION="$TIZEN_TOOLCHAIN"
		local TIZEN_EMULATOR_TOOLCHAIN_WITH_VERSION="$TIZEN_EMULATOR_TOOLCHAIN"
	fi


	cd out/$TIZEN_ARM_DIR/arm/release.shared
	TCF="-fPIC -march=armv7-a -O2 -mthumb -finline-limit=64 --sysroot="${TIZEN_SYS_ROOT_WITH_VERSION}
	../../../../configure $GCCONFFLAGS --disable-gc-debug --with-sysroot=$TIZEN_SYS_ROOT_WITH_VERSION --with-cross-host=arm-linux-gnueabi CFLAGS="$TCF" CC=$TIZEN_TOOLCHAIN_WITH_VERSION/bin/arm-linux-gnueabi-gcc CXX=$TIZEN_TOOLCHAIN_WITH_VERSION/bin/arm-linux-gnueabi-g++ LD=$TIZEN_TOOLCHAIN_WITH_VERSION/bin/arm-linux-gnueabi-ld
	make -j$NUMPROC
	cd -

	cd out/$TIZEN_WEARABLE_ARM_DIR/arm/release.shared
	TWCF="-DTIZEN_WEARABLE -fPIC -march=armv7-a -Os -mthumb -finline-limit=64 --sysroot="${TIZEN_WEARABLE_SYS_ROOT_WITH_VERSION}
	../../../../configure $GCCONFFLAGS --disable-gc-debug --with-sysroot=$TIZEN_WEARABLE_SYS_ROOT_WITH_VERSION --host=arm-linux-gnueabi CFLAGS="$TWCF" CC=$TIZEN_TOOLCHAIN_WITH_VERSION/bin/arm-linux-gnueabi-gcc CXX=$TIZEN_TOOLCHAIN_WITH_VERSION/bin/arm-linux-gnueabi-g++ LD=$TIZEN_TOOLCHAIN_WITH_VERSION/bin/arm-linux-gnueabi-ld
	make -j$NUMPROC
	cd -

	cd out/$TIZEN_WEARABLE_ARM_DIR/arm/debug.shared
	TWCF="-DTIZEN_WEARABLE -fPIC -march=armv7-a -Os -g3 -mthumb -finline-limit=64 --sysroot="${TIZEN_WEARABLE_SYS_ROOT_WITH_VERSION}
	../../../../configure $GCCONFFLAGS --enable-gc-debug --with-sysroot=$TIZEN_WEARABLE_SYS_ROOT_WITH_VERSION --host=arm-linux-gnueabi CFLAGS="$TWCF" CC=$TIZEN_TOOLCHAIN_WITH_VERSION/bin/arm-linux-gnueabi-gcc CXX=$TIZEN_TOOLCHAIN_WITH_VERSION/bin/arm-linux-gnueabi-g++ LD=$TIZEN_TOOLCHAIN_WITH_VERSION/bin/arm-linux-gnueabi-ld
	make -j$NUMPROC
	cd -

	cd out/$TIZEN_WEARABLE_EMULATOR_DIR/x86/release.shared
	TWCF="-DTIZEN_WEARABLE -fPIC -m32 -Os -g0 -finline-limit=64 --sysroot="${TIZEN_WEARABLE_EMULATOR_SYS_ROOT_WITH_VERSION}
	../../../../configure $GCCONFFLAGS --enable-gc-debug --with-sysroot=$TIZEN_WEARABLE_EMULATOR_SYS_ROOT_WITH_VERSION --host=i386-linux-gnueabi CFLAGS="$TWCF" CC=$TIZEN_EMULATOR_TOOLCHAIN_WITH_VERSION/bin/i386-linux-gnueabi-gcc CXX=$TIZEN_EMULATOR_TOOLCHAIN_WITH_VERSION/bin/i386-gnueabi-g++ LD=$TIZEN_EMULATOR_TOOLCHAIN_WITH_VERSION/bin/i386-linux-gnueabi-ld
	make -j$NUMPROC
	cd -

	cd out/$TIZEN_WEARABLE_EMULATOR_DIR/x86/debug.shared
	TWCF="-DTIZEN_WEARABLE -fPIC -m32 -Os -g3 -finline-limit=64 --sysroot="${TIZEN_WEARABLE_EMULATOR_SYS_ROOT_WITH_VERSION}
	../../../../configure $GCCONFFLAGS --enable-gc-debug --with-sysroot=$TIZEN_WEARABLE_EMULATOR_SYS_ROOT_WITH_VERSION --host=i386-linux-gnueabi CFLAGS="$TWCF" CC=$TIZEN_EMULATOR_TOOLCHAIN_WITH_VERSION/bin/i386-linux-gnueabi-gcc CXX=$TIZEN_EMULATOR_TOOLCHAIN_WITH_VERSION/bin/i386-gnueabi-g++ LD=$TIZEN_EMULATOR_TOOLCHAIN_WITH_VERSION/bin/i386-linux-gnueabi-ld
	make -j$NUMPROC
	cd -

	cd out/$TIZEN_ARM_DIR/arm/release
	TCF="-march=armv7-a -O2 -mthumb -finline-limit=64 --sysroot="${TIZEN_SYS_ROOT_WITH_VERSION}
	../../../../configure $GCCONFFLAGS --disable-gc-debug --with-sysroot=$TIZEN_SYS_ROOT_WITH_VERSION --with-cross-host=arm-linux-gnueabi CFLAGS="$TCF" CC=$TIZEN_TOOLCHAIN_WITH_VERSION/bin/arm-linux-gnueabi-gcc CXX=$TIZEN_TOOLCHAIN_WITH_VERSION/bin/arm-linux-gnueabi-g++ LD=$TIZEN_TOOLCHAIN_WITH_VERSION/bin/arm-linux-gnueabi-ld
	make -j$NUMPROC
	cd -

	cd out/$TIZEN_WEARABLE_ARM_DIR/arm/release
	TWCF="-DTIZEN_WEARABLE -march=armv7-a -Os -mthumb -finline-limit=64 --sysroot="${TIZEN_WEARABLE_SYS_ROOT_WITH_VERSION}
	../../../../configure $GCCONFFLAGS --disable-gc-debug --with-sysroot=$TIZEN_WEARABLE_SYS_ROOT_WITH_VERSION --host=arm-linux-gnueabi CFLAGS="$TWCF" CC=$TIZEN_TOOLCHAIN_WITH_VERSION/bin/arm-linux-gnueabi-gcc CXX=$TIZEN_TOOLCHAIN_WITH_VERSION/bin/arm-linux-gnueabi-g++ LD=$TIZEN_TOOLCHAIN_WITH_VERSION/bin/arm-linux-gnueabi-ld
	make -j$NUMPROC
	cd -

  cd out/$TIZEN_WEARABLE_ARM_DIR/arm/debug
	TWCF="-DTIZEN_WEARABLE -march=armv7-a -Os -g3 -mthumb -finline-limit=64 --sysroot="${TIZEN_WEARABLE_SYS_ROOT_WITH_VERSION}
	../../../../configure $GCCONFFLAGS --enable-gc-debug --with-sysroot=$TIZEN_WEARABLE_SYS_ROOT_WITH_VERSION --host=arm-linux-gnueabi CFLAGS="$TWCF" CC=$TIZEN_TOOLCHAIN_WITH_VERSION/bin/arm-linux-gnueabi-gcc CXX=$TIZEN_TOOLCHAIN_WITH_VERSION/bin/arm-linux-gnueabi-g++ LD=$TIZEN_TOOLCHAIN_WITH_VERSION/bin/arm-linux-gnueabi-ld
	make -j$NUMPROC
	cd -

}

function build_gc_for_tizen() {
	if [ $TIZEN == "1" ]
	then
		build_gc_for_tizen_with_version
	fi
	if [ $TIZEN3 == "1" ]
	then
		build_gc_for_tizen_with_version 3
	fi
}

build_gc_for_tizen


#cd out/arm/release
#../../../configure $GCCONFFLAGS --disable-gc-debug --with-sysroot=$TIZEN_SYS_ROOT --host=arm-linux-gnueabi CFLAGS='-march=armv7-a'
#make -j$NUMPROC
#cd -


#cd out/arm/debug
#../../../configure $GCCONFFLAGS --enable-debug --with-sysroot=$TIZEN_SYS_ROOT --host=arm-linux-gnueabi CFLAGS='-g3 -march=armv7-a'
#make -j$NUMPROC
#cd -
