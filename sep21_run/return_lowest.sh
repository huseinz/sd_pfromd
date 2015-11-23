#/bin/bash

for dir in log_*; do
	ls "$dir" | head -n 1 | tr "\n" " "
	#grep ^sat "$dir"/*out* | sed -r 's|.*auto\.([0-9]+)\.out\.txt:sat|\1|g' | sort -h  | head -n 1 | tr "\n" " "
	grep -H ^sat "$dir"/*out* | grep -o -E '\.[0-9]+\.' | sed 's/\.//g' | sort -h  | head -n 1 | tr "\n" " "
	echo
done
