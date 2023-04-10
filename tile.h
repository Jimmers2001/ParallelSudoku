#include <iostream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <string>
#include <mpi.h>
#include <mutex>
using namespace std;

/// @param val The value at that specific coordinate value
/// @param pos_values The range of numbers that could be valid in this location
class Tile{
    private:
        int val = 0;
        vector<int> pos_values;
    public:
        Tile(int v){/////////////////////////loop through boardsize instead of 9
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


