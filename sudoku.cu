#include <cuda.h>
#include <cuda_runtime.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <string>
#include <numeric>
#include <cmath>
     
extern "C++" void CudaElimination(char *sudoku_str){
  int i, j, k, l, num;
  int sudoku_matrix[9][9];
  char temp[2];
  char *ptr;
  int updated = 0;

  ///////////////////////////////////////////////////////////////
  //                    Making String a Matrix                //
  //////////////////////////////////////////////////////////////
  ptr = sudoku_str;
  for (i = 0; i < 9; i++) {
      for (j = 0; j < 9; j++) {
          strncpy(temp, ptr++, 1);
          sudoku_matrix[i][j] = atoi(temp);
      }
  }
  
  ///////////////////////////////////////////////////////////////
  //                           ELIM BELOW                     //
  //////////////////////////////////////////////////////////////
  char *value = sudoku_str;
  for (i = 0; i < 9; i++) {
      for (j = 0; j < 9; j++) {
          if (sudoku_matrix[i][j] != 0) {
              continue;
          }
              
          int possible_numbers[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
          int num_possible_numbers = 9;
          
          
          for (k = 0; k < 9; k++) {
              num = sudoku_matrix[i][k];
              if (num != 0) {
                  for (l = 0; l < num_possible_numbers; l++) {
                      if (possible_numbers[l] == num) {
                          possible_numbers[l] = possible_numbers[--num_possible_numbers];
                          break;
                      }
                  }
              }
          }
          

          for (k = 0; k < 9; k++) {
              num = sudoku_matrix[k][j];
              if (num != 0) {
                  for (l = 0; l < num_possible_numbers; l++) {
                      if (possible_numbers[l] == num) {
                          possible_numbers[l] = possible_numbers[--num_possible_numbers];
                          break;
                      }
                  }
              }
          }
          
          // Check 3x3 block
          int block_i = (i / 3) * 3;
          int block_j = (j / 3) * 3;
          for (k = block_i; k < block_i + 3; k++) {
              for (l = block_j; l < block_j + 3; l++) {
                  num = sudoku_matrix[k][l];
                  if (num != 0) {
                      for (int m = 0; m < num_possible_numbers; m++) {
                          if (possible_numbers[m] == num) {
                              possible_numbers[m] = possible_numbers[--num_possible_numbers];
                              break;
                          }
                      }
                  }
              }
          }
          
          // If only one possible value, fill cell
          if (num_possible_numbers == 1) {
              sudoku_matrix[i][j] = possible_numbers[0];
              updated = 1;
              break;
          }
      }
      if (updated) {
          break;
      }
  }
  value++;
 /////////////////////////////////////////////////// 
  //  CONVERT BACK TO STRING TO SEND TO MAIN //
///////////////////////////////////////////////////
  ptr = sudoku_str;
  for (i = 0; i < 9; i++) {
      for (j = 0; j < 9; j++) {
          sprintf(temp, "%d", sudoku_matrix[i][j]);
          strncpy(ptr++, temp, 1);
      }
  }
  
  //set to global shared mem
  value = sudoku_str;

}
/*
int main(){
    return 0;
}*/