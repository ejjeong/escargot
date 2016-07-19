#!/bin/bash
engines32=("escargot32.interp" "escargot32.jit" "jsc32.interp" "jsc32.jit" "v832.jit" "v832.full.jit" "duktape32")
engines=("escargot.interp" "escargot.jit" "jsc.interp" "jsc.jit" "v8.jit" "v8.full.jit" "duktape")

num=$(date +%y%m%d_%H_%M_%S)
curdir=`pwd`;
resfile=$(echo $curdir'/test/out/memoryAll_'$num'.res')
echo "############## OCTANE #############" > $resfile
for t in "${engines32[@]}"; do
    echo "==============="$t"===============" >> $resfile
    ./measure.sh $t octane memory
    cat test/out/octaneMemory.res >> $resfile
    echo "---------------FINISH $t"
done

for t in "${engines[@]}"; do
    echo "==============="$t"===============" >> $resfile
    ./measure.sh $t octane memory
    cat test/out/octaneMemory.res >> $resfile
    echo "---------------FINISH $t"
done

echo "############## SUNSPIDER ###########" >> $resfile
for t in "${engines32[@]}"; do
    echo "==============="$t"===============" >> $resfile
    ./measure.sh $t memory
    cat test/out/memory.res >> $resfile
    echo "---------------FINISH $t"
done

for t in "${engines[@]}"; do
    echo "==============="$t"===============" >> $resfile
    ./measure.sh $t memory
    cat test/out/memory.res >> $resfile
    echo "---------------FINISH $t"
done
