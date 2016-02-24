#!/bin/bash

BIN_ID=../../../escargot
TEST=test.js
TC_FILE=TC.stress

i=1;
ALL_TC=0;
EX_TC=0;
SUCC_TC=0;
FAIL_TC=0;

#cat $TC_FILE |
while read t
do
    if [[ $t == "//"* ]]
    then
        echo [$i] $t ..... Excluded;
        let i=i+1;
        EX_TC=$(($EX_TC+1));
    else
        echo -n [$i] $t .....;
        let i=i+1;
        OUTPUT_MSG=$($BIN_ID $TEST $t 2>&1)
        if [[ `echo $?` == "0" ]]
        then
            SUCC_TC=$(($SUCC_TC+1));
            echo " Succ"
        else
            FAIL_TC=$(($FAIL_TC+1));
            echo " Fail ($OUTPUT_MSG)"
        fi
    fi
    ALL_TC=$(($ALL_TC+1));
done < $TC_FILE;

echo "All      : " $ALL_TC;
echo "Excluded : " $EX_TC;
echo "Succeed  : " $SUCC_TC;
echo "Failed   : " $FAIL_TC;

