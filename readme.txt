Run 'make' to compile the pfromd executable. 
See pfromd_readme.txt for information regarding compiling/usage of the pfromd executable


Usage of run_pfromd.sh wrapper script: 

./run_pfromd.sh [-j N] [-n N] FILE

        -j N: run N instances of pfromd concurrently
        -n N: project at most N points
        FILE: FCS formatted file

INSTALLING DEPENDENCIES


```
# Installing a ton of stuff

sudo apt-get install g++ gcc build-essential make git r-base parallel libopenmpi-dev openmpi-bin libx11-dev libgl1-mesa-dev libglu1-mesa-dev

# Clone, build, and install Z3 (may take a while)

git clone https://github.com/Z3Prover/z3
cd z3
./configure
cd build
make -j4
sudo make install

# Install flowCore

cd
sudo R
source("http://bioconductor.org/biocLite.R")
biocLite("flowCore")

# Say NO to any further updates.
# Exit R by pressing Ctrl-D and say YES to saving workspace image.

# Clone and build pfromd

cd
git clone https://github.com/huseinz/sd_pfromd
cd sd_pfromd
make

# Test

cd
./sd_pfromd/run_pfromd.sh -j1 -n10 ./sd_pfromd/fcsFiles/028.fcs
```
