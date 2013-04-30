//If you want to build the file directly at the command prompt then use the following commands. 
//AMD commands
//cl /c Example1_SAXPY.cpp /I"%AMDAPPSDKROOT%\include"
//link  /OUT:"Example.exe" "%AMDAPPSDKROOT%\lib\x86_64\OpenCL.lib" Example1_SAXPY.obj
//nVIDIA commands
//cl /c Example1_SAXPY.cpp /I"%NVSDKCOMPUTE_ROOT%\OpenCL\common\inc"
//link  /OUT:"Example.exe" "%NVSDKCOMPUTE_ROOT%\OpenCL\common\lib\x64\OpenCL.lib" Example1_SAXPY.obj

#include <windows.h>//Koushik

#include <ocl_macros.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <CL/cl.h>
//32 by 32 matrix 
//#define VECTOR_SIZE 1048576
//#define VECTOR_SIZE 1024
//#define MATRIX_WIDTH 1024
//#define MATRIX_HEIGHT MATRIX_WIDTH
bool verify = true;

#define BLOCK_SIZE 8

__int64 CounterStart = 0;
double PCFreq = 0.0;
void StartCounter();



const char *MatrixMul_kernel_basic =
"__kernel                                                                                   \n"
"void MatrixMul_kernel_basic(int dim,                                                            \n"
"                  __global float *A,                                                       \n"
"                  __global float *B,                                                       \n"
"                  __global float *C)                                                       \n"
"{                                                                                          \n"
"    //Get the index of the work-item                                                       \n"
"    int iCol = get_global_id(0);                                                           \n"
"    int iRow = get_global_id(1);                                                           \n"
"    float result = 0.0;                                                                    \n"
"    for(int i=0;i< dim;++i)                                                                \n"
"    {                                                                                      \n"
"	     result +=                                                                          \n"
"        A[iRow*dim + i]*B[i*dim + iCol];                                                   \n"
"    }                                                                                      \n"
"    C[iRow*dim + iCol] = result;                                                           \n"
"}                                                                                          \n";

const char *MatrixMul_kernel_localA =
"__kernel                                                                                           \n"
"void MatrixMul_kernel_localA(int dim,                                                             \n"
"                  __global float *A,                                                               \n"
"                  __global float *B,                                                               \n"
"                  __global float *C,                                                               \n"
"                  __local  float *lA)                                                              \n"
"{                                                                                                  \n"
"    //Get the index of the work-item                                                               \n"
"    int iCol = get_global_id(0);                                                                   \n"
"    int iRow = get_global_id(1);                                                                   \n"
"    int localIdx = get_local_id(0);                                                                \n"
"    int localSizex = get_local_size(0);                                                            \n"
"    float result = 0.0f;                                                                           \n"
"    int numElements = dim/localSizex;                                                          \n"
"    for(int i=0; i<numElements ; i++)                                                          \n"
"    {                                                                                              \n"
"        lA[localIdx*numElements + i] = A[iRow*dim + localIdx*numElements + i];             \n"
"    }                                                                                              \n"
"    barrier(CLK_LOCAL_MEM_FENCE);                                                                  \n"
"    for(int i=0;i< dim;++i)                                                                        \n"
"    {                                                                                              \n"
"	     result +=                                                                                  \n"
"        lA[i]*B[i*dim + iCol];                                                                     \n"
"        //printf(\"%d, %d = %f - %f\\n\",iCol, iRow, lA[i],B[i*dim + iCol]);                       \n"
"    }                                                                                              \n"
"    C[iRow*dim + iCol] = result;                                                                   \n"
"}										                                                            \n";


const char *MatrixMul_kernel_localA_coallesced =
"__kernel                                                                                           \n"
"void MatrixMul_kernel_localA_coallesced(int dim,                                                             \n"
"                  __global float *A,                                                               \n"
"                  __global float *B,                                                               \n"
"                  __global float *C,                                                               \n"
"                  __local  float *lA)                                                              \n"
"{                                                                                                  \n"
"    //Get the index of the work-item                                                               \n"
"    int iCol = get_global_id(0);                                                                   \n"
"    int iRow = get_global_id(1);                                                                   \n"
"    int localIdx = get_local_id(0);                                                                \n"
"    int localSizex = get_local_size(0);                                                            \n"
"    float result = 0.0f;                                                                           \n"
"    int numElements = dim/localSizex;                                                          \n"
"    for(int i=0; i<numElements ; i++)                                                          \n"
"    {                                                                                              \n"
"        lA[i*localSizex + localIdx] = A[iRow*dim + i*localSizex + localIdx];                       \n"
"    }                                                                                              \n"
"    barrier(CLK_LOCAL_MEM_FENCE);                                                                  \n"
"    for(int i=0;i< dim;++i)                                                                        \n"
"    {                                                                                              \n"
"	     result +=                                                                                  \n"
"        lA[i]*B[i*dim + iCol];                                                                     \n"
"        //printf(\"%d, %d = %f - %f\\n\",iCol, iRow, lA[i],B[i*dim + iCol]);                       \n"
"    }                                                                                              \n"
"    C[iRow*dim + iCol] = result;                                                                   \n"
"}										                                                            \n";

//4 eleemnts per work Item
const char *MatrixMul_kernel_coallesced_row =
"__kernel                                                                                           \n"
"void MatrixMul_kernel_coallesced_row(int dim,                                                      \n"
"                  __global float *A,                                                               \n"
"                  __global float *B,                                                               \n"
"                  __global float *C)                                                               \n"
"{                                                                                                  \n"
"    //Get the index of the work-item                                                               \n"
"    int iCol = get_global_id(0);                                                                   \n"
"    int iRow = get_global_id(1);                                                                   \n"
"    int localIdx = get_local_id(0);                                                                \n"
"    int localSizex = get_local_size(0);                                                            \n"
"    float result = 0.0f;                                                                           \n"
"    int numElements = dim/localSizex;                                                              \n"
"    for(int j=0; j<numElements; j++)                                                               \n"
"    {                                                                                              \n"
"       result = 0.0f;                                                                               \n"
"       for(int i=0;i< dim;++i)                                                                     \n"
"       {                                                                                           \n"
"	        result += A[iRow*dim + i]*B[i*dim + j*localSizex + localIdx];                                     \n"
"       }                                                                                           \n"
"       C[iRow*dim + j*localSizex + iCol] = result;                                                 \n"
"    }                                                                                              \n"
"}										                                                            \n";

const char *MatrixMul_kernel_localA_coallesced_row =
"__kernel                                                                                           \n"
"void MatrixMul_kernel_localA_coallesced_row(int dim,                                               \n"
"                  __global float *A,                                                               \n"
"                  __global float *B,                                                               \n"
"                  __global float *C,                                                               \n"
"                  __local  float *lA)                                                              \n"
"{                                                                                                  \n"
"    //Get the index of the work-item                                                               \n"
"    int iCol = get_global_id(0);                                                                   \n"
"    int iRow = get_global_id(1);                                                                   \n"
"    int localIdx = get_local_id(0);                                                                \n"
"    int localSizex = get_local_size(0);                                                            \n"
"    float result = 0.0f;                                                                           \n"
"    int numElements = dim/localSizex;                                                              \n"
"    for(int i=0; i<numElements ; i++)                                                              \n"
"    {                                                                                              \n"
"        lA[i*localSizex + localIdx] = A[iRow*dim + i*localSizex + localIdx];                       \n"
"    }                                                                                              \n"
"    barrier(CLK_LOCAL_MEM_FENCE);                                                                  \n"
"    for(int j=0; j<numElements; j++)                                                               \n"
"    {                                                                                              \n"
"       result = 0.0;                                                                               \n"
"       for(int i=0;i< dim;++i)                                                                     \n"
"       {                                                                                           \n"
"	        result += lA[i]*B[i*dim + j*localSizex + localIdx];                                     \n"
"       }                                                                                           \n"
"       C[iRow*dim + j*localSizex + iCol] = result;                                                 \n"
"    }                                                                                              \n"
"}										                                                            \n";

const char *MatrixMul_kernel_RowPerWI =
"__kernel                                                                                           \n"
"void MatrixMul_kernel_RowPerWI(int dim,                                                            \n"
"                  __global float *A,                                                               \n"
"                  __global float *B,                                                               \n"
"                  __global float *C)                                                              \n"
"{                                                                                                  \n"
"    //Get the index of the work-item                                                               \n"
"    int iRow = get_global_id(0);                                                                   \n"
"    float result = 0.0;                                                                            \n"
"    for(int j=0;j< dim;++j)                                                                        \n"
"    {                                                                                              \n"
"       result = 0.0f;                                                                              \n"
"       for(int i=0;i< dim;++i)                                                                     \n"
"       {                                                                                           \n"
"	        result +=                                                                               \n"
"               A[iRow*dim + i]*B[i*dim + j];                                                         \n"
"       }                                                                                           \n"
"       C[iRow*dim + j] = result;                                                                \n"
"    }                                                                                              \n"
"}                                                                                                  \n";

const char *MatrixMul_kernel_RowPerWI_APriv =
"__kernel                                                                                           \n"
"void MatrixMul_kernel_RowPerWI_APriv(int dim,                                                            \n"
"                  __global float *A,                                                               \n"
"                  __global float *B,                                                               \n"
"                  __global float *C)                                                              \n"
"{                                                                                                  \n"
"    //Get the index of the work-item                                                               \n"
"    int iRow = get_global_id(0);                                                                   \n"
"    float result = 0.0;                                                                            \n"
"    float lA[1024];                         \n"
"    for(int i=0; i<dim ; i++)                                                              \n"
"    {                                                                                              \n"
"        lA[i] = A[iRow*dim + i];                                                           \n"
"    }                                                                                              \n"
"    for(int j=0;j< dim;++j)                                                                        \n"
"    {                                                                                              \n"
"       result = 0.0f;                                                                              \n"
"       for(int i=0;i< dim;++i)                                                                     \n"
"       {                                                                                           \n"
"	        result +=                                                                               \n"
"               lA[i]*B[i*dim + j];                                                         \n"
"       }                                                                                           \n"
"       C[iRow*dim + j] = result;                                                                \n"
"    }                                                                                              \n"
"}                                                                                                  \n";

const char *MatrixMul_kernel_RowPerWI_APriv_BLocal =
"__kernel                                                                                           \n"
"void MatrixMul_kernel_RowPerWI_APriv_BLocal(int dim,                                               \n"
"                  __global float *A,                                                               \n"
"                  __global float *B,                                                               \n"
"                  __global float *C,                                                               \n"
"                  __local  float *lB)                                                              \n"
"{                                                                                                  \n"
"    //Get the index of the work-item                                                               \n"
"    int iRow = get_global_id(0);                                                                   \n"
"    int localIDx = get_local_id(0);                                                                   \n"
"    float result = 0.0;                                                                            \n"
"    float lA[1024];                                                                        \n"
"    for(int i=0; i<dim ; i++)                                                                      \n"
"    {                                                                                              \n"
"        lA[i] = A[iRow*dim + i];                                                                   \n"
"    }                                                                                              \n"
"    for(int j=0;j< dim;++j)                                                                        \n"
"    {                                                                                              \n"
"       result = 0.0f;                                                                              \n"
"       int numElements = dim/get_local_size(0);                                                    \n"
"                                                                                           \n"
"       for (int k=0; k<numElements; k++)                                                          \n"
"            lB[localIDx*numElements + k] = B[(localIDx*numElements)*dim+ k*dim + j];       \n"
"       barrier(CLK_LOCAL_MEM_FENCE);                                                              \n"
"       for(int i=0;i< dim;++i)                                                                     \n"
"       {                                                                                           \n"
"	        result +=                                                                               \n"
"               lA[i]*lB[i];                                                                \n"
"       }                                                                                           \n"
"       C[iRow*dim + j] = result;                                                                \n"
"    }                                                                                              \n"
"}                                                                                                  \n";



//#define ENABLE_BASIC
//#define ENABLE_LOCAL_A
//#define ENABLE_LOCAL_A_COALLESCED
//#define ENABLE_LOCAL_A_COALLESCED_ROW
//#define ENABLE_COALLESCED_ROW
//#define ENABLE_ROW_PER_WI
#define ENABLE_ROW_PER_WI_A_PRIVATE
//#define ENABLE_ROW_PER_WI_A_PRIVATE_B_LOCAL
//"                                                                                  \n"
//

//One kernel computes 1 Row of C
//1 fixed row of A is used to compute that row of C
//But, for each element  of C one distinct column is required.
// We buffer that row in private memory : Extra copy time but
// saving repated global memory fetch

#if 0
const char *MatrixMul_kernel200 =
"__kernel                                             \n"
"void MatrixMul_kernel200(int dim,                      \n"
"                  __global float *A,                 \n"
"                  __global float *B,                 \n"
"                  __global float *C)                 \n"
"{                                                    \n"
"    //Get the index of the work-item                 \n"
"    int iRowComputed = get_global_id(0);             \n"
"    int iCol;                                        \n"
"    int iLoop;                                                 \n"
"    float privateAOneRow[32];               \n"
"    if(iRowComputed < dim) // valid row               \n"
"    {            \n"
"        //Take the iRowComputed-th Row in Private memory           \n"
"        for(iLoop=0;iLoop< dim;++iLoop)           \n"
"        {           \n"
"            privateAOneRow[iLoop] = A[iLoop + dim*iRowComputed];           \n"
"        }           \n"
"                                                                                                           \n"
"        //This WI would compute entire row i.e. iCol=0 to dim           \n"
"        for(iCol=0;iCol< dim;++iCol)           \n"
"        {           \n"
"            float privateSum = 0.0;                       \n"
"            for(iLoop=0;iLoop< dim;++iLoop)           \n"
"            {                                             \n"
"	            privateSum +=            \n"
"                privateAOneRow[iLoop]*B[iCol + iLoop*dim];           \n"
"            }           \n"
"            C[iCol + dim*iRowComputed] = privateSum;           \n"
"        }// End of for(iCol=0;iCol< dim;++iCol)           \n"
"    }// End of if(iRowComputed < dim)                                         \n"      
"}                                                    \n";

//================================================================
//Same as "MatrixMul_kernel200" except B is saved in Column major form 
//to have better access pattern

const char *MatrixMul_kernel300 =
"__kernel                                             \n"
"void MatrixMul_kernel300(int dim,                      \n"
"                  __global float *A,                 \n"
"                  __global float *B,                 \n"
"                  __global float *C)                 \n"
"{                                                    \n"
"    //Get the index of the work-item                 \n"
"    int iRowComputed = get_global_id(0);             \n"
"    int iCol;                                        \n"
"    int iLoop;                                                 \n"
"    if(iRowComputed < dim) // valid row               \n"
"    {           \n"
"        //Take the iRowComputed-th Row in Private memory           \n"
"        float privateAOneRow[MATRIX_DIM];           \n"
"        for(int iLoop=0;iLoop< dim;++iLoop)           \n"
"        {           \n"
"            privateAOneRow[iLoop] = A[iLoop + dim*iRowComputed];           \n"
"        }           \n"
"                                                                                                      \n"
"        //This WI would compute entire row i.e. iCol=0 to dim           \n"
"        for(int iCol=0;iCol< dim;++iCol)           \n"
"        {           \n"
"            float privateSum = 0.0;                       \n"
"            for(int iLoop=0;iLoop< dim;++iLoop)           \n"
"            {                                             \n"
"	            privateSum +=            \n"
"                privateAOneRow[iLoop]*B[iLoop + iCol*dim];           \n"
"            }           \n"
"            C[iCol + dim*iRowComputed] = privateSum;           \n"
"        }// End of for(int iCol=0;iCol< dim;++iCol)           \n"
"    }// End of if(iRowComputed < dim)                                         \n"      
"}                                                    \n";

//================================================================
// Use of local for B, a bit more complex
// each WG would have a copy of B to be used by all WIs of that WG
// Who would copy ? Ok, equi-partition of task - each WI of that WG would do equal part of copy
// 

const char *MatrixMul_kernel400 =
"__kernel                                                                                             \n"
"void MatrixMul_kernel400(int dim,                                                             \n"
"                  __global float *A,                                                                \n"
"                  __global float *B,                                                                \n"
"                  __global float *C)                                                                \n"
"{                                                                                                       \n"
"    //Get the index of the work-item                                                        \n"
"    int iRowComputed = get_global_id(0);                                                \n"
"    int iLocId = get_local_id(0);                                                \n"
"    int wgSize = get_local_size(0);                                                \n"
"    int iCol;                                                                                          \n"
"    int iLoop;                                                                                        \n"
"    if(iRowComputed < dim) // valid row                                                   \n"
"    {                                                                                                    \n"
"        //Take the iRowComputed-th Row in Private memory                          \n"
"        float4 privateAOneRow[8];// 8 = 32/4                                              \n"
"        for(iLoop=0;iLoop< dim/4;iLoop+=1)                                              \n"
"        {                                                                                                \n"
"            privateAOneRow[iLoop].x = A[iLoop*4 + dim*iRowComputed];             \n"
"            privateAOneRow[iLoop].y = A[iLoop*4 + dim*iRowComputed + 1];             \n"
"            privateAOneRow[iLoop].z = A[iLoop*4 + dim*iRowComputed + 2];             \n"
"            privateAOneRow[iLoop].w = A[iLoop*4 + dim*iRowComputed + 3];             \n"
"        }                                                                                               \n"
"                                                                                                        \n"
"        __local float4 localBFull[256];\n"
"        //This WI would compute entire row i.e. iCol=0 to dim                     \n"
"        for(iCol=0;iCol< dim;++iCol)                                                       \n"
"        {                                                                                                   \n"
"            for(iLoop=iLocId;iLoop<dim/4 ;iLoop+=(wgSize*4))                                                   \n"
"            { \n"
"               localBFull[iLoop].x = B[iCol + iLoop*dim];              \n"
"               localBFull[iLoop].y = B[iCol + (iLoop+1)*dim];              \n"
"               localBFull[iLoop].z = B[iCol + (iLoop+2)*dim];              \n"
"               localBFull[iLoop].w = B[iCol + (iLoop+3)*dim];              \n"
"            } \n"
"            barrier(CLK_LOCAL_MEM_FENCE);                                                              \n"
"            float4 privateSum = (float4)0;                                                                   \n"
"            float4 Avect = (float4)(0);                         \n"
"            float4 Bvect = (float4)(0);                         \n"
"            float4 sum = (float4)(0);                         \n"
"            for(iLoop=0;iLoop< dim/4;++iLoop)                                            \n"
"            {                                                                                               \n"
"	            privateSum +=                                                                   \n"
"                privateAOneRow[iLoop]*localBFull[iLoop];                                 \n"
"            }                                                                                                   \n"
"            C[iCol + dim*iRowComputed] = privateSum.x +privateSum.y +privateSum.z +privateSum.w;                                 \n"
"        }                                                                          \n"

"    }// End of if(iRowComputed < dim)                                                               \n"      
"}                                                                                                           \n";


/*



                               \n"
"        }// End of for(int iCol=0;iCol< dim;++iCol)                                            \n"

*/











//OpenCL kernel with local memory.
//Where are you passing BLOCK_SIZE to the kernel. Is this kernel compiling? 
//I am sure it is not compiling. I don't know how it is working. 
const char *MatrixMul_kernel2 =
"__kernel __attribute__((reqd_work_group_size(BLOCK_SIZE, BLOCK_SIZE, 1)))                             \n"
"void MatrixMul_kernel2(__global int *q,                                                               \n"
"                  __global float *A,                                                                  \n"
"                  __global float *B,                                                                  \n"
"                  __global float *C)                                                                  \n"
"{                                                                                                     \n"
"    int dim = q[0];                                                                                   \n"
"                                                                                                      \n"
"    //Get the index of the work-item                                                                  \n"
"    int iRow = get_global_id(0);                                                                      \n"
"    int iCol = get_global_id(1);                                                                      \n"
"                                                                                                      \n"
"    //Identification of this workgroup                                                                \n"
"    int iGroup = get_group_id(0);                                                                     \n"
"    int jGroup = get_group_id(1);                                                                     \n"
"                                                                                                      \n"
"    int iLocRow = get_local_id(0);                                                                    \n"
"    int iLocCol = get_local_id(1);                                                                    \n"
"                                                                                                      \n"
"    int numTiles = dim/BLOCK_SIZE;                                                                    \n"
"    float4 tile = (float4)(0,0,0,0);                                                                  \n"
"    __local float SubA[BLOCK_SIZE][BLOCK_SIZE];                                                       \n"
"    __local float SubB[BLOCK_SIZE][BLOCK_SIZE];                                                       \n"
"    for (int k = 0; k < numTiles; k++)                                                                \n"
"    {                                                                                                 \n"
"	//koushik done upto                                                                                \n"
"        SubA[iLocRow][iLocCol] = A[BLOCK_SIZE*iGroup + iLocRow + dim*(BLOCK_SIZE*k+iLocCol)];         \n"     
"        SubB[iLocRow][iLocCol] = A[BLOCK_SIZE*iGroup + iLocRow + dim*(BLOCK_SIZE*k+iLocCol)];         \n"  
"        barrier(CLK_LOCAL_MEM_FENCE);                                                                 \n"
"        for (int r = 0; r < BLOCK_SIZE; r+=4)                                                         \n"
"        {                                                                                             \n"
"            float4 temp1=(float4)(A[iLocRow][r],A[iLocRow][r+1],A[iLocRow][r+2],A[iLocRow][r+3]);     \n"
"            float4 temp2=(float4)(B[r][iLocCol],B[r+1][iLocCol],B[r+2][iLocCol],B[r+3][iLocCol]);     \n"
"            tile += temp1 * temp2;                                                                    \n"
"        }                                                                                             \n"
"        barrier(CLK_LOCAL_MEM_FENCE);                                                                 \n"
"    }                                                                                                 \n"
"    C[BLOCK_SIZE*iRow + iLocRow + dim*(BLOCK_SIZE*iCol+iLocCol)] = tile.x+tile.y+tile.z+tile.w;       \n"
"}                                                                                                     \n";
#endif
bool resultIsCorrect(float* pA,float* pB,float* pCTest, size_t dim);
void matrixMultWithLocal();
void matrixMultSimpleKernel();
void matrixMultWithLocal2();

int callMatrixMult1(int MATRIX_WIDTH, int MATRIX_HEIGHT, bool verify)
{
	cl_event events;
    int i;
    // Allocate space for vectors A, B and C
    int alpha = MATRIX_WIDTH;
    float *A = (float*)malloc(sizeof(float)*MATRIX_WIDTH*MATRIX_HEIGHT);
    float *B = (float*)malloc(sizeof(float)*MATRIX_WIDTH*MATRIX_HEIGHT);
    float *C = (float*)malloc(sizeof(float)*MATRIX_WIDTH*MATRIX_HEIGHT);
    for(i = 0; i < MATRIX_WIDTH*MATRIX_HEIGHT; i++)
    {
        A[i] = (float) (rand() % 10);
        B[i] = (float) (rand() % 10);
        C[i] = 0;
    }

    // Get platform and device information
    cl_platform_id * platforms = NULL;
    cl_uint     num_platforms;
    //Set up the Platform
    cl_int clStatus = clGetPlatformIDs(0, NULL, &num_platforms);
    platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id)*num_platforms);
    clStatus = clGetPlatformIDs(num_platforms, platforms, NULL);

    //Get the devices list and choose the type of device you want to run on
    cl_device_id     *device_list = NULL;
    cl_uint       num_devices;

    clStatus = clGetDeviceIDs( platforms[0], CL_DEVICE_TYPE_GPU, 0,
            NULL, &num_devices);
    device_list = (cl_device_id *)malloc(sizeof(cl_device_id)*num_devices);
    clStatus = clGetDeviceIDs( platforms[0], CL_DEVICE_TYPE_GPU, num_devices,
            device_list, NULL);

    // Create one OpenCL context for each device in the platform
    cl_context context;
    cl_context_properties props[3] =
    {
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)platforms,
        0
    };
    context = clCreateContext( NULL, num_devices, device_list, NULL, NULL, &clStatus);

    // Create a command queue
    cl_command_queue_properties prop = 0;
    prop |= CL_QUEUE_PROFILING_ENABLE;
    cl_command_queue command_queue = clCreateCommandQueue(context, device_list[0], prop, &clStatus);
	//cl_uint buf_uint;
	//clGetDeviceInfo(device_list[0], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(buf_uint), &buf_uint, NULL);
	//clGetDeviceInfo(device_list[0], CL_DEVICE_MAX_COMPUTE_UNITS, 4, &buf_uint, &koushik);
	//printf("KOUSHIK FUN:  DEVICE_MAX_COMPUTE_UNITS = %u\n", (unsigned int)buf_uint);
	
    // Create memory buffers on the device for each vector
    cl_mem A_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY,
            MATRIX_WIDTH*MATRIX_HEIGHT * sizeof(float), NULL, &clStatus);
    cl_mem B_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY,
            MATRIX_WIDTH*MATRIX_HEIGHT * sizeof(float), NULL, &clStatus);
    cl_mem C_clmem = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
            MATRIX_WIDTH*MATRIX_HEIGHT * sizeof(float), NULL, &clStatus);

    // Copy the Buffer A and B to the device
    clStatus = clEnqueueWriteBuffer(command_queue, A_clmem, CL_TRUE, 0,
            MATRIX_WIDTH*MATRIX_HEIGHT * sizeof(float), A, 0, NULL, NULL);
    clStatus = clEnqueueWriteBuffer(command_queue, B_clmem, CL_TRUE, 0,
            MATRIX_WIDTH*MATRIX_HEIGHT * sizeof(float), B, 0, NULL, NULL);

    // Create a program from the kernel source
    cl_program program = clCreateProgramWithSource(context, 1,
#ifdef ENABLE_LOCAL_A
            (const char **)&MatrixMul_kernel_localA, NULL, &clStatus);
            printf("\n ENABLE_LOCAL_A \n");
#endif
#ifdef ENABLE_LOCAL_A_COALLESCED
            (const char **)&MatrixMul_kernel_localA_coallesced, NULL, &clStatus);
            printf("\n ENABLE_LOCAL_A_COALLESCED \n");
#endif
#ifdef ENABLE_LOCAL_A_COALLESCED_ROW
            (const char **)&MatrixMul_kernel_localA_coallesced_row, NULL, &clStatus);
            printf("\n ENABLE_LOCAL_A_COALLESCED_ROW \n");
#endif
#ifdef ENABLE_COALLESCED_ROW
            (const char **)&MatrixMul_kernel_coallesced_row, NULL, &clStatus);
            printf("\n ENABLE_COALLESCED_ROW \n");
#endif
#ifdef ENABLE_BASIC
            (const char **)&MatrixMul_kernel_basic, NULL, &clStatus);
            printf("\n ENABLE_BASIC \n");
#endif
#ifdef ENABLE_ROW_PER_WI
            (const char **)&MatrixMul_kernel_RowPerWI, NULL, &clStatus);
            printf("\n ENABLE_ROW_PER_WI \n");
#endif
#ifdef ENABLE_ROW_PER_WI_A_PRIVATE
            (const char **)&MatrixMul_kernel_RowPerWI_APriv, NULL, &clStatus);
            printf("\n ENABLE_ROW_PER_WI_A_PRIVATE \n");
#endif
#ifdef ENABLE_ROW_PER_WI_A_PRIVATE_B_LOCAL
            (const char **)&MatrixMul_kernel_RowPerWI_APriv_BLocal, NULL, &clStatus);
            printf("\n ENABLE_ROW_PER_WI_A_PRIVATE_B_LOCAL \n");
#endif

    LOG_OCL_ERROR(clStatus, "clCreateProgramWithSource Failed" );

    // Build the program
    clStatus = clBuildProgram(program, 1, device_list, NULL, NULL, NULL);
    LOG_OCL_ERROR(clStatus, "clBuildProgram Failed" );
    if(clStatus != CL_SUCCESS)
    {
        if(clStatus == CL_BUILD_PROGRAM_FAILURE)
            LOG_OCL_COMPILER_ERROR(program, device_list[0]);
        LOG_OCL_ERROR(clStatus, "clBuildProgram Failed" );
    }


    // Create the OpenCL kernel
#ifdef ENABLE_LOCAL_A
    cl_kernel kernel = clCreateKernel(program, "MatrixMul_kernel_localA", &clStatus);
#endif
#ifdef ENABLE_LOCAL_A_COALLESCED
    cl_kernel kernel = clCreateKernel(program, "MatrixMul_kernel_localA_coallesced", &clStatus);
#endif
#ifdef ENABLE_LOCAL_A_COALLESCED_ROW
    cl_kernel kernel = clCreateKernel(program, "MatrixMul_kernel_localA_coallesced_row", &clStatus);
#endif
#ifdef ENABLE_COALLESCED_ROW
    cl_kernel kernel = clCreateKernel(program, "MatrixMul_kernel_coallesced_row", &clStatus);
#endif
#ifdef ENABLE_BASIC
    cl_kernel kernel = clCreateKernel(program, "MatrixMul_kernel_basic", &clStatus);
#endif
#ifdef ENABLE_ROW_PER_WI
    cl_kernel kernel = clCreateKernel(program, "MatrixMul_kernel_RowPerWI", &clStatus);
#endif    // Set the arguments of the kernel
#ifdef ENABLE_ROW_PER_WI_A_PRIVATE
    cl_kernel kernel = clCreateKernel(program, "MatrixMul_kernel_RowPerWI_APriv", &clStatus);
#endif
#ifdef ENABLE_ROW_PER_WI_A_PRIVATE_B_LOCAL
    cl_kernel kernel = clCreateKernel(program, "MatrixMul_kernel_RowPerWI_APriv_BLocal", &clStatus);
#endif
    clStatus = clSetKernelArg(kernel, 0, sizeof(float), (void *)&alpha);
    clStatus = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&A_clmem);
    clStatus = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&B_clmem);
    clStatus = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&C_clmem);
#ifdef ENABLE_LOCAL_A
    clStatus = clSetKernelArg(kernel, 4, MATRIX_WIDTH*sizeof(float), NULL);
#endif
#ifdef ENABLE_LOCAL_A_COALLESCED
    clStatus = clSetKernelArg(kernel, 4, MATRIX_WIDTH*sizeof(float), NULL);
#endif
#ifdef ENABLE_LOCAL_A_COALLESCED_ROW
    clStatus = clSetKernelArg(kernel, 4, MATRIX_WIDTH*sizeof(float), NULL);
#endif
#ifdef ENABLE_ROW_PER_WI_A_PRIVATE_B_LOCAL
    clStatus = clSetKernelArg(kernel, 4, MATRIX_HEIGHT*sizeof(float), NULL);
#endif

    LOG_OCL_ERROR(clStatus, "clSetKernelArg Failed" );
	
    // Execute the OpenCL kernel on the list
    size_t global_size[2];    size_t local_size[2];

#ifdef ENABLE_LOCAL_A 
    global_size[0] = MATRIX_WIDTH;   global_size[1] = MATRIX_HEIGHT;
    local_size[0] =  256;	local_size[1] =  1;
#endif
#ifdef ENABLE_LOCAL_A_COALLESCED
    global_size[0] = MATRIX_WIDTH;   global_size[1] = MATRIX_HEIGHT;
	local_size[0] =  128;   local_size[0] =  256; local_size[1] =  1;
#endif
#ifdef ENABLE_LOCAL_A_COALLESCED_ROW
    global_size[0] = 128; global_size[1] = MATRIX_HEIGHT;
    local_size[0] =  128; local_size[1]  = 1;
#endif
#ifdef ENABLE_COALLESCED_ROW
    global_size[0] = 128;   global_size[1] = MATRIX_HEIGHT;
    local_size[0] =  128; local_size[1] =  1;
#endif
#ifdef ENABLE_BASIC 
    global_size[0] = MATRIX_WIDTH;   global_size[1] = MATRIX_HEIGHT;
	local_size[0] =  BLOCK_SIZE;
    local_size[0] =  BLOCK_SIZE*2;
	local_size[1] =  BLOCK_SIZE;
    local_size[1] =  BLOCK_SIZE*2;
#endif
#ifdef ENABLE_ROW_PER_WI
    global_size[0] = MATRIX_HEIGHT;
    local_size[0] =  128;//for size of 512
#endif
#ifdef ENABLE_ROW_PER_WI_A_PRIVATE
    global_size[0] = MATRIX_HEIGHT;
    local_size[0] =  128;//for size of 512
#endif
#ifdef ENABLE_ROW_PER_WI_A_PRIVATE_B_LOCAL
    global_size[0] = MATRIX_HEIGHT;
    local_size[0] =  128;//for size of 512
#endif
    printf("Running for GLobal = %d %d, Local = %d %d\n",global_size[0], global_size[1], local_size[0], local_size[1]);
#ifdef ENABLE_ROW_PER_WI_A_PRIVATE //ENABLE_ROW_PER_WI_A_PRIVATE_B_LOCAL // ENABLE_ROW_PER_WI_A_PRIVATE // ENABLE_ROW_PER_WI
    clStatus = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
            global_size, local_size, 0, NULL, &events);
#else
    clStatus = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL,
            global_size, local_size, 0, NULL, &events);
#endif
    LOG_OCL_ERROR(clStatus, "clEnqueueNDRangeKernel Failed" );
	//if(CL_SUCCESS != clStatus) printf("\n clEnqueueNDRangeKernel Failed -- %d", clStatus);
	clWaitForEvents(1, &events);

    cl_ulong startTime;
    cl_ulong endTime;
    
    /* Get kernel profiling info */
    clStatus = clGetEventProfilingInfo(events,
                                     CL_PROFILING_COMMAND_START,
                                     sizeof(cl_ulong),
                                     &startTime,
                                     0);
	if(CL_SUCCESS != clStatus) printf("\nclGetEventProfilingInfo Failed -- %d", clStatus);

    clStatus = clGetEventProfilingInfo(events,
                                     CL_PROFILING_COMMAND_END,
                                     sizeof(cl_ulong),
                                     &endTime,
                                     0);
	if(CL_SUCCESS != clStatus) printf("\nclGetEventProfilingInfo Failed -- %d", clStatus);
    double sec = 1e-9 * (endTime - startTime);

    // Read the memory buffer C_clmem on the device to the local variable C
    clStatus = clEnqueueReadBuffer(command_queue, C_clmem, CL_TRUE, 0,
            MATRIX_WIDTH*MATRIX_HEIGHT * sizeof(float), C, 0, NULL, NULL);
	LOG_OCL_ERROR(clStatus, "clEnqueueReadBuffer Failed" );
    // Clean up and wait for all the comands to complete.
    //clStatus = clFlush(command_queue);
    clStatus = clFinish(command_queue);

	printf("\n Kernel1................................................\n");
	printf("\n Time to execute Kernel=%f ms", sec * 1000);
    // Display the result to the screen
    /*for(i = 0; i < VECTOR_SIZE; i++)
        printf("\n%d-th Elements  A=%f B=%f C=%f", i, A[i], B[i], C[i]);*/
	if(verify)
    {
	    if(resultIsCorrect(A,B,C,MATRIX_WIDTH))
	    {
		    printf("\nKernel 1 - PASSED");
	    }
	    else
	    {
		    printf("\nKernel 1 - FAILED");
	    }
    }
    // Finally release all OpenCL allocated objects and host buffers.
    clStatus = clReleaseKernel(kernel);
    clStatus = clReleaseProgram(program);
    clStatus = clReleaseMemObject(A_clmem);
    clStatus = clReleaseMemObject(B_clmem);
    clStatus = clReleaseMemObject(C_clmem);
    clStatus = clReleaseCommandQueue(command_queue);
    clStatus = clReleaseContext(context);
    free(A);
    free(B);
    free(C);
    free(platforms);
    free(device_list);
    return CL_SUCCESS;
}

#if 0
void callMatrixMult2()
{
	cl_event events;
    int i;
    // Allocate space for vectors A, B and C
    int alpha = 32;
    float *A = (float*)malloc(sizeof(float)*VECTOR_SIZE);
    float *B = (float*)malloc(sizeof(float)*VECTOR_SIZE);
    float *C = (float*)malloc(sizeof(float)*VECTOR_SIZE);
    for(i = 0; i < VECTOR_SIZE; i++)
    {
        A[i] = (float)i;
        B[i] = (float)(VECTOR_SIZE - i);
        C[i] = 0;
    }

    // Get platform and device information
    cl_platform_id * platforms = NULL;
    cl_uint     num_platforms;
    //Set up the Platform
    cl_int clStatus = clGetPlatformIDs(0, NULL, &num_platforms);
    platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id)*num_platforms);
    clStatus = clGetPlatformIDs(num_platforms, platforms, NULL);

    //Get the devices list and choose the type of device you want to run on
    cl_device_id     *device_list = NULL;
    cl_uint       num_devices;

    clStatus = clGetDeviceIDs( platforms[0], CL_DEVICE_TYPE_GPU, 0,
            NULL, &num_devices);
    device_list = (cl_device_id *)malloc(sizeof(cl_device_id)*num_devices);
    clStatus = clGetDeviceIDs( platforms[0], CL_DEVICE_TYPE_GPU, num_devices,
            device_list, NULL);

    // Create one OpenCL context for each device in the platform
    cl_context context;
    cl_context_properties props[3] =
    {
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)platforms,
        0
    };
    context = clCreateContext( NULL, num_devices, device_list, NULL, NULL, &clStatus);

    // Create a command queue
    cl_command_queue_properties prop = 0;
    prop |= CL_QUEUE_PROFILING_ENABLE;
    cl_command_queue command_queue = clCreateCommandQueue(context, device_list[0], prop, &clStatus);

    // Create memory buffers on the device for each vector
    cl_mem A_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY,
            VECTOR_SIZE * sizeof(float), NULL, &clStatus);
    cl_mem B_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY,
            VECTOR_SIZE * sizeof(float), NULL, &clStatus);
    cl_mem C_clmem = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
            VECTOR_SIZE * sizeof(float), NULL, &clStatus);

    // Copy the Buffer A and B to the device
    clStatus = clEnqueueWriteBuffer(command_queue, A_clmem, CL_TRUE, 0,
            VECTOR_SIZE * sizeof(float), A, 0, NULL, NULL);
    clStatus = clEnqueueWriteBuffer(command_queue, B_clmem, CL_TRUE, 0,
            VECTOR_SIZE * sizeof(float), B, 0, NULL, NULL);

    // Create a program from the kernel source 
    cl_program program = clCreateProgramWithSource(context, 1,
            (const char **)&MatrixMul_kernel400, NULL, &clStatus);

    // Build the program
    clStatus = clBuildProgram(program, 1, device_list, NULL, NULL, NULL); 
	if(CL_SUCCESS != clStatus)
	{
		printf("KOUSHIK Kerne200 failed1 %d",clStatus);
	}
    // Create the OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "MatrixMul_kernel400", &clStatus);
	if(CL_SUCCESS != clStatus)
	{
		printf("KOUSHIK Kerne200 failed2 %d", clStatus);
	}
    // Set the arguments of the kernel
    clStatus = clSetKernelArg(kernel, 0, sizeof(float), (void *)&alpha);
    clStatus = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&A_clmem);
    clStatus = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&B_clmem);
    clStatus = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&C_clmem);


    // Execute the OpenCL kernel on the list
    size_t global_size;
    size_t local_size;
	global_size = MATRIX_DIM;
	local_size =  BLOCK_SIZE;//BHATTACHARYYA

    clStatus = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
            &global_size, &local_size, 0, NULL, &events);
	
	clWaitForEvents(1, &events);

    cl_ulong startTime;
    cl_ulong endTime;
    
    /* Get kernel profiling info */
    clStatus = clGetEventProfilingInfo(events,
                                     CL_PROFILING_COMMAND_START,
                                     sizeof(cl_ulong),
                                     &startTime,
                                     0);

	if(CL_SUCCESS != clStatus) printf("\nKoushik11 %d", clStatus); 
    clStatus = clGetEventProfilingInfo(events,
                                     CL_PROFILING_COMMAND_END,
                                     sizeof(cl_ulong),
                                     &endTime,
                                     0);
	if(CL_SUCCESS != clStatus) printf("\nKoushik22 %d", clStatus); 
    double sec = 1e-9 * (endTime - startTime);

    // Read the memory buffer C_clmem on the device to the local variable C
    clStatus = clEnqueueReadBuffer(command_queue, C_clmem, CL_TRUE, 0,
            VECTOR_SIZE * sizeof(float), C, 0, NULL, NULL);
	if(CL_SUCCESS != clStatus) printf("Koushik33"); 
    // Clean up and wait for all the comands to complete.
    clStatus = clFlush(command_queue);
    clStatus = clFinish(command_queue);

	printf("\n Kernel1................................................\n");
	printf("\n Time to execute Kernel=%f ms", sec * 1000);
    // Display the result to the screen
    for(i = 0; i < VECTOR_SIZE; i++)
        printf("\n%d-th Elements  A=%f B=%f C=%f", i, A[i], B[i], C[i]);
	
	if(resultIsCorrect(A,B,C,MATRIX_DIM))
	{
		printf("\nKernel 200 is OK");
	}
    // Finally release all OpenCL allocated objects and host buffers.
    clStatus = clReleaseKernel(kernel);
    clStatus = clReleaseProgram(program);
    clStatus = clReleaseMemObject(A_clmem);
    clStatus = clReleaseMemObject(B_clmem);
    clStatus = clReleaseMemObject(C_clmem);
    clStatus = clReleaseCommandQueue(command_queue);
    clStatus = clReleaseContext(context);
    free(A);
    free(B);
    free(C);
    free(platforms);
    free(device_list);
}

void matrixMultSimpleKernel()
{
    int i;
	//timeStamps[0] = getTime();

    // Allocate space for vectors A, B and C
    //int dimOfSq = 32;
	int dimOfSq = 32;
    float *A = (float*)malloc(sizeof(float)*VECTOR_SIZE);
    float *B = (float*)malloc(sizeof(float)*VECTOR_SIZE);
    float *C = (float*)malloc(sizeof(float)*VECTOR_SIZE);
    for(i = 0; i < VECTOR_SIZE; i++)
    {
        A[i] = (float)i;
        B[i] = (float)(VECTOR_SIZE - i);
        C[i] = 1;
    }

    // Get platform and device information
    cl_platform_id * platforms = NULL;
    cl_uint     num_platforms;
    //Set up the Platform
    cl_int clStatus = clGetPlatformIDs(0, NULL, &num_platforms);
    platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id)*num_platforms);
    clStatus = clGetPlatformIDs(num_platforms, platforms, NULL);

    //Get the devices list and choose the type of device you want to run on
    cl_device_id     *device_list = NULL;
    cl_uint       num_devices;

    clStatus = clGetDeviceIDs( platforms[0], CL_DEVICE_TYPE_GPU, 0,
            NULL, &num_devices);
    device_list = (cl_device_id *)malloc(sizeof(cl_device_id)*num_devices);
    clStatus = clGetDeviceIDs( platforms[0], CL_DEVICE_TYPE_GPU, num_devices,
            device_list, NULL);

    // Create one OpenCL context for each device in the platform
    cl_context context;
    cl_context_properties props[3] =
    {
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)platforms,
        0
    };
    context = clCreateContext( NULL, num_devices, device_list, NULL, NULL, &clStatus);

    // Create a command queue
    cl_command_queue command_queue = clCreateCommandQueue(context, device_list[0], 0, &clStatus);

    // Create memory buffers on the device for each vector
    cl_mem A_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY,
            VECTOR_SIZE * sizeof(float), NULL, &clStatus);
    cl_mem B_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY,
            VECTOR_SIZE * sizeof(float), NULL, &clStatus);
    cl_mem C_clmem = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
            VECTOR_SIZE * sizeof(float), NULL, &clStatus);

    // Copy the Buffer A and B to the device
    clStatus = clEnqueueWriteBuffer(command_queue, A_clmem, CL_TRUE, 0,
            VECTOR_SIZE * sizeof(float), A, 0, NULL, NULL);
    clStatus = clEnqueueWriteBuffer(command_queue, B_clmem, CL_TRUE, 0,
            VECTOR_SIZE * sizeof(float), B, 0, NULL, NULL);

    // Create a program from the kernel source
    cl_program program = clCreateProgramWithSource(context, 1,
            (const char **)&MatrixMul_kernel1, NULL, &clStatus);
    if(CL_SUCCESS != clStatus) printf("Koushik1"); 
    // Build the program
    clStatus = clBuildProgram(program, 1, device_list, NULL, NULL, NULL);
    if(CL_SUCCESS != clStatus) printf("Koushik2"); 
    // Create the OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "MatrixMul_kernel1", &clStatus);
    if(CL_SUCCESS != clStatus) printf("Koushik3"); 
    // Set the arguments of the kernel
    clStatus = clSetKernelArg(kernel, 0, sizeof(int),    (void *)&dimOfSq);
    clStatus = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&A_clmem);
    clStatus = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&B_clmem);
    clStatus = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&C_clmem);
    if(CL_SUCCESS != clStatus) printf("Koushik4"); 
    // Execute the OpenCL kernel on the list
    size_t global_size = VECTOR_SIZE; // Process the entire lists
    size_t local_size = 32;           // Process one item at a time
    clStatus = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
            &global_size, &local_size, 0, NULL, NULL);

    // Read the memory buffer C_clmem on the device to the local variable C
    clStatus = clEnqueueReadBuffer(command_queue, C_clmem, CL_TRUE, 0,
            VECTOR_SIZE * sizeof(float), C, 0, NULL, NULL);
	if(CL_SUCCESS != clStatus) printf("Koushik5"); 
    // Clean up and wait for all the comands to complete.
    clStatus = clFlush(command_queue);
    clStatus = clFinish(command_queue);

    // Display the result to the screen
    for(i = 0; i < VECTOR_SIZE; i++)
        printf("dim=%d i=%d  A=%f  B=%f  C=%f\n", dimOfSq, i, A[i], B[i], C[i]);

	if(resultIsCorrect(A,B,C,32))
	{
		printf("\nKernel 1 is OK");
	}
	
    // Finally release all OpenCL allocated objects and host buffers.
    clStatus = clReleaseKernel(kernel);
    clStatus = clReleaseProgram(program);
    clStatus = clReleaseMemObject(A_clmem);
    clStatus = clReleaseMemObject(B_clmem);
    clStatus = clReleaseMemObject(C_clmem);
    clStatus = clReleaseCommandQueue(command_queue);
    clStatus = clReleaseContext(context);
    free(A);
    free(B);
    free(C);
    free(platforms);
    free(device_list);
}



void matrixMultWithLocal()
{
    int i;
	//timeStamps[0] = getTime();

    // Allocate space for vectors A, B and C
    int dimOfSq = 32;
    float *A = (float*)malloc(sizeof(float)*VECTOR_SIZE);
    float *B = (float*)malloc(sizeof(float)*VECTOR_SIZE);
    float *C = (float*)malloc(sizeof(float)*VECTOR_SIZE);
    for(i = 0; i < VECTOR_SIZE; i++)
    {
        A[i] = (float)i;
        B[i] = (float)(VECTOR_SIZE - i);
        C[i] = 0;
    }

    // Get platform and device information
    cl_platform_id * platforms = NULL;
    cl_uint     num_platforms;
    //Set up the Platform
    cl_int clStatus = clGetPlatformIDs(0, NULL, &num_platforms);
    platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id)*num_platforms);
    clStatus = clGetPlatformIDs(num_platforms, platforms, NULL);

    //Get the devices list and choose the type of device you want to run on
    cl_device_id     *device_list = NULL;
    cl_uint       num_devices;

    clStatus = clGetDeviceIDs( platforms[0], CL_DEVICE_TYPE_GPU, 0,
            NULL, &num_devices);
    device_list = (cl_device_id *)malloc(sizeof(cl_device_id)*num_devices);
    clStatus = clGetDeviceIDs( platforms[0], CL_DEVICE_TYPE_GPU, num_devices,
            device_list, NULL);

    // Create one OpenCL context for each device in the platform
    cl_context context;
    cl_context_properties props[3] =
    {
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)platforms,
        0
    };
    context = clCreateContext( NULL, num_devices, device_list, NULL, NULL, &clStatus);

    // Create a command queue
    cl_command_queue command_queue = clCreateCommandQueue(context, device_list[0], 0, &clStatus);

    // Create memory buffers on the device for each vector
    cl_mem A_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY,
            VECTOR_SIZE * sizeof(float), NULL, &clStatus);
    cl_mem B_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY,
            VECTOR_SIZE * sizeof(float), NULL, &clStatus);
    cl_mem C_clmem = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
            VECTOR_SIZE * sizeof(float), NULL, &clStatus);

    // Copy the Buffer A and B to the device
    clStatus = clEnqueueWriteBuffer(command_queue, A_clmem, CL_TRUE, 0,
            VECTOR_SIZE * sizeof(float), A, 0, NULL, NULL);
    clStatus = clEnqueueWriteBuffer(command_queue, B_clmem, CL_TRUE, 0,
            VECTOR_SIZE * sizeof(float), B, 0, NULL, NULL);

    // Create a program from the kernel source
    cl_program program = clCreateProgramWithSource(context, 1,
            (const char **)&MatrixMul_kernel2, NULL, &clStatus);

    // Build the program
    clStatus = clBuildProgram(program, 1, device_list, NULL, NULL, NULL);

    // Create the OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "MatrixMul_kernel2", &clStatus);

    // Set the arguments of the kernel
    clStatus = clSetKernelArg(kernel, 0, sizeof(int),    (void *)&dimOfSq);
    clStatus = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&A_clmem);
    clStatus = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&B_clmem);
    clStatus = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&C_clmem);

	//Koushik DOne upto
    // Execute the OpenCL kernel on the list
    size_t global_size = 32*32;
    size_t local_size = BLOCK_SIZE;   // BLOCK_SIZE is factor of 32
    clStatus = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
            &global_size, &local_size, 0, NULL, NULL);

    // Read the memory buffer C_clmem on the device to the local variable C
    clStatus = clEnqueueReadBuffer(command_queue, C_clmem, CL_TRUE, 0,
            VECTOR_SIZE * sizeof(float), C, 0, NULL, NULL);

    // Clean up and wait for all the comands to complete.
    clStatus = clFlush(command_queue);
    clStatus = clFinish(command_queue);

    // Display the result to the screen
	printf("\n\n MATRIX MULTIPLICATION WITH LOCAL.......\n");

    for(i = 0; i < VECTOR_SIZE; i++)
        printf("Dim= %f  A=%f  B=%f  C=%f\n", dimOfSq, A[i], B[i], C[i]);
	if(resultIsCorrect(A,B,C,32))
	{
		printf("\nKernel 2 is OK");
	}
	
	getchar();

    // Finally release all OpenCL allocated objects and host buffers.
    clStatus = clReleaseKernel(kernel);
    clStatus = clReleaseProgram(program);
    clStatus = clReleaseMemObject(A_clmem);
    clStatus = clReleaseMemObject(B_clmem);
    clStatus = clReleaseMemObject(C_clmem);
    clStatus = clReleaseCommandQueue(command_queue);
    clStatus = clReleaseContext(context);
    free(A);
    free(B);
    free(C);
    free(platforms);
    free(device_list);
}

void matrixMultWithLocal2()
{
    int i;
	//timeStamps[0] = getTime();

    // Allocate space for vectors A, B and C
    int dimOfSq = 32;
    float *A = (float*)malloc(sizeof(float)*VECTOR_SIZE);
    float *B = (float*)malloc(sizeof(float)*VECTOR_SIZE);
    float *C = (float*)malloc(sizeof(float)*VECTOR_SIZE);
    for(i = 0; i < VECTOR_SIZE; i++)
    {
        A[i] = (float)i;
        B[i] = (float)(VECTOR_SIZE - i);
        C[i] = 1;
    }

    // Get platform and device information
    cl_platform_id * platforms = NULL;
    cl_uint     num_platforms;
    //Set up the Platform
    cl_int clStatus = clGetPlatformIDs(0, NULL, &num_platforms);
    platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id)*num_platforms);
    clStatus = clGetPlatformIDs(num_platforms, platforms, NULL);

    //Get the devices list and choose the type of device you want to run on
    cl_device_id     *device_list = NULL;
    cl_uint       num_devices;

    clStatus = clGetDeviceIDs( platforms[0], CL_DEVICE_TYPE_GPU, 0,
            NULL, &num_devices);
    device_list = (cl_device_id *)malloc(sizeof(cl_device_id)*num_devices);
    clStatus = clGetDeviceIDs( platforms[0], CL_DEVICE_TYPE_GPU, num_devices,
            device_list, NULL);

    // Create one OpenCL context for each device in the platform
    cl_context context;
    cl_context_properties props[3] =
    {
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)platforms,
        0
    };
    context = clCreateContext( NULL, num_devices, device_list, NULL, NULL, &clStatus);

    // Create a command queue
    cl_command_queue command_queue = clCreateCommandQueue(context, device_list[0], 0, &clStatus);

    // Create memory buffers on the device for each vector
    cl_mem A_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY,
            VECTOR_SIZE * sizeof(float), NULL, &clStatus);
    cl_mem B_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY,
            VECTOR_SIZE * sizeof(float), NULL, &clStatus);
    cl_mem C_clmem = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
            VECTOR_SIZE * sizeof(float), NULL, &clStatus);

    // Copy the Buffer A and B to the device
    clStatus = clEnqueueWriteBuffer(command_queue, A_clmem, CL_TRUE, 0,
            VECTOR_SIZE * sizeof(float), A, 0, NULL, NULL);
    clStatus = clEnqueueWriteBuffer(command_queue, B_clmem, CL_TRUE, 0,
            VECTOR_SIZE * sizeof(float), B, 0, NULL, NULL);

    // Create a program from the kernel source
    cl_program program = clCreateProgramWithSource(context, 1,
            (const char **)&MatrixMul_kernel1, NULL, &clStatus);

    // Build the program
    clStatus = clBuildProgram(program, 1, device_list, NULL, NULL, NULL);

    // Create the OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "MatrixMul_kernel1", &clStatus);

    // Set the arguments of the kernel
    clStatus = clSetKernelArg(kernel, 0, sizeof(int),    (void *)&dimOfSq);
    clStatus = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&A_clmem);
    clStatus = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&B_clmem);
    clStatus = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&C_clmem);

	//Koushik DOne upto
    // Execute the OpenCL kernel on the list
    size_t global_size = 32*32;
    size_t local_size = BLOCK_SIZE;   // BLOCK_SIZE is factor of 32
    clStatus = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
            &global_size, &local_size, 0, NULL, NULL);

    // Read the memory buffer C_clmem on the device to the local variable C
    clStatus = clEnqueueReadBuffer(command_queue, C_clmem, CL_TRUE, 0,
            VECTOR_SIZE * sizeof(float), C, 0, NULL, NULL);

    // Clean up and wait for all the comands to complete.
    clStatus = clFlush(command_queue);
    clStatus = clFinish(command_queue);

    // Display the result to the screen
	printf("\n\n MATRIX MULTIPLICATION WITH LOCAL.......\n");

    for(i = 0; i < VECTOR_SIZE; i++)
        printf("Dim= %d  A=%f  B=%f  C=%f\n", dimOfSq, A[i], B[i], C[i]);
	if(resultIsCorrect(A,B,C,32))
	{
		printf("\nKernel 2 is OK");
	}
	
	getchar();

    // Finally release all OpenCL allocated objects and host buffers.
    clStatus = clReleaseKernel(kernel);
    clStatus = clReleaseProgram(program);
    clStatus = clReleaseMemObject(A_clmem);
    clStatus = clReleaseMemObject(B_clmem);
    clStatus = clReleaseMemObject(C_clmem);
    clStatus = clReleaseCommandQueue(command_queue);
    clStatus = clReleaseContext(context);
    free(A);
    free(B);
    free(C);
    free(platforms);
    free(device_list);
}
#endif
int main(int argc, char** argv) 
{
    int MATRIX_WIDTH = atoi(argv[1]);

    bool verify = true;
	callMatrixMult1(MATRIX_WIDTH, MATRIX_WIDTH, verify);
	//callMatrixMult2();
	//matrixMultWithLocal2();
	//matrixMultWithLocal();
	//getchar();
    return 0;
}
bool resultIsCorrect(float* pA,float* pB,float* pCTest, size_t dim)
{
	bool result = true;
	long long int arrayLength = dim*dim;
	float *pGoldenValue = (float *)malloc(sizeof(float)*arrayLength);
	//for(long long int i=0; i<arrayLength;++i)
	//{
	//	printf("Koushik %");
	//}
	//compute the values
	printf ("Computing Golden");
	for(int i=0; i<dim; ++i)
	{
		for(int j=0; j<dim; ++j)
		{
			//init the (i,j)-th element to 0
			pGoldenValue[i*dim+j] = 0;
			//compute the (i,j)-th element
			for(int k=0; k<dim; ++k)
			{
				pGoldenValue[i*dim + j] += pA[i*dim + k]*pB[k*dim + j];
			}
		}
		printf (".");
	}
	int errorCount = 10;
	for(long long int i=0; i<arrayLength && errorCount;++i)
	{
		if(pGoldenValue[i] != pCTest[i] )
		{
			errorCount--;
			result =false;
			printf("\n%d-th elements A=%f B=%f are %f and %f",i, pA[i], pB[i],pGoldenValue[i], pCTest[i] );
		}
	}

	free(pGoldenValue);

	return result;
}


void StartCounter()
{
    LARGE_INTEGER li;
    if(!QueryPerformanceFrequency(&li))
    std::cout << "QueryPerformanceFrequency failed!\n";

    PCFreq = double(li.QuadPart)/1000.0;

    QueryPerformanceCounter(&li);
    CounterStart = li.QuadPart;
}