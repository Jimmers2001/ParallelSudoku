all: sudoku.c

sudoku.c: 
	g++ sudoku.cpp -g -o sudoku.out && ./sudoku.out
