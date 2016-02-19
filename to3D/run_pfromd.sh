#!/bin/bash

set -e

usage() {
	echo "Usage: $0 [-j N] [-n N] FILE"
	echo "	-j N: run N instances of pfromd concurrently"
	echo "	-n N: project at most N points" 
	echo "	FILE: an FCS formatted file"
	exit 1
}
TOPDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
GENMATRICES_PATH="${TOPDIR}/generate_matrices.R"
POINTFINDER_PATH="${TOPDIR}/pointfinder.sh"
ANCHORPOINTFINDER_PATH="${TOPDIR}/project_anchorpoints.sh"
DRIVER_PATH="${TOPDIR}/driver.bin"
APOINTPARSE_PATH="${TOPDIR}/parse_anchorpoints.sh"
BESTPOINTPARSE_PATH="${TOPDIR}/parse_bestpoints.sh"

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

BASEFILE=`basename $FCSFILE`
DATE=`date "+%F_%H_%M_%S"`
LOGDIR="log_$BASEFILE_$DATE"

mkdir "$LOGDIR"
cp "$FCSFILE" "$LOGDIR"
cd "$LOGDIR"
echo "Log directory: `pwd`"

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
if [ -f "$POINTFINDER_PATH" ]; then
cp "$POINTFINDER_PATH" .
fi
if [ -f "$ANCHORPOINTFINDER_PATH" ]; then
cp "$ANCHORPOINTFINDER_PATH" .
fi


echo "Parsing FCS file, generating submatrices, and projecting anchorpoints..."
./generate_matrices.R "$FCSFILE" "$MAXPOINTS" > gmf

if [ $? -ne 0 ]; then
	exit 1
fi


NUMPOINTS=`grep -o '\[1\] [0-9]\+' gmf | sed 's/\[1\] //g'`
echo "Final number of points: $NUMPOINTS" 
DIR=`grep -o '".\+"' gmf | tr -d '"'`
rm gmf

ANCHORPOINTS=`$APOINTPARSE_PATH log_*`
echo
echo "Projected anchorpoints: $ANCHORPOINTS"

echo "Projecting points..."
ls matrices | parallel --eta -j$JOBS "\"$POINTFINDER_PATH\" matrices/{} 1500 2000 500 1000000000 $ANCHORPOINTS"

echo
echo "Parsing output files, this will take a while..."
"$BESTPOINTPARSE_PATH" > ${LOGDIR}_points.txt

echo "Done" 
echo "Projected points written to ${TOPDIR}/points.txt"
