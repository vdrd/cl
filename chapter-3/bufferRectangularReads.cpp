#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#include <iostream>
#include <ocl_macros.h>

#define NUM_OF_ELEMENTS 32 
#define DEVICE_TYPE CL_DEVICE_TYPE_GPU

int main(int argc, char *argv[])
{
    cl_int status = 0;
    //cl_platform_id platform = NULL;
    //cl_device_id   device;
    cl_context     context;
    cl_command_queue commandQueue;
    cl_mem clBuffer;
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

    // Get platform and device information
    cl_platform_id * platforms = NULL;
    OCL_CREATE_PLATFORMS( platforms );

    // Get the devices list and choose the type of device you want to run on
    cl_device_id *device_list = NULL;
    OCL_CREATE_DEVICE( platforms[0], DEVICE_TYPE, device_list);
    
    //Create an execution context for the selected platform and device. 
    cl_context_properties cps[3] = 
    {
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)platforms[0],
        0
    };
    context = clCreateContextFromType(
        cps,
        DEVICE_TYPE,
        NULL,
        NULL,
        &status);
    LOG_OCL_ERROR(status, "clCreateContextFromType Failed..." );

    // Create command queue
    commandQueue = clCreateCommandQueue(context,
                                        device_list[0],
                                        0,
                                        &status);
    LOG_OCL_ERROR(status, "clCreateCommandQueue Failed..." );

    //Create OpenCL device input buffer
    clBuffer = clCreateBuffer(
        context,
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(cl_uint) * NUM_OF_ELEMENTS,
        hostBuffer,
        &status); 
    LOG_OCL_ERROR(status, "clCreateBuffer Failed..." );

    //Read a 2D rectangular object from the clBuffer of 32 elements
    int hostPtr2D[6] = {0, 0, 0, 0, 0, 0};

    /*Note - For all of buffer Origin, host origin and region 2D, the first element is in bytes.*/
    size_t bufferOrigin2D[3] = {1*sizeof(int), 6, 0}; /*Bytes, rows, slices respectively*/
    size_t hostOrigin2D[3] = {0 ,0, 0};         
    size_t region2D[3] = {3* sizeof(int), 2,1}; /*width in bytes, height in rows, depth in slices*/

    status = clEnqueueReadBufferRect(
                        commandQueue,
                        clBuffer,
                        CL_TRUE,        /*Blocking read, Hence the events are NULL below*/
                        bufferOrigin2D, /*Start of a 2D buffer to read from*/
                        hostOrigin2D,
                        region2D,
                        (NUM_OF_ELEMENTS / 8) * sizeof(int), /*buffer_row_pitch  */
                        0,                                   /*buffer_slice_pitch*/
                        0,                                   /*host_row_pitch    */
                        0,                                   /*host_slice_pitch  */
                        (void*)(hostPtr2D), /*Elements are read into this buffer*/
                        0,
                        NULL,
                        NULL);
    LOG_OCL_ERROR(status, "clEnqueueReadBufferRect Failed..." );
    std::cout << "2D rectangle selected is as follows" << std::endl;
    std::cout << " " << hostPtr2D[0];
    std::cout << " " << hostPtr2D[1];
    std::cout << " " << hostPtr2D[2] << std::endl;
    std::cout << " " << hostPtr2D[3];
    std::cout << " " << hostPtr2D[4];
    std::cout << " " << hostPtr2D[5] << std::endl;

    //Read a 3D rectangular object from the clBuffer of 32 elements
    int hostPtr3D[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    size_t bufferOrigin3D[3] = {1*sizeof(int), 1, 0};
    size_t hostOrigin3D[3] = {0 ,0, 0};
    size_t region3D[3] = {3* sizeof(int), 1,3};
    status = clEnqueueReadBufferRect(
                        commandQueue,
                        clBuffer,
                        CL_TRUE,        /*Blocking read, Hence the events are NULL below*/
                        bufferOrigin3D, /*Start of a 2D buffer to read from*/
                        hostOrigin3D,
                        region3D,
                        (NUM_OF_ELEMENTS / 8) * sizeof(int), /*buffer_row_pitch  - each row is of 4 elements*/
                        (NUM_OF_ELEMENTS / 4) * sizeof(int), /*buffer_slice_pitch - 4 slices are created */
                        0,                                   /*host_row_pitch    */
                        0,                                   /*host_slice_pitch  */
                        (void*)(hostPtr3D), /*Elements are read into this buffer*/
                        0,
                        NULL,
                        NULL);
    LOG_OCL_ERROR(status, "clEnqueueReadBufferRect Failed..." );
    std::cout << "3D rectangle selected is as follows" << std::endl;
    std::cout << " " << hostPtr3D[0];
    std::cout << " " << hostPtr3D[1];
    std::cout << " " << hostPtr3D[2] << std::endl;
    std::cout << " " << hostPtr3D[3];
    std::cout << " " << hostPtr3D[4];
    std::cout << " " << hostPtr3D[5] << std::endl;
    std::cout << " " << hostPtr3D[6];
    std::cout << " " << hostPtr3D[7];
    std::cout << " " << hostPtr3D[8] << std::endl;

    return 0;
}