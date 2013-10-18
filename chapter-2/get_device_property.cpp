#include <stdio.h>
#include <stdlib.h>
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif


void PrintDeviceInfo(cl_device_id device)
{
    char queryBuffer[1024];
    int queryInt;
    cl_int clError;
    clError = clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(queryBuffer), &queryBuffer, NULL);
    printf("CL_DEVICE_NAME: %s\n", queryBuffer);
    queryBuffer[0] = '\0';
    clError = clGetDeviceInfo(device, CL_DEVICE_VENDOR, sizeof(queryBuffer), &queryBuffer, NULL);
    printf("CL_DEVICE_VENDOR: %s\n", queryBuffer);
    queryBuffer[0] = '\0';
    clError = clGetDeviceInfo(device, CL_DRIVER_VERSION, sizeof(queryBuffer), &queryBuffer, NULL);
    printf("CL_DRIVER_VERSION: %s\n", queryBuffer);
    queryBuffer[0] = '\0';
    clError = clGetDeviceInfo(device, CL_DEVICE_VERSION, sizeof(queryBuffer), &queryBuffer, NULL);
    printf("CL_DEVICE_VERSION: %s\n", queryBuffer);
    queryBuffer[0] = '\0';
    clError = clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(int), &queryInt, NULL);
    printf("CL_DEVICE_MAX_COMPUTE_UNITS: %d\n", queryInt);
}

int main(void) {


    // Get platform and device information
    cl_platform_id   platform;
    cl_device_id    *devices;
    cl_uint          num_platforms;
    cl_uint          num_devices;
    cl_int           clError;
    char             queryBuffer[1024];
    //Get the Number of Platforms available
    /*cl_int clGetPlatformIDs (cl_uint num_entries,
                               cl_platform_id *platforms,
                               cl_uint *num_platforms);*/
    // Note that the second parameter "platforms" is set to NULL. If this is NULL then this argument is ignored
    // and the API returns the total number of OpenCL platforms available.
    clError = clGetPlatformIDs(1, &platform, &num_platforms);
    if(clError != CL_SUCCESS)
    {
        printf("Error in call to clGetPlatformIDs....\n Exiting");
        exit(0);
    }
    else
    {
        if (num_platforms == 0)
        {
            printf("No OpenCL Platforms Found ....\n Exiting");
        }
        else
        {
            // We have obtained one platform here.
            // Lets enumerate the devices available in this Platform.
            printf("Printing all the OpenCL Device Info:\n\n");
            clError = clGetDeviceIDs (platform, CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices);
            if (clError != CL_SUCCESS)
            {
                printf("Error Getting number of devices... Exiting\n ");
                exit(0);
            }
            // If successfull the num_devices contains the number of devices available in the platform
            // Now lets get all the device list. Before that we need to malloc devices
            devices = (cl_device_id *)malloc(sizeof(cl_device_id) * num_devices);
            clError = clGetDeviceIDs (platform, CL_DEVICE_TYPE_ALL, num_devices, devices, &num_devices);
            if (clError != CL_SUCCESS)
            {
                printf("Error Getting number of devices... Exiting\n ");
                exit(0);
            }
            //
            for (int index = 0; index < num_devices; index++)
            {
                 printf("==================Device No %d======================\n",index);
                 PrintDeviceInfo(devices[index]);
                 printf("====================================================\n");
            }
        }
    }
}