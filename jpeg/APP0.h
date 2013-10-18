#ifndef APP0_H
#define APP0_H

#include "image.h"
#include "debug.h"

class APP0
{
    public:
        static unsigned char units;
        static unsigned short int xDensity;
        static unsigned short int yDensity;
        static unsigned char RGBn[1]; // need to be done later..contains RGB values for the thumbnail

    public:
        static void processAPP0();
};

#endif