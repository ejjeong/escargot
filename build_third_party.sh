#!/bin/bash
#set -x

if [ ! -f /proc/cpuinfo ]; then
    echo "Is this Linux? Cannot find or read /proc/cpuinfo"
    exit 1
fi
NUMPROC=$(grep 'processor' /proc/cpuinfo | wc -l)

###########################################################
# GC build
###########################################################
cd third_party/bdwgc/
autoreconf -vif
automake --add-missing
#./autogen.sh
#make distclean

rm -rf out

GCCONFFLAGS_COMMON=" --disable-parallel-mark " # --enable-large-config --enable-cplusplus"
CFLAGS_COMMON=" -g3 "
LDFLAGS_COMMON=

GCCONFFLAGS_wearable=
CFLAGS_wearable=" -Os "
LDFLAGS_wearable=

GCCONFFLAGS_x86=
CFLAGS_x86=" -m32 "
LDFLAGS_x86=" -m32 "

GCCONFFLAGS_arm=
CFLAGS_arm=" -march=armv7-a -mthumb -finline-limit=64 "
LDFLAGS_arm=

GCCONFFLAGS_release=" --disable-debug --disable-gc-debug "
GCCONFFLAGS_debug=" --enable-debug --enable-gc-debug "
CFLAGS_release=' -O2 '
CFLAGS_debug=' -O0 '

GCCONFFLAGS_static=
GCCONFFLAGS_shared=
CFLAGS_static=
CFLAGS_shared=' -fPIC '

function build_gc_for_linux() {

    for host in linux; do
    for arch in x86 x64; do
    for mode in debug release; do
    for libtype in shared static; do
        echo =========================================================================
        echo Building bdwgc for $host $arch $mode $libtype

        BUILDDIR=out/$host/$arch/$mode.$libtype
        rm -rf $BUILDDIR
        mkdir -p $BUILDDIR
        cd $BUILDDIR

        GCCONFFLAGS_HOST=GCCONFFLAGS_$host CFLAGS_HOST=CFLAGS_$host LDFLAGS_HOST=LDFLAGS_$host
        GCCONFFLAGS_ARCH=GCCONFFLAGS_$arch CFLAGS_ARCH=CFLAGS_$arch LDFLAGS_ARCH=LDFLAGS_$arch
        GCCONFFLAGS_MODE=GCCONFFLAGS_$mode CFLAGS_MODE=CFLAGS_$mode LDFLAGS_MODE=LDFLAGS_$mode
        GCCONFFLAGS_LIBTYPE=GCCONFFLAGS_$libtype CFLAGS_LIBTYPE=CFLAGS_$libtype LDFLAGS_LIBTYPE=LDFLAGS_$libtype

        GCCONFFLAGS="$GCCONFFLAGS_COMMON ${!GCCONFFLAGS_HOST} ${!GCCONFFLAGS_ARCH} ${!GCCONFFLAGS_MODE} ${!GCCONFFLAGS_LIBTYPE}"
        CFLAGS="$CFLAGS_COMMON ${!CFLAGS_HOST} ${!CFLAGS_ARCH} ${!CFLAGS_MODE} ${!CFLAGS_LIBTYPE}"
        LDFLAGS="$LDFLAGS_COMMON ${!LDFLAGS_HOST} ${!LDFLAGS_ARCH} ${!LDFLAGS_MODE} ${!LDFLAGS_LIBTYPE}"

        ../../../../configure $GCCONFFLAGS CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS $CFLAGS" > /dev/null
        make -j$NUMPROC > /dev/null

        echo Building bdwgc for $host $arch $mode $libtype done
        cd -
    done
    done
    done
    done
}

function build_gc_for_tizen() {

    for version in 2.3.1 3.0; do
    for host in wearable mobile; do
    for arch in arm i386; do
    for mode in debug release; do
    for libtype in shared static; do

        if [[ $arch == arm ]]; then
            device=device
        elif [[ $arch == i386 ]]; then
            device=emulator
        fi

        TIZEN_SYSROOT=$TIZEN_SDK_HOME/platforms/tizen-$version/$host/rootstraps/$host-$version-$device.core
        COMPILER_PREFIX=$arch-linux-gnueabi
        TIZEN_TOOLCHAIN=$TIZEN_SDK_HOME/tools/$COMPILER_PREFIX-gcc-4.9

        echo =========================================================================
        if [[ -a $TIZEN_SYSROOT ]]; then
            echo Building bdwgc for tizen $version $host $arch $mode $libtype
        else
            echo Skipping bdwgc build for tizen $version $host $arch $mode $libtype
            continue
        fi

        BUILDDIR=out/tizen_${version}_${host}/$arch/$mode.$libtype
        rm -rf $BUILDDIR
        mkdir -p $BUILDDIR
        cd $BUILDDIR

        GCCONFFLAGS_HOST=GCCONFFLAGS_$host CFLAGS_HOST=CFLAGS_$host LDFLAGS_HOST=LDFLAGS_$host
        GCCONFFLAGS_ARCH=GCCONFFLAGS_$arch CFLAGS_ARCH=CFLAGS_$arch LDFLAGS_ARCH=LDFLAGS_$arch
        GCCONFFLAGS_MODE=GCCONFFLAGS_$mode CFLAGS_MODE=CFLAGS_$mode LDFLAGS_MODE=LDFLAGS_$mode
        GCCONFFLAGS_LIBTYPE=GCCONFFLAGS_$libtype CFLAGS_LIBTYPE=CFLAGS_$libtype LDFLAGS_LIBTYPE=LDFLAGS_$libtype

        GCCONFFLAGS="$GCCONFFLAGS_COMMON ${!GCCONFFLAGS_HOST} ${!GCCONFFLAGS_ARCH} ${!GCCONFFLAGS_MODE} ${!GCCONFFLAGS_LIBTYPE}"
        CFLAGS="$CFLAGS_COMMON ${!CFLAGS_HOST} ${!CFLAGS_ARCH} ${!CFLAGS_MODE} ${!CFLAGS_LIBTYPE} --sysroot=$TIZEN_SYSROOT -flto -ffat-lto-objects"
        LDFLAGS="$LDFLAGS_COMMON ${!LDFLAGS_HOST} ${!LDFLAGS_ARCH} ${!LDFLAGS_MODE} ${!LDFLAGS_LIBTYPE}"

        ../../../../configure --host=$COMPILER_PREFIX $GCCONFFLAGS CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" \
            ARFLAGS="--plugin=$TIZEN_TOOLCHAIN/libexec/gcc/$COMPILER_PREFIX/4.9.2/liblto_plugin.so" \
            NMFLAGS="--plugin=$TIZEN_TOOLCHAIN/libexec/gcc/$COMPILER_PREFIX/4.9.2/liblto_plugin.so" \
            RANLIBFLAGS="--plugin=$TIZEN_TOOLCHAIN/libexec/gcc/$COMPILER_PREFIX/4.9.2/liblto_plugin.so" \
            CC=$TIZEN_TOOLCHAIN/bin/$COMPILER_PREFIX-gcc \
            CXX=$TIZEN_TOOLCHAIN/bin/$COMPILER_PREFIX-g++ \
            AR=$TIZEN_TOOLCHAIN/bin/$COMPILER_PREFIX-gcc-ar \
            NM=$TIZEN_TOOLCHAIN/bin/$COMPILER_PREFIX-gcc-nm \
            RANLIB=$TIZEN_TOOLCHAIN/bin/$COMPILER_PREFIX-gcc-ranlib \
            LD=$TIZEN_TOOLCHAIN/bin/$COMPILER_PREFIX-ld > /dev/null
        make -j$NUMPROC > /dev/null

        echo Building bdwgc for tizen $version $host $arch $mode $libtype done
        cd -
    done
    done
    done
    done
    done
}

build_gc_for_linux

if [ -z "$TIZEN_SDK_HOME" ]; then
    echo "Do not build for Tizen"
else
    echo "TIZEN_SDK_HOME env is ...""$TIZEN_SDK_HOME"
    build_gc_for_tizen
fi


