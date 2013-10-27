#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#ifndef WIN32
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),(mode)))==NULL
#endif

#define BINARY_BUFFER_SIZE 4096*10

void createBinary(const char * kernelCode, cl_context context)
{
    cl_int        clStatus = CL_SUCCESS;
    cl_device_id  *device_list = NULL;
    cl_uint       num_devices;
    size_t        param_value_size_ret = 0;
    size_t        bytes_read;
    char          kernelName[100];

    cl_program program = clCreateProgramWithSource(context, 1,
            (const char **)&kernelCode, NULL, &clStatus);
    // Build the program
    clStatus = clGetContextInfo(context,CL_CONTEXT_NUM_DEVICES,sizeof(num_devices),&num_devices,NULL);
    device_list = new cl_device_id[num_devices];
    clStatus = clGetContextInfo(context,CL_CONTEXT_DEVICES,num_devices*sizeof(cl_device_id),device_list,NULL);

    clStatus = clBuildProgram(program, num_devices, device_list, NULL, NULL, NULL);
    if(clStatus != CL_SUCCESS)
        std::cout << "Error # "<< clStatus<<":: clBuildProgram\n";

    //Get back the number of devices associated with the program object
    clStatus = clGetProgramInfo(program, CL_PROGRAM_NUM_DEVICES, 
                                sizeof(cl_uint), &num_devices, 
                                &bytes_read);
    size_t *binarySize = new size_t[num_devices];//Create size array
    clStatus = clGetProgramInfo(program, CL_PROGRAM_DEVICES,
                                sizeof(cl_device_id) * num_devices,
                                device_list, &bytes_read);
    //Load the size of each binary associated with the corresponding device
    clStatus = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, 
                                sizeof(size_t)*num_devices, 
                                binarySize, &bytes_read);
    char** programBin = new char* [num_devices];
    //Create the binary array
    for(cl_uint i = 0; i < num_devices; i++)
        programBin[i] = new char[binarySize[i]]; 
    //Read the Binary
    clStatus = clGetProgramInfo(program, CL_PROGRAM_BINARIES, 
                                sizeof(unsigned char *) * num_devices, 
                                programBin, &bytes_read); 
    
    //Get the name of the kernel
    clStatus = clGetProgramInfo(program, CL_PROGRAM_KERNEL_NAMES,
                                sizeof(kernelName), &kernelName,
                                &bytes_read); 
    std::cout << "Kernel name is \"" << kernelName << "\"" << std::endl;
    if(clStatus != CL_SUCCESS)
        std::cout << "Error # "<< clStatus<<":: clCreateProgramWithBinary\n";

    for(cl_uint i=0; i<num_devices; i++)
    {
        FILE *fp;
        // To find out which device the program binary in the array refers to, use
        // the CL_PROGRAM_DEVICES query to get the list of devices. There is a one-to-one
        // correspondence between the array of n pointers returned by CL_PROGRAM_BINARIES 
        // and array of devices returned by CL_PROGRAM_DEVICES
        cl_device_type device_type;
        clStatus = clGetDeviceInfo (device_list[i], CL_DEVICE_TYPE, sizeof(cl_device_type), &device_type, &bytes_read);
        if(device_type == CL_DEVICE_TYPE_CPU)
        {
            std::string kernelStr(kernelName);
            std::string kernelExtn("_binary_cpu.clbin");
            kernelStr += kernelExtn;
            std::cout << "Writing File for kernel \"" << kernelName << "\" with CPU device Type" << std::endl;
            fopen_s(&fp,kernelStr.c_str(), "wb");  
            fwrite(programBin[i], sizeof(char), binarySize[0], fp); // Save the binary, but my file stay empty
            std::cout << "File " << kernelStr.c_str() << " written"<< std::endl; 
            fclose(fp);
        }
        else if(device_type == CL_DEVICE_TYPE_GPU)
        {
            std::string kernelStr(kernelName);
            std::string kernelExtn("_binary_gpu.clbin");
            kernelStr += kernelExtn;
            std::cout << "Writing File for kernel \"" << kernelName << "\" with GPU device Type" << std::endl;
            fopen_s(&fp,kernelStr.c_str(), "wb");  
            fwrite(programBin[i], sizeof(char), binarySize[0], fp); // Save the binary, but my file stay empty  
            std::cout << "File " << kernelStr.c_str() << " written"<< std::endl; 
            fclose(fp);
        }
    }

    for(cl_uint i = 0; i < num_devices; i++)  
        delete(programBin[i]); 
    delete(programBin);  
    delete(binarySize);
    delete (device_list);
    clStatus = clReleaseProgram(program);
    if(clStatus != CL_SUCCESS)
        std::cout << "Error # "<< clStatus<<":: clReleaseProgram\n";

    return;
}

//OpenCL kernel which is run for every work item created.
const char *saxpy_kernel =
"__kernel                                   \n"
"void saxpy_kernel(float alpha,     \n"
"                  __global float *A,       \n"
"                  __global float *B,       \n"
"                  __global float *C)       \n"
"{                                          \n"
"    //Get the index of the work-item       \n"
"    int index = get_global_id(0);          \n"
"    C[index] = alpha* A[index] + B[index]; \n"
"}                                          \n";

int main(void) {
    // Get platform and device information
    cl_platform_id * platforms = NULL;
    cl_uint     num_platforms;
    //Set up the Platform
    cl_int clStatus = clGetPlatformIDs(0, NULL, &num_platforms);
    platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id)*num_platforms);
    clStatus = clGetPlatformIDs(num_platforms, platforms, NULL);
    
    //Get the devices list and choose the type of device you want to run on
    cl_device_id  *device_list = NULL;
    cl_uint       num_devices;

    clStatus = clGetDeviceIDs( platforms[0], CL_DEVICE_TYPE_ALL, 0,
            NULL, &num_devices);
    device_list = (cl_device_id *)malloc(sizeof(cl_device_id)*num_devices);
    clStatus = clGetDeviceIDs( platforms[0], CL_DEVICE_TYPE_ALL, num_devices,
            device_list, NULL);

    // Create one OpenCL context for each device in the platform
    cl_context context_cpu;
    cl_context context_gpu;
    cl_context context;
    cl_context_properties properties[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platforms[0], NULL};
    context_cpu = clCreateContextFromType( properties, CL_DEVICE_TYPE_CPU, NULL, NULL, &clStatus);
    context_gpu = clCreateContextFromType( properties, CL_DEVICE_TYPE_GPU, NULL, NULL, &clStatus);
    context = clCreateContext( NULL, num_devices, device_list, NULL, NULL, &clStatus);
    std::cout << "======Creating binary for all devices in the context======\n";
    createBinary(saxpy_kernel,context);
    std::cout << "\n\n======Creating binary for CPU device in the context======\n";
    createBinary(saxpy_kernel,context_cpu);
    std::cout << "\n\n======Creating binary for GPU device in the context======\n";
    createBinary(saxpy_kernel,context_gpu);
    clStatus = clReleaseContext(context);
    clStatus = clReleaseContext(context_cpu);
    clStatus = clReleaseContext(context_gpu);
    free(platforms);
    return 0;
}