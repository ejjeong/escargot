#!/bin/bash

tests=("bitops-bitwise-and" "bitops-bits-in-byte" "bitops-3bit-bits-in-byte")
if [[ $1 == duk* ]]; then
  cmd="~/webTF/duktape-1.2.2/duk"
  tc="duktape"
elif [[ $1 == v8* ]]; then
  cmd="~/webTF/201504llvm/v8/out/x64.release/d8"
  tc="v8"
else
  cmd="./escargot"
  tc="escargot"
fi
echo $cmd
#testpath="./"
testpath="./test/SunSpider/tests/sunspider-1.0.2/"

mkdir test/out
rm test/out/*.out
num=$(echo "0731")
resfile=$(echo 'test/out/'$tc'_x86_mem_'$num'.res')

function measure(){
  $finalcmd=$1
  eval $finalcmd
  PID=$!
  #echo $PID
  (while [ "$PID" ]; do
    [ -f "/proc/$PID/smaps" ] || { exit 1;};
    ./memps -p $PID 2> /dev/null
    echo \"=========\"; sleep 0.0001;
  done ) >> $outfile &
  sleep 1s;
  echo $t >> $resfile
  MAXV=`cat $outfile | grep 'PSS:' | sed -e 's/,//g' | awk '{ if(max < $2) max=$2} END { print max}'`
  MAXR=`cat $outfile | grep 'RSS:' | sed -e 's/,//g' | awk '{ if(max < $2) max=$2} END { print max}'`
  echo 'MaxPSS:'$MAXV', MaxRSS:'$MAXR >> $resfile
  rm $outfile
  echo $MAXV
}

if [[ $tc == escargot ]]; then
  tmpfile=$(echo "test/out/time.out")
  for j in {1..10}; do
    ./run-Sunspider.sh >> $tmpfile
  done
fi

for t in "${tests[@]}"; do
  sleep 1s;
  filename=$(echo $testpath$t'.js')
  outfile=$(echo "test/out/"$t".out")
  echo '-----'$t
  finalcmd="sleep 0.5; $cmd $filename &"
  summem=""
  echo '===================' >> $resfile
  for j in {1..10}; do
    MAXV='Error'
    measure $finalcmd 2> /dev/null
    summem=$summem$MAXV"\\n"
    sleep 0.5s;
  done
  echo $(echo -e $summem | awk '{s+=$1} END {printf("Avg. MaxPSS: %.4f", s/10)}')
  if [[ $tc == escargot ]]; then
    cat $tmpfile | grep $t | sed -e 's/://g' | sed -e 's/,//g' | awk '{s+=$2;} END {printf("Avg. Time: %.4f\n", s/10)}'
  fi
done

echo '-------------------------------------------------finish exe'

