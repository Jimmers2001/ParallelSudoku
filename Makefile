all: sudoku.c

#old version: mpic++ -D BOARDSIZE9 sudoku.cpp -Wall -g -o sudoku.out && mpirun -np 10 ./sudoku.out
#module load xl_r spectrum-mpi cuda/11.2
sudoku.c: 
	nvcc -O3 sudoku.cu -c -o sudokucuda.o;
	mpicxx -O3 -std=c++11 -D BOARDSIZE9 sudoku.cpp -c -o sudokumpi.o;
	mpicxx -O3 sudokumpi.o sudokucuda.o -o program -L/usr/local/cuda-11.2/lib64/ -lcudadevrt -lcudart -lstdc++;
	mpirun -np 10 ./program $(FILE_NAME) $(NUM_TESTS)

#	nvcc -O3 -gencode arch=compute_70,code=sm_70 -c sudoku.cu -o sudokucuda.o;
#	mpixlc -O3 -std=c++11 -D BOARDSIZE9 -c sudoku.cpp -o sudokumpi.o;
#	mpixlc -O3 -L/usr/local/cuda-11.2/lib64/ -lcudadevrt -lcudart -L/usr/lib64/libstdc++ -lstdc++ \
#		sudokumpi.o sudokucuda.o -o program;
#	mpirun -np 10 ./program $(FILE_NAME) $(NUM_TESTS)
clean: 
	rm *.o

#nvcc -c sudoku.cu -o sudokucuda.o;
#mpic++ -D BOARDSIZE9 -c sudoku.cpp -o sudokumpi.o;
#mpic++ sudokumpi.o sudokucuda.o -lcudart -L/apps/CUDA/cuda-5.0/lib64/ -o program;
#./program 
