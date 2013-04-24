//AMD commands
//cl /c chap2_get_platform_property.cpp /I"%AMDAPPSDKROOT%\include"
//link  /OUT:"get_platform_property.exe" "%AMDAPPSDKROOT%\lib\x86_64\OpenCL.lib" chap2_get_platform_property.obj
//NVIDIA commands
//cl /c chap2_get_platform_property.cpp /I"%NVSDKCOMPUTE_ROOT%\OpenCL\common\inc"
//link  /OUT:"get_platform_property.exe" "%NVSDKCOMPUTE_ROOT%\OpenCL\common\lib\x64\OpenCL.lib" chap2_get_platform_property.obj


#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>


int main(void) {


    // Get platform and device information
    cl_platform_id * platforms = NULL;
    cl_uint          num_platforms;
    cl_int           clError;
    char             queryBuffer[1024];
    //Get the Number of Platforms available
    /*cl_int clGetPlatformIDs (cl_uint num_entries,
                               cl_platform_id *platforms,
                               cl_uint *num_platforms);*/
    // Note that the second parameter "platforms" is set to NULL. If this is NULL then this argument is ignored
    // and the API returns the total number of OpenCL platforms available.
    clError = clGetPlatformIDs(0, NULL, &num_platforms);
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
			//Allocate memory for platform ID
			printf ("Found %d Platforms\n", num_platforms);
			platforms = (cl_platform_id *)malloc(num_platforms*sizeof(cl_platform_id));
			//Get the platform info
			//In contrast to the above call with "platforms" as NULL. The below call actually fills the buffer with
			//the platform IDs. It will list the platforms upto the value specified by num_platforms. One should make
			//sure that the appropriate size buffer is allocated.
            clError = clGetPlatformIDs (num_platforms, platforms, NULL);
			// for each platform now start printing the information
            for(int index=0;index<num_platforms; index++)
            {
				printf ("Printing Details of Platform %d\n", index);
				clError = clGetPlatformInfo (platforms[index], CL_PLATFORM_NAME, 1024, &queryBuffer, NULL);
				if(clError == CL_SUCCESS)
				{
					printf("CL_PLATFORM_NAME   : %s\n", queryBuffer);
				}
				clError = clGetPlatformInfo (platforms[index], CL_PLATFORM_VENDOR, 1024, &queryBuffer, NULL);
				if(clError == CL_SUCCESS)
				{
					printf("CL_PLATFORM_VENDOR : %s\n", queryBuffer);
				}
				clError = clGetPlatformInfo (platforms[index], CL_PLATFORM_VERSION, 1024, &queryBuffer, NULL);
				if (clError == CL_SUCCESS)
				{
					printf("CL_PLATFORM_VERSION: %s\n", queryBuffer);
				}
				clError = clGetPlatformInfo (platforms[index], CL_PLATFORM_PROFILE, 1024, &queryBuffer, NULL);
				if (clError == CL_SUCCESS)
				{
					printf("CL_PLATFORM_PROFILE: %s\n", queryBuffer);
				}
				clError = clGetPlatformInfo (platforms[index], CL_PLATFORM_EXTENSIONS, 1024, &queryBuffer, NULL);
				if (clError == CL_SUCCESS)
				{
					printf("CL_PLATFORM_EXTENSIONS: %s\n", queryBuffer);
				}
			}
		}
	}
}