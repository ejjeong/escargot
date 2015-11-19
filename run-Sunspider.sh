BIN_PATH=/system/bin
if [[ -e $BIN_PATH/sh ]]; then
  PLATFORM=android
  ROOT=/data/local/tmp
  TEST_DIR=$ROOT/sunspider/SunSpider/tests/sunspider-1.0.2
else
  PLATFORM=linux
  ROOT=.
  TEST_DIR=test/SunSpider/tests/sunspider-1.0.2
fi

tests=("3d-cube" "3d-morph" "3d-raytrace" "access-binary-trees" "access-fannkuch" "access-nbody" "access-nsieve" "bitops-3bit-bits-in-byte" "bitops-bits-in-byte" "bitops-bitwise-and" "bitops-nsieve-bits" "controlflow-recursive" "crypto-aes" "crypto-md5" "crypto-sha1" "date-format-tofte" "date-format-xparb" "math-cordic" "math-partial-sums" "math-spectral-norm" "regexp-dna" "string-base64" "string-fasta" "string-tagcloud" "string-unpack-code" "string-validate-input" );

function check-jit(){
BIN=$1
OPT=$2
if [ "$OPT" = "-rcf" ]
then
  for test in "${tests[@]}"; do
    echo "===> $TEST_DIR/$test.js"
    $BIN -rcf $TEST_DIR/$test.js
  done
elif [ "$OPT" = "-rof" ]
then
  for test in "${tests[@]}"; do
    echo "===> $TEST_DIR/$test.js"
    $BIN -rof $TEST_DIR/$test.js
  done
fi
}

function run-sunspider(){
for i in 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 ; do
    cd test/SunSpider/
    ./run-jit.sh
    cd ../../
done
}

if [[ $PLATFORM == "android" ]]; then
  if [ "$#" -eq 2 ]; then
    if [ "$2" = "-rcf" ]; then
      check-jit $1 -rcf
    elif [ "$2" = "-rof" ]; then
      check-jit $1 -rof
    else
      echo "Usage: ./run-Sunspider.sh bin [-rcf|-rof]?"
      exit 1
    fi
  else
    echo "Usage: ./run-Sunspider.sh bin [-rcf|-rof]?"
    exit 1
  fi
else
  if [ "$#" -eq 0 ]; then
    run-sunspider
  elif [ "$#" -eq 1 ]; then
    if [ "$1" = "-rcf" ]; then
      check-jit ./escargot -rcf
    elif [ "$1" = "-rof" ]; then
      check-jit ./escargot -rof
    else
      echo "Usage: ./run-Sunspider.sh [-rcf|-rof]?"
      exit 1
    fi
  else
    echo "Usage: ./run-Sunspider.sh [-rcf|-rof]?"
    exit 1
  fi
fi
