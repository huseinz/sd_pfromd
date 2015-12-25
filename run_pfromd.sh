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

echo "Parsing FCS file and generating submatrices..."
"$GENMATRICES_PATH" "$FCSFILE" > gmf

if [ $? -ne 0 ]; then
	exit 1
fi

echo
NUMPOINTS=`grep -o '\[1\] [0-9]\+' gmf | sed 's/\[1\] //g'`
echo "Final number of points: $NUMPOINTS" 
DIR=`grep -o '".\+"' gmf | tr -d '"'`
echo "Log directory: $DIR"
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

echo "Projecting anchorpoints..."
"$POINTFINDER_PATH" anchorpoints 0 3 1 1000000000

ANCHORPOINTS=`$APOINTPARSE_PATH log_*`
echo
echo "Projected anchorpoints: $ANCHORPOINTS"

ls matrices | parallel --eta -j$JOBS "\"$POINTFINDER_PATH\" matrices/{} 250 1250 250 1000000000 $ANCHORPOINTS"

"$BESTPOINTPARSE_PATH" > points.txt

echo 
echo "Done" 
echo "Projected points written to `pwd`/points.txt"
