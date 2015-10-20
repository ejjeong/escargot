#!/bin/bash

if [ "$#" -eq 1 ]
then
  if [ "$1" = "-rcf" ]
  then
    tests=("access-binary-trees" "access-nbody" "access-nsieve" "bitops-3bit-bits-in-byte" "bitops-bits-in-byte" "controlflow-recursive" "math-cordic" "string-unpack-code" "string-fasta" "string-tagcloud")
    for test in "${tests[@]}"; do
      ./escargot -rcf test/SunSpider/tests/sunspider-1.0.2/$test.js
    done
  fi
else
  for ((i=0;i<10;i++)); do
    cd test/SunSpider/
    ./run-jit.sh
    cd ../../
done
fi

