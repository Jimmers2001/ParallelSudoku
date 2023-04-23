#include <cuda.h>
#include <cuda_runtime.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <string>
#include <numeric>
#include <cmath>
 
/*
__global__ void __multiply__ (){
    printf("In multiply\n");
}*/
 
 extern "C++" void CudaThings(){
   //printf("In Cuda Things\n");
   //std::cout << "in Cuda Things and confirming C++\n" << std::endl;

     /* ... Load CPU data into GPU buffers  */
     //__multiply__ <<< ...block configuration... >>> (x, y);
 
     /* ... Transfer data from GPU to CPU */
}