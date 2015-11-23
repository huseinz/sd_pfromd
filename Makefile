.PHONY: clean cleanlogs

#Need as many slave threads as .smt2 files generated.
#There is one .smt2 file for each value of the error bound (0 <= delta <= 100, i.e. 101 files)

#num_procs == ((DISTANCE_ERROR_UPPER_BOUND - DISTANCE_ERROR_LOWER_BOUND)/DISTANCE_ERROR_INCREMENT) +2
NUMTHREADS = 9

CC = mpicc
FLAGS = -std=gnu99 -Wall -g

driver.bin: driver.o
	$(CC) -o driver.bin driver.o -lm

driver.o: driver.c
	$(CC) $(FLAGS) -c driver.c 

clean:
	-rm -f a.out *~ *.o *.bin *gen.smt2

cleanall: clean
	-rm -fr log_*

cleanlogs:
	-rm -fr log_*

run12:	driver.bin
	mpirun -np $(NUMTHREADS) driver.bin samples/s12.txt

run: driver.bin
	mpirun -np $(NUMTHREADS) driver.bin samples/s1.txt

