#include <stdio.h>
#include <stdlib.h>
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#define VECTOR_SIZE 1024
typedef struct
{
   cl_float8 y;
   cl_float3 x;
} OpenCLStruct;

//OpenCL kernel which is run for every work item created.
const char *sizeof_kernel =
"typedef struct                             \n"
"{                                          \n"
"   float8 y;                               \n"
"   float3 x;                               \n"
"} OpenCLStruct;                            \n"
"__kernel                                   \n"
"void sizeof_kernel(  )                     \n"
"{                                          \n"
"    printf(\"The size of the OpenCLStruct provided by the OpenCL compiler is = %d bytes.\\n \",sizeof(OpenCLStruct));   \n"
"}                                          \n";

int main(void) {
    OpenCLStruct* oclStruct = (OpenCLStruct*)malloc(sizeof(OpenCLStruct)*VECTOR_SIZE);
    printf("The size of the OpenCLStruct provided by the host compiler is = %d bytes\n",sizeof(OpenCLStruct) );

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

    // Create a program from the kernel source
    cl_program program = clCreateProgramWithSource(context, 1,
            (const char **)&sizeof_kernel, NULL, &clStatus);

    // Build the program
    clStatus = clBuildProgram(program, 1, device_list, NULL, NULL, NULL);
    if(clStatus != CL_BUILD_SUCCESS)
    {
        printf("Program Build failed\n");
        size_t length;
        char buildLog[2048];
        clGetProgramBuildInfo(program, *device_list, CL_PROGRAM_BUILD_LOG, sizeof(buildLog), buildLog, &length);
        printf("--- Build log ---\n%s",buildLog);
        exit(1);
    }
    // Create the OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "sizeof_kernel", &clStatus);

    // Execute the OpenCL kernel. Lauch only one work item to see what is the sizeof OpenCLStruct
    size_t global_size = 1; // Process the entire lists
    size_t local_size  = 1; // Process one item at a time
    clStatus = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
            &global_size, &local_size, 0, NULL, NULL);

    // Clean up and wait for all the comands to complete.
    clStatus = clFlush(command_queue);
    clStatus = clFinish(command_queue);

    // Finally release all OpenCL allocated objects and host buffers.
    clStatus = clReleaseKernel(kernel);
    clStatus = clReleaseProgram(program);
    clStatus = clReleaseCommandQueue(command_queue);
    clStatus = clReleaseContext(context);
    free(platforms);
    free(device_list);

    return 0;
}