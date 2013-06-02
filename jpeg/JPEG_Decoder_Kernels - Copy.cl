/*
 *  Calculates the naive matrix multiplication using the local memory 
 *  loads the blocks along the common dimension to local memories
 *  performs naive matrix multiplication on the blocks that are loaded
 *  
 */

#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable

void Idct(const short  *pSrc, unsigned char *pDst)
{
	int i, j, k, l;
	float partialProduct;
	float tmp[64];
    const float c[8][8] = {
    { 0.35355338F,      0.35355338F,      0.35355338F,      0.35355338F,      0.35355338F,      0.35355338F,      0.35355338F,      0.35355338F },
    {0.49039263F,      0.41573480F,      0.27778512F,     0.097545162F,    -0.097545162F,     -0.27778512F,     -0.41573480F,     -0.49039263F },
    {0.46193975F,     0.19134171F,     -0.19134171F,     -0.46193975F,     -0.46193975F,     -0.19134171F,      0.19134171F,      0.46193975F },
    {0.41573480F,    -0.097545162F,     -0.49039263F,     -0.27778512F,      0.27778512F,      0.49039263F,     0.097545162F,     -0.41573480F },
    {0.35355338F,     -0.35355338F,     -0.35355338F,      0.35355338F,      0.35355338F,     -0.35355338F,     -0.35355338F,      0.35355338F },
    {0.27778512F,     -0.49039263F,     0.097545162F,      0.41573480F,     -0.41573480F,    -0.097545162F,      0.49039263F,     -0.27778512F },
    {0.19134171F,     -0.46193975F,      0.46193975F,     -0.19134171F,     -0.19134171F,      0.46193975F,     -0.46193975F,      0.19134171F },
    {0.097545162F,     -0.27778512F,      0.41573480F,     -0.49039263F,      0.49039263F,     -0.41573480F,      0.27778512F,    -0.097545162F}
 };

	for (i=0; i<8; i++)
		for (j=0; j<8; j++)
		{
			partialProduct = 0.0F;

			for (k=0; k<8; k++)
				partialProduct+= c[k][j]*pSrc[8*i+k];

			tmp[8*i+j] = partialProduct;
		}

		// Transpose operation is integrated into address mapping by switching 
		// loop order of i and j 

		for (j=0; j<8; j++)
			for (i=0; i<8; i++)
			{
				partialProduct = 0.0F;

				for (k=0; k<8; k++)
					partialProduct+= c[k][i]*tmp[8*k+j];
				
				l = (int)(partialProduct+0.5F);
				l = l+ 128;
				if(l < 0)
					l = 0;
				if(l > 255)
					l = 255;
				pDst[8*i+j] = l;
			}
}

void 
DCTQuantInv8x8LS_JPEG_16s8u(__global short int *pSrc, unsigned char *pDst, int dstStp, __global unsigned short *pQuantInvTable)
{
		int q, i, j;
	    short int ppSrc[64];
		for(i=0; i<8; i++) {
		  for(j=0; j<8; j++) {
		 	q = pQuantInvTable[8*i+j];
			*(ppSrc+i*8+j) = (short)(*(pSrc+i*8+j)*q);
			}
		}

		Idct(ppSrc, pDst);
}


__kernel void 
JPEGdecoder_MCU(__global short *pMCUdata1,
				__global unsigned short *pQuantTable1,
				__global short *pMCUdata2,
				__global unsigned short *pQuantTable2,				
				__global short *pMCUdata3,
				__global unsigned short *pQuantTable3,
                __global unsigned char * output, 
				const unsigned int width,
				const unsigned int height,
				const unsigned int mcuWidth,
				const unsigned int mcuHeight)
{
	
	/* get the block ids in both the directions */
	int bx = get_global_id(0);
	int by = get_global_id(1);
	int tbx = get_global_size(0);
	int tby = get_global_size(1);
	int index = 0;
	//unsigned char valueY, valueCb, valueCr;
	int imageSize=0;
	//float tempValue=0.0F;

	//short int src[64];
	unsigned char dst[64];
	__global short *tempPtr1;
	
	/* get the local ids within the block */

	/*4 is a hard coded entry*/

        /*Y component*/
        tempPtr1 = pMCUdata1 + by * tbx *(4*64) + (bx * 4 * 64) ;/*This is what is reqd*/
	DCTQuantInv8x8LS_JPEG_16s8u(tempPtr1,dst,8,(pQuantTable1));

	for (int i=0;i<8;i++)
	{
		for (int j=0;j<8;j++)
		{
			index = width*by*mcuHeight + bx*mcuWidth + i*width + j;
			output[index] = dst[i*8 + j];
		}
	}

    tempPtr1 = (pMCUdata1 + by * tbx *(4*64) + (bx * 4 * 64) + (( 0 * 64) +  1 *64));
    DCTQuantInv8x8LS_JPEG_16s8u(tempPtr1,dst,8,pQuantTable1);
	for (int i=0;i<8;i++)
	{
		for (int j=0;j<8;j++)
		{	
			index = (width*by*mcuHeight) + (bx*mcuWidth + 8) + i*width + j;
			output[index] = dst[i*8 + j];
		}	
	}

    tempPtr1 = pMCUdata1 + by * tbx *(4*64) + (bx * 4 * 64) + (( 2 * 64) +  0 *64);
    DCTQuantInv8x8LS_JPEG_16s8u(tempPtr1,dst,8,pQuantTable1);
	for (int i=0;i<8;i++)
	{
		for (int j=0;j<8;j++)
		{
			index = (width*by*mcuHeight) + (bx*mcuWidth) + (i+8)*width + j;
			output[index] = dst[i*8 + j];
		}
	}

    tempPtr1 = pMCUdata1 + by * tbx *(4*64) + (bx * 4 * 64) + (( 2 * 64) +  1 *64);
    DCTQuantInv8x8LS_JPEG_16s8u(tempPtr1,dst,8,pQuantTable1);
	for (int i=0;i<8;i++)
	{
		for (int j=0;j<8;j++)
		{
			index = (width*by*mcuHeight) + (bx*mcuWidth + 8) + (i+8)*width + j;
			output[index] = dst[i*8 + j];
		}
	}

    /*Cb component*/
	//pMCUdata2[65536] = 0;

    tempPtr1 = pMCUdata2 + by * tbx *(1*64) + (bx * 1 * 64);
    DCTQuantInv8x8LS_JPEG_16s8u(tempPtr1,dst,8,pQuantTable2);

	imageSize = (width * height);
	for (int i=0;i<8;i++)
	{
		for (int j=0;j<8;j++)
		{	
			index = imageSize + width*by*mcuHeight + bx*mcuWidth + 2*i*width + 2*j ;
			output[index] = dst[i*8 + j];
			index = imageSize + width*by*mcuHeight + bx*mcuWidth + 2*i*width + 2*j + 1;
			output[index] = dst[i*8 + j];
			index = imageSize + width*by*mcuHeight + bx*mcuWidth + (2*i + 1)*width + 2*j;
			output[index] = dst[i*8 + j];
			index = imageSize + width*by*mcuHeight + bx*mcuWidth + (2*i + 1)*width + 2*j + 1;
			output[index] = dst[i*8 + j];
		}	
	}

    /*Cr component*/
    tempPtr1 = pMCUdata3 + by * tbx *(1*64) + (bx * 1 * 64);
    DCTQuantInv8x8LS_JPEG_16s8u(tempPtr1,dst,8,pQuantTable3);
	imageSize = 2*(width * height);
	for (int i=0;i<8;i++)
	{
		for (int j=0;j<8;j++)
		{	
			index = imageSize + width*by*mcuHeight + bx*mcuWidth + 2*i*width + 2*j ;
			output[index] = dst[i*8 + j];
			index = imageSize + width*by*mcuHeight + bx*mcuWidth + 2*i*width + 2*j + 1;
			output[index] = dst[i*8 + j];
			index = imageSize + width*by*mcuHeight + bx*mcuWidth + (2*i + 1)* width + 2*j;
			output[index] = dst[i*8 + j];
			index = imageSize + width*by*mcuHeight + bx*mcuWidth + (2*i + 1)* width + 2*j + 1;
			output[index] = dst[i*8 + j];
		}
	}
	
	/*for(int i=0;i<8;i++)
	{
		for(int j=0;j<8;j++)
		{
			index = width*by*mcuHeight + bx*mcuWidth + i*width + j;
			valueY = output[index];
			index += (width * height) ;
			valueCb = output[index];
			index += (width * height);
			valueCr = output[index];
			

		}
	}*/
	barrier(CLK_LOCAL_MEM_FENCE);
		
	
	/* write the accumulator to the output */
    
}




__kernel void 
JPEGdecoder_Reference(__global short *pMCUdata1,
				__global unsigned short *pQuantTable1,
				__global short *pMCUdata2,
				__global unsigned short *pQuantTable2,			
				__global short *pMCUdata3,
				__global unsigned short *pQuantTable3,
                __global unsigned char * output,
				const unsigned int width,
				const unsigned int height,
				const unsigned int mcuWidth,
				const unsigned int mcuHeight)

{
	int noOfxMCU = width/mcuWidth;
	int noOfyMCU = height/mcuHeight;
	int noOfComponents=3;
	int verticalSamplingFactor[3] = {2,1,1};
	int horizontalSamplingFactor[3] = {2,1,1};
	int noOfMCUComponent;
	unsigned char dst[64];
	__global short *tempPtr1;
	__global short *pMCUdata[3] = {pMCUdata1,pMCUdata2,pMCUdata3};
	__global unsigned short *pQuantTable[3] = {pQuantTable1,pQuantTable2,pQuantTable3};
	int index = 0;

	for(int i=0;i<noOfComponents;i++) 
	{
		for(int yMCU=0;yMCU<noOfyMCU;yMCU++)
		{
			for(int xMCU=0;xMCU<noOfxMCU;xMCU++) 
			{
				for(unsigned int j=0;j<verticalSamplingFactor[i];j++)
                {
                    for(unsigned int k=0;k<horizontalSamplingFactor[i];k++)
                    {
						noOfMCUComponent = verticalSamplingFactor[i]*horizontalSamplingFactor[i];
						tempPtr1 = pMCUdata[i] + 
									yMCU * noOfxMCU * (noOfMCUComponent*64) + 
									(xMCU * noOfMCUComponent * 64) + 
									(j*verticalSamplingFactor[i] + k)*64;
						DCTQuantInv8x8LS_JPEG_16s8u(tempPtr1,dst,8,(pQuantTable[i]));
						for (int l=0;l<8;l++)
						{
							for (int m=0;m<8;m++)
							{
								if (i == 0)
								{
								index = width*yMCU*mcuHeight + xMCU*mcuWidth + 
										j*8*width + k*8 +
										l*width + m;
								output[index] = dst[l*8 + m];
								}
								else
								{
									index = (i*width*height) + width*yMCU*mcuHeight + xMCU*mcuWidth + 2*l*width + 2*m ;
									output[index] = dst[l*8 + m];
									index = (i*width*height) + width*yMCU*mcuHeight + xMCU*mcuWidth + 2*l*width + 2*m + 1;
									output[index] = dst[l*8 + m];
									index = (i*width*height) + width*yMCU*mcuHeight + xMCU*mcuWidth + (2*l + 1)* width + 2*m;
									output[index] = dst[l*8 + m];
									index = (i*width*height) + width*yMCU*mcuHeight + xMCU*mcuWidth + (2*l + 1)* width + 2*m + 1;
									output[index] = dst[l*8 + m];
								}
							}
						}
					}
				}
			}
		}
	}
}


__kernel void 
JPEGdecoder_Devices(__global short *pMCUdata1,
				__global unsigned short *pQuantTable1,
				__global short *pMCUdata2,
				__global unsigned short *pQuantTable2,			
				__global short *pMCUdata3,
				__global unsigned short *pQuantTable3,
                __global unsigned char * output,
				const unsigned int width,
				const unsigned int height,
				const unsigned int mcuWidth,
				const unsigned int mcuHeight)

{
	int noOfxMCU = width/mcuWidth;
	int noOfyMCU = height/mcuHeight;
	int noOfComponents=3;
	int verticalSamplingFactor[3] = {2,1,1};
	int horizontalSamplingFactor[3] = {2,1,1};
	int noOfMCUComponent;
	unsigned char dst[64];
	__global short *tempPtr1;
	__global short *pMCUdata[3] = {pMCUdata1,pMCUdata2,pMCUdata3};
	__global unsigned short *pQuantTable[3] = {pQuantTable1,pQuantTable2,pQuantTable3};
	int index = 0;
	int tby = get_global_size(1);
	int by = get_global_id(1);
	noOfyMCU = noOfyMCU/tby;
	for(int i=0;i<noOfComponents;i++) 
	{
		for(int yMCU=0;yMCU<noOfyMCU;yMCU++)
		{
			for(int xMCU=0;xMCU<noOfxMCU;xMCU++) 
			{
				for(unsigned int j=0;j<verticalSamplingFactor[i];j++)
                {
                    for(unsigned int k=0;k<horizontalSamplingFactor[i];k++)
                    {
						noOfMCUComponent = verticalSamplingFactor[i]*horizontalSamplingFactor[i];
						tempPtr1 = pMCUdata[i] + noOfxMCU * by *noOfyMCU* (noOfMCUComponent*64)+
									yMCU * noOfxMCU * (noOfMCUComponent*64) + 
									(xMCU * noOfMCUComponent * 64) + 
									(j*verticalSamplingFactor[i] + k)*64;
						DCTQuantInv8x8LS_JPEG_16s8u(tempPtr1,dst,8,(pQuantTable[i]));
						for (int l=0;l<8;l++)
						{
							for (int m=0;m<8;m++)
							{
								if (i == 0)
								{
									index = by * noOfyMCU*width*mcuHeight +  width*yMCU*mcuHeight + xMCU*mcuWidth + 
											j*8*width + k*8 +
											l*width + m;
									output[index] = dst[l*8 + m];
								}
								else
								{
									index = (i*width*height) + by*noOfyMCU*width*mcuHeight + width*yMCU*mcuHeight + xMCU*mcuWidth + 2*l*width + 2*m ;
									output[index] = dst[l*8 + m];
									index = (i*width*height) + by*noOfyMCU*width*mcuHeight + width*yMCU*mcuHeight + xMCU*mcuWidth + 2*l*width + 2*m + 1;
									output[index] = dst[l*8 + m];
									index = (i*width*height) + by*noOfyMCU*width*mcuHeight + width*yMCU*mcuHeight + xMCU*mcuWidth + (2*l + 1)* width + 2*m;
									output[index] = dst[l*8 + m];
									index = (i*width*height) + by*noOfyMCU*width*mcuHeight + width*yMCU*mcuHeight + xMCU*mcuWidth + (2*l + 1)* width + 2*m + 1;
									output[index] = dst[l*8 + m];
								}
							}
						}
					}
				}
			}
		}
	}
}

