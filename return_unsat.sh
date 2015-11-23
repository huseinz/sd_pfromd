#/bin/bash

for dir in log_*; do
	ls "$dir" | head -n 1 | tr "\n" " "
	grep ^unsat "$dir"/*out* | sed -r 's|.*auto\.([0-9]+)\.out\.txt:sat|\1|g' | sort -h  | head -n 1 | tr "\n" " "
	echo
done
