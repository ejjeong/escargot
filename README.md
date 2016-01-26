ESCARGOT
========

# Building

    git clone ssh://git@10.251.44.23:7999/escar/escargot.git
    cd escargot
    ./build_third_party.sh
    make [x86|x64].[interpreter|jit].[debug|release] -j
e.g. $ make x64.interpreter.debug

### Tizen wearable Build & Execution
    make tizen_wearable_arm.interpreter.[debug|release] -j
    sdb push escargot $(dest)
    sdb push tizen_2.4/tools/arm-linux-gnueabi-gcc-4.9/arm-linux-gnueabi/lib/libstdc++.so.6 $(dest)

    on $(dest),
    export LD_LIBRARY_PATH=./
    ./escargot
    (on tizen wearable emulator, it won't work well because the emulator may use x86 instead of arm)

# Running

## Running sunspider
    make run-sunspider

## Running octane
    ./run-octane.sh

## Running test262
    make run-test262 [OPT=**]
e.g. make run-test262 S7.2
[test262:fbba29f](https://github.com/tc39/test262)

## Measuring
    ./measure.sh [escargot | v8 | jsc.interp | jsc.jit] [time | memory]

## Testing JIT
	make check-jit

# Contributing

[Webkit coding style guidelines](https://www.webkit.org/coding/coding-style.html)


