NAME: 
pointfinder - find 2d points given their pairwise distances

installation requirements:
install z3 from https://github.com/Z3Prover/z3
Add ~/z3/build/ to your system PATH

building pointfinder:
make cleanall
make

running pointfinder:
pointfinder matrix_file_full_path min_distance_error max_distance_error error_increments point_coordinates_upper_bound [anchorpt1.x anchorpt1.y anchorpt2.x anchorpt2.y anchorpt3.x anchorpt3.y] &

Note about output: 
Its recommended that you run the pointfinder script in the background for large matrices because
it may take hours to terminate. 
Every invocation of pointfinder created an appropriately named log directly 
which contains all files generated for the constraint solver (z3), 
as well as files that contain output given by z3.
Also, all  information about the generated point relevant to an end user 
is written to 'main.log' in newly created directory.

Examples:
time the entire process:
time ./pointfinder.sh samples/dstmatrix_3pts_1.txt 0 100 10 1000

run in background:
note: the example below gives "unsat" for distortion (error) 0. 
./pointfinder.sh samples/unsat_ex_1_3x3.txt 0 900 100 1000 &
./pointfinder.sh samples/dstmatrix_4pts_1.txt 0 900 100 1000 &


run with anchor points:
./pointfinder.sh samples/dstmatrix_4pts_1.txt 0 900 100 1000  322 264 193 528 189 536 &


Output format when points found:
[comma separated list of points][comma separated list of distance distortions between points (maximum allowed distortion)]

example output:
[(2, 704), (1, 448), (2, 512), (0, 712)];[255, 192, 2, 64, 256, 192, (300)]


Output format when no points exist:
UNSAT for max. distortion = value.

example output::
UNSAT for max. distortion = 10.

note: 
If you  want to know which specific points the distortions are for, 
look at the relevant auto.del.smt2 file. 
For example the file for 3 points for some distortion value shows:

(get-value (p0x p0y p1x p1y p2x p2y))
(get-value (del_p0_p1 del_p0_p2 del_p1_p2))



Other helpful  commands:
See Makefile for targets that clean the directory

View files with size 0:
find . -type f -size 0

View z3 results in log dir:
grep "sat" *out*

View all processes z3:
ps auxww | grep z3 

Killing z3 on *nix:
ps auxww | grep z3 | awk '{print $2}' | xargs kill -9


Notes:
[.] Diagonal entries of the distance matrix are always 0.
[.] Distance matrix must be symmetric.
[.] All distance values must be integers
[.] 0<= distance values <= 1000: !!!not enforced!!!
[important] Bit vector size is still hardcoded (16 in the current version) -- it really should be dependent 
on the max size of the coordinate space and should be calculated automatically.
[.] either you provide 0 or exactly 3 anchor points.

-- end --

