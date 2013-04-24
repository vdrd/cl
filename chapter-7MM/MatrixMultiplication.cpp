//If you want to build the file directly at the command prompt then use the following commands. 
//AMD commands
//cl /c Example1_SAXPY.cpp /I"%AMDAPPSDKROOT%\include"
//link  /OUT:"Example.exe" "%AMDAPPSDKROOT%\lib\x86_64\OpenCL.lib" Example1_SAXPY.obj
//nVIDIA commands
//cl /c Example1_SAXPY.cpp /I"%NVSDKCOMPUTE_ROOT%\OpenCL\common\inc"
//link  /OUT:"Example.exe" "%NVSDKCOMPUTE_ROOT%\OpenCL\common\lib\x64\OpenCL.lib" Example1_SAXPY.obj

#include <windows.h>//Koushik


#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <CL/cl.h>
//32 by 32 matrix 
#define VECTOR_SIZE 1024
#define BLOCK_SIZE 8

__int64 CounterStart = 0;
double PCFreq = 0.0;
void StartCounter();

//OpenCL kernel which is run for every work item created.
const char *MatrixMul_kernel1 =
"__kernel                                   \n"
"void MatrixMul_kernel1(__global int *q,    \n"
"                  __global float *A,       \n"
"                  __global float *B,       \n"
"                  __global float *C)       \n"
"{                                          \n"
"    //Get the index of the work-item       \n"
"    int dim = q[0];                        \n"
"    int indexRow = get_global_id(0);       \n"
"    int indexCol = get_global_id(1);       \n"
"    int indexRowSZ = get_global_size(0);   \n"
"    int indexColSZ = get_global_size(1);   \n"
"    C[indexRow + dim*indexCol] = 0;        \n"
"    for (int k = 0; k < dim; k++)          \n"
"    {                                      \n"
"        C[indexRow + dim*indexCol] +=      \n"
"        A[k + dim*indexRow] +              \n"
"        B[indexCol + dim*k];               \n"
"    }                                      \n"
"}                                          \n";

//OpenCL kernel with local memory.
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

void matrixMultWithLocal();
int main(void) {
	//Koushik
	const int noOfTimeStamps = 5;
	LARGE_INTEGER timeStamps[noOfTimeStamps];
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

    // Execute the OpenCL kernel on the list
    size_t global_size = VECTOR_SIZE; // Process the entire lists
    size_t local_size = 32;           // Process one item at a time
    clStatus = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
            &global_size, &local_size, 0, NULL, NULL);

    // Read the memory buffer C_clmem on the device to the local variable C
    clStatus = clEnqueueReadBuffer(command_queue, C_clmem, CL_TRUE, 0,
            VECTOR_SIZE * sizeof(float), C, 0, NULL, NULL);

    // Clean up and wait for all the comands to complete.
    clStatus = clFlush(command_queue);
    clStatus = clFinish(command_queue);

    // Display the result to the screen
    for(i = 0; i < VECTOR_SIZE; i++)
        printf("%f * %f + %f = %f\n", dimOfSq, A[i], B[i], C[i]);

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

	matrixMultWithLocal();
	int dummy;
	scanf("%d",&dummy);
    return 0;
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
    size_t global_size = VECTOR_SIZE; // 32*32
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
        printf("%f * %f + %f = %f\n", dimOfSq, A[i], B[i], C[i]);

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

