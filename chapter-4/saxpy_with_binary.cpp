//If you want to build the file directly at the command prompt then use the following commands. 
//AMD commands
//cl /c Example1_SAXPY.cpp /I"%AMDAPPSDKROOT%\include"
//link  /OUT:"Example.exe" "%AMDAPPSDKROOT%\lib\x86_64\OpenCL.lib" Example1_SAXPY.obj
//nVIDIA commands
//cl /c Example1_SAXPY.cpp /I"%NVSDKCOMPUTE_ROOT%\OpenCL\common\inc"
//link  /OUT:"Example.exe" "%NVSDKCOMPUTE_ROOT%\OpenCL\common\lib\x64\OpenCL.lib" Example1_SAXPY.obj


#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

#define VECTOR_SIZE 1024

int main(void) {
    int i;
	FILE *fp;
    float alpha = 2.0;
	// Allocate space for vectors A, B and C
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
    context = clCreateContext( NULL, num_devices, 
		                       device_list, NULL, 
							   NULL, &clStatus);
    // Create a command queue
    cl_command_queue command_queue = clCreateCommandQueue(context, 
		                                device_list[0], 0, &clStatus);

    cl_program program = clCreateProgramWithBuiltInKernels (context, num_devices, device_list, kernel_names, errcode_ret);

    // Clean up and wait for all the comands to complete.
    clStatus = clFlush(command_queue);
    clStatus = clFinish(command_queue);




    clStatus = clReleaseCommandQueue(command_queue);
    clStatus = clReleaseContext(context);
    free(A);
    free(B);
    free(C);
    free(platforms);
    free(device_list);

    return 0;
}