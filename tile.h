#include <iostream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <string>
#include <mpi.h>
#include <mutex>
using namespace std;

#ifdef BOARDSIZE9
int boardsize = 9;
#endif

#ifdef BOARDSIZE16
int boardsize = 16;
#endif

/// @param val The value at that specific coordinate value
/// @param pos_values The range of numbers that could be valid in this location
class Tile{
    private:
        int val = 0;
        vector<int> pos_values;
    public:
        Tile(int v){
            val = v;
            for (int i = 1; i <= boardsize; i++){
                pos_values.push_back(i);
            }
        }

        /// @brief Change the value of a tile
        /// @param v value input
        /// @return 0 on success
        int setVal(int v){
            val = v;
            return 0;
        }

        /// @brief Get the value of a tile
        /// @return the value being gotten
        int getVal(){
            return val;
        }
};


