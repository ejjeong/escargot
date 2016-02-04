#!/bin/sh

PATH_TO_ENGINE=$1
DATA_JS=$2

RAW_RESULTS_FILENAME="jetstream-results-raw"

echoerr() { echo "$@" >&2; }

NUM_SETS=$3
SLEEP_TIME=$4

echo "var output = [" > $RAW_RESULTS_FILENAME
j=1
while [ $j -le $NUM_SETS ]; do
    if [ $j -ne 1 ]; then
        sleep $SLEEP_TIME
    fi

    echo $DATA_JS
        # iterate 1 time for warmup
    echo "warm up finished"

    i=1
    while [ $i -le $NUM_TESTS ]; do
        $PATH_TO_ENGINE run.js $PLAN_NAME >> $RAW_RESULTS_FILENAME
        if [ $i -ne $NUM_TESTS ] || [ $j -ne $NUM_TESTS ]; then
            echo "," >> $RAW_RESULTS_FILENAME
        fi
        i=$(( i + 1 ))
    done
    j=$(( j + 1 ))
done

echo "];" >> $RAW_RESULTS_FILENAME



