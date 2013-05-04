#include <cstdlib>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <CL/cl.h>
typedef struct {
    const char * filename;
    unsigned int height;
    unsigned int width;
    unsigned char *pixels;
}Image;

__pragma( pack(push, 1) ) 
typedef struct
{
    unsigned char x;
    unsigned char y;
    unsigned char z;
    unsigned char w;
} ColorPalette;

typedef ColorPalette PixelColor;

typedef struct {
    short id;
    int size;
    short reserved1;
    short reserved2;
    int offset;
} BMPHeader ; 

typedef struct {
    unsigned int sizeInfo;
    unsigned int width;
    unsigned int height;
    unsigned short planes;
    unsigned short bitsPerPixel;
    unsigned int compression;
    unsigned int imageSize;
    unsigned int xPelsPerMeter;
    unsigned int yPelsPerMeter;
    unsigned int clrUsed;
    unsigned int clrImportant;
}  BMPInfoHeader ;
__pragma( pack(pop) )

static const short bitMapID = 19778;

void
ReadBMPImage(std::string filename,  Image **image)
{
    BMPHeader header;
    BMPInfoHeader infoHeader;
    ColorPalette *colors_;
    PixelColor   *pixelColor;
    unsigned int numColors_;
    // Open BMP file
    FILE *fd;
    *image = (Image *)calloc(1,sizeof(Image));
    (*image)->filename = filename.c_str();
    
    fopen_s(&fd, (*image)->filename, "rb");
    (*image)->width = 0;
    (*image)->height = 0;
    (*image)->pixels = NULL;
    

    // Opened OK
    if (fd != NULL) {
        // Read the BMP header
        fread(&header, sizeof(BMPHeader), 1, fd);
        if (ferror(fd)) {
            fclose(fd);
            goto fileReadFail;
        }

        // Confirm that we have a bitmap file
        if (header.id != bitMapID) {
            fclose(fd);
            goto fileReadFail;
        }

        // Read map info header
        fread(&infoHeader, sizeof(BMPInfoHeader), 1, fd);

        // Failed to read map info header
        if (ferror(fd)) {
            fclose(fd);
            return;
        }

        // Store number of colors
        numColors_ = 1 << infoHeader.bitsPerPixel;

        //load the palate for 8 bits per pixel
        if(infoHeader.bitsPerPixel == 8) {
            colors_ = (ColorPalette*)malloc(numColors_ * sizeof(ColorPalette));
            fread( (char *)colors_, numColors_ * sizeof(ColorPalette), 1, fd);
        }

        // Allocate buffer to hold all pixels
        unsigned int sizeBuffer = header.size - header.offset;
        unsigned char *tmpPixels = (unsigned char*)malloc(sizeBuffer*sizeof(unsigned char));

        // Read pixels from file, including any padding
        fread(tmpPixels, sizeBuffer * sizeof(unsigned char), 1, fd);

        // Allocate image
        pixelColor = (PixelColor*)malloc(infoHeader.width * infoHeader.height*sizeof(PixelColor));

        // Set image, including w component (white)
        memset(pixelColor, 0xff, infoHeader.width * infoHeader.height * sizeof(PixelColor));

        unsigned int index = 0;
        for(unsigned int y = 0; y < infoHeader.height; y++) {
            for(unsigned int x = 0; x < infoHeader.width; x++) {
                // Read RGB values
                if (infoHeader.bitsPerPixel == 8) {
                    pixelColor[(y * infoHeader.width + x)] = colors_[tmpPixels[index++]];
                }
                else { // 24 bit
					//pixelColor[(y * infoHeader.width + x)].w = 0;
                    pixelColor[(y * infoHeader.width + x)].z = tmpPixels[index++];
                    pixelColor[(y * infoHeader.width + x)].y = tmpPixels[index++];
                    pixelColor[(y * infoHeader.width + x)].x = tmpPixels[index++];
                }
            }

            // Handle padding
            for(unsigned int x = 0; x < (4 - (3 * infoHeader.width) % 4) % 4; x++) {
                index++;
            }
        }

        // Loaded file so we can close the file.
        fclose(fd);
        free(tmpPixels);
		if(infoHeader.bitsPerPixel == 8) {
            free(colors_);
        }
        (*image)->width = infoHeader.width;
        (*image)->height = infoHeader.height;
        (*image)->pixels = (unsigned char *)pixelColor;
        return;
    }
fileReadFail:
    free (*image);
    *image = NULL;    
    return;
}

void ReleaseBMPImage(Image **image)
{
    if(*image != NULL)
        if((*image)->pixels !=NULL)
        {
            free((*image)->pixels);
            free(*image);
        }
    return;
}

const char *histogram_kernel =
"#define BIN_SIZE 256                                                                  \n"
"#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable                       \n"
"__kernel                                                                              \n"
"void histogram_kernel(__global const uint* data,                                      \n"
"                  __local uchar* sharedArray,                                         \n"
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
    std::string filename = "sample_color_small.bmp";
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
        CL_MEM_READ_ONLY,
        &image_format,
        &image_desc,
		image->pixels,
        &status); 
    if(status != CL_SUCCESS)
        std::cout << "Error # "<< status<<":: clCreateImage";
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
    cl_kernel kernel = clCreateKernel(program, "histogram_kernel", &status);

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