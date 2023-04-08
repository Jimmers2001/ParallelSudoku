all: sudoku.c
#eventually need to compile a .cu file to use cuda or do special mpi compilation stuff
sudoku.c: 
	mpic++ sudoku.cpp -g -o sudoku.out && mpirun -np 10 ./sudoku.out
