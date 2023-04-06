#include <iostream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <string>

using namespace std;

//GLOBAL DEFINES
int boardsize = 9;
//define some board examples
#if 1
    string input1 = "\
        1234\
        2341\
        3412\
        4123\
        ";

    string incomplete_board = "\
        120000089\
        789123456\
        456780000\
        300005967\
        600000005\
        000690002\
        201504608\
        960231004\
        500060031\
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
#endif

class Tile{
    private:
        int val = 0;
        vector<int> pos_values;
    public:
        Tile(int v){
            val = v;
            pos_values.push_back(1);
            pos_values.push_back(2);
            pos_values.push_back(3);
            pos_values.push_back(4);
            pos_values.push_back(5);
            pos_values.push_back(6);
            pos_values.push_back(7);
            pos_values.push_back(8);
            pos_values.push_back(9);
        }

        /// @brief Change the value of a tile
        /// @param v value input
        /// @return 0 on success, 1 on failure
        int setVal(int v){
            if (v < 0 || v > boardsize){
                fprintf(stderr, "Changing to an invalid value %d\n", v);
                return 1;
            }
            val = v;
            return 0;
        }

        /// @brief Get the value of a tile
        /// @return the value being gotten
        int getVal(){
            return val;
        }
};

/// @brief A square sudoku board defined by a boardsize global variable. It 
/// @brief requires that all the numbers from 1-boardsize inclusive exist in each row and column without repeats
/// @brief a 4x4 board would have 1,2,3,4 and ignore the rest of the numbers
class SudokuBoard{
    private:
        vector<vector<Tile>> tiles;
        int width = boardsize; 
        int height = boardsize;

    public:
        SudokuBoard(string input){
            if (input.size() < 0){
                fprintf(stderr, "Too small of a board input string\n");
            }
            else{
                //initialize boardsize x boardsize board to all 0s
                for (int i = 0; i < boardsize; i++){
                    vector<Tile> row;
                    tiles.push_back(row);
                    for (int j = 0; j < boardsize; j++){
                        tiles[i].push_back(Tile(0));
                    }
                }

                //add existing numbers from input after removing whitespace
                input.erase(std::remove_if(input.begin(), input.end(), ::isspace),input.end());
                if (input.size() != boardsize * boardsize){
                    if (input.size() > boardsize * boardsize){
                        fprintf(stderr, "Input is too big to create a square grid\n\tbased on boardsize: %d\n\n", boardsize);
                    }
                    else{
                        fprintf(stderr, "Input is too small to create a square grid\n\tbased on boardsize: %d\n\n", boardsize);
                    }
                    
                    printf("Input: %s\n", input.c_str());
                }
                else{
                    for (int i = 0; i < boardsize; i++){
                        for (int j = 0; j < boardsize; j++){
                            tiles[j][i].setVal((int)input[(j*boardsize) + i]-48); //convert char ASCII to integer
                        }
                    }
                }
            }
        }
        
        //kinda useless if i always just use tiles[y][x]
        /// @brief Attempts to retrieve a value of a particular tile in the grid
        /// @param x the horizontal x position 
        /// @param y the vertical y position 
        /// @param val the return value at that position in the grid
        /// @return 0 on success, positive number on failure
        int getValue(int x, int y, int& val){
            if (x > width || x < 0 || y > height || y < 0){
                fprintf(stderr, "Cannot get value at invalid position: %d, %d\n", x, y);
                return 1;
            }
            val = tiles[y][x].getVal();
            return 0;
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
            tiles[y][x].setVal(val);
            return 0;
        }

        void printBoard(){
            for (int i = 0; i < boardsize; i++){
                for (int j = 0; j < boardsize; j++){
                    printf("%d\t", tiles[i][j].getVal());
                }
                printf("\n");
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
                    if (val == 0){
                        fprintf(stderr, "There exists a 0 on the board (incomplete)\n");
                        return 1;
                    }

                    //confirm each element is valid
                    if (val < 0 || val > boardsize){
                        fprintf(stderr, "There exists an invalid number %d at position (%d, %d)\n", val, j, i);
                        return 1;
                    }

                    //check row, column, and block
                    if (isInRow(i,j) || isInCol(i,j) || isInBlock(i,j)){
                        return 1;
                    }
                }
            }
            return 0;
        }
        
        /// @brief Automatic attempt to fill in every empty grid in the board
        /// @return 0 on success, 1 on error, tiles will be filled correctly on success
        int solveBoard(){
            //go through and fill every element of the board (write solution to file) IN PARALLEL

            //test board
            if (checkBoard() == 0){return 0;}
            return 1;
        }
};



//POTENTIALLY ADD THE 3X3 OR BOARDSIZE%3 X BOARDSIZE%3 BLOCKS AS WELL
int main(){
    SudokuBoard* board = new SudokuBoard(complete_board);
    if (board->solveBoard() == 0){
        printf("\n***********************\nBOARD IS SOLVED\n***********************\n");
    }
    else{
        printf("board is incomplete or incorrect\n");
    }


    board->printBoard();    
    delete board;
    return 0;
}

/* 
TODO:
implement blocks 
make each tile its own class with a value variable and a list of all possible numbers that can go in it
make a random sudoku board generator script so when I run it on the supercomputer I can run it on like 100 different sudoku boards


HOW TO PARALLELIZE SUDOKU
https://cse.buffalo.edu/faculty/miller/Courses/CSE633/Sankar-Spring-2014-CSE633.pdf

RASYAS WAY:
    You know how we look through the 1's first and find if there are "forced spots" 
    for the 1 to be in because of other rows and columns? Run that in parallel. 
    Jim: In other words, for a 9x9 board, run 9 different concurrent processes testing 1-9.
    If any of them make a change, cancel all of them and rerun them all. If they all
    finish and dont make any changes, recurse after guessing a value and try again.













*/