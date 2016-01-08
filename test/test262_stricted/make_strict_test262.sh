#!/bin/sh

cur=`pwd`
root_dir="${cur}"
test_dir="${root_dir}/test"

exec < "sortAll.txt"

while read line
do
  rm "${test_dir}/suite/${line}"
done
