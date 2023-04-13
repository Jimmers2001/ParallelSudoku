#include <iostream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <string>
#include <mpi.h>
#include <mutex>
#include <numeric>
#include <cmath>
#include "tile.h"
using namespace std;

//GLOBAL DEFINES
//numbers 1-9 and then continuing with letters a-z for 10-36
//boardsize defined in tile.h

int EMPTY = 0;
std::mutex global_mutex;

//define some board examples
string test_board = "\
    0fb04e19a050cd00\
    0d0600b290g000a0\
    e90180000b0c005f\
    a08507002400bg60\
    000000fb00a7090e\
    000000000d000084\
    065b70000g081023\
    0ag0068ce00b07d0\
    000g00d8000a2f0c\
    503e0a9000bgd806\
    00df0231009e0000\
    00406f0012d000b7\
    3000e00a09000002\
    10690000g0700c0d\
    0000g00006803000\
    00041b0000f3e00g\
    ";

#ifdef BOARDSIZE9
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
#endif
#ifdef BOARDSIZE16
string empty_board = "\
    0000000000000000\
    0000000000000000\
    0000000000000000\
    0000000000000000\
    0000000000000000\
    0000000000000000\
    0000000000000000\
    0000000000000000\
    0000000000000000\
    0000000000000000\
    0000000000000000\
    0000000000000000\
    0000000000000000\
    0000000000000000\
    0000000000000000\
    0000000000000000\
    ";
#endif

/// @brief A square sudoku board defined by a boardsize global variable initialized to 0. It 
/// @brief requires that all the numbers from 1-boardsize inclusive exist in each row and column without repeats
class SudokuBoard{
    private:
        vector<vector<Tile>> tiles;
        string boardtostring;
        int width = boardsize; 
        int height = boardsize;

    public:
        SudokuBoard(string input){
            input.erase(std::remove_if(input.begin(), input.end(), ::isspace),input.end());
            boardtostring = input;
            if (input.size() < (unsigned int) boardsize*boardsize){
                fprintf(stderr, "Too small of a board input string. size: %u, boardsize: %d\n", (unsigned int) input.size(), boardsize);
                //throw;
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
                        fprintf(stderr, "Input (%u) is too big to create a square grid\n\tbased on boardsize: %d\n\n", (unsigned int) input.size(), boardsize);
                        throw;
                    }
                    else{
                        fprintf(stderr, "Input is too small to create a square grid\n\tbased on boardsize: %d\n\n", boardsize);
                        throw;
                    }
                    
                }
                else{
                    for (int i = 0; i < boardsize; i++){
                        for (int j = 0; j < boardsize; j++){
                            char item = input[(j*boardsize) + i];
                            int num;
                            //it is a number
                            if (isdigit(item)){
                                num = (int)input[(j*boardsize) + i]-48; //convert char ASCII to integer
                            } else{ //it is a lowercase letter (no support for uppercase)
                                num = 9+((int)item)-48-48; //letter a = 10, b = 11... etc
                            }
                            tiles[i][j].setVal(num);
                            //printf("tile[%d][%d] possible values: ", i, j);
                            //printSet(*(tiles[i][j].getPosValues()));

                            //remove possible values from tiles affected by this number
                            if (num != 0){ //we always read from empty board
                                removePosValue(num, i, j);
                                printf("never happens\n");
                            }                            
                        }
                    }
                }
            }
        }
#if 1
        vector<vector<Tile>>& getTiles(){
            return tiles;
        }
        
        /// @brief Gets the string representation of the board
        /// @return boardtostring
        string boardToString(){
            return boardtostring;
        }
        
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
            if (boardsize == 9){
                for (int i = 0; i < boardsize; i++){
                    if( i%3 == 0 ){ 
                        printf("+====+====+====+====+====+====+====+====+====+\n");
                    }
                    else { 
                        printf("+----+----+----+----+----+----+----+----+----+\n");
                    }

                    for (int j = 0; j < boardsize; j++){
                        if( j%3 == 0 ) {
                            if(tiles[j][i].getVal() == 0){
                                printf("║    ");
                            } else {  printf("║ %02d ", tiles[j][i].getVal()); }
                        } 
                        else { 
                        if(tiles[j][i].getVal() == 0){
                                printf("|    ");
                            } else {  printf("| %02d ", tiles[j][i].getVal()); }
                        }
                    }
                    printf("║\n");
                } 
                printf("+====+====+====+====+====+====+====+====+====+\n");
            }
            if (boardsize == 16) {
                for (int i = 0; i < boardsize; i++) {
                    if (i % 4 == 0) {
                        printf("+======+======+======+======+======+======+======+======+======+======+======+======+======+======+======+======+\n");
                    } else {
                        printf("+------+------+------+------+------+------+------+------+------+------+------+------+------+------+------+------+\n");
                    }
                    for (int j = 0; j < boardsize; j++) {
                        if (j % 4 == 0) {
                            if (tiles[j][i].getVal() == 0) {
                                printf("║      ");
                            } else {
                                printf("║ %02d   ", tiles[j][i].getVal());
                            }
                        } else {
                            if (tiles[j][i].getVal() == 0) {
                                printf("|      ");
                            } else {
                                printf("| %02d   ", tiles[j][i].getVal());
                            }
                        }
                    }
                    printf("║\n");
                }
                printf("+======+======+======+======+======+======+======+======+======+======+======+======+======+======+======+======+\n");
            }
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

        /// @brief Checks the same block for duplicate value of current position
        /// @param curx current x
        /// @param cury current y
        /// @returns true if there is a duplicate, false otherwise
        bool isInBlock(int curx, int cury){
            if (boardsize == 9){
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
            }
            else if (boardsize == 16){
                //find which block the current position is in
                int xstart = (curx / 4) * 4; //round down and then scale up
                int ystart = (cury / 4) * 4;
                int xend = xstart+4;
                int yend = ystart+4;

                for (int i = xstart; i < xend; i++){
                    for (int j = ystart; j < yend; j++){
                        if (cury == j && curx == i){continue;}
                        
                        if (tiles[cury][curx].getVal() == tiles[j][i].getVal()){
                            return true;
                        }
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
                    if (SequentialRecursiveBacktrackSolve() == 0){
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

        /// @brief Checks if the given val can put placed anywhere in the row
        /// @param row the row being checked
        /// @param val the value being checked
        /// @return false if the value already exists, so it cannot be supported. otherwise true
        bool canSupportinRow(int row, int val){
            for (int i = 0; i < boardsize; i++)
                if (tiles[row][i].getVal() == val)
                    return false;
            return true;
        }

        /// @brief Checks if the given val can put placed anywhere in the column
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
            //3x3 blocks
            if (boardsize == 9){
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
            }
            //4x4 blocks
            else if (boardsize == 16){
                //find which block to check in
                int xstart = (col / 4) * 4; //round down and then scale up
                int ystart = (row / 4) * 4;
                int xend = xstart+4;
                int yend = ystart+4;

                for (int i = xstart; i < xend; i++){
                    for (int j = ystart; j < yend; j++){ 
                        if (val == tiles[j][i].getVal()){
                            return false;
                        }
                    }
                }
            }
            return true;
        }
#endif
        void addPosValue(int val, int originalx, int originaly){
            //go through the row, col, and block containing coordinate (x,y) and remove val from all tiles' pos_values
            
            //row
            for (int x = 0; x < boardsize; x++){
                tiles[x][originaly].addPosVal(val);
            }

            //col
            for (int y = 0; y < boardsize; y++){
                tiles[originalx][y].addPosVal(val);
            }

            //block
            //find which block the current position is in
            int sq = sqrt(boardsize);
            int xstart = (originalx / sq) * sq; //round down and then scale up
            int ystart = (originaly / sq) * sq;
            int xend = xstart+sq;
            int yend = ystart+sq;
            for (int x = xstart; x < xend; x++){
                for (int y = ystart; y < yend; y++){
                    tiles[x][y].addPosVal(val);
                }
            }
        }

        void removePosValue(int val, int originalx, int originaly){
            //go through the row, col, and block containing coordinate (x,y) and remove val from all tiles' pos_values
            
            //row
            for (int x = 0; x < boardsize; x++){
                tiles[x][originaly].removePosVal(val);
            }

            //col
            for (int y = 0; y < boardsize; y++){
                tiles[originalx][y].removePosVal(val);
            }

            //block
            //find which block the current position is in
            int sq = sqrt(boardsize);
            int xstart = (originalx / sq) * sq; //round down and then scale up
            int ystart = (originaly / sq) * sq;
            int xend = xstart+sq;
            int yend = ystart+sq;
            for (int x = xstart; x < xend; x++){
                for (int y = ystart; y < yend; y++){
                    tiles[x][y].removePosVal(val);
                }
            }
        }
        
        /// @brief Attempts the humanistic "elimination" rule as many times as possible
        /// @param xstart the x position of the top left of the block
        /// @param ystart the y position of the top left of the block
        /// @param xend the x position of the bottom right of the block
        /// @param yend the y position of the bottom right of the block
        /// @return true if any changes were made, false otherwise
        bool eliminationRule(int xstart, int ystart, int xend, int yend){///////////////this is the only rule that can iterate and go again and again since its the first one we attempt
            //look through all of this blocks' tiles' possible_values lists. If any are of size 1, 
            //then that must be the answer, so make the change
                //what if we guess and then we use a rule, are we guaranteed a solution?
            //have functionality for each block running this
            int numChanges = 0;
            for (int x = xstart; x < xend; x++){
                for (int y = ystart; y < yend; y++){
                    if (tiles[x][y].getVal() == 0){
                        set<int>* pos_values = tiles[x][y].getPosValues();
                        if (pos_values->size() == 1){
                            //////////////////MUST START MUTEX OR SEND MESSAGE OR SOMETHING HERE FOR PARALLEL
                            tiles[x][y].setVal(*(pos_values->begin()));//set to the remaining element
                            numChanges++;
                            //update pos_moves of all tiles affected by this change
                            removePosValue(*(pos_values->begin()), x, y);

                            //RESTART THE LOOP SINCE A CHANGE WAS MADE
                            x = xstart;
                            y = ystart;
                        }
                        else{
                            printf("possible values at (%d, %d): ", x, y);
                            printSet(*tiles[x][y].getPosValues());
                        }
                    } 

                }
            }
            printf("Made %d changes\n", numChanges);
            return (numChanges>0);
        }
        
        /// @brief Attempts first to use humanistic rules to solve, and brute forces as a last resort
        /// @return 0 on success, 1 on inability to solve
        int SequentialHumanisticSolve(){
            //bool madeChange = false;
            //since sequential, the entire board is the block
            int xstart = 0;
            int ystart = 0;
            int xend = boardsize;
            int yend = boardsize;

            //elimination
            int changes;
            changes = eliminationRule(xstart, ystart, xend, yend);
            printf("Made %d elim changes\n", changes);

            return checkBoard();
        }

        /// @brief Sequential attempt to fill in every empty grid in the board and backtrack on failure
        /// @return 0 on success, 1 on inability to solve, tiles will be filled correctly on success
        int SequentialRecursiveBacktrackSolve(){
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

                    if (SequentialRecursiveBacktrackSolve() == 0){
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
    /// @return 0 on success, 1 on inability to solve
    int randomBoardSolve(){
        //test board
        int row = 0;
        int col = 0;
        if (!findEmptyTile(row, col)){
            return 0;
        }
        
        //vector begins full and after random attempts removes elements until empty
        vector<int> nums(boardsize);
        iota(nums.begin(), nums.end(), 1);//vector where ith element is i+1 (1,2,3...boardsize)
        int n;

        while (nums.size() > 0){
            n = (rand() % (nums.size())); //[0,boardsize] exclusive index of nums array

            // the tile can support the value in (row, col)
            if (canSupportinRow(row, nums[n]) && canSupportinCol(col, nums[n]) && canSupportinBlock(row, col, nums[n])){
                //test out that number and continue
                tiles[row][col].setVal(nums[n]);
                removePosValue(nums[n], row, col);

                if (randomBoardSolve() == 0){
                    //filled board successfully
                    return 0;
                }
                
                //undo and try a different number
                addPosValue(nums[n], row, col);
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
    int total_squares = boardsize*boardsize;
    if (difficulty == "evil"){
        number_of_givens = 3 + (rand() % static_cast<int>(6 - 3 + 1)); //~[3,6] exclusive range
    } else if (difficulty == "easy"){
        number_of_givens = (total_squares-20) + (rand() % static_cast<int>((total_squares-10) - (total_squares-20) + 1)); //~[70,90] exclusive range
    } else if (difficulty == "trivial"){
        number_of_givens = (total_squares-10) + (rand() % static_cast<int>((total_squares-5) - (total_squares-10) + 1)); //~[70,90] exclusive range
    } else { 
        number_of_givens = (total_squares/4) + (rand() % static_cast<int>((total_squares/2) - (total_squares/4) + 1)); //~[20,30] exclusive range
    }
     
    printf("# of givens: %d\n", number_of_givens);

    //initialize a board to empty and add in random given numbers
    SudokuBoard* random_board = new SudokuBoard(empty_board);
    random_board->randomBoardSolve(); //complete the board randomly and then remove elements

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
            //must add possible values back because number is getting 0'd out
            random_board->addPosValue(random_board->getValue(x,y), x, y);

            random_board->setValue(x,y,0);
            removals++;
        }
    }
    return random_board; //returns the board* for dereferencing in the future
}

/// @brief Runs all solvers potentially many times
/// @param number_of_tests The number of iterations to do 
void runTestsSequential(int number_of_tests){
    for (int i = 0; i < number_of_tests; i++){
        SudokuBoard* b_seq = generateSudokuBoard("trivial"); //random board
        //SudokuBoard* b_par = new SudokuBoard(b_seq); //copy to compare runtime between sequential and parallel
        printf("*******************************\nINITIAL SEQUENTIAL BOARD STATE\n*******************************\n\n");
        b_seq->printBoard();
    
        /*printf("pos values of tiles[0][0]: ");
        Tile firstTile = (b_seq->getTiles())[0][0];
        printSet(*(firstTile.getPosValues()));
        */
        
        if (b_seq->SequentialHumanisticSolve() == 0){
            printf("\n*******************************\nBOARD IS SOLVED\n*******************************\n\n");
        } else {
            printf("board is incomplete or incorrect\n");
        }

        /*printf("pos values of tiles[0][0]: ");
        printSet(*(b_seq->getTiles()[0][0].getPosValues()));
        */
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

    //int sudoku_size = boardsize*boardsize;

    //character that represents the "{board} {#tile to solve}" which is space separated and has null terminating character
    //char board[sudoku_size] = "Hello";

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank); //this processes' individual rank
    MPI_Comm_size(MPI_COMM_WORLD, &number_of_ranks); //the total number of processes in the world

    if( myrank == 0 ){
        runTestsSequential(1); 
        
        /*Running 16x16 random board is super time consuming because of the time it takes to produce an evil board. 
        Sometimes it takes forever and sometimes its instant, depending on the random numbers chosen i think.
        I should probably exclude the board creation times when gathering real data.

        Also, sequential filling of an empty board is much faster than random because it narrows down places numbers can go or something?

        */

        /*SudokuBoard* test = new SudokuBoard(empty_board);

        printf("*******************************\nINITIAL SEQUENTIAL BOARD STATE\n*******************************\n\n");
        test->printBoard(); 
        
        if (test->SequentialRecursiveBacktrackSolve() == 0){
            printf("\n*******************************\nBOARD IS SOLVED\n*******************************\n\n");
        } else {
            printf("board is incomplete or incorrect\n");
        }

        test->printBoard();  
        printf("\n*******************************\nEND OF SEQUENTIAL SOLVER\n*******************************\n\n");
       */
    } 
    
    /*---------------------------------------------------*/
    /*              YOU THREADS SHALL NOT PASS           */
    /*---------------------------------------------------*/
    MPI_Barrier(MPI_COMM_WORLD);
    /*---------------------------------------------------*/
#if 0
    if( myrank == 0 ){
        vector<bool> sendToArray(9, false);/*IS A 9*/
        //vector<char> sendToValues(9, "1");/*IS A 9*/
        bool sudokuNotComplete = true;
        while( sudokuNotComplete ){
            // STEP 1: generate spots to check {boardsize} at most 1 at least
            // STEP 2: flag vector with processes that match the block
            

            for (unsigned int i = 0; i < sendToArray.size(); i++){
                bool shouldSend = sendToArray[i];
                //char tile = sendToValues[i];

                //  strcat(board, );

                if( shouldSend ){  MPI_Send(board, sudoku_size + 3, MPI_CHAR, i + 1, 0, MPI_COMM_WORLD); }
            }
            
            if( true /* IS BOARD FILLED ( Complete Board Function ) */ ){ sudokuNotComplete = false; }
        }
    } else {
        bool keepLooping = true;
        while ( keepLooping ){
            MPI_Status status;
            MPI_Recv(board, sudoku_size, MPI_CHAR , MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
            printf("Rank %d received a message from rank %d that is: %s\n", myrank, status.MPI_SOURCE, board);

            if( true /* IS BOARD FILLED  ( Complete Board Function ) */){ keepLooping = false; }
            //send message to all ranks to break and terminate
        }
    }
#endif
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize(); //mpi causes false positives for memory leaks. 
    //DRMEMORY expects 427,000 bytes reachable and valgrind expects 71,000 bytes. Dont worry :)
    return 0;
}
