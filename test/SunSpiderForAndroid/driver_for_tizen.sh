#!/bin/sh

PATH_TO_D8=$1
DATA_JS=$2
NUM_TESTS=10		# number of test runs a set (fixed)

RAW_RESULTS_FILENAME="sunspider-results-raw"

echoerr() { echo "$@" >&2; }

if [ $# -eq 4 ]
then
  NUM_SETS=$3 		# number of test sets (can be changed with 4th argument)
  SLEEP_TIME=$4		# how much it should wait for the device to cool down?
elif [ $# -eq 5 ]
then
  ARGS=$3
  NUM_SETS=$4 		# number of test sets (can be changed with 4th argument)
  SLEEP_TIME=$5		# how much it should wait for the device to cool down?
else
  echoerr "Usage; $0 path-to-d8 data-js args num_sets sleep_time"
  exit 1
fi

rm -f sunspider-results-*
echo "var output = [" > $RAW_RESULTS_FILENAME

j=1
while [ $j -le $NUM_SETS ]; do
	if [ $j -ne 1 ]; then
		sleep $SLEEP_TIME
	fi

  echo $DATA_JS
	# warmup
	$PATH_TO_D8 $ARGS $DATA_JS SunSpider/resources/sunspider-standalone-driver.js > /dev/null
  echo "warm up finished"

	# execute one set
	i=1
	while [ $i -le $NUM_TESTS ]; do
		$PATH_TO_D8 $ARGS $DATA_JS SunSpider/resources/sunspider-standalone-driver.js >> $RAW_RESULTS_FILENAME
		if [ $i -ne $NUM_TESTS ] || [ $j -ne $NUM_TESTS ]; then
			echo "," >> $RAW_RESULTS_FILENAME
		fi
		i=$(( i + 1 ))
	done
	j=$(( j + 1 ))
done

echo "];" >> $RAW_RESULTS_FILENAME

# execute analyze routine
$PATH_TO_D8 $ARGS $DATA_JS $RAW_RESULTS_FILENAME SunSpider/resources/sunspider-analyze-results.js

