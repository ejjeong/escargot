ESCARGOT
========

# Building

    git clone ssh://git@10.251.44.23:7999/escar/escargot.git
    cd escargot
    ./init_third_party.sh
    ./build_third_party.sh
    make -j8

# Running

## Running sunspider
    make run-sunspider

## Running octane
    ./escargot test/octane/**.js

## Running test262
    make run-test262 [OPT=**]
e.g. make run-test262 S7.2

## Measuring
    ./measure.sh [escargot | v8 | jsc.interp | jsc.jit] [time | memory]

# Contributing

[Webkit coding style guidelines](https://www.webkit.org/coding/coding-style.html)

