#ifndef INIT_H
#define INIT_H

//#include "fwBase.h"
#include "image.h"

Fw64u image::curIndex = 0;					 
Fw8u* image::rawImageBuffer = NULL;


#include "APP0.h"

unsigned char APP0::units = 0;
unsigned short int APP0::xDensity = 0;
unsigned short int APP0::yDensity = 0;
unsigned char APP0::RGBn[1];


#include "SOF0.h"

unsigned char SOF0::samplePrecision; 
unsigned short int SOF0::noOfLines;
unsigned short int SOF0::samplesPerLine;
unsigned char SOF0::noOfComponents; 
SOF0ComponentData_t SOF0::componentData[4]; 


#endif