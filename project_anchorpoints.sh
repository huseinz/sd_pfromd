#/bin/bash

cp ../driver.bin .
../pointfinder.sh anchorpoints 0 3 1 1000000000

GREPVAL=`grep 'UNSAT for max. distortion = 0.' log*/main.log`

if [ -z "$GREPVAL" ]; then
	exit 0
else 
	rm -rf log*
	exit 1
fi
