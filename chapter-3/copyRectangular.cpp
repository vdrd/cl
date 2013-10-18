#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <CL/cl.h>
#include <iostream>
#define NUM_OF_ELEMENTS 32 

int main(int argc, char *argv[])
{
    cl_int status = 0;
    cl_device_type dType = CL_DEVICE_TYPE_GPU;
    cl_platform_id platform = NULL;
    cl_device_id   device;
    cl_context     context;
    cl_command_queue commandQueue;
    cl_mem clBufferSrc, clBufferDst;
    cl_int hostBuffer[NUM_OF_ELEMENTS] =
    {
         0,  1,  2,  3, 
         4,  5,  6,  7,
         8,  9, 10, 11, 
        12, 13, 14, 15,
        16, 17, 18, 19,
        20, 21, 22, 23, 
        24, 25, 26, 27,
        28, 29, 30, 31, 
    };

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

    //Create OpenCL device input buffer
    clBufferSrc = clCreateBuffer(
        context,
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(cl_uint) * NUM_OF_ELEMENTS,
        hostBuffer,
        &status); 
    if(status != CL_SUCCESS)
        std::cout << "Error # "<< status<<":: clCreateBuffer";

    //Copy commands
    cl_uint copyBuffer[NUM_OF_ELEMENTS] = 
    {
        0,  0,  0,  0, 
        0,  0,  0,  0,
        0,  0,  0,  0, 
        0,  0,  0,  0,
        0,  0,  0,  0,
        0,  0,  0,  0, 
        0,  0,  0,  0,
        0,  0,  0,  0
    };
    
    clBufferDst = clCreateBuffer(
        context,
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(cl_uint) * NUM_OF_ELEMENTS,
        copyBuffer,
        &status); 
    if(status != CL_SUCCESS)
        std::cout << "Error # "<< status<<":: clCreateBuffer";

    //Copy the contents of the rectangular buffer pointed by clBufferSrc to  clBufferSrc
    size_t src_origin[3] = {0, 4*sizeof(cl_uint), 0};
    size_t dst_origin[3] = {0, 1*sizeof(cl_uint), 0};
    size_t region[3] = {3* sizeof(int), 2,1};
    cl_event copyEvent;
    status = clEnqueueCopyBufferRect ( commandQueue, 	//copy command will be queued
                    clBufferSrc,		
                    clBufferDst,	
                    src_origin,	  //offset associated with src_buffer
                    dst_origin,   //offset associated with src_buffer
                    region,		  //(width, height, depth) in bytes of the 2D or 3D rectangle being copied
                    (NUM_OF_ELEMENTS / 8) * sizeof(int), /*buffer_row_pitch  */
                    0,            //Its a 2D buffers. Hence the slice size is 0
                    (NUM_OF_ELEMENTS / 8) * sizeof(int), /*buffer_row_pitch  */
                    0,            //Its a 2D buffers. Hence the slice size is 0
                    0,
                    NULL,
                    &copyEvent);
    status = clWaitForEvents(1, &copyEvent);
    
    status = clEnqueueReadBuffer(
        commandQueue,
        clBufferDst,
        CL_TRUE,
        0,
        NUM_OF_ELEMENTS * sizeof(cl_uint),
        copyBuffer,
        0,
        NULL,
        NULL);
    if(status != CL_SUCCESS)
        std::cout << "Error # "<< status<<":: clEnqueueReadBuffer";

    std::cout << "The copied destination buffer is as follows" << std::endl;
    for(int i=0; i<8; i++)
    {
        std::cout << std::endl; 
        for(int j=0; j<NUM_OF_ELEMENTS/8; j++)
        {
            std::cout << " " << copyBuffer[i*(NUM_OF_ELEMENTS/8) + j];
        }
    }
    return 0;
}