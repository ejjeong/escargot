#!/bin/bash
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.


#set -eu

exitcode=0

execute=1

if [ "$1" = "--asm" ]; then
  execute=0
  shift
fi

LIRASM="adb shell /data/local/tmp/lirasm/escargot -a lirasm"
if [ $# -gt 1 ]; then
TESTFLOAT=$2
else
  TESTFLOAT=float
fi

TESTS_DIR=tests
TESTS_DIR_ANDROID=/data/local/tmp/lirasm

function runtest {
    local infile=$TESTS_DIR_ANDROID/$1
    local options=${2-}

    # Catch a request for the random tests.
    if [[ $infile == *--random* ]] ; then
		local infile=--random
        local outfile=$TESTS_DIR/random.out
    else
        local outfile=`echo $1 | sed 's/\.in/\.out/'`
    fi

    if [[ ! -e "$outfile" ]] ; then
        echo "$0: error: no out file $outfile"
        exit 1
    fi

    # sed used to strip extra leading zeros from exponential values 'e+00' (see bug 602786)
    if $LIRASM $options --execute $infile | tr -d '\r' | sed -e 's/e+00*/e+0/g' > testoutput.txt && cmp -s testoutput.txt $outfile ; then
        echo "TEST-PASS | lirasm | $LIRASM $options --execute $infile"
    else
        echo "TEST-UNEXPECTED-FAIL | lirasm | lirasm $options --execute $infile"
        echo "expected output"
        cat $outfile
        echo "actual output"
        cat testoutput.txt
        exitcode=1
    fi
}

function emitasm {
    local infile=$1
    local options=${2-}

    # Catch a request for the random tests.
    if [[ $infile == --random* ]] ; then
        local outfile=$TESTS_DIR/random.out
    else
        local outfile=`echo $infile | sed 's/\.in/\.out/'`
    fi

    if [[ ! -e "$outfile" ]] ; then
        echo "$0: error: no out file $outfile"
        exit 1
    fi

    $LIRASM $options --verbose $infile
}

function runtests {
    local testdir=$1
    local options=${2-}
    for infile in "$TESTS_DIR"/"$testdir"/*.in ; do
        if [ "$execute" -eq 1 ]; then
        runtest $infile "$options"
        else
            emitasm $infile "$options"
        fi
    done
    if [[ $TESTFLOAT == float ]] ; then
        if [[ -e "$TESTS_DIR"/"$testdir"/float ]] ; then
            for infile in "$TESTS_DIR"/"$testdir"/float/*.in ; do
                runtest $infile "$options"
            done
        fi
    fi
}

# ARMv7 with VFP.  We could test without VFP but such a platform seems
# unlikely.  ARM is bi-endian but usually configured as little-endian.
runtests "."
runtests "hardfloat"
runtests "32-bit"
runtests "littleendian"
runtest "--random 1000000"
runtest "--random 1000000" "--optimize"

rm testoutput.txt

exit $exitcode
