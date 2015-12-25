#!/bin/bash

#"Exit immediately if a command exits with a non-zero status".
set -e 

#The tool needs precisely one argument when invoked: a (plain text) description file.
if [ "$#" -ne 5 ] && [ "$#" -ne 11 ] ||  [ ! -e $1 ]
then
  echo "[pointfinder] usage: $0 DISTANCE_MATRIX_FILE_PATH  MIN_DISTANCE_ERROR MAX_DISTANCE_ERROR ERROR_INCREMENT POINT_COORDINATES_UPPER_BOUND [anchorp1.x anchorp1.y anchorp2.x anchorp2.y anchorp3.x anchorp3.y]" >&2
  echo "[pointfinder] Make sure that (MAX_DISTANCE_ERROR - MIN_DISTANNCE_ERROR)/ERROR_INCREMENT is integral"
  exit 1
fi

DISTANCE_MATRIX_FILE_PATH=$1
DISTANCE_ERROR_LOWER_BOUND=$2
DISTANCE_ERROR_UPPER_BOUND=$3
DISTANCE_ERROR_INCREMENT=$4
POINT_COORDINATES_UPPER_BOUND=$5

NOSLASH_DISTANCE_MATRIX_FILE_PATH="${DISTANCE_MATRIX_FILE_PATH/\//_}"

DATE_TIME_INFO=`date "+%H_%M_%S_%Z_%d_%b_%Y"`
USERINFO=`echo $USER`
MACHINEINFO=`echo $HOSTNAME`
OUTPUT_DATA_DIR_NAME="log_"${NOSLASH_DISTANCE_MATRIX_FILE_PATH}_${DATE_TIME_INFO}"_"${USERINFO}"_on_"${MACHINEINFO}
OUTPUT_DATA_DIR_FULLPATH=${OUTPUT_DATA_ROOT_LOCATION}${OUTPUT_DATA_DIR_NAME}

#echo -e ${OUTPUT_DATA_DIR_NAME}

mkdir ${OUTPUT_DATA_DIR_FULLPATH}
if [ $? -ne 0 ] 
then
    echo -e "\n[pointfinder] error: could not create log dir. quitting."
    exit 1
else
    echo -e "\n[pointfinder] successfully created log dir ${OUTPUT_DATA_DIR_FULLPATH}."
fi



DIFF=$((${DISTANCE_ERROR_UPPER_BOUND} - ${DISTANCE_ERROR_LOWER_BOUND}))

if (( ${DIFF} % DISTANCE_ERROR_INCREMENT != 0 ))
then
    echo "[pointfinder] usage: $0 DISTANCE_MATRIX_FILE_PATH  MIN_DISTANNCE_ERROR MAX_DISTANCE_ERROR ERROR_INCREMENT" >&2
    echo "[pointfinder] Make sure (MAX_DISTANCE_ERROR - MIN_DISTANNCE_ERROR)/ERROR_INCREMENT is integral"
exit 1
fi

DIV=$((${DIFF} / ${DISTANCE_ERROR_INCREMENT}))
NUMTHREADS=$((${DIV}+2))
#echo -e "[pointfinder] Will create ${NUMTHREADS} threads."

if [ "$#" -eq 5 ]
then 
mpirun -np ${NUMTHREADS} ./driver.bin ${DISTANCE_MATRIX_FILE_PATH}  ${DISTANCE_ERROR_LOWER_BOUND} ${DISTANCE_ERROR_UPPER_BOUND} ${DISTANCE_ERROR_INCREMENT} ${POINT_COORDINATES_UPPER_BOUND} ${OUTPUT_DATA_DIR_FULLPATH}
else
mpirun -np ${NUMTHREADS} ./driver.bin ${DISTANCE_MATRIX_FILE_PATH}  ${DISTANCE_ERROR_LOWER_BOUND} ${DISTANCE_ERROR_UPPER_BOUND} ${DISTANCE_ERROR_INCREMENT} ${POINT_COORDINATES_UPPER_BOUND} ${OUTPUT_DATA_DIR_FULLPATH} $6 $7 $8 $9 ${10} ${11}
fi

echo -e "[pointfinder] pointfinder has terminated."
