#!/bin/bash

BIN_ID=../../../escargot
TEST=test.js
TC_FILE=TC.stress

i=1;

echo "stress test"
cat $TC_FILE |
while read -r t
do
    if [[ $t == "//"* ]]
    then
        echo "(EXCLUDED)" test $i " : " $t
        let i=i+1
    else
        echo "--------->" test $i " : " $t
        let i=i+1
        $BIN_ID $TEST $t
    fi
done

