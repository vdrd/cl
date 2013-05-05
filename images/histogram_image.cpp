#include <cstdlib>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <CL/cl.h>
#include <bmp_image.h>

const char *histogram_kernel =
"#define BIN_SIZE 256                                                                  \n"
"#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable                       \n"
"__kernel                                                                              \n"
"void histogram_image_kernel(__read_only image2d_t data,                                      \n"
"                  __read_only image2d_t sharedArray,                                         \n"
"                  __global uint* binResultR,                                          \n"
"                  __global uint* binResultG,                                          \n"
"                  __global uint* binResultB)                                          \n"
"{                                                                                     \n"
"    size_t localId = get_local_id(0);                                                 \n"
"    size_t globalId = get_global_id(0);                                               \n"
"    size_t groupId = get_group_id(0);                                                 \n"
"    size_t groupSize = get_local_size(0);                                             \n"
"     __local uchar* sharedArrayR = sharedArray;                                       \n"
"     __local uchar* sharedArrayG = sharedArray + groupSize * BIN_SIZE;                \n"
"     __local uchar* sharedArrayB = sharedArray + 2 * groupSize * BIN_SIZE;            \n"
"     sampler_t smp = CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;                  \n"
"                                                                                      \n"
"    /* initialize shared array to zero */                                             \n"
"    for(int i = 0; i < BIN_SIZE; ++i)                                                 \n"
"    {                                                                                 \n"
"        sharedArrayR[localId * BIN_SIZE + i] = 0;                                     \n"
"        sharedArrayG[localId * BIN_SIZE + i] = 0;                                     \n"
"        sharedArrayB[localId * BIN_SIZE + i] = 0;                                     \n"
"    }                                                                                 \n"
"                                                                                      \n"
"    barrier(CLK_LOCAL_MEM_FENCE);                                                     \n"
"                                                                                      \n"
"    /* calculate thread-histograms */                                                 \n"
"    for(int i = 0; i < BIN_SIZE; ++i)                                                 \n"
"    {                                                                                 \n"
"        uint value = data[globalId * BIN_SIZE + i];                                   \n"
"        uint valueR = value & 0xFF;                                                   \n"
"        uint valueG = (value & 0xFF00) >> 8;                                          \n"
"        uint valueB = (value & 0xFF0000) >> 16;                                       \n"
"        sharedArrayR[localId * BIN_SIZE + valueR]++;                                  \n"
"        sharedArrayG[localId * BIN_SIZE + valueG]++;                                  \n"
"        sharedArrayB[localId * BIN_SIZE + valueB]++;                                  \n"
"    }                                                                                 \n"
"                                                                                      \n"
"    barrier(CLK_LOCAL_MEM_FENCE);                                                     \n"
"                                                                                      \n"
"    /* merge all thread-histograms into block-histogram */                            \n"
"    for(int i = 0; i < BIN_SIZE / groupSize; ++i)                                     \n"
"    {                                                                                 \n"
"        uint binCountR = 0;                                                           \n"
"        uint binCountG = 0;                                                           \n"
"        uint binCountB = 0;                                                           \n"
"        for(int j = 0; j < groupSize; ++j)                                            \n"
"        {                                                                             \n"
"            binCountR += sharedArrayR[j * BIN_SIZE + i * groupSize + localId];        \n"
"            binCountG += sharedArrayG[j * BIN_SIZE + i * groupSize + localId];        \n"
"            binCountB += sharedArrayB[j * BIN_SIZE + i * groupSize + localId];        \n"
"        }                                                                             \n"
"                                                                                      \n"
"        binResultR[groupId * BIN_SIZE + i * groupSize + localId] = binCountR;         \n"
"        binResultG[groupId * BIN_SIZE + i * groupSize + localId] = binCountG;         \n"
"        binResultB[groupId * BIN_SIZE + i * groupSize + localId] = binCountB;         \n"
"    }                                                                                 \n"
"}                                                                                     \n"
"                                                                                      \n";



int main(int argc, char *argv[])
{
    cl_int status = 0;
    cl_int binSize = 256;
    cl_int groupSize = 16;
    cl_int subHistgCnt;
    cl_device_type dType = CL_DEVICE_TYPE_GPU;
    cl_platform_id platform = NULL;
    cl_device_id   device;
    cl_context     context;
    cl_command_queue commandQueue;
    cl_mem         imageBuffer;
    cl_mem     intermediateHistR, intermediateHistG, intermediateHistB; /*Intermediate Image Histogram buffer*/
    cl_uint *  midDeviceBinR, *midDeviceBinG, *midDeviceBinB;
    cl_uint  *deviceBinR,*deviceBinG,*deviceBinB;
    //Read a BMP Image
    Image *image;
    std::string filename = "sample_color.bmp";
    ReadBMPImage(filename, &image);
    if(image == NULL)
        return 0;
    subHistgCnt  = (image->width * image->height)/(binSize*groupSize);
    midDeviceBinR = (cl_uint*)malloc(binSize * subHistgCnt * sizeof(cl_uint));
    midDeviceBinG = (cl_uint*)malloc(binSize * subHistgCnt * sizeof(cl_uint));
    midDeviceBinB = (cl_uint*)malloc(binSize * subHistgCnt * sizeof(cl_uint));
    deviceBinR    = (cl_uint*)malloc(binSize * sizeof(cl_uint));
    deviceBinG    = (cl_uint*)malloc(binSize * sizeof(cl_uint));
    deviceBinB    = (cl_uint*)malloc(binSize * sizeof(cl_uint));
    
	//Setup the OpenCL Platform, 
	//Get the first available platform. Use it as the default platform
    status = clGetPlatformIDs(1, &platform, NULL);
    if(status != CL_SUCCESS)
        std::cout << "Error # "<< status<<":: clGetPlatformIDs";

	//Get the first available device 
    status = clGetDeviceIDs (platform, dType, 1, &device, NULL);
    if(status != CL_SUCCESS)
        std::cout << "Error # "<< status<<":: clGetDeviceIDs";
	
	//Create an execution context for the selected platform and device. 
    cl_context_properties cps[3] = 
    {
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)platform,
        0
    };
    context = clCreateContextFromType(
        cps,
        dType,
        NULL,
        NULL,
        &status);
    if(status != CL_SUCCESS)
        std::cout << "Error # "<< status<<":: clCreateContextFromType";

    // Create command queue
    commandQueue = clCreateCommandQueue(context,
                                        device,
                                        0,
                                        &status);
    if(status != CL_SUCCESS)
        std::cout << "Error # "<< status<<":: clCreateCommandQueue";

	//Create OpenCL device input image
    
	cl_image_format image_format;
	image_format.image_channel_data_type = CL_UNSIGNED_INT8;
	image_format.image_channel_order = CL_RGBA;
	cl_image_desc image_desc;
	image_desc.image_type = CL_MEM_OBJECT_IMAGE2D;
	image_desc.image_width = image->width;
	image_desc.image_height = image->height;
	image_desc.image_depth = 1;
	image_desc.image_array_size = 1;
	image_desc.image_row_pitch = image->width * 4; //RGBA
	image_desc.image_slice_pitch = image->width * image->height * 4;
	image_desc.num_mip_levels = 0;
	image_desc.num_samples = 0;
	image_desc.buffer= NULL;
    cl_mem clImage = clCreateImage(
        context,
        CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR,
        &image_format,
        &image_desc,
		image->pixels, //R,G,B,A, R,G,B,A, R,G,B,A
        &status); 
    if(status != CL_SUCCESS)
        std::cout << "Error # "<< status<<":: clCreateImage\n";
	//Create OpenCL device output buffer
    intermediateHistR = clCreateBuffer(
        context, 
        CL_MEM_WRITE_ONLY,
        sizeof(cl_uint) * binSize * subHistgCnt, 
        NULL, 
        &status);
    if(status != CL_SUCCESS)
        std::cout << "Error # "<< status<<":: clCreateBuffer";

    intermediateHistG = clCreateBuffer(
        context,
        CL_MEM_WRITE_ONLY,
        sizeof(cl_uint) * binSize * subHistgCnt,
        NULL,
        &status);
    if(status != CL_SUCCESS)
        std::cout << "Error # "<< status<<":: clCreateBuffer";

    intermediateHistB = clCreateBuffer(
        context,
        CL_MEM_WRITE_ONLY,
        sizeof(cl_uint) * binSize * subHistgCnt,
        NULL,
        &status);
    if(status != CL_SUCCESS)
        std::cout << "Error # "<< status<<":: clCreateBuffer";

    // Create a program from the kernel source
    cl_program program = clCreateProgramWithSource(context, 1,
            (const char **)&histogram_kernel, NULL, &status);
    if(status != CL_SUCCESS)
        std::cout << "Error # "<< status<<":: clCreateProgramWithSource";

    // Build the program
    status = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if(status != CL_SUCCESS)
        std::cout << "Error # "<< status<<":: clBuildProgram";

    // Create the OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "histogram_image_kernel", &status);

    // Set the arguments of the kernel
    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&clImage); 
    status = clSetKernelArg(kernel, 1, 3 * groupSize * binSize * sizeof(cl_uchar), NULL); 
    status = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&intermediateHistR);
    status = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void*)&intermediateHistG);
    status = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void*)&intermediateHistB);
    // Execute the OpenCL kernel on the list
    cl_event ndrEvt;
    size_t globalThreads = (image->width * image->height) / (binSize*groupSize) * groupSize;
    size_t localThreads = groupSize;
    status = clEnqueueNDRangeKernel(
        commandQueue,
        kernel,
        1,
        NULL,
        &globalThreads,
        &localThreads,
        0,
        NULL,
        &ndrEvt);
    if(status != CL_SUCCESS)
        std::cout << "Error # "<< status<<":: clEnqueueNDRangeKernel";

    status = clFlush(commandQueue);
    if(status != CL_SUCCESS)
        std::cout << "Error # "<< status<<":: clFlush";
    //Read the histogram back into the host memory.
    memset(deviceBinR, 0, binSize * sizeof(cl_uint));
    memset(deviceBinG, 0, binSize * sizeof(cl_uint));
    memset(deviceBinB, 0, binSize * sizeof(cl_uint));
    cl_event readEvt[3];
    status = clEnqueueReadBuffer(
        commandQueue,
        intermediateHistR,
        CL_FALSE,
        0,
        subHistgCnt * binSize * sizeof(cl_uint),
        midDeviceBinR,
        0,
        NULL,
        &readEvt[0]);
    if(status != CL_SUCCESS)
        std::cout << "Error # "<< status<<":: clEnqueueReadBuffer";
    
    status = clEnqueueReadBuffer(
        commandQueue,
        intermediateHistG,
        CL_FALSE,
        0,
        subHistgCnt * binSize * sizeof(cl_uint),
        midDeviceBinG,
        0,
        NULL,
        &readEvt[1]);
    if(status != CL_SUCCESS)
        std::cout << "Error # "<< status<<":: clEnqueueReadBuffer";
    
    status = clEnqueueReadBuffer(
        commandQueue,
        intermediateHistB,
        CL_FALSE,
        0,
        subHistgCnt * binSize * sizeof(cl_uint),
        midDeviceBinB,
        0,
        NULL,
        &readEvt[2]);
    if(status != CL_SUCCESS)
        std::cout << "Error # "<< status<<":: clEnqueueReadBuffer";
    
    status = clWaitForEvents(3, readEvt);
    //status = clFinish(commandQueue);
    if(status != CL_SUCCESS)
        std::cout << "Error # "<< status<<":: clFinish";

    // Calculate final histogram bin 
    for(int i = 0; i < subHistgCnt; ++i)
    {
        for(int j = 0; j < binSize; ++j)
        {
            deviceBinR[j] += midDeviceBinR[i * binSize + j];
            deviceBinG[j] += midDeviceBinG[i * binSize + j];
            deviceBinB[j] += midDeviceBinB[i * binSize + j];
        }
    }

    // Validate the histogram operation. 
    // The idea behind this is that once a histogram is computed the sum of all the bins should be equal to the number of pixels.
    int totalPixelsR = 0;
    int totalPixelsG = 0;
    int totalPixelsB = 0;
    for(int j = 0; j < binSize; ++j)
    {
        totalPixelsR += deviceBinR[j];
        totalPixelsG += deviceBinG[j];
        totalPixelsB += deviceBinB[j];
    }
    printf ("Total Number of Red Pixels = %d\n",totalPixelsR);
    printf ("Total Number of Green Pixels = %d\n",totalPixelsG);
    printf ("Total Number of Blue Pixels = %d\n",totalPixelsB);
    ReleaseBMPImage(&image);
    //free all allocated memory
    free(midDeviceBinR);
    free(midDeviceBinG);
    free(midDeviceBinB);
    free(deviceBinR);
    free(deviceBinG);
    free(deviceBinB);

    return 0;
}