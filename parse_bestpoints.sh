#!/bin/bash

if [ -z "$1" ]; then
    for DIR in log_matrices_*; do
	grep -E '\([0-9]+\)' "$DIR"/main.log | sed -r 's|(^.*)\(([0-9]+)\)|\2 \1|g' | sort -n | head -n 1 | grep -Eo '\([0-9]+, [0-9]+\)\];' | sed 's/[];,)(]//g' 
    done
else
	DIR=$1
	grep -E '\([0-9]+\)' "$DIR"/main.log | sed -r 's|(^.*)\(([0-9]+)\)|\2 \1|g' | sort -n | head -n 1 | grep -Eo '\([0-9]+, [0-9]+\)\];' | sed 's/[];,)(]//g' 
fi
