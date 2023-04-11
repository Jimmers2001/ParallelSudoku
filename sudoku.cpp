#include <iostream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <string>
#include <mpi.h>
#include <mutex>
#include "tile.h"
using namespace std;

//GLOBAL DEFINES
int boardsize = 9;
int EMPTY = 0;
std::mutex global_mutex;

//define some board examples
#if 1
    string input1 = "\
        1234\
        2341\
        3412\
        4123\
        ";

    string empty_board = "\
        000000000\
        000000000\
        000000000\
        000000000\
        000000000\
        000000000\
        000000000\
        000000000\
        000000000\
        ";

    string complete_board = "\
        123456789\
        789123456\
        456789123\
        312845967\
        697312845\
        845697312\
        231574698\
        968231574\
        574968231\
        ";

    string evil_board = "\
        020040009\
        080000700\
        704600001\
        309080100\
        000005006\
        070000000\
        002000000\
        401090300\
        000800090\
        ";
#endif

/// @brief A square sudoku board defined by a boardsize global variable initialized to 0. It 
/// @brief requires that all the numbers from 1-boardsize inclusive exist in each row and column without repeats
class SudokuBoard{
    private:
        vector<vector<Tile>> tiles;
        int width = boardsize; 
        int height = boardsize;

    public:
        SudokuBoard(string input){
            if (input.size() < 81){
                fprintf(stderr, "Too small of a board input string\n");
                throw;
            }
            else{
                //initialize boardsize x boardsize board to all 0s
                for (int i = 0; i < boardsize; i++){
                    vector<Tile> row;
                    tiles.push_back(row);
                    for (int j = 0; j < boardsize; j++){
                        tiles[i].push_back(Tile(EMPTY));
                    }
                }

                //add existing numbers from input after removing whitespace
                input.erase(std::remove_if(input.begin(), input.end(), ::isspace),input.end());
                if (input.size() != (unsigned int) boardsize * boardsize){
                    if (input.size() > (unsigned int) boardsize * boardsize){
                        fprintf(stderr, "Input is too big to create a square grid\n\tbased on boardsize: %d\n\n", boardsize);
                        throw;
                    }
                    else{
                        fprintf(stderr, "Input is too small to create a square grid\n\tbased on boardsize: %d\n\n", boardsize);
                        throw;
                    }
                    
                    printf("Input: %s\n", input.c_str());
                }
                else{
                    for (int i = 0; i < boardsize; i++){
                        for (int j = 0; j < boardsize; j++){
                            tiles[i][j].setVal((int)input[(j*boardsize) + i]-48); //convert char ASCII to integer
                        }
                    }
                }
            }
        }
#if 1
        //kinda useless if i always just use tiles[x][y]
        /// @brief Attempts to retrieve a value of a particular tile in the grid
        /// @param x the horizontal x position 
        /// @param y the vertical y position 
        /// @return the value
        int getValue(int x, int y){
            if (x > width || x < 0 || y > height || y < 0){
                fprintf(stderr, "Cannot get value at invalid position: %d, %d\n", x, y);
                return 1;
            }
            return tiles[x][y].getVal();;
        }

        /// @brief Attempts to assign a value to a particular tile in the grid
        /// @param x the horizontal x position 
        /// @param y the vertical y position 
        /// @param val the value being used for assignment
        /// @return 0 on success, positive number on failure
        int setValue(int x, int y, int val){
            if (x > width || x < 0 || y > height || y < 0){
                fprintf(stderr, "Cannot set value %d at invalid position: %d, %d\n", x, y, val);
                return 1;
            }
            tiles[x][y].setVal(val);
            return 0;
        }

        void printBoard(){
            for (int i = 0; i < boardsize; i++){
                if( i%3 == 0 ){ /////////////////////////loop through boardsize instead of 9
                     printf("+====+====+====+====+====+====+====+====+====+\n");
                }
                else { /////////////////////////loop through boardsize instead of 9
                    printf("+----+----+----+----+----+----+----+----+----+\n");
                }

                for (int j = 0; j < boardsize; j++){
                    if( j%3 == 0 ) {
                        if(tiles[i][j].getVal() == 0){
                            printf("║    ");
                        } else {  printf("║ %02d ", tiles[i][j].getVal()); }
                    } 
                    else { 
                       if(tiles[i][j].getVal() == 0){
                            printf("|    ");
                        } else {  printf("| %02d ", tiles[i][j].getVal()); }
                    }

                 
                }
                printf("║\n");
            }
            
            /////////////////////////loop through boardsize instead of 9
            printf("+====+====+====+====+====+====+====+====+====+\n");
        }
        
        /// @brief Checks the same column for duplicate value of current position
        /// @param curx current x
        /// @param cury current y
        /// @returns true if there is a duplicate, false otherwise
        bool isInCol(int curx, int cury){
            for (int a = 0; a < boardsize; a++){
                if (a == cury){continue;}
                if (tiles[cury][curx].getVal() == tiles[a][curx].getVal()){
                    fprintf(stderr, "Duplicate value in column\n");
                    return true;
                }
            }
            return false;
        }

        /// @brief Checks the same row for duplicate value of current position
        /// @param curx current x
        /// @param cury current y
        /// @returns true if there is a duplicate, false otherwise
        bool isInRow(int curx, int cury){
            for (int a = 0; a < boardsize; a++){
                if (a == curx){continue;}
                if (tiles[cury][curx].getVal() == tiles[cury][a].getVal()){
                    fprintf(stderr, "Duplicate value between (%d, %d) and (%d, %d)\n", cury, curx, cury, a);
                    return true;
                }
            }
            return false;
        }

        ///THIS IS HARDCODED TO 9X9 FOR NOW///////////////////////////////////////////
        /// @brief Checks the same block for duplicate value of current position
        /// @param curx current x
        /// @param cury current y
        /// @returns true if there is a duplicate, false otherwise
        bool isInBlock(int curx, int cury){
            //find which block the current position is in
            int xstart = (curx / 3) * 3; //round down and then scale up
            int ystart = (cury / 3) * 3;
            int xend = xstart+3;
            int yend = ystart+3;

            for (int i = xstart; i < xend; i++){
                for (int j = ystart; j < yend; j++){
                    if (cury == j && curx == i){continue;}
                    
                    if (tiles[cury][curx].getVal() == tiles[j][i].getVal()){
                        return true;
                    }
                }
            }
            
            return false;
        }

        
        /// @brief Tests if the board is valid aka 
        /// @brief every number from 1-boardsize exists once in each row and column
        /// @return 0 on success and positive number on failure
        int checkBoard(){
            for (int i = 0; i < boardsize; i++){
                for (int j = 0; j < boardsize; j++){
                    int val = tiles[j][i].getVal(); //current tile being looked at
                    
                    //confirm board is complete
                    if (val == EMPTY){
                        fprintf(stderr, "There exists a 0 on the board (incomplete)\n");
                        return 1;
                    }

                    //confirm each element is valid
                    if (val < 0 || val > boardsize){
                        fprintf(stderr, "There exists an invalid number %d at position (%d, %d)\n", val, j, i);
                        return 1;
                    }

                    //check row, column, and block for duplicate values
                    if (isInRow(i,j) || isInCol(i,j) || isInBlock(i,j)){
                        return 1;
                    }
                }
            }
            return 0;
        }

        /// @brief Determines if the board is empty
        /// @param row the x coordinate of an empty tile
        /// @param col the y coordinate of an empty tile
        /// @return returns true if the board has an empty tile, false otherwise
        bool findEmptyTile(int& row, int& col){
            for (row = 0; row < boardsize; row++){
                for (col = 0; col < boardsize; col++){
                    if (tiles[row][col].getVal() == EMPTY){
                        return true;
                    }
                }
            }
            return false;
        }

        /// @brief Assigns a particular rank its block to solve in parallel
        /// @param myrank the rank itself
        /// @param startx starting x value
        /// @param starty starting y value
        /// @param endx INCLUSIVE ending x
        /// @param endy INCLUSIVE ending y
        void setBlockCoordinates(int myrank, int& startx, int& starty, int& endx, int& endy){
            //set coordinates for a 9x9 board
            if (boardsize == 9){
                if( myrank == 1 ){ startx = 0; endx = 2; starty = 0; endy = 2; return; }
                if( myrank == 2 ){ startx = 3; endx = 5; starty = 0; endy = 2; return; }
                if( myrank == 3 ){ startx = 6; endx = 8; starty = 0; endy = 2; return; }
                if( myrank == 4 ){ startx = 0; endx = 2; starty = 3; endy = 5; return; }
                if( myrank == 5 ){ startx = 3; endx = 5; starty = 3; endy = 5; return; }
                if( myrank == 6 ){ startx = 6; endx = 8; starty = 3; endy = 5; return; }
                if( myrank == 7 ){ startx = 0; endx = 2; starty = 6; endy = 8; return; }
                if( myrank == 8 ){ startx = 3; endx = 5; starty = 6; endy = 8; return; }
                if( myrank == 9 ){ startx = 6; endx = 8; starty = 6; endy = 8; return; }
            }
            else if (boardsize == 16){
                printf("setblock coordinates function bad\n");
                throw;
            }
        }

#endif
        /// @brief Getter function for board access
        /// @return a copy of the board
        vector<vector<Tile>> getBoard(){
            vector<vector<Tile>> board_copy = tiles;////////////////////////check if this is deep or shallow copy
            return board_copy;
        }

        /// @brief called by solveBoardParallel (the driver), this function recurses and solves in parallel
        /// @return 0 on success, positive number on error
        int recursiveParallel(int currRank){
            int startx, starty, endx, endy;
            setBlockCoordinates(currRank, startx, starty, endx, endy);

            printf("rank: %d, (%d, %d, %d, %d)\n", currRank, startx, starty, endx, endy );
            //test board
            int row = 0;
            int col = 0;
            if (!findEmptyTile(row, col)){return 0;}
            
            //try to fill the empty element
            /*for (int i = 1; i <= boardsize; i++){
                // the tile can support the value in (row, col)
                if (canSupportinRow(row, i) && canSupportinCol(col, i) && canSupportinBlock(row, col, i)){
                    //test out that number and continue
                    tiles[row][col].setVal(i);
                    if (solveBoardSequential() == 0){
                        //passed so end
                        return 0;
                    }
                    
                    //undo and try a different number
                    tiles[row][col].setVal(EMPTY);
                }
            }*/
            
            //could not solve the board for some reason
            return 1;
        }

        /// @brief Parallel attempt to fill in every empty grid in the board
        /// @return 0 on success, 1 on error, tiles will be filled correctly on success
        int solveBoardParallelDriver(int argc, char** argv){
            //initialize mpi
            int myrank;
            int number_of_ranks;
            MPI_Comm_rank(MPI_COMM_WORLD, &myrank); //this processes' individual rank
            MPI_Comm_size(MPI_COMM_WORLD, &number_of_ranks); //the total number of processes in the world
            
            printf("I am rank %d in driver)\n", myrank);
            MPI_Barrier(MPI_COMM_WORLD);
            //go through and fill every element of the board (write solution to file) IN PARALLEL
            
            if( myrank != 0 ){
                recursiveParallel(myrank);
            } else { 
                while( false /*fix later*/ ){
                    //wait until all threads do magic
                    //do things depending on magic
                    
                }
             }
                 
            //test board and terminate
            //MPI_Finalize();
            return checkBoard();
        }

        /// @brief  Checks if the given val can put placed anywhere in the row
        /// @param row the row being checked
        /// @param val the value being checked
        /// @return false if the value already exists, so it cannot be supported. otherwise true
        bool canSupportinRow(int row, int val){
            for (int i = 0; i < boardsize; i++)
                if (tiles[row][i].getVal() == val)
                    return false;
            return true;
        }

        /// @brief  Checks if the given val can put placed anywhere in the column
        /// @param col the column being checked
        /// @param val the value being checked
        /// @return false if the value already exists, so it cannot be supported. otherwise true
        bool canSupportinCol(int col, int val){
            for (int i = 0; i < boardsize; i++)
                if (tiles[i][col].getVal() == val)
                    //printf("found match of %d at row: %d, col: %d\n", val, i, col);
                    return false;
            return true;
        }

        ///THIS IS HARDCODED TO 9X9 FOR NOW///////////////////////////////////////////
        /// @brief Checks the same block for existence of val
        /// @param col current x
        /// @param row current y
        /// @returns true if can support because of no duplicate, false otherwise
        bool canSupportinBlock(int row, int col, int val){
            //find which block to check in
            int xstart = (col / 3) * 3; //round down and then scale up
            int ystart = (row / 3) * 3;
            int xend = xstart+3;
            int yend = ystart+3;

            for (int i = xstart; i < xend; i++){
                for (int j = ystart; j < yend; j++){                    
                    if (val == tiles[j][i].getVal()){
                        return false;
                    }
                }
            }
            
            return true;
        }

        /// @brief Sequential attempt to fill in every empty grid in the board
        /// @return 0 on success, 1 on error, tiles will be filled correctly on success
        int solveBoardSequential(){
            //test board
            int row = 0;
            int col = 0;
            if (!findEmptyTile(row, col)){return 0;}
            
            //try to fill the empty element
            for (int i = 1; i <= boardsize; i++){
                // the tile can support the value in (row, col)
                if (canSupportinRow(row, i) && canSupportinCol(col, i) && canSupportinBlock(row, col, i)){
                    //test out that number and continue
                    tiles[row][col].setVal(i);
                    if (solveBoardSequential() == 0){
                        //passed so end
                        return 0;
                    }
                    
                    //undo and try a different number
                    tiles[row][col].setVal(EMPTY);
                }
            }
            
            //could not solve the board for some reason
            return 1;
        }

    /// @brief Randomly fills up a board
    /// @return returns 0 on success, 1 on failure to fill board
    int randomBoardFill(){
        //test board
        int row = 0;
        int col = 0;
        if (!findEmptyTile(row, col)){
            return 0;
        }
        
        //vector begins full and after random attempts removes elements until empty
        vector<int> nums = {1,2,3,4,5,6,7,8,9};
        int n;

        while (nums.size() > 0){
            n = (rand() % (nums.size())); //[0,9] exclusive index of nums array

            // the tile can support the value in (row, col)
            if (canSupportinRow(row, nums[n]) && canSupportinCol(col, nums[n]) && canSupportinBlock(row, col, nums[n])){
                //test out that number and continue
                tiles[row][col].setVal(nums[n]);
                if (randomBoardFill() == 0){
                    //filled board successfully
                    return 0;
                }

                //undo and try a different number
                tiles[row][col].setVal(EMPTY);
            }

            //already guessed n so dont randomly guess again
            nums.erase(nums.begin() + n);
        }
        
        //could not fill the board for some reason
        return 1;
    }
};

/// @brief Randomly generates a sudoku board
/// @param difficulty string that determines the range of the number of givens to randomly start the board with
/// @return Uses boardsize to make a sudoku board to solve
SudokuBoard* generateSudokuBoard(string difficulty){
    //Seed the random number generator with the current time
    srand(time(0));
    //chose the number of givens to include in the generated board
    unsigned int number_of_givens;
    if (difficulty == "evil"){
        number_of_givens = 3 + (rand() % static_cast<int>(6 - 3 + 1)); //[3,6] exclusive range
    } else if (difficulty == "expert"){
        number_of_givens = 7 + (rand() % static_cast<int>(14 - 7 + 1)); //[7,14] exclusive range
    } else { 
        number_of_givens = 40 + (rand() % static_cast<int>(70 - 40 + 1)); //[40,70] exclusive range
    }
     
    printf("# of givens: %d\n", number_of_givens);

    //initialize a board to empty and add in random given numbers
    SudokuBoard* random_board = new SudokuBoard(empty_board);
    random_board->randomBoardFill(); //complete the board randomly and then remove elements

    //current coordinate position
    int x;
    int y;

    //remove elements in the board until only number_of_givens are left
    unsigned int removals = 0;
    while (removals < (boardsize*boardsize)-number_of_givens){
        x = rand() % (boardsize);
        y = rand() % (boardsize);

        //if we havent assigned a value to that spot yet
        if (random_board->getValue(x,y) != 0){
            random_board->setValue(x,y,0);
            removals++;
        }
    }
    return random_board; //returns the board* for dereferencing in the future
}

/// @brief Runs all solvers potentially many times
/// @param number_of_tests The number of iterations to do 
void runTests(int number_of_tests){
    for (int i = 0; i < number_of_tests; i++){
        SudokuBoard* b_seq = generateSudokuBoard("evil"); //random board
        //SudokuBoard* b_par = new SudokuBoard(b_seq); //copy to compare runtime between sequential and parallel
        printf("*******************************\nINITIAL SEQUENTIAL BOARD STATE\n*******************************\n\n");
        b_seq->printBoard();  
    
        if (b_seq->solveBoardSequential() == 0){
            printf("\n*******************************\nBOARD IS SOLVED\n*******************************\n\n");
        }
        else{
            printf("board is incomplete or incorrect\n");
        }

        b_seq->printBoard();  
        printf("\n*******************************\nEND OF SEQUENTIAL SOLVER\n*******************************\n\n");
        
        /***********************************************************************************/

        //solveBoardParallelDriver

        
        delete b_seq;
        //delete b_par;
    }
    
    return;
}

//POTENTIALLY ADD THE 3X3 OR BOARDSIZE%3 X BOARDSIZE%3 BLOCKS AS WELL
int main(int argc, char** argv){
    int myrank = 0;
    int number_of_ranks = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank); //this processes' individual rank
    MPI_Comm_size(MPI_COMM_WORLD, &number_of_ranks); //the total number of processes in the world

    if( myrank == 0 ){
        runTests(1);///////////////////////////////////////can all mpi ranks do whats inside of this even though its within the myrank==0
    } 
    
    /*---------------------------------------------------*/
    /*              YOU THREADS SHALL NOT PASS           */
    /*---------------------------------------------------*/
    MPI_Barrier(MPI_COMM_WORLD);
    /*---------------------------------------------------*/
    
    // Each process modifies a subset of the board array
    /*int i, j;
    for (i = myrank; i < boardsize; i += boardsize) {
        for (j = 0; j < boardsize; j++) {
           global_board_array[i][j] += myrank;
        }
    }*/

    /*
     if ( myrank == 0) {
        int i, j;
        printf("Board array after all processes have modified:\n");
        for (i = 0; i < boardsize; i++) {
            for (j = 0; j < boardsize; j++) {
                printf("%d ", global_board_array[i][j]);
            }
            printf("\n");
        }
    }*/

    /*if( myrank == 0 ){
        printf("\n*******************************\nSTART OF PARALLEL SOLVER\n*******************************\n\n");
        //Driver Code Here
        //SudokuBoard* pBoard = new SudokuBoard(complete_board); 
        //global_board->printBoard(); 

    } else { 
        // Recursive Parallel
    }*/
    //printf("My rank: %d and global_board_array[0] is: %d\n", myrank, global_board_array[0][0]);///////
    
      

    //Parrallel Killer Things

    /* --------------------- MULT RANKS --------------------- */
    // if( myrank != 0 ){
    //My board = constructor(global_board_array)
        //recursiveParallel(myrank);
    /*} else { 

        while( false  ){
            //wait until all threads do magic
            //do things depending on magic
            
        }
    }

    //printf("my rank is: %d\n\n", myrank);

    SudokuBoard* b4 = new SudokuBoard(evil_board);
    printf("***********************\nINITIAL BOARD STATE\n***********************\n");
    b4->printBoard();
    if (b4->solveBoardParallelDriver(argc, argv) == 0){
        //printf("\n***********************\nBOARD IS SOLVED\n***********************\n");
    }

    //b4->printBoard();    
    delete b4;

*/
    MPI_Finalize(); //mpi causes false positives for memory leaks. 
    //DRMEMORY expects 427,000 bytes reachable and valgrind expects 71,000 bytes. Dont worry :)
    return 0;
}
