#!/bin/bash

#make full
tests=("3d-cube" "3d-morph" "3d-raytrace" "access-binary-trees" "access-fannkuch" "access-nbody" "access-nsieve" "bitops-3bit-bits-in-byte" "bitops-bits-in-byte" "bitops-bitwise-and" "bitops-nsieve-bits" "controlflow-recursive" "crypto-aes" "crypto-md5" "crypto-sha1" "date-format-tofte" "date-format-xparb" "math-cordic" "math-partial-sums" "math-spectral-norm" "regexp-dna" "string-base64" "string-fasta" "string-tagcloud" "string-unpack-code" "string-validate-input")
cp test/SunSpider/resources/sunspider-standalone-driver.orig.js test/SunSpider/resources/sunspider-standalone-driver.js
if [[ $1 == duk* ]]; then
  cmd="./test/bin/duk_v1.3.0"
  tc="duktape"
elif [[ $1 == v8.full* ]]; then
  cmd="./test/bin/d8"
  args="--nocrankshaft"
  tc="v8"
  if [[ $2 == time ]]; then
      cp test/SunSpider/resources/sunspider-standalone-driver-v8.js test/SunSpider/resources/sunspider-standalone-driver.js
  fi
elif [[ $1 == v8.4.8.full ]]; then
  cmd="./test/bin/d8_v4.8.1"
  args="--nocrankshaft"
  tc="v8"
  if [[ $2 == time ]]; then
      cp test/SunSpider/resources/sunspider-standalone-driver-v8.js test/SunSpider/resources/sunspider-standalone-driver.js
  fi
elif [[ $1 == v8.4.8 ]]; then
  cmd="./test/bin/d8_v4.8.1"
  tc="v8"
  if [[ $2 == time ]]; then
      cp test/SunSpider/resources/sunspider-standalone-driver-v8.js test/SunSpider/resources/sunspider-standalone-driver.js
  fi
elif [[ $1 == v8* ]]; then
  cmd="./test/bin/d8"
  tc="v8"
  if [[ $2 == time ]]; then
      cp test/SunSpider/resources/sunspider-standalone-driver-v8.js test/SunSpider/resources/sunspider-standalone-driver.js
  fi
elif [[ $1 == jsc.interp* ]]; then
  cmd="./test/bin/jsc.interp"
  tc="jsc.interp"
elif [[ $1 == jsc.base* ]]; then
  cmd="./test/bin/jsc.jit"
  tc="jsc.baselineJIT"
  args="--useDFGJIT=false"
elif [[ $1 == jsc.jit || $1 == jsc.dfg* ]]; then
  cmd="./test/bin/jsc.jit"
  tc="jsc.jit"
elif [[ $1 == escargot.interp* ]]; then
  make x64.interpreter.release -j8
  cmd="./out/x64/interpreter/release/escargot"
  tc="escargot.interp"
elif [[ $1 == escargot.jit ]]; then
  make x64.jit.release -j8
  cmd="./out/x64/jit/release/escargot"
  tc="escargot.jit"
else
  make x64.interpreter.release -j8
  cmd="./out/x64/interpreter/release/escargot"
  tc="escargot.interp"
fi
echo $cmd
testpath="./test/SunSpider/tests/sunspider-1.0.2/"

mkdir -p test/out
rm test/out/*.out -f
num=$(date +%y%m%d_%H_%M_%S)
memresfile=$(echo 'test/out/'$tc'_x86_mem_'$num'.res')

function measure(){
  $finalcmd=$1
  eval $finalcmd
  PID=$!
  #echo $PID
  (while [ "$PID" ]; do
    [ -f "/proc/$PID/smaps" ] || { exit 1;};
    ./test/bin/memps -p $PID 2> /dev/null
    echo \"=========\"; sleep 0.0001;
  done ) >> $outfile &
  sleep 1s;
  echo $t >> $memresfile
  MAXV=`cat $outfile | grep 'PSS:' | sed -e 's/,//g' | awk '{ if(max < $2) max=$2} END { print max}'`
  MAXR=`cat $outfile | grep 'RSS:' | sed -e 's/,//g' | awk '{ if(max < $2) max=$2} END { print max}'`
  echo 'MaxPSS:'$MAXV', MaxRSS:'$MAXR >> $memresfile
  rm $outfile
  echo $MAXV
}

if [[ $2 == test262 ]]; then
  echo 'run test262 with option: ' $3
  run=$(echo 'python tools/packaging/test262.py --command ../../'$cmd' '$3)
  echo $run
  cd test/test262
  eval $run
 # python tools/packaging/test262.py --command ../../$cmd $3
  cd ../..
  exit 1;
fi

timeresfile=$(echo 'test/out/'$tc'_x86_time_'$num'.res')
echo '' > $timeresfile
if [[ $2 == mem* ]]; then
  echo 'No Measure Time'
else
  cd test/SunSpider
  ./sunspider --shell=../../$cmd --suite=sunspider-1.0.2 --args="$args" | tee ../../$timeresfile
  cd -
fi

if [[ $2 == time ]]; then
  echo 'No Measure Memory'
else
  echo '' > tmp
  for t in "${tests[@]}"; do
    sleep 1s;
    filename=$(echo $testpath$t'.js')
    outfile=$(echo "test/out/"$t".out")
    echo '-----'$t
    finalcmd="sleep 0.5; $cmd $filename &"
    summem=""
    echo '===================' >> $memresfile
    for j in {1..10}; do
      MAXV='Error'
      measure $finalcmd 2> /dev/null
      summem=$summem$MAXV"\\n"
      sleep 0.5s;
    done
    echo $(echo -e $summem | awk '{s+=$1;if(NR==1||max<$1){max=$1};if(NR==1||($1!="" && $1<min)){min=$1}} END {printf("Avg. MaxPSS: %.4f", (s-max-min)/8)}')
    echo $t $(echo -e $summem | awk '{s+=$1;if(NR==1||max<$1){max=$1};if(NR==1||($1!="" && $1<min)){min=$1}} END {printf(": %.4f", (s-max-min)/8)}') >> tmp
  done
  cat tmp
  cat tmp > test/out/memory.res
  rm tmp
fi

if [[ $1 == v8* ]]; then
  cp test/SunSpider/resources/sunspider-standalone-driver.orig.js test/SunSpider/resources/sunspider-standalone-driver.js
fi

echo '-------------------------------------------------finish exe'
