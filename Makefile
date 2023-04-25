all: sudoku.c

#old version: mpic++ -D BOARDSIZE9 sudoku.cpp -Wall -g -o sudoku.out && mpirun -np 10 ./sudoku.out
sudoku.c: 
	nvcc -c sudoku.cu -o sudokucuda.o;
	mpic++ -D BOARDSIZE9 -c sudoku.cpp -o sudokumpi.o;
	mpic++ sudokumpi.o sudokucuda.o -lcudart -L/apps/CUDA/cuda-5.0/lib64/ -o program;
	mpirun -np 10 ./program sample_tests/9x9_boards/tests1000easy81.txt


#nvcc -c sudoku.cu -o sudokucuda.o;
#mpic++ -D BOARDSIZE9 -c sudoku.cpp -o sudokumpi.o;
#mpic++ sudokumpi.o sudokucuda.o -lcudart -L/apps/CUDA/cuda-5.0/lib64/ -o program;
#./program 
#turn ./program into mpirun -np 10 ./program