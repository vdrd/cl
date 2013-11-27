#ifndef IMAGE_H
#define IMAGE_H

//#include <io.h>
#include "debug.h"
#include "markers.h"
#include "SOF0.h"
#include "APP0.h"
#include "Framewave.h"
#define ENABLE_FRAMEWAVE 1
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#define OCL_SUCCESS 0
#define OCL_FAILURE -1
typedef struct
{
    unsigned char ComponentId;
    unsigned char DCTableSelector;
    unsigned char ACTableSelector;
}SOSComponentData_t;



class image
{
public:
    FILE *fp;		 				 // Input file pointer
    static Fw64u curIndex;					 // Index pointing to current byte in the rawImageBuffer that is about to be read
    Fw64u fileLength;					
    static Fw8u *rawImageBuffer;
    FwiDecodeHuffmanSpec *pHuffDcTable[4];
    FwiDecodeHuffmanSpec *pHuffAcTable[4];
    unsigned char precision;
    Fw16u quantInvTable[4][64]; // right now supporting only 8 bit quantisation values
    bool restartEnabled;  // this flag is set by DRI marker segment and is required while decoding entropy coded segments
    unsigned short int restartInterval;
    SOSComponentData_t component[4]; // this is the reason we support only 4 components(max) in each scan. Change this to support more components
    unsigned char startSpectralSelector;
    unsigned char endSpectralSelector;
    unsigned char bitPositionHigh;
    unsigned char bitPositionLow;
    SOF0ComponentData_t componentData[4];
    unsigned short int samplesPerLine; //specifies the no. of samples per line
    unsigned short int noOfLines; // specifies the maximum number of lines in the source image
    cl_short *pMCUdst[4];
    cl_uchar *pOutputFinalOpenclDst;
    cl_uchar *pOutputFinalReferenceDst;
    cl_uchar *pOutputFinalNoOfDevicesDst;
    cl_uint imageWidth;
    cl_uint imageHeight;
    cl_int noOfComponents;
    cl_int noOfxMCU;
    cl_int noOfyMCU;
    int    mcuWidth;
    int    mcuHeight;

public:
    int open(const char *inputFile);
    void close();
    static unsigned char getNextByte();
    static unsigned short int getNext2Bytes();
    unsigned char getNextMarker();
    static void moveCurIndex(Fw64u noOfBytes);
    void decode();	
    void processTablenMisc(unsigned char marker);
    void decodeFrame(unsigned char SOFmarker);
    void processDHT();
    void processDQT();
    void processDRI();
    void processScan();
    void decodeScan();
    /*Converts the YCbCr data to BGR and writes the BMP file*/
    void write(const char *fname, Fw8u *imageYCbCr);
};

#endif
