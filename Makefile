all: sudoku.c
#eventually need to compile a .cu file to use cuda or do special mpi compilation stuff
sudoku.c: 
	mpic++ -D BOARDSIZE16 -std=c++11 sudoku.cpp -Wall -g -o sudoku.out && mpirun -np 10 ./sudoku.out
