all: sudoku.c
#eventually need to compile a .cu file to use cuda or do special mpi compilation stuff
sudoku.c: 
	g++ sudoku.cpp -g -o sudoku.out && ./sudoku.out
