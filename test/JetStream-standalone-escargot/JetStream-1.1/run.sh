#!/bin/sh

PATH_TO_ENGINE=$1

DATE=$(date +%y%m%d_%H_%M_%S)
OUTFILE=$(echo "jetstream-result-raw_"$DATE".res")

i=0
echo "" > $OUTFILE;
while [ $i -lt 37 ]; do
#    echo $PATH_TO_ENGINE
    $PATH_TO_ENGINE ./runOnePlan.js -- $i > tmp;
    tail -1 tmp
    tail -1 tmp >> $OUTFILE;
    i=$(( i + 1 ))
done
rm tmp;
cat $OUTFILE;
