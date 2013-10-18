#ifndef APP0_CPP
#define APP0_CPP

#include "APP0.h"
#include "init.h"
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

#endif

