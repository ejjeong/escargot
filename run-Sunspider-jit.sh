#!/bin/bash
for ((i=0;i<10;i++)); do
	cd test/SunSpider/
	./run-jit.sh
	cd ../../
done

