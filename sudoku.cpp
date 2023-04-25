#include <iostream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <string>
#include <mpi.h>
#include <numeric>
#include <cmath>
#include <cuda_runtime.h>
#include <fstream>
#include <chrono>
#include <cstring>
#include <sstream>
#include "tile.h"
using namespace std;

extern void CudaThings();

//numbers 1-9 and then continuing with letters a-z for 10-36

// JIMMOTHY TILES[Y][X]

int EMPTY = 0;
string outfilename = "sample_tests.txt";

//define some board examples
string test2 = "\
    305201807\
    189647053\
    260050100\
    851934720\
    020570401\
    734162008\
    493705602\
    672489315\
    010326974";


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
    00041b0000f3e00g";

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



long long getCurrentTimeMicros() {
    auto time = std::chrono::high_resolution_clock::now();
    auto duration = time.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}


/// @brief Assigns a particular rank its block to solve in parallel
/// @param myrank the rank itself
/// @param startx starting x value
/// @param starty starting y value
/// @param endx INCLUSIVE ending x
/// @param endy INCLUSIVE ending y
void setBlockCoordinates(int myrank, int& startx, int& starty, int& endx, int& endy){
    //set coordinates for a 9x9 board
    int blockSize = (int) sqrt(boardsize);
    int blockX = (myrank - 1) % blockSize;
    int blockY = (myrank - 1) / blockSize;
    startx = blockX * blockSize;
    starty = blockY * blockSize;
    endx = startx + blockSize - 1;
    endy = starty + blockSize - 1;
    return;
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


string maxString(string str1, string str2) {
    int n = str1.size();
    int m = str2.size();
    int maxLen = max(n, m);

    // Make both strings the same length by adding leading zeros
    str1 = string(maxLen - n, '0') + str1;
    str2 = string(maxLen - m, '0') + str2;

    string result = "";

    for (int i = 0; i < maxLen; i++) {
        char c = max(str1[i], str2[i]);
        result += c;
    }

    return result;
}


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
            if (input.size() != (unsigned int) sudoku_size){
                fprintf(stderr, "Bad board input string. size: %u, boardsize: %d\n", (unsigned int) input.size(), boardsize);
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
                            char item = input[(i*boardsize) + j];
                            int num;
                            //it is a number
                            if (isdigit(item)){
                                num = (int)input[(i*boardsize) + j]-48; //convert char ASCII to integer
                            } else{ //it is a lowercase letter (no support for uppercase)
                                num = 9+((int)item)-48-48; //letter a = 10, b = 11... etc
                            }
                            setValue(i,j,num);                          
                        }
                    }
                }
            }
        }
#if 1
        /// @brief MAKE SURE TO INCLUDE THE & WHEN CALLING THIS FUNCTION OR IT WILL COPY THE REFERENCE, NOT THE ORIGINAL DATA
        /// @return the reference to tiles
        vector<vector<Tile>>& getTiles(){
            return tiles;
        }
        
        /// @brief Gets the string representation of the board (with letters for double digit numbers)
        /// @return s the string 
        string boardToString(){
            string s = "";
            for (int i = 0; i < boardsize; i++){
                for (int j = 0; j < boardsize; j++){
                    if (tiles[j][i].getVal() > 9){
                        //double digit so make char
                        char c = char((tiles[j][i].getVal() - 10) + 'a');
                        s += c;
                    }
                    else{ //single digit so its a number
                        s += to_string(tiles[j][i].getVal());
                    }
                    
                }
            }
            return s;
        }
        
        /// @brief Attempts to retrieve a value of a particular tile in the grid
        /// @param x the horizontal x position 
        /// @param y the vertical y position 
        /// @return the value
        int getValue(int x, int y){
            if (x >= width || x < 0 || y >= height || y < 0){
                fprintf(stderr, "Cannot get value at invalid position: %d, %d\n", x, y);
                return 1;
            }
            return tiles[y][x].getVal();;
        }

        /// @brief Attempts to assign a value to a particular tile in the grid
        /// @param x the horizontal x position 
        /// @param y the vertical y position 
        /// @param val the value being used for assignment
        /// @return 0 on success, positive number on failure
        int setValue(int x, int y, int val){
            if (x >= width || x < 0 || y >= height || y < 0){
                fprintf(stderr, "Cannot set value %d at invalid position: %d, %d\n", x, y, val);
                //return 1;
            }
            int existing_val = getValue(x,y);
            if (val == 0){
                //dont add 0s to possible values
                if (existing_val != 0){
                    addPosValue(existing_val, x, y); //add a non-zero to pos val
                }   
            } else{
                if (existing_val != 0){throw;} //confirms you never change a number into another number. 
                //You always empty the tile and then add a new number in two separate states
                removePosValue(val, x, y);
            }

            tiles[y][x].setVal(val);
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
                    //fprintf(stderr, "Duplicate value in column\n");
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
                    //fprintf(stderr, "Duplicate value in row between (%d, %d) and (%d, %d)\n", cury, curx, cury, a);
                    return true;
                }
            }
            return false;
        }

        /// @brief Checks the given coordinate for its block and if there exists val already
        /// @param curx current x
        /// @param cury current y
        /// @returns true if there is a duplicate, false otherwise
        bool isInBlock(int curx, int cury, int val){
            int sq = sqrt(boardsize);
            int xstart = (curx / sq) * sq; //round down and then scale up
            int ystart = (cury / sq) * sq;
            int xend = xstart+sq;
            int yend = ystart+sq;
            for (int i = xstart; i < xend; i++){
                for (int j = ystart; j < yend; j++){ 
                    if (cury == j && curx == i){continue;}                   
                    else if (val == tiles[j][i].getVal()){
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
                        //fprintf(stderr, "There exists a 0 on the board (incomplete)\n");
                        //printf("\nThere exists a 0 at (%d, %d)\n\n", j, i);
                        //printBoard();
                        return 1;
                    }

                    //confirm each element is valid
                    if (val < 0 || val > boardsize){
                        fprintf(stderr, "There exists an invalid number %d at position (%d, %d)\n", val, j, i);
                        return 1;
                    }

                    //check row, column, and block for duplicate values
                    if (isInRow(i,j) || isInCol(i,j) || isInBlock(i,j, tiles[j][i].getVal())){
                        //printf("\nfailed one of these\n\n");
                        return 1;
                    }
                }
            }
            return 0;
        }

        /// @brief Getter function for board access
        /// @return a copy of the board
        /*vector<vector<Tile>> getBoard(){
            vector<vector<Tile>> board_copy = tiles;////////////////////////check if this is deep or shallow copy
            return board_copy;
        }*/

        /// @brief called by solveBoardParallel (the driver), this function recurses and solves in parallel
        /// @return 0 on success, positive number on error
        int recursiveParallel(int currRank){
            int startx, starty, endx, endy;
            setBlockCoordinates(currRank, startx, starty, endx, endy);

            //test board
            int row = 0;
            int col = 0;
            if (!findEmptyTile(row, col)){return 0;}
            
            //try to fill the empty element
            /*for (int i = 1; i <= boardsize; i++){
                // the tile can support the value in (row, col)
                if (canSupportinRow(row, i) && canSupportinCol(col, i) && canSupportinBlock(row, col, i)){
                    //test out that number and continue
                    setValue(col,row,i);
                    if (SequentialRecursiveBacktrackSolve() == 0){
                        //passed so end
                        return 0;
                    }
                    
                    //undo and try a different number
                    setValue(col,row,EMPTY);
                }
            }*/
            
            //could not solve the board for some reason
            return 1;
        }


        /// @brief Checks if the given val can put placed anywhere in the y
        /// @param y the y being checked
        /// @param val the value being checked
        /// @return false if the value already exists, so it cannot be supported. otherwise true
        bool canSupportinRow(int y, int val){
            for (int i = 0; i < boardsize; i++)
                if (tiles[y][i].getVal() == val)
                    return false;
            return true;
        }

        /// @brief Checks if the given val can put placed anywhere in the x
        /// @param x the x being checked
        /// @param val the value being checked
        /// @return false if the value already exists, so it cannot be supported. otherwise true
        bool canSupportinCol(int x, int val){
            for (int i = 0; i < boardsize; i++)
                if (tiles[i][x].getVal() == val)
                    return false;
            return true;
        }

        /// @brief Checks the same block for existence of val
        /// @param originalx current x
        /// @param originaly current y
        /// @returns true if can support because of no duplicate, false otherwise
        bool canSupportinBlock(int originaly, int originalx, int val){ 
            int sq = sqrt(boardsize);
            int xstart = (originalx / sq) * sq; //round down and then scale up
            int ystart = (originaly / sq) * sq;
            int xend = xstart+sq;
            int yend = ystart+sq;
            for (int i = xstart; i < xend; i++){
                for (int j = ystart; j < yend; j++){                    
                    if (val == tiles[j][i].getVal()){
                        return false;
                    }
                }
            }
            return true;
        }
#endif
        void addPosValue(int val, int originalx, int originaly){
            //go through the row, col, and block containing coordinate (x,y) and remove val from all tiles' pos_values

            //you cant always just add a possible value back 
            /*
            if you try a number on the far left of a row, and it fails, you add that number as a possible value to all other elements of the row
            what if the far right which is also empty already has that number in the block, you dont want to add it as a possible value

            */

            //row
            for (int x = 0; x < boardsize; x++){
                if (!isInBlock(x, originaly, val)){
                    tiles[originaly][x].addPosVal(val); 
                }
                
            }

            //col
            for (int y = 0; y < boardsize; y++){
                if (!isInBlock(originalx, y, val)){
                    tiles[y][originalx].addPosVal(val);
                }
                
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
                    if (!isInBlock(x, y, val)){
                        tiles[y][x].addPosVal(val);
                    }
                    
                }
            }
        }

        void removePosValue(int val, int originalx, int originaly){
            //go through the row, col, and block containing coordinate (x,y) and remove val from all tiles' pos_values
            
            //row
            for (int x = 0; x < boardsize; x++){
                tiles[originaly][x].removePosVal(val);
            }

            //col
            for (int y = 0; y < boardsize; y++){
                //if (y == originaly){continue;}
                tiles[y][originalx].removePosVal(val);
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
                    tiles[y][x].removePosVal(val);
                }
            }
        }
        
        bool ParallelEliminationRule(int xstart, int ystart, int xend, int yend){
            int numChanges = 0;
            //char* board;
            string boardcopy = boardToString();
            /*cudaMallocManaged(&board, sizeof(int)*sudoku_size);

            for (unsigned int i = 0; i < boardcopy.size(); i++){
                board[i] = boardcopy[i];
            }

            for (unsigned int i = 0; i < boardcopy.size(); i++){
                printf("%c", board[i]);
            }
            printf("\n\n\n");
            cudaFree(board);*/

            /*I think this existing thought process is incorrect. 
            all of this cuda initialization needs to happen in a function that initializes a char[] 
            for the board. Then it calls a new variation of elimination (like parallelElimRule) 
            that actually sets the changes of the board instead of just returning how many changes 
            were made. That way, the device has a board variable that gets changed in cuda and it can be returned and analyzed.
            */
            
            
            /*
            int block_size = boardsize;//for 16x16 we want 16 threads per block, and 9 for 9x9
            int num_blocks = (sudoku_size + block_size - 1) / block_size;////////////unsure about dimensions
            
            
            //int blockSize = 256;
            //int numBlocks = (vec.size() + blockSize - 1) / blockSize;
            

            bool madechange = true;

            while (madechange){
                madechange = false;
                //call 9 cuda kernels, one for each block
                eliminationRule<<<numBlocks, blockSize>>>(devArr, vec.size(), devResult);

                if a kernel made a change, set madechange to true and store that changed board as our board
            }

            for (int x = xstart; x <= xend; x++){
                for (int y = ystart; y <= yend; y++){
                    if (tiles[y][x].getVal() == 0){
                        set<int>* pos_values = tiles[y][x].getPosValues();
                        if (pos_values->size() == 1){
                            setValue(x,y,*(pos_values->begin()));
                            numChanges++;

                            //RESTART THE LOOP SINCE A CHANGE WAS MADE
                            x = xstart;
                            y = ystart-1;
                        }
                    } 
                }
            }*/
            return (numChanges>0);
        }

        void recalculatePosValues(){
            int y = 0;
            int x = 0;

            vector<vector<Tile>>& tiles = getTiles();
            vector<pair<int, int>> empty_tile_coords; 
            pair<int, int> target;
            //bool tileExists = find(empty_tile_coords.begin(), empty_tile_coords.end(), target) != empty_tile_coords.end();

            //fill in empty_tile_coords with all empty tiles, filling them in with a temporary 1
            while (findEmptyTile(y, x)){
                target = make_pair(y,x);
                empty_tile_coords.push_back(target);
                tiles[y][x].setVal(boardsize+1);
            }

            //set those coordinates back to 0
            for (pair<int, int> p : empty_tile_coords){
                int y = p.first;
                int x = p.second;
                tiles[y][x].setVal(EMPTY);
            }

            //update the pos_values
            for (pair<int, int> p : empty_tile_coords){
                int originaly = p.first; 
                int originalx = p.second;
                set<int>* p_vs = tiles[originaly][originalx].getPosValues();

                //row
                for (int x = 0; x < boardsize; x++){
                    p_vs->erase(tiles[originaly][x].getVal());
                }

                //col
                for (int y = 0; y < boardsize; y++){
                    p_vs->erase(tiles[y][originalx].getVal());
                }

                //block
                int sq = sqrt(boardsize);
                int xstart = (originalx / sq) * sq; //round down and then scale up
                int ystart = (originaly / sq) * sq;
                int xend = xstart+sq;
                int yend = ystart+sq;
                for (int x = xstart; x < xend; x++){
                    for (int y = ystart; y < yend; y++){
                        p_vs->erase(tiles[y][x].getVal());
                    }
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
            for (int x = xstart; x <= xend; x++){
                for (int y = ystart; y <= yend; y++){
                    if (tiles[y][x].getVal() == 0){
                        set<int>* pos_values = tiles[y][x].getPosValues();
                        if (pos_values->size() == 1){
                            setValue(x,y,*(pos_values->begin()));
                            numChanges++;

                            //RESTART THE LOOP SINCE A CHANGE WAS MADE
                            x = xstart;
                            y = ystart-1;
                        }
                    } 
                }
            }

            //serious bandage fixing garbage
            int r;
            int c;
            if (!findEmptyTile(r, c)){recalculatePosValues();}
            for (int x = xstart; x <= xend; x++){
                for (int y = ystart; y <= yend; y++){
                    if (tiles[y][x].getVal() == 0){
                        set<int>* pos_values = tiles[y][x].getPosValues();
                        if (pos_values->size() == 1){
                            setValue(x,y,*(pos_values->begin()));
                            numChanges++;

                            //RESTART THE LOOP SINCE A CHANGE WAS MADE
                            x = xstart;
                            y = ystart-1;
                        }
                    } 
                }
            }
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
            eliminationRule(xstart, ystart, xend, yend);
            ///////////////////////////////////////////////////////////////////////needs to do backtracking and try elim again
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
                    setValue(col, row, i);

                    if (SequentialRecursiveBacktrackSolve() == 0){
                        //passed so end
                        return 0;
                    }
                    
                    //undo and try a different number
                    setValue(col, row, EMPTY);
                }
            }
            
            //could not solve the board for some reason
            return 1;
        }

    /// @brief Randomly fills up a board
    /// @return 0 on success, 1 on inability to solve
    int randomBoardSolve(){
        //if (tiles.size()*tiles.size() != (unsigned int) sudoku_size){printf("tiles has len: %d\n", (int) (tiles.size()*tiles.size())); printBoard(); throw;}
        //HOW IS IT POSSIBLE THAT THE LENGTH OF TILES IS CHANGING FROM 256 TO 368 FOR A 16X16 BOARD
        //else{printf("tiles is good with length: %u\n", (unsigned int) tiles.size()*tiles.size());}
        int y = 0;
        int x = 0;
        if (!findEmptyTile(y, x)){
            return 0;
        }
        
        //vector begins full and after random attempts removes elements until empty
        vector<int> nums(boardsize);
        iota(nums.begin(), nums.end(), 1);//vector where ith element is i+1 (1,2,3...boardsize)
        int n;

        while (nums.size() > 0){
            n = (rand() % (nums.size())); //[0,boardsize] exclusive index of nums array

            // the tile can support the value in (y, x)
            if (canSupportinRow(y, nums[n]) && canSupportinCol(x, nums[n]) && canSupportinBlock(y, x, nums[n])){
                //test out that number and continue
                setValue(x,y,nums[n]);

                if (randomBoardSolve() == 0){
                    //filled board successfully
                    return 0;
                }
                
                //undo and try a different number
                setValue(x,y,EMPTY);
            }

            //already guessed n so dont randomly guess again
            nums.erase(nums.begin() + n);
        }
        
        //could not fill the board for some reason
        return 1;
    }

    /// @brief Determines if the board is empty
    /// @param row the x coordinate of an empty tile
    /// @param col the y coordinate of an empty tile
    /// @return returns true if the board has an empty tile, false otherwise
    bool findEmptyTile(int& row, int& col){
        for (col = 0; col < boardsize; col++){
            for (row = 0; row < boardsize; row++){
                if (tiles[row][col].getVal() == EMPTY){
                    return true;
                }
            }
        }
        return false;
    }

    vector<pair<int, int>> getAllEmptyTiles() {
        vector<pair<int, int>> pairs;
        for (int x = 0; x < boardsize; x++) {
            for (int y = 0; y < boardsize; y++) {
                if (getValue(x,y) == 0){ //empty
                    pairs.push_back(make_pair(x, y));
                }
            }
        }
        return pairs;
    }
    
    int ParallelHumanisticSolve(string paramBoard){
        vector<bool> sendToArray(9, true); // Array of if we are sending a message to the a specific rank 0 is rank 1 etc etc.
        vector<char> sendToValues(9, '1'); // Array of what location each rank is meant to check for <===========================

       
        
        int tiles_updated = 0;
        int blocks_updated = 0;
        int count = 0;
   
        blocks_updated = 0;

        //test board
        int _row = 0, _col = 0;
        if (this->checkBoard() == 0){ /*this->printBoard();*/ return 0; }

        /* OPTIMIZATION SAVE FOR LATER */
        //Step 1: check all blocks to see if any empty spaces in blocks
        //vector<pair<int, int>> empty_tiles = this->getAllEmptyTiles();

        //check what block each tile is in, and set that block's index to true in sendToArray
        //for (unsigned int i = 0; i < empty_tiles.size(); i++){
            //   pair<int, int> coord = empty_tiles[i];
            //   int bnum = tileToBlock(coord.first, coord.second);
            //   sendToArray[bnum] = true;
        // }

        //if all of sendToArray is false board is full so check if board is done
        //if board is done send terminate to all ranks through messages and print board
        
        for (unsigned int i = 0; i < sendToArray.size(); i++){
            if( sendToArray[i] ){  

                char tile = sendToValues[i]; // Tile location 1-z as a char
                char message[sudoku_size + 3] = ""; //Message to be sent to rank i + 1

                string b = this->boardToString(); 
                
                char send_board[b.length() + 2]; //
                strcpy( send_board, b.c_str() );
            

                // Concat the String to be 'Board' + ' ' + 'Location'
                // Note: Location is a single char 1-z
                strcat(message,  send_board );
                strcat(message, " ");
                strncat(message, &tile, 1);
                strcat(message, "\0");

                MPI_Send(message, sudoku_size + 3, MPI_CHAR, i + 1, 0, MPI_COMM_WORLD);
                blocks_updated++;
            } 
        }

        tiles_updated = 0;
        string newBoardString = this->boardToString(); 
        
    
        for (unsigned int i = 0; i < sendToArray.size(); i++) {
            if( sendToArray[i] ){  
                char incoming_message[sudoku_size + 3] = "";
                char incoming_board[sudoku_size + 1],  incoming_tiles_updated[1];
                MPI_Recv(&incoming_message, sudoku_size + 3, MPI_CHAR, i + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);                   
            
                // split the message
                // mesh current board into main board
                istringstream iss(incoming_message);
                iss.getline(incoming_board, sudoku_size + 3 , ' ');
                iss.getline(incoming_tiles_updated, 2);

                int incoming_tile_updated_count = incoming_tiles_updated[0] - '0';
                tiles_updated += incoming_tile_updated_count;

                string boardstring(incoming_board);
                newBoardString = maxString(newBoardString , incoming_board); //IT IS HERE
            }
        }

        int xstart = 0, ystart = 0, xend = 9, yend = 9, _count = 0;
        for (int x = xstart; x < xend; x++){
            for (int y = ystart; y < yend; y++){   
                tiles[y][x].setVal(newBoardString[_count] - '0' );      
                _count++;   
            } 
        }
        
        recalculatePosValues();
        //<------------------
        //this(newBoardString);

       

        //RECURSIVE BACKTRACKING BEGINS
        if( tiles_updated == 0 ){
            //printf("--------------------NO UPDATES, RUNNING BT----------------------\n" );
            // printf("\tRank 0 Here Main Board is Currently:\n\t %s\n",  this->boardToString().c_str()); 
            
            int __row = 0; int __col = 0;
            if (!this->findEmptyTile(__row, __col)){ return 0; }
            //printf("\tWith the guess at: (%d,%d)\n", __row, __col); 

            for (int i = 1; i <= boardsize; i++){
                // the tile can support the value in (row, col)
                if (canSupportinRow(__row, i) && canSupportinCol(__col, i) && canSupportinBlock(__row, __col, i)){
                    //test out that number and continue
                    setValue(__col, __row, i);

                    if (ParallelHumanisticSolve(this->boardToString()) == 0){ return 0; }
                    
                    //undo this change AND ALL CHANGES MADE BY ELIMINATION and try again
                    setValue(__col, __row, EMPTY);

                    int _xstart = 0, _ystart = 0, _xend = boardsize, _yend = boardsize, __count = 0;
                    for (int x = _xstart; x < _xend; x++){
                        for (int y = _ystart; y < _yend; y++){   
                            tiles[y][x].setVal(paramBoard[__count] - '0' );      
                            __count++;   
                        } 
                    }


                }
            }


        } else { // free old memory and update the board }
           // printf("-------------------- UPDATES KEEP GOING----------------------\n" );
           // printf("\tRank 0 Here Main Board is Currently:\n\t %s\n",  this->boardToString().c_str()); 
            //if ( ParallelHumanisticSolve() == 0 ){ 

            return  ParallelHumanisticSolve(this->boardToString()); 
            //} 
        }
        
        return 1; 
    }
};

/// @brief Randomly generates a sudoku board
/// @param difficulty string that determines the range of the number of givens to randomly start the board with
/// @return Uses boardsize to make a sudoku board to solve
SudokuBoard* generateSudokuBoard(string difficulty){
    //Seed the random number generator with the current time
    srand(getCurrentTimeMicros());
    //chose the number of givens to include in the generated board
    unsigned int number_of_givens;
    int total_squares = sudoku_size;
    if (difficulty == "evil"){
        number_of_givens = 3 + (rand() % static_cast<int>(6 - 3 + 1)); //~[3,6] exclusive range
    } else if (difficulty == "easy"){
        number_of_givens = (total_squares-20) + (rand() % static_cast<int>((total_squares-10) - (total_squares-20) + 1)); //missing 10-20 squares
    } else if (difficulty == "medium"){
        number_of_givens = (total_squares-40) + (rand() % static_cast<int>((total_squares-30) - (total_squares-40) + 1)); //missing 30-40 squares
    } else if (difficulty == "expert"){
        number_of_givens = 10 + (rand() % static_cast<int>(15 - 10 + 1)); //~[10,15] exclusive range
    } else if (difficulty == "trivial"){
        number_of_givens = (total_squares-5) + (rand() % static_cast<int>((total_squares-2) - (total_squares-5) + 1)); //missing 2-5 squares
    } else { 
        number_of_givens = 30 + (rand() % static_cast<int>(50 - 30 + 1)); //[30,50] exclusive range
    }
     
    printf("# of givens: %d\n", number_of_givens);

    //initialize a board to empty and add in random given numbers
    SudokuBoard* random_board = new SudokuBoard(empty_board);
    //if (random_board->boardToString().size() != (unsigned int) sudoku_size){printf("bad empty board with len: %u\n", (unsigned int) random_board->boardToString().size());throw;}
    random_board->randomBoardSolve(); //complete the board randomly and then remove elements
    
    //current coordinate position
    int x;
    int y;

    //remove elements in the board until only number_of_givens are left
    unsigned int removals = 0;
    while (removals < sudoku_size-number_of_givens){
        x = rand() % (boardsize);
        y = rand() % (boardsize);

        //if we havent assigned a value to that spot yet
        if (random_board->getValue(x,y) != 0){
            //must add possible values back because number is getting 0'd out

            random_board->setValue(x,y,0);
            removals++;
        }
    }
    return random_board; //returns the board* for dereferencing in the future
}

void createTestBoards(int numtests, string difficulty, string filename){
    ofstream outfile(filename);
    if (!outfile) {
        cerr << "Error opening file!" << endl;
        return;
    }
    
    for (int i = 0; i < numtests; i++) {
        SudokuBoard* tmp = generateSudokuBoard(difficulty);
        //tmp->printBoard();
        string bstring = tmp->boardToString();
        //if (bstring.size() != (unsigned int) sudoku_size){printf("bad bstring\n"); throw;}
        outfile << bstring << endl;
        delete tmp;
    }
    
    outfile.close();
}

/// @brief Runs all solvers potentially many times
/// @param number_of_tests The number of iterations to do 
void runTestsSequential(int number_of_tests){
    int seq_passes = 0;
    int human_passes = 0;
    for (int i = 0; i < number_of_tests; i++){
        SudokuBoard* board_recursive_sequential = generateSudokuBoard("easy"); //random board
        SudokuBoard* board_humanistic = new SudokuBoard(board_recursive_sequential->boardToString()); //same random board
            board_recursive_sequential->printBoard(); 
             board_humanistic->printBoard(); 
        if (board_recursive_sequential->boardToString() != board_humanistic->boardToString()){
            printf("OH NO\n\n\n"); throw;
        }
        // <-------------------------------------------

        //SudokuBoard* b_par = new SudokuBoard(b_seq); //copy to compare runtime between sequential and parallel
        printf("*******************************\nINITIAL SEQUENTIAL BOARD STATE\n*******************************\n\n");
        board_recursive_sequential->printBoard();
        
        if (board_recursive_sequential->SequentialRecursiveBacktrackSolve() == 0){
            seq_passes++;
            printf("\n********************************\nrecursive backtrack solved the board!\n********************************\n");
            board_recursive_sequential->printBoard();
        } else {
            printf("recursive backtracking is incomplete or incorrect\n");
            board_recursive_sequential->printBoard(); 
        }

        if (board_humanistic->SequentialHumanisticSolve() == 0){
            human_passes++;
            printf("\n********************************\nsequential humanistic solved the board!\n********************************\n");
            board_humanistic->printBoard(); 
        } else {
            printf("humanistic is incomplete or incorrect\n");
            board_humanistic->printBoard(); 
        }
                
        delete board_recursive_sequential;
        delete board_humanistic;
        //delete b_par;
    }

    printf("\n*******************************\nEND OF TESTS\n*******************************\n\n");
    printf("Recursive backtracking passed %d out of %d tests\n", seq_passes, number_of_tests);
    printf("Humanistic passed %d out of %d tests\n", human_passes, number_of_tests);
    
    return;
}

/// @brief Match the coordinate point to the block containing it
/// @param x 
/// @param y 
/// @return 
int tileToBlock(int x, int y){
    int blockSize = (int) sqrt(boardsize);
    int blockX = x / blockSize;
    int blockY = y / blockSize;
    return blockY * blockSize + blockX + 1;
}


/*
#define NUM_THREADS 9
__global__ void cuda_kernel() {
  // your CUDA kernel code here
  printf("Hello World\n");
}*/



int ParallelDriver(string input_board){
        //initialize the board
        SudokuBoard* myBoard = new SudokuBoard(input_board);
        string boardString = myBoard->boardToString(); 

        //convert to char[] to send with mpi
        char board[boardString.length() + 2];
        strcpy( board, boardString.c_str() );
        strcat(board, "\0");  
        
        int value = myBoard->ParallelHumanisticSolve(boardString);

    
        vector<bool> rankCount(9, true); // Array of if we are sending a message to the a specific rank 0 is rank 1 etc etc.
        
        for (unsigned int i = 0; i < rankCount.size(); i++) {
            const char* message = "terminate";
            MPI_Send(const_cast<char*>(message), sudoku_size + 3, MPI_CHAR, i + 1, 0, MPI_COMM_WORLD);
        }


        
        delete myBoard;
        return value;
}



int main(int argc, char** argv){
    int myrank = 0;
    int number_of_ranks = 0;
    string infil = argv[1];
    const int LINES_PER_RANK = 10;
    const int CHARACTERS_PER_LINE = sudoku_size;
    const int CHARACTERS_PER_RANK = boardsize;
    double ntests = 10000.0;
    
    long long seq_brute_force_total_time  =  0.0;
    long long rand_brute_force_total_time  = 0.0;
    long long seq_humanistic_total_time  = 0.0;
    long long parallel_humanistic_total_time  =  0.0;
               

    //character that represents the "{board} {#tile to solve}" which is space separated and has null terminating character
    //char board[sudoku_size] = "Hello";

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank); //this processes' individual rank
    MPI_Comm_size(MPI_COMM_WORLD, &number_of_ranks); //the total number of processes in the world

    // Open the input file
    ifstream input_file(infil);

    // Check that the file was opened successfully
    if (!input_file.is_open()) {
        std::cerr << "Failed to open input file." << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Read lines from the input file
    string line;

    /*
        for each line in the file:
            make a char[boardsize] for each rank
            fill in respective spots
            BARRIER()

            gather all character arrays into one chunky char[sudoku_size] array

            make a sudoku board from that char array

            DO THE REST OF OUR CODE GIVEN THAT BOARD
    */
   //initialize timers
   std::chrono::duration<double, std::milli> elapsed;









    /*****************************START OF SEQUENTIAL BRUTE FORCE*******************************************/
    int line_counter = -1;
    auto start = std::chrono::high_resolution_clock::now();

    while (getline(input_file, line) ) {
        //PARALLEL IO
        #if 1
            line_counter++; //increment from -1 to 0 to start
            //printf("On line: %d\t", line_counter);

            // Allocate memory for the character array
            char char_array[boardsize]; //no extra for null terminating yet because we will gather

            if (myrank != 0){
                // Compute the starting and ending character indices for this rank
                int start_char_index = (myrank - 1) * CHARACTERS_PER_RANK;
                int end_char_index = myrank * CHARACTERS_PER_RANK; ///////////EXCLUSIVE SO MUST DO < NOT <=
                
                //mpi doesnt have shared memory, so i have to make a bunch of char arrays 
                //that are small and then gather them together into one bigger rank

                // Read the characters for this rank from the current line
                //printf("i am rank %d and I am reading: ", myrank);
                for (int i = 0; i < CHARACTERS_PER_RANK; i++) {
                    int char_index = start_char_index + i;
                    if (char_index < end_char_index) {
                        char_array[i] = line[char_index];
                        //printf("%c", char_array[i]);
                    }
                }
                //printf("\n");
            }
            MPI_Barrier(MPI_COMM_WORLD);
            //ALL RANKS RUN THIS PORTION:
            // Gather the character arrays from all the ranks to the root rank
            char gathered_char_array[CHARACTERS_PER_LINE+1]; //+1 for null terminating character
            MPI_Gather(char_array, CHARACTERS_PER_RANK, MPI_CHAR,
            gathered_char_array, CHARACTERS_PER_RANK, MPI_CHAR,
            0, MPI_COMM_WORLD);

            // Print the gathered character array from the root rank
            MPI_Barrier(MPI_COMM_WORLD);

            SudokuBoard* seq_brute_force;
            SudokuBoard* rand_brute_force;
            SudokuBoard* seq_humanistic;
            SudokuBoard* parallel_humanistic;

            if (myrank == 0) {
                //create the string in this weird way
                string s = "";
                //printf("The entire line says: \n");
                for (int i = 0; i < CHARACTERS_PER_LINE; i++) {
                    //cout << gathered_char_array[i+9]; //NEEDS THE +9
                    s.push_back(gathered_char_array[i+9]);
                }
    
                seq_brute_force = new SudokuBoard(s);
                rand_brute_force = new SudokuBoard(s);
                seq_humanistic = new SudokuBoard(s);
                parallel_humanistic = new SudokuBoard(s);
            }
        #endif

        if( myrank == 0 ){ 
            //sequential brute force
            if (seq_brute_force->SequentialRecursiveBacktrackSolve() == 0){
                //success
                //printf("\nWe have solved the board for sequential brute force!\n");
            } else{
                //failure
                printf("\nWe have not solved the board for sequential brute force\n");
                throw;
            }
        }

        if (myrank == 0){delete seq_brute_force; delete rand_brute_force; delete seq_humanistic; delete parallel_humanistic;}

    }
    auto end = std::chrono::high_resolution_clock::now();
    elapsed = end - start;
    seq_brute_force_total_time = elapsed.count();

    if (myrank == 0){
        std::cout << "\nSEQUENTIAL BRUTE FORCE Algorithm has an average time to solve of: " <<   seq_brute_force_total_time/ntests << "ms.\n";
    }
    /*****************************END OF SEQUENTIAL BRUTE FORCE*******************************************/




    // Reset the file pointer to the beginning of the file
    //input_file.seekg(0);

  
    input_file =  ifstream(infil);

    // Check that the file was opened successfully
    if (!input_file.is_open()) {
        std::cerr << "Failed to open input file." << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }






    /*****************************START OF RANDOM BRUTE FORCE*******************************************/
    line_counter = -1;
    start = std::chrono::high_resolution_clock::now();
    while (getline(input_file, line) ) {
        line_counter++; //increment from -1 to 0 to start
        //PARALLEL IO
        #if 1
            char char_array[boardsize]; //no extra for null terminating yet because we will gather

            if (myrank != 0){
                // Compute the starting and ending character indices for this rank
                int start_char_index = (myrank - 1) * CHARACTERS_PER_RANK;
                int end_char_index = myrank * CHARACTERS_PER_RANK; ///////////EXCLUSIVE SO MUST DO < NOT <=
                
                //mpi doesnt have shared memory, so i have to make a bunch of char arrays 
                //that are small and then gather them together into one bigger rank

                // Read the characters for this rank from the current line
                //printf("i am rank %d and I am reading: ", myrank);
                for (int i = 0; i < CHARACTERS_PER_RANK; i++) {
                    int char_index = start_char_index + i;
                    if (char_index < end_char_index) {
                        char_array[i] = line[char_index];
                        //printf("%c", char_array[i]);
                    }
                }
                //printf("\n");
            }
            MPI_Barrier(MPI_COMM_WORLD);
            //ALL RANKS RUN THIS PORTION:
            // Gather the character arrays from all the ranks to the root rank
            char gathered_char_array[CHARACTERS_PER_LINE+1]; //+1 for null terminating character
            MPI_Gather(char_array, CHARACTERS_PER_RANK, MPI_CHAR,
            gathered_char_array, CHARACTERS_PER_RANK, MPI_CHAR,
            0, MPI_COMM_WORLD);

            // Print the gathered character array from the root rank
            MPI_Barrier(MPI_COMM_WORLD);

            SudokuBoard* seq_brute_force;
            SudokuBoard* rand_brute_force;
            SudokuBoard* seq_humanistic;
            SudokuBoard* parallel_humanistic;

            if (myrank == 0) {
                //create the string in this weird way
                string s = "";
                //printf("The entire line says: \n");
                for (int i = 0; i < CHARACTERS_PER_LINE; i++) {
                    //cout << gathered_char_array[i+9]; //NEEDS THE +9
                    s.push_back(gathered_char_array[i+9]);
                }
    
                seq_brute_force = new SudokuBoard(s);
                rand_brute_force = new SudokuBoard(s);
                seq_humanistic = new SudokuBoard(s);
                parallel_humanistic = new SudokuBoard(s);
            }
        #endif

        if( myrank == 0 ){ 
            long long x = getCurrentTimeMicros();
            //random brute force
            if (rand_brute_force->randomBoardSolve() == 0){
                //success
                //printf("\nWe have solved the board for random brute force!\n");
            } else{
                //failure
                printf("\nWe have not solved the board for random brute force\n");
                throw;
            }
            //randomBoardSolve();
            x = getCurrentTimeMicros() - x;
            rand_brute_force_total_time += x;
        }

         if (myrank == 0){delete seq_brute_force; delete rand_brute_force; delete seq_humanistic; delete parallel_humanistic;}
        
    }


    end = std::chrono::high_resolution_clock::now();
    elapsed = end - start;
    rand_brute_force_total_time = elapsed.count();

    if (myrank == 0){
        std::cout << "\nRANDOM BRUTE FORCE Algorithm has an average time to solve of: " <<  rand_brute_force_total_time/ntests << "ms.\n";
    }
    
    /*****************************END OF RANDOM BRUTE FORCE*******************************************/





    // Reset the file pointer to the beginning of the file
    //input_file.seekg(0);
      
    input_file =  ifstream(infil);

    // Check that the file was opened successfully
    if (!input_file.is_open()) {
        std::cerr << "Failed to open input file." << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }






    /*****************************START OF PARALLEL HUMANISTIC*******************************************/
    //parallel humanistic
    line_counter = -1;
    start = std::chrono::high_resolution_clock::now();
    while (getline(input_file, line) ) {
        line_counter++; //increment from -1 to 0 to start
        #if 1
            char char_array[boardsize]; //no extra for null terminating yet because we will gather

            if (myrank != 0){
                // Compute the starting and ending character indices for this rank
                int start_char_index = (myrank - 1) * CHARACTERS_PER_RANK;
                int end_char_index = myrank * CHARACTERS_PER_RANK; ///////////EXCLUSIVE SO MUST DO < NOT <=
                
                //mpi doesnt have shared memory, so i have to make a bunch of char arrays 
                //that are small and then gather them together into one bigger rank

                // Read the characters for this rank from the current line
                //printf("i am rank %d and I am reading: ", myrank);
                for (int i = 0; i < CHARACTERS_PER_RANK; i++) {
                    int char_index = start_char_index + i;
                    if (char_index < end_char_index) {
                        char_array[i] = line[char_index];
                        //printf("%c", char_array[i]);
                    }
                }
                //printf("\n");
            }
            MPI_Barrier(MPI_COMM_WORLD);
            //ALL RANKS RUN THIS PORTION:
            // Gather the character arrays from all the ranks to the root rank
            char gathered_char_array[CHARACTERS_PER_LINE+1]; //+1 for null terminating character
            MPI_Gather(char_array, CHARACTERS_PER_RANK, MPI_CHAR,
            gathered_char_array, CHARACTERS_PER_RANK, MPI_CHAR,
            0, MPI_COMM_WORLD);

            // Print the gathered character array from the root rank
            MPI_Barrier(MPI_COMM_WORLD);

            SudokuBoard* seq_brute_force;
            SudokuBoard* rand_brute_force;
            SudokuBoard* seq_humanistic;
            SudokuBoard* parallel_humanistic;

            if (myrank == 0) {
                //create the string in this weird way
                string s = "";
                //printf("The entire line says: \n");
                for (int i = 0; i < CHARACTERS_PER_LINE; i++) {
                    //cout << gathered_char_array[i+9]; //NEEDS THE +9
                    s.push_back(gathered_char_array[i+9]);
                }
    
                seq_brute_force = new SudokuBoard(s);
                rand_brute_force = new SudokuBoard(s);
                seq_humanistic = new SudokuBoard(s);
                parallel_humanistic = new SudokuBoard(s);
            }
        #endif

        if( myrank == 0 ){ // My main man
            long long x = getCurrentTimeMicros();
            
            if (ParallelDriver(parallel_humanistic->boardToString()) == 0){
                //success
                //printf("\nWe have solved the board for parallel humanistic solve!\n");
            } else{
                //failure
                printf("\nWe have not solved the board for parallel humanistic solve!\n");
                throw;
            }

            x = getCurrentTimeMicros() - x;
            parallel_humanistic_total_time += x;
        } else { //we are a child process
            char message[sudoku_size + 3] = "";
            bool keepLooping = true;
            int count = 0;

            while ( keepLooping ){
                count++;
            
                char board[sudoku_size + 1], tile[1];
                MPI_Status status;
                MPI_Recv(message, sudoku_size + 3, MPI_CHAR , MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

                string terminationCheck(message);
                if( terminationCheck == "terminate" ){ keepLooping = false; continue; }
                
                // Message is in the form: board + " " + tile_location
                // So we split it to get the data
                istringstream iss(message);
                iss.getline(board, sudoku_size + 1 , ' ');
                iss.getline(tile, 2);
                // ------------------------- DONT TOUCH ABOVE THIS LINE -------------------------- //

                string boardstring(board);
                SudokuBoard* myboard = new SudokuBoard(boardstring);


                /*  CUDA HERE */
                // launch the CUDA kernel using the CUDA runtime API
                
                //cuda_kernel<<<1, NUM_THREADS>>>();


    
                // Elim Goes here
                int startx, starty, endx, endy;
                setBlockCoordinates(myrank, startx, starty, endx, endy);
                int changes = myboard->eliminationRule(startx, starty, endx, endy);
                


                //convert to char array
                char send_board[boardstring.length() + 3];
                char change_count = changes + '0';

                strcpy( send_board, myboard->boardToString().c_str() );
                strcat( send_board, " ");
                strncat(send_board, &change_count, 1);
                strcat( send_board, "\0");

                //printf("In rank %d we have a board of:\n\tMessage: %s\n", myrank, send_board);


                // -------------------------- All Code above this line ----------------------------//
                MPI_Send(send_board, sudoku_size + 3, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
                delete myboard;
            }
        }
        //STOP CLOCK HERE//////////////////////////////////////////////////

        if (myrank == 0){delete seq_brute_force; delete rand_brute_force; delete seq_humanistic; delete parallel_humanistic;}
    }

    end = std::chrono::high_resolution_clock::now();
    elapsed = end - start;
    parallel_humanistic_total_time = elapsed.count();

    
    if (myrank == 0){
        std::cout << "\n PARALLEL HUMANISTIC Algorithm has an average time to solve of: " <<   parallel_humanistic_total_time/ntests << "ms.\n";
    }
        
    /*****************************END OF PARALLEL HUMANISTIC*******************************************/

        

    
    
#if 0
    while (getline(input_file, line) ) {
        line_counter++; //increment from -1 to 0 to start
        //printf("On line: %d\t", line_counter);

        // Allocate memory for the character array
        char char_array[boardsize]; //no extra for null terminating yet because we will gather

        if (myrank != 0){
            // Compute the starting and ending character indices for this rank
            int start_char_index = (myrank - 1) * CHARACTERS_PER_RANK;
            int end_char_index = myrank * CHARACTERS_PER_RANK; ///////////EXCLUSIVE SO MUST DO < NOT <=
            
            //mpi doesnt have shared memory, so i have to make a bunch of char arrays 
            //that are small and then gather them together into one bigger rank

            // Read the characters for this rank from the current line
            //printf("i am rank %d and I am reading: ", myrank);
            for (int i = 0; i < CHARACTERS_PER_RANK; i++) {
                int char_index = start_char_index + i;
                if (char_index < end_char_index) {
                    char_array[i] = line[char_index];
                    //printf("%c", char_array[i]);
                }
            }
            //printf("\n");
        }
        MPI_Barrier(MPI_COMM_WORLD);
        //ALL RANKS RUN THIS PORTION:
        // Gather the character arrays from all the ranks to the root rank
        char gathered_char_array[CHARACTERS_PER_LINE+1]; //+1 for null terminating character
        MPI_Gather(char_array, CHARACTERS_PER_RANK, MPI_CHAR,
        gathered_char_array, CHARACTERS_PER_RANK, MPI_CHAR,
        0, MPI_COMM_WORLD);

        // Print the gathered character array from the root rank
        MPI_Barrier(MPI_COMM_WORLD);

        SudokuBoard* seq_brute_force;
        SudokuBoard* rand_brute_force;
        SudokuBoard* seq_humanistic;
        SudokuBoard* parallel_humanistic;

        if (myrank == 0) {
            //create the string in this weird way
            string s = "";
            //printf("The entire line says: \n");
            for (int i = 0; i < CHARACTERS_PER_LINE; i++) {
                //cout << gathered_char_array[i+9]; //NEEDS THE +9
                s.push_back(gathered_char_array[i+9]);
            }
  
            seq_brute_force = new SudokuBoard(s);
            rand_brute_force = new SudokuBoard(s);
            seq_humanistic = new SudokuBoard(s);
            parallel_humanistic = new SudokuBoard(s);
        }
    
        //START CLOCK HERE//////////////////////////////////////////////////
        if( myrank == 0 ){ 
            long long x = getCurrentTimeMicros();
            auto start = std::chrono::high_resolution_clock::now();
            //sequential brute force
            if (seq_brute_force->SequentialRecursiveBacktrackSolve() == 0){
                //success
                //printf("\nWe have solved the board for sequential brute force!\n");
            } else{
                //failure
                printf("\nWe have not solved the board for sequential brute force\n");
                throw;
            }
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end - start;

            seq_brute_force_total_time += elapsed.count();
            
        }
        // stop clock here

        //START CLOCK HERE//////////////////////////////////////////////////
        if( myrank == 0 ){ 
            long long x = getCurrentTimeMicros();
            //random brute force
            if (rand_brute_force->randomBoardSolve() == 0){
                //success
                //printf("\nWe have solved the board for random brute force!\n");
            } else{
                //failure
                printf("\nWe have not solved the board for random brute force\n");
                throw;
            }
            //randomBoardSolve();
            x = getCurrentTimeMicros() - x;
            rand_brute_force_total_time += x;
        }
        // stop clock here


        //START CLOCK HERE//////////////////////////////////////////////////
        //this code doesnt work because humanistic doesnt have backtracking as last resort implemented so it doesnt guarantee a solution
        if( myrank == 0 ){ 
           /* long long x = getCurrentTimeMicros();
            //sequential humanistic code 
            if (seq_humanistic->SequentialHumanisticSolve() == 0){
                //success
                //printf("\nWe have solved the board for sequential humanistic solve!\n");
            } else{
                //failure
                printf("\nWe have not solved the board for sequential humanistic solve!\n");
                throw;
            }
            x = getCurrentTimeMicros() - x;
            seq_humanistic_total_time += x;*/
        }
        // stop clock here

        //START CLOCK HERE//////////////////////////////////////////////////
        //parallel humanistic code
        if( myrank == 0 ){ // My main man
            long long x = getCurrentTimeMicros();
            
            if (ParallelDriver(parallel_humanistic->boardToString()) == 0){
                //success
                //printf("\nWe have solved the board for parallel humanistic solve!\n");
            } else{
                //failure
                printf("\nWe have not solved the board for parallel humanistic solve!\n");
                throw;
            }

            x = getCurrentTimeMicros() - x;
            parallel_humanistic_total_time += x;
        } else { //we are a child process
            char message[sudoku_size + 3] = "";
            bool keepLooping = true;
            int count = 0;

            while ( keepLooping ){
                count++;
            
                char board[sudoku_size + 1], tile[1];
                MPI_Status status;
                MPI_Recv(message, sudoku_size + 3, MPI_CHAR , MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

                string terminationCheck(message);
                if( terminationCheck == "terminate" ){ keepLooping = false; continue; }
                
                // Message is in the form: board + " " + tile_location
                // So we split it to get the data
                istringstream iss(message);
                iss.getline(board, sudoku_size + 1 , ' ');
                iss.getline(tile, 2);
                // ------------------------- DONT TOUCH ABOVE THIS LINE -------------------------- //

                string boardstring(board);
                SudokuBoard* myboard = new SudokuBoard(boardstring);


                /*  CUDA HERE */
                // launch the CUDA kernel using the CUDA runtime API
                
                //cuda_kernel<<<1, NUM_THREADS>>>();


    
                // Elim Goes here
                int startx, starty, endx, endy;
                setBlockCoordinates(myrank, startx, starty, endx, endy);
                int changes = myboard->eliminationRule(startx, starty, endx, endy);
                


                //convert to char array
                char send_board[boardstring.length() + 3];
                char change_count = changes + '0';

                strcpy( send_board, myboard->boardToString().c_str() );
                strcat( send_board, " ");
                strncat(send_board, &change_count, 1);
                strcat( send_board, "\0");

                //printf("In rank %d we have a board of:\n\tMessage: %s\n", myrank, send_board);


                // -------------------------- All Code above this line ----------------------------//
                MPI_Send(send_board, sudoku_size + 3, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
                delete myboard;
            }
        }
        //STOP CLOCK HERE//////////////////////////////////////////////////

        if (myrank == 0){delete seq_brute_force; delete rand_brute_force; delete seq_humanistic; delete parallel_humanistic;}
    }

    if (myrank == 0){
        std::cout << " _______ Algorithm has an average time to solve of:" << seq_brute_force_total_time/1000.0 << "ms.\n";
        std::cout << " _______ Algorithm has an average time to solve of:" << rand_brute_force_total_time/1000.0 << "ms.\n";
        std::cout << " _______ Algorithm has an average time to solve of:" << seq_humanistic_total_time/1000.0 << "ms.\n"; 
        std::cout << " _______ Algorithm has an average time to solve of:" << parallel_humanistic_total_time/1000.0 << "ms.\n";
    }

#endif

    MPI_Barrier(MPI_COMM_WORLD);
    // Close the input file and finalize MPI
    input_file.close();
    MPI_Finalize(); //mpi causes false positives for memory leaks. 
    //DRMEMORY expects 427,000 bytes reachable and valgrind expects 71,000 bytes. Dont worry :)
    return 0;
}
