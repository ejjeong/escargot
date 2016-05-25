#!/bin/bash

#make full
tests=("3d-cube" "3d-morph" "3d-raytrace" "access-binary-trees" "access-fannkuch" "access-nbody" "access-nsieve" "bitops-3bit-bits-in-byte" "bitops-bits-in-byte" "bitops-bitwise-and" "bitops-nsieve-bits" "controlflow-recursive" "crypto-aes" "crypto-md5" "crypto-sha1" "date-format-tofte" "date-format-xparb" "math-cordic" "math-partial-sums" "math-spectral-norm" "regexp-dna" "string-base64" "string-fasta" "string-tagcloud" "string-unpack-code" "string-validate-input")
cp test/SunSpider/resources/sunspider-standalone-driver.orig.js test/SunSpider/resources/sunspider-standalone-driver.js
if [[ $1 == duk*32 ]]; then
  curdir=`pwd`
  export LD_LIBRARY_PATH="$curdir/test/bin/x86/duk/"
  cmd="./test/bin/x86/duk/duk"
  tc="duktape32"
elif [[ $1 == duk* ]]; then
  cmd="./test/bin/x64/duk/duk"
  tc="duktape"
elif [[ $1 == v8* ]]; then
  if [[ $2 == time ]]; then
      cp test/SunSpider/resources/sunspider-standalone-driver-v8.js test/SunSpider/resources/sunspider-standalone-driver.js
  fi
  if [[ $1 == v8.full.jit ]]; then
    cmd="./test/bin/x64/v8/d8"
    args="--nocrankshaft"
    tc="v8.full"
  elif [[ $1 == v8.jit ]]; then
    cmd="./test/bin/x64/v8/d8"
    tc="v8"
  elif [[ $1 == v832.full.jit ]]; then
    cmd="./test/bin/x86/v8/d8"
    args="--nocrankshaft"
    tc="v832.full"
  elif [[ $1 == v832.jit ]]; then
    cmd="./test/bin/x86/v8/d8"
    tc="v832"
  else
    echo "choose one between ([escargot|jsc|v8](32)?.(full)?.[interp|jit|full.jit])|duk"
    exit 1
  fi
elif [[ $1 == jsc.interp* ]]; then
  curdir=`pwd`
  export LD_LIBRARY_PATH="$curdir/test/bin/x64/jsc/lib:$curdir/test/bin/x64/jsc/libicu"
  cmd="./test/bin/x64/jsc/interp/jsc"
  tc="jsc.interp"
elif [[ $1 == jsc.jit ]]; then
  curdir=`pwd`
  export LD_LIBRARY_PATH="$curdir/test/bin/x86/jsc/lib:$curdir/test/bin/x86/jsc/libicu"
  cmd="./test/bin/x64/jsc/baseline/jsc"
  tc="jsc.jit"
elif [[ $1 == jsc.full.jit ]]; then
  curdir=`pwd`
  export LD_LIBRARY_PATH="$curdir/test/bin/x64/jsc/lib:$curdir/test/bin/x64/jsc/libicu"
  cmd="./test/bin/x64/jsc/baseline/jsc"
  args="--useDFGJIT=false"
  tc="jsc.baselineJIT"
elif [[ $1 == jsc32.interp* ]]; then
  curdir=`pwd`
  export LD_LIBRARY_PATH="$curdir/test/bin/x86/jsc/lib:$curdir/test/bin/x86/jsc/libicu"
  cmd="./test/bin/x86/jsc/interp/jsc"
  tc="jsc32.interp"
elif [[ $1 == jsc32.jit ]]; then
  curdir=`pwd`
  export LD_LIBRARY_PATH="$curdir/test/bin/x86/jsc/lib:$curdir/test/bin/x86/jsc/libicu"
  cmd="./test/bin/x86/jsc/baseline/jsc"
  tc="jsc32.jit"
elif [[ $1 == jsc32.full.jit ]]; then
  curdir=`pwd`
  export LD_LIBRARY_PATH="$curdir/test/bin/x86/jsc/lib:$curdir/test/bin/x86/jsc/libicu"
  cmd="./test/bin/x86/jsc/baseline/jsc"
  args="--useDFGJIT=false"
  tc="jsc32.baselineJIT"
elif [[ $1 == escargot.interp* ]]; then
  make x64.interpreter.release -j8
  cmd="./out/linux/x64/interpreter/release/escargot"
  tc="escargot64.interp"
elif [[ $1 == escargot.jit ]]; then
  make x64.jit.release -j8
  cmd="./out/linux/x64/jit/release/escargot"
  tc="escargot64.jit"
elif [[ $1 == escargot32.interp* ]]; then
  make x86.interpreter.release -j8
  cmd="./out/linux/x86/interpreter/release/escargot"
  tc="escargot32.interp"
elif [[ $1 == escargot32.jit ]]; then
  make x86.jit.release -j8
  cmd="./out/linux/x86/jit/release/escargot"
  tc="escargot32.jit"
else
  echo "choose one between ([escargot|jsc|v8](32)?.(full)?.[interp|jit|base])|duk"
  exit 1
fi
echo $cmd
testpath="./test/SunSpider/tests/sunspider-1.0.2/"

mkdir -p test/out
rm test/out/*.out -f
num=$(date +%y%m%d_%H_%M_%S)
curdir=`pwd`;
memresfile=$(echo $curdir'/test/out/'$tc'_mem_'$num'.res')

function measure(){
  finalcmd=$1" "$2" "$3
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

if [[ $2 == octane ]]; then
  if [[ $3 == memory ]]; then
    outfile=$(echo "../out/"$1"_octane_memory.out")
    cd test/octane
    ../../$cmd $args run.js &
    PID=$!
    (while [ "$PID" ]; do
      [ -f "/proc/$PID/smaps" ] || { exit 1;};
      ../bin/memps -p $PID 2> /dev/null
      echo \"=========\"$PID; sleep 0.0001;
     done ) >> $outfile &
      sleep 340s;
      echo "==========" >> $memresfile
      MAXV=`cat $outfile | grep 'PSS:' | sed -e 's/,//g' | awk '{ if(max < $2) max=$2} END { print max}'`
      MAXR=`cat $outfile | grep 'RSS:' | sed -e 's/,//g' | awk '{ if(max < $2) max=$2} END { print max}'`
      echo 'MaxPSS:'$MAXV', MaxRSS:'$MAXR >> $memresfile
      echo $MAXV
    cd -
    exit 1;
  fi
  cd test/octane
  ../../$cmd $args run.js
  cd -
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
    finalcmd="$cmd $filename &"
    summem=""
    echo '===================' >> $memresfile
    for j in {1..10}; do
      MAXV='Error'
      measure $finalcmd
      summem=$summem$MAXV"\\n"
      sleep 0.5s;
    done
    echo $(echo -e $summem | awk '{s+=$1;if(NR==1||max<$1){max=$1};if(NR==1||($1!="" && $1<min)){min=$1}} END {printf("Avg. MaxPSS: %.4f", (s-max-min)/8)}')
    echo $t $(echo -e $summem | awk '{s+=$1;if(NR==1||max<$1){max=$1};if(NR==1||($1!="" && $1<min)){min=$1}} END {printf(": %.4f", (s-max-min)/8)}') >> tmp
  done
  cat tmp | awk '{s+=$3} END {print s/26}' >> tmp
  cat tmp
  cat tmp > test/out/memory.res
  rm tmp
fi

if [[ $1 == v8* ]]; then
  cp test/SunSpider/resources/sunspider-standalone-driver.orig.js test/SunSpider/resources/sunspider-standalone-driver.js
fi

echo '-------------------------------------------------finish exe'
