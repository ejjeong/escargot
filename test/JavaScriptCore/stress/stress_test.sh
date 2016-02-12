#!/bin/bash

BIN_ID=../../../escargot
TEST=test.js
TC_FILE=TC.stress

TC=`cat $TC_FILE | xargs`
i=1;

echo "stress test"
for t in $TC; do
	echo "------->" test $i " : " $t
	let i=i+1
	$BIN_ID $TEST $t
done

