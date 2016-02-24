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
        echo "(EXCLUDED)" test $i " : " $t;
        let i=i+1;
        EX_TC=$(($EX_TC+1));
    else
        echo "--------->" test $i " : " $t;
        let i=i+1;
        $BIN_ID $TEST $t
        if [[ `echo $?` == "0" ]]
        then
            SUCC_TC=$(($SUCC_TC+1));
        else
            FAIL_TC=$(($FAIL_TC+1));
        fi
    fi
    ALL_TC=$(($ALL_TC+1));
done < $TC_FILE;

echo "All      : " $ALL_TC;
echo "Excluded : " $EX_TC;
echo "Succeed  : " $SUCC_TC;
echo "Failed   : " $FAIL_TC;

