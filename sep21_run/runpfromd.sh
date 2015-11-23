#/bin/bash/

ls dist | sort -h | parallel -j10 ./pointfinder.sh dist/{} 400 2000 400 1000000000
