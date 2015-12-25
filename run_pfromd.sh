#!/bin/bash

if [ -z "$1" ]; then 
	echo "No FCS file supplied"
	exit 1
fi

FCSFILE="$1"

./generate_matrices.sh "$FCSFILE" > gmf.txt

if [ $? -ne 0 ]; then
	exit 1
fi

DIR=`grep -o '".\+"' gmf.txt | tr -d '"'`
rm gmf.txt

if [ -z "$2" ]; then 
	echo "No job arg supplied"
	exit 1
fi

JOBS=$2

cd "$DIR"

if [ -f "../pointfinder.sh" ]; then
cp ../pointfinder.sh .
fi
if [ -f "../driver.bin" ]; then
cp ../driver.bin .
fi
if [ -f "../parse_anchorpoints.sh" ]; then
cp ../parse_anchorpoints.sh .
fi
if [ -f "../parse_bestpoints.sh" ]; then
cp ../parse_bestpoints.sh .
fi

./pointfinder.sh anchorpoints 0 3 1 1000000000

ANCHORPOINTS=`./parse_anchorpoints.sh log_*`

ls matrices | parallel -j$JOBS ./pointfinder.sh matrices/{} 500 2500 500 1000000000 $ANCHORPOINTS

./givepoints.sh
