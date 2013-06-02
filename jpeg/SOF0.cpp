#ifndef SOF0_CPP
#define SOF0_CPP

#include "SOF0.h"
#include "Framewave.h"
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