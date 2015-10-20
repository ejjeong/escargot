#!/bin/bash

if [ "$#" -eq 1 ]
then
  if [ "$1" = "-rcf" ]
  then
	tests=("3d-cube" "3d-morph" "access-binary-trees" "access-nbody" "access-nsieve" "bitops-3bit-bits-in-byte" "bitops-bits-in-byte" "bitops-bitwise-and" "bitops-nsieve-bits" "controlflow-recursive" "crypto-aes" "date-format-tofte" "math-cordic" "math-partial-sums" "math-spectral-norm" "regexp-dna" "string-base64" "string-fasta" "string-tagcloud" "string-unpack-code" );
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

