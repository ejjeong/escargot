#!/bin/bash

if [ "$#" -eq 1 ]
then
  if [ "$1" = "-rcf" ]
  then
	tests=("3d-cube" "3d-morph" "3d-raytrace" "access-binary-trees" "access-fannkuch" "access-nbody" "access-nsieve" "bitops-3bit-bits-in-byte" "bitops-bits-in-byte" "bitops-bitwise-and" "bitops-nsieve-bits" "controlflow-recursive" "crypto-aes" "crypto-md5" "crypto-sha1" "date-format-tofte" "date-format-xparb" "math-cordic" "math-partial-sums" "math-spectral-norm" "regexp-dna" "string-base64" "string-fasta" "string-tagcloud" "string-unpack-code" "string-validate-input" );
    for test in "${tests[@]}"; do
      echo "===> test/SunSpider/tests/sunspider-1.0.2/$test.js"
      ./escargot -rcf test/SunSpider/tests/sunspider-1.0.2/$test.js
    done
  elif [ "$1" = "-rof" ]
  then
	tests=("3d-cube" "3d-morph" "3d-raytrace" "access-binary-trees" "access-fannkuch" "access-nbody" "access-nsieve" "bitops-3bit-bits-in-byte" "bitops-bits-in-byte" "bitops-bitwise-and" "bitops-nsieve-bits" "controlflow-recursive" "crypto-aes" "crypto-md5" "crypto-sha1" "date-format-tofte" "date-format-xparb" "math-cordic" "math-partial-sums" "math-spectral-norm" "regexp-dna" "string-base64" "string-fasta" "string-tagcloud" "string-unpack-code" "string-validate-input" );
    for test in "${tests[@]}"; do
      echo "===> test/SunSpider/tests/sunspider-1.0.2/$test.js"
      ./escargot -rof test/SunSpider/tests/sunspider-1.0.2/$test.js
    done
  fi
else
  for ((i=0;i<10;i++)); do
    cd test/SunSpider/
    ./run-jit.sh
    cd ../../
done
fi

