#!/bin/bash

set -e

usage() {
	echo "Usage: $0 [-j N] [-n N] FILE"
	echo "	-j N: run N instances of pfromd concurrently"
	echo "	-n N: project at most N points" 
	echo "	FILE: an FCS formatted file"
	exit 1
}

GENMATRICES_PATH="./generate_matrices.R"
POINTFINDER_PATH="../pointfinder.sh"
DRIVER_PATH="../driver.bin"
APOINTPARSE_PATH="../parse_anchorpoints.sh"
BESTPOINTPARSE_PATH="../parse_bestpoints.sh"

JOBS=1
MAXPOINTS=0
while getopts "j:n:" o; do 
	case "$o" in
		j)
			JOBS="$OPTARG"
			(( JOBS >= 1 )) || usage
			;;
		n)	
			MAXPOINTS="$OPTARG" 
			(( MAXPOINTS > 0 )) || usage  
			;;
		?)
			usage		
			;;
	esac
done 

shift "$((OPTIND - 1))"
if  [ -z "$1" ]; then 
	echo "No FCS file supplied"
	exit 1
fi

FCSFILE="$1" 

echo "FCS file:" $FCSFILE
if [ ! -f "$FCSFILE" ]; then
	echo "Supplied FCS file does not exist"
	usage
fi

echo "Jobs:" $JOBS 
if  (( MAXPOINTS )); then
	echo "Max points:" $MAXPOINTS
fi 

echo "Parsing FCS file and generating submatrices..."
"$GENMATRICES_PATH" "$FCSFILE" "$MAXPOINTS" > gmf

if [ $? -ne 0 ]; then
	exit 1
fi


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

#echo "Projecting anchorpoints..."
#"$POINTFINDER_PATH" anchorpoints 0 3 1 1000000000

ANCHORPOINTS=`$APOINTPARSE_PATH log_*`
echo
echo "Projected anchorpoints: $ANCHORPOINTS"

ls matrices | parallel --eta -j$JOBS "\"$POINTFINDER_PATH\" matrices/{} 1000 2000 500 1000000000 $ANCHORPOINTS"

"$BESTPOINTPARSE_PATH" > points.txt

echo 
echo "Done" 
echo "Projected points written to `pwd`/points.txt"
