BIN_PATH=/system/bin


cp /data/local/tmp/sunspider/SunSpider/resources/sunspider-standalone-driver.js.bak /data/local/tmp/sunspider/SunSpider/resources/sunspider-standalone-driver.js
#make full
tests=("3d-cube" "3d-morph" "3d-raytrace" "access-binary-trees" "access-fannkuch" "access-nbody" "access-nsieve" "bitops-3bit-bits-in-byte" "bitops-bits-in-byte" "bitops-bitwise-and" "bitops-nsieve-bits" "controlflow-recursive" "crypto-aes" "crypto-md5" "crypto-sha1" "date-format-tofte" "date-format-xparb" "math-cordic" "math-partial-sums" "math-spectral-norm" "regexp-dna" "string-base64" "string-fasta" "string-tagcloud" "string-unpack-code" "string-validate-input")
if [[ $1 == jsc32.interp ]]; then
  cmd="/data/local/tmp/arm32/jsc/interp/jsc"
  tc="jsc32.interp"
  ldpath="/data/local/tmp/arm32/jsc/interp"
elif [[ $1 == jsc32.base ]]; then
  cmd="/data/local/tmp/arm32/jsc/baseline/jsc"
  tc="jsc32.baseline"
  ldpath="/data/local/tmp/arm32/jsc/baseline"
elif [[ $1 == escargot32.interp ]]; then
  cmd="/data/local/tmp/arm32/escargot/escargot"
  tc="escargot32.interp"
elif [[ $1 == escargot.interp ]]; then
  cmd="/data/local/tmp/arm64/escargot/escargot"
  tc="escargot.interp"
elif [[ $1 == v8.jit ]]; then
  cmd="/data/local/tmp/arm64/v8/d8"
  tc="v8.jit"
  if [[ $2 == time ]]; then
    cp /data/local/tmp/sunspider/SunSpider/resources/sunspider-standalone-driver-d8.js /data/local/tmp/sunspider/SunSpider/resources/sunspider-standalone-driver.js
  fi
elif [[ $1 == v832.jit ]]; then
  cmd="/data/local/tmp/arm32/v8/d8"
  tc="v832.jit"
  if [[ $2 == time ]]; then
    cp /data/local/tmp/sunspider/SunSpider/resources/sunspider-standalone-driver-d8.js /data/local/tmp/sunspider/SunSpider/resources/sunspider-standalone-driver.js
  fi
elif [[ $1 == v8.full.jit ]]; then
  cmd="/data/local/tmp/arm64/v8/d8"
  tc="v8.full.jit"
  args="--nocrankshaft"
  if [[ $2 == time ]]; then
    cp /data/local/tmp/sunspider/SunSpider/resources/sunspider-standalone-driver-d8.js /data/local/tmp/sunspider/SunSpider/resources/sunspider-standalone-driver.js
  fi
elif [[ $1 == v832.full.jit ]]; then
  cmd="/data/local/tmp/arm32/v8/d8"
  tc="v832.full.jit"
  args="--nocrankshaft"
  if [[ $2 == time ]]; then
    cp /data/local/tmp/sunspider/SunSpider/resources/sunspider-standalone-driver-d8.js /data/local/tmp/sunspider/SunSpider/resources/sunspider-standalone-driver.js
  fi
else
  echo "wrong argument : $1 please choose between [jsc|escargot|v8].[|32].(full)?.[base|interp|jit]"
  exit 1
fi

echo $cmd
testpath="/data/local/tmp/sunspider/SunSpider/tests/sunspider-1.0.2"
export LD_LIBRARY_PATH=$ldpath

mkdir -p test/out
rm test/out/*.out -f
num=$(date +%y%m%d_%H_%M_%S)
memps='./memps_arm'
driver='./driver.sh'
memresfile=$(echo 'test/out/'$tc'_mem_'$num'.res')

if [[ $2 == mem* ]]; then
  echo 'No Measure Time'
else
  cd sunspider
  $driver $cmd $args sunspider.js 1 1
  cd -
fi


function getmaxvalue(){
  max=-1
  idx=0
  for v in "$@"; do
    if [ $(( $idx % 2 )) -eq 1 ]; then
      v=${v/,/}
      if [ $v -gt $max ]; then
        max=$v
      fi
    fi
    idx=$(( $idx + 1 ))
  done
}

function measure(){
  $finalcmd=$1
  eval $finalcmd
  PID=$!
  while [ "$PID" ] ; do
    if [ -f "/proc/$PID/smaps" ]; then
      #echo "$memps -p $PID"
      $memps -p $PID 2> /dev/null >> $outfile
      #echo \"=========\"
      sleep 0.0001
    else
      break
    fi 
  done 
  sleep 1;
  echo $t >> $memresfile
  PSSLIST=`cat $outfile | grep 'PSS:'`
  getmaxvalue $PSSLIST
  pss=$max
  RSSLIST=`cat $outfile | grep 'RSS:'`
  getmaxvalue $RSSLIST
  rss=$max
  result='MaxPSS:'$pss', MaxRSS:'$rss''
  result >> $memresfile
  rm $outfile
  echo $pss
}

averagetotal=""
if [[ $2 == time ]]; then
  echo 'No Measure Memory'
else
  echo '' > tmp
  for t in "${tests[@]}"; do
    sleep 1;
    filename=$(echo $testpath/$t'.js')
    outfile=$(echo "test/out/"$t".out")
    echo '-----'$t
    finalcmd="sleep 0.5; $cmd $args $filename &"
    summem=""
    echo '===================' >> $memresfile
    sum=0
    pssmax=0
    pssmin=100000
    for j in 0, 1, 2, 3, 4, 5, 6, 7, 8, 9; do
      measure $finalcmd 2> /dev/null
      if [ $pss -gt $pssmax ]; then
        pssmax=$pss
      fi
      if [ $pss -lt $pssmin ]; then
        pssmin=$pss
      fi
      sum=$(($sum + $pss))
      sleep 0.5;
    done
   
    sum=$(($sum - $pssmax - $pssmin))
    average=$(($sum / 8))
    echo 'average : '$average
    averagetotal=$averagetotal'\n'$t': '$average
  done
  echo $averagetotal
  cat tmp
  cat tmp > test/out/memory.res
  rm tmp
fi

cp /data/local/tmp/sunspider/SunSpider/resources/sunspider-standalone-driver.js.bak /data/local/tmp/sunspider/SunSpider/resources/sunspider-standalone-driver.js
echo '-------------------------------------------------finish exe'
