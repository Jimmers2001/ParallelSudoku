#include <iostream>
#include <vector>
#include <set>
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

void printVector(vector<int> v){
    for (unsigned int i = 0; i < v.size(); i++) {
        cout << v[i] << " ";
    }
    cout << endl;
    printf("Vector is of size %u\n\n", (unsigned int) v.size());
}

void printSet(set<int> v){
    set<int>::iterator itr = v.begin();
    for (; itr != v.end(); itr++) {
        cout << *itr << " ";
    }
    cout << endl;
    printf("Set is of size %u\n\n", (unsigned int) v.size());
}

/// @param val The value at that specific coordinate value
/// @param pos_values The range of numbers that could be valid in this location
class Tile{
    private:
        int val = 0;
        set<int>* pos_values; //not guaranteed to be in order
    public:
        Tile(int v){
            val = v;
            pos_values = new set<int>();
            for (int i = 1; i <= boardsize; i++){
                pos_values->insert(i);
            }
        }

        /*~Tile(){
            delete pos_values;
        }*/

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

        /// @brief Getter for pos_values
        /// @return pos_values
        set<int>* getPosValues(){
            return pos_values;
        }

        /// @brief Removes the possible value from pos_values list
        /// @param val 
        /// @return 0 on successful removal, 1 on no removes
        int removePosVal(int val){
            for (set<int>::iterator itr = pos_values->begin(); itr != pos_values->end(); itr++){
                if (*itr == val){
                    pos_values->erase(*itr);
                    return 0;
                }
            }

            return 1;
        }

        /// @brief Adds the possible value to pos_values list
        /// @param val 
        /// @return 0 on successful addition
        int addPosVal(int val){
            if (find(pos_values->begin(), pos_values->end(), val) != pos_values->end()) {
                //printf("already has %d\n", val);
            }

            pos_values->insert(val);
            return 0;
        }
};


