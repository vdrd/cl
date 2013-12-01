#ifndef APP0_CPP
#define APP0_CPP
#include "jpeg_marker.h"
#include "Framewave.h"

Fw64u image::curIndex = 0;                                         
Fw8u* image::rawImageBuffer = NULL;

unsigned char APP0::units = 0;
unsigned short int APP0::xDensity = 0;
unsigned short int APP0::yDensity = 0;
unsigned char APP0::RGBn[1];

unsigned char SOF0::samplePrecision; 
unsigned short int SOF0::noOfLines;
unsigned short int SOF0::samplesPerLine;
unsigned char SOF0::noOfComponents; 
SOF0ComponentData_t SOF0::componentData[4]; 

//#include "APP0.h"
//#include "init.h"
void APP0::processAPP0()
{
    unsigned short int length;
    char identifier[5]; 
    unsigned char version[2];
    unsigned char xThumbnail;
    unsigned char yThumbnail; 

    length = image::getNext2Bytes();
    identifier[0] = image::getNextByte();
    identifier[1] = image::getNextByte();
    identifier[2] = image::getNextByte();
    identifier[3] = image::getNextByte();
    identifier[4] = image::getNextByte();

    version[0] = image::getNextByte();
    version[1] = image::getNextByte();

    units = image::getNextByte();

    xDensity = image::getNext2Bytes();
    yDensity = image::getNext2Bytes();
    xThumbnail = image::getNextByte();
    yThumbnail = image::getNextByte();

    // TODO: should read the RGBn here(for the thubnail).. skipping it for now
    image::moveCurIndex(length-16);

    printInfo(1,"\ninfo : found APP0 marker\n");
    printInfo(1,"\n********************** APP0 HEADER *****************************\n");
    printInfo(1,"\nlength : %u\n", length);
    printInfo(1,"\nidentifier : %s\n", identifier);
    printInfo(1,"\nversion : %d.%d\n", version[0], version[1]); 
    printInfo((units == 0),"\no units for X an Y Densities\n");
    printInfo((units == 1),"\nX and Y are dots per Inch\n");
    printInfo((units == 2),"\nX and Y are dots per CM\n");
    printInfo(1,"\nxDensity : %u\n", xDensity);
    printInfo(1,"\nyDensity : %u\n", yDensity);
    printInfo(1,"\nxThumbnail : %u\n", xThumbnail);
    printInfo(1,"\nyThumbnail : %u\n", yThumbnail);
    printInfo(1,"\n****************************************************************\n");
}


void SOF0::processSOF0()
{
        unsigned short int length;
        unsigned char nextByte;

        length = image::getNext2Bytes();              
        samplePrecision = image::getNextByte();
        noOfLines = image::getNext2Bytes();
        samplesPerLine = image::getNext2Bytes();
        noOfComponents = image::getNextByte(); 

        //read the component info
        for(int i=0;i<noOfComponents;i++)
        {
                componentData[i].componentId = image::getNextByte();
                nextByte = image::getNextByte();
                componentData[i].horizontalSamplingFactor = nextByte>>4;
                componentData[i].verticalSamplingFactor = (nextByte&0x0F);
                componentData[i].DQTTableSelector = image::getNextByte();
        }//end for loop

        printInfo(1,"\ninfo : found SOF0 marker : BaseLine DCT\n");
        printInfo(1,"\n********************** SOF0 HEADER *****************************\n");
        printInfo(1,"\nlength : %d\n", length);  
        printInfo(1,"\nno. of bits for each sample : %u\n",samplePrecision);
        printInfo(1,"\nnumber of lines in the image : %u\n",noOfLines);
        printInfo(1,"\nsamples per line : %u\n", samplesPerLine);
        printInfo(1,"\nnumber of components in the image : %u\n",noOfComponents);
        for(int i=0;i<noOfComponents;i++)
        {
                printInfo(1,"\ncomponent ID : %u\n", componentData[i].componentId);      
                printInfo(1,"\nhorizontal sampling factor : %u\n", componentData[i].horizontalSamplingFactor); 
                printInfo(1,"\nvertical sampling factor : %u\n",componentData[i].verticalSamplingFactor);
                printInfo(1,"\nquantisation table selector : %u\n",componentData[i].DQTTableSelector);
        }
        printInfo(1,"\n****************************************************************\n");
} //end readSOF0 fn


#endif

