#!/bin/bash

GENMATRICES_PATH="./generate_matrices.R"
POINTFINDER_PATH="../pointfinder.sh"
DRIVER_PATH="../driver.bin"
APOINTPARSE_PATH="../parse_anchorpoints.sh"
BESTPOINTPARSE_PATH="../parse_bestpoints.sh"

if [ -z "$1" ]; then 
	echo "No FCS file supplied"
	exit 1
fi

FCSFILE="$1"

if [ -z "$2" ]; then 
	echo "No job arg supplied"
	exit 1
fi

JOBS=$2

"$GENMATRICES_PATH" "$FCSFILE" > gmf

if [ $? -ne 0 ]; then
	exit 1
fi

DIR=`grep -o '".\+"' gmf | tr -d '"'`
rm gmf

cd "$DIR"

if [ -f "$GENMATRICES_PATH" ]; then
cp "$GENMATRICES_PATH" .
fi
if [ -f "$DRIVER_PATH" ]; then
cp "$DRIVER_PATH" .
fi
if [ -f "$APOINTPARSE_PATH" ]; then
cp "$APOINTPARSE_PATH" .
fi
if [ -f "$BESTPOINTPARSE_PATH" ]; then
cp "$BESTPOINTPARSE_PATH" .
fi

"$POINTFINDER_PATH" anchorpoints 0 3 1 1000000000

ANCHORPOINTS=`$APOINTPARSE_PATH log_*`

ls matrices | parallel -j$JOBS "$POINTFINDER_PATH" matrices/{} 500 2500 500 1000000000 $ANCHORPOINTS

"$BESTPOINTPARSE_PATH" > points.txt
