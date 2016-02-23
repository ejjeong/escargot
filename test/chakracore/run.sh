TEST_ROOT=$(pwd)
BIN=

TEMPORARY_OUTPUT_FILE=$TEST_ROOT/tmp
TEMPORARY_DIFF_FILE=$TEST_ROOT/diff
LOG_FILE=$TEST_ROOT/chakracorelog.verbose.txt

INCLUDE=" $TEST_ROOT/include.js $TEST_ROOT/UnitTestFramework/UnitTestFramework.js"

ARRAY_KEYS=()
declare -A TABLE_COUNT
declare -A TABLE_PASS
declare -A TABLE_FAIL
declare -A TABLE_SKIP

print_usage() {
	echo "$ cd test/chakracore"
	echo "$ ./run.sh \$BIN \$TESTDIR "
}

read_dom() {
	local IFS=\>
	read -d \< ENTITY CONTENT
}

print_count() {
	TESTNAME=$1
	COUNT=$2
	PASS=$3
	FAIL=$4
	SKIP=$5

	PASS_RATIO=$((PASS*100/COUNT))

	printf "%20s\t\t%d\t\t%d\t\t%d\t\t%d\t\t(%d%%)\n" $TESTNAME $COUNT		$PASS		$FAIL		$SKIP		$PASS_RATIO
}

run_test() {
	FILES=$1
	BASELINE=$2
	SKIP=$3

	CMD="../$BIN $INCLUDE $FILES"
	echo "==========================================================" >> $LOG_FILE
	echo -n "[$DIR] $FILES .......... "

	echo "[$DIR] $FILES" >> $LOG_FILE
	echo $CMD >> $LOG_FILE

	if [[ $SKIP != "" ]]; then
		((LOCAL_SKIP+=1))
		printf "Skip ($SKIP)\n" | tee -a $LOG_FILE
	else
		$($CMD > $TEMPORARY_OUTPUT_FILE 2>> $LOG_FILE)
		$(diff -Z -i $TEMPORARY_OUTPUT_FILE $BASELINE 2>&1 > $TEMPORARY_DIFF_FILE)
		DIFF_EXIT_CODE=$?
		if [[ "$DIFF_EXIT_CODE" != "0" ]]; then
			printf "Fail\n" | tee -a $LOG_FILE
			cat $TEMPORARY_DIFF_FILE >> $LOG_FILE
			((LOCAL_FAIL+=1))
		else
			printf "Pass\n" | tee -a $LOG_FILE
			((LOCAL_PASS+=1))
		fi
	fi
}

run_dir() {
	DIR=$1
	FILES=
	BASELINE=
	SKIP=

	LOCAL_COUNT=0
	LOCAL_PASS=0
	LOCAL_FAIL=0
	LOCAL_SKIP=0
	cd $DIR;
	while read_dom; do
		if [[ $ENTITY == test ]]; then
			FILES=
			BASELINE=
			SKIP=
		fi
		if [[ $ENTITY == files ]]; then
			((LOCAL_COUNT+=1))
			FILES=$CONTENT
		fi
		if [[ $ENTITY == baseline ]]; then
			BASELINE=$CONTENT
		fi
		if [[ $ENTITY == escargot-skip ]]; then
			SKIP=$CONTENT
		fi
		if [[ "$ENTITY" == "/test" && "$FILES" != "" ]]; then
			REAL_FILES=$(find . -iname $FILES -printf "%P\n")
			if [[ $BASELINE == "" ]]; then
				REAL_BASELINE=$TEST_ROOT/baseline
			else
				REAL_BASELINE=$(find . -iname $BASELINE -printf "%P\n")
			fi
			run_test "$REAL_FILES" "$REAL_BASELINE" "$SKIP"
		fi
	done < rlexe.xml;

	ARRAY_KEYS+=($DIR)
	TABLE_COUNT[$DIR]=$LOCAL_COUNT
	TABLE_PASS[$DIR]=$LOCAL_PASS
	TABLE_FAIL[$DIR]=$LOCAL_FAIL
	TABLE_SKIP[$DIR]=$LOCAL_SKIP

	cd ..
}

main() {
	BIN=$1
	TESTDIR=$2

	echo "" > $LOG_FILE

	if [[ !(-x $BIN) ]]; then
		echo "JS Engine $BIN is not valid"
		print_usage
		exit 1
	fi

	if [[ $TESTDIR != "" ]]; then
		echo "Running $TESTDIR" | tee -a $LOG_FILE
		REAL_TESTDIR=$(find . -iname $TESTDIR -printf "%P\n")
		if [[ -d $REAL_TESTDIR ]]; then
			run_dir $REAL_TESTDIR
		else
			echo "Invalid dir $TESTDIR"
			exit 1
		fi
	else
		echo "Running all tests" | tee -a $LOG_FILE
		TESTDIR=
		SKIP=
		while read_dom; do
			if [[ $ENTITY == dir ]]; then
				TESTDIR=
				SKIP=
			fi
			if [[ $ENTITY == files ]]; then
				TESTDIR=$CONTENT
			fi
			if [[ $ENTITY == escargot-skip ]]; then
				SKIP=$CONTENT
			fi
			if [[ $ENTITY == /dir ]]; then
				REAL_TESTDIR=$(find . -iname $TESTDIR -printf "%P\n")
				if [[ $SKIP == "" ]]; then
					run_dir $REAL_TESTDIR
				else
					echo "[$REAL_TESTDIR] Skipping whole directory ($SKIP)"
				fi
			fi
		done < rlexedirs.xml
	fi

	TOTAL_COUNT=0
	TOTAL_PASS=0
	TOTAL_FAIL=0
	TOTAL_SKIP=0
	echo "==========================================================" | tee -a $LOG_FILE
	echo "TESTNAME					TOTAL	PASS	FAIL	SKIP	PASS_RATIO"
	for key in ${ARRAY_KEYS[@]}; do
		print_count $key ${TABLE_COUNT[$key]} ${TABLE_PASS[$key]} ${TABLE_FAIL[$key]} ${TABLE_SKIP[$key]}
		((TOTAL_COUNT+=${TABLE_COUNT[$key]}))
		((TOTAL_PASS+=${TABLE_PASS[$key]}))
		((TOTAL_FAIL+=${TABLE_FAIL[$key]}))
		((TOTAL_SKIP+=${TABLE_SKIP[$key]}))
	done
	echo "==========================================================" | tee -a $LOG_FILE
	print_count "Total" $TOTAL_COUNT $TOTAL_PASS $TOTAL_FAIL $TOTAL_SKIP
}

main $1 $2
rm $TEMPORARY_OUTPUT_FILE
rm $TEMPORARY_DIFF_FILE

