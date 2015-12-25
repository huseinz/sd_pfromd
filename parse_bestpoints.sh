#!/bin/bash

#this script searches through the all the main.log files and locates the entry with the lowest error
#it then parses and returns the points
for DIR in log_matrices_*; do
	grep -E '\([0-9]+\)' "$DIR"/main.log | sed -r 's|(^.*)\(([0-9]+)\)|\2 \1|g' | sort -n | head -n 1 | grep -Eo '\([0-9]+, [0-9]+\)\];' | sed 's/[];,)(]//g' 
done
