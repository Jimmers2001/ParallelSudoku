all: sudoku.c
sudoku.c: 
	nvcc -O3 sudoku.cu -c -o sudokucuda.o;
	mpicxx -O3 -std=c++11 -D BOARDSIZE9 sudoku.cpp -c -o sudokumpi.o;
	mpicxx -O3 sudokumpi.o sudokucuda.o -o program -L/usr/local/cuda-11.2/lib64/ -lcudadevrt -lcudart -lstdc++;
	mpirun -np 10 ./program $(FILE_NAME) $(NUM_TESTS)
clean: 
	rm *.o