#!/bin/bash
engines32=("escargot32.interp" "escargot32.jit" "jsc32.interp" "jsc32.jit" "v832.jit" "v832.full.jit" "duktape32")
engines=("escargot.interp" "escargot.jit" "jsc.interp" "jsc.jit" "v8.jit" "v8.full.jit" "duktape")

for t in "${engines32[@]}"; do
    echo "==============="$t"===============" >> test/out/memoryAll.res
    ./measure.sh $t memory
    cat test/out/memory.res >> test/out/memoryAll.res
    echo "---------------FINISH $t"
done

for t in "${engines[@]}"; do
    echo "==============="$t"===============" >> test/out/memoryAll.res
    ./measure.sh $t memory
    cat test/out/memory.res >> test/out/memoryAll.res
    echo "---------------FINISH $t"
done
