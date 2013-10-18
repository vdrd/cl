/*
Copyright (c) 2006-2009 Advanced Micro Devices, Inc. All Rights Reserved.
This software is subject to the Apache v2.0 License.
*/

//************************************************************************* 
//This file include JPEG Encoder and decoder functions from JPEG Chapter
//	fwiEncodeHuffmanRawTableInit_JPEG_8u
//	fwiEncodeHuffmanSpecGetBufSize_JPEG
//  fwiEncodeHuffmanSpecInit_JPEG
//  fwiEncodeHuffmanSpecInitAlloc_JPEG
//  fwiEncodeHuffmanSpecFree_JPEG
//	fwiEncodeHuffmanStateGetBufSize_JPEG
//  fwiEncodeHuffmanStateInit_JPEG
//  fwiEncodeHuffmanStateInitAlloc_JPEG
//  fwiEncodeHuffmanStateFree_JPEG
//  fwiEncodeHuffman8x8_JPEG
//  fwiEncodeHuffman8x8_Direct_JPEG
//  fwiGetHuffmanStatistic8x8_JPEG
//  fwiGetHuffmanStatistic8x8_DCFirst_JPEG
//  fwiGetHuffmanStatistic8x8_ACFirst_JPEG
//  fwiGetHuffmanStatistic8x8_ACRefine_JPEG
//  fwiEncodeHuffman8x8_DCFirst_JPEG
//  fwiEncodeHuffman8x8_DCRefine_JPEG
//  fwiEncodeHuffman8x8_ACFirst_JPEG
//  fwiEncodeHuffman8x8_ACRefine_JPEG
//	fwiDecodeHuffmanSpecGetBufSize_JPEG
//  fwiDecodeHuffmanSpecInit_JPEG
//  fwiDecodeHuffmanSpecInitAlloc_JPEG
//  fwiDecodeHuffmanSpecFree_JPEG
//	fwiDecodeHuffmanStateGetBufSize_JPEG
//  fwiDecodeHuffmanStateInit_JPEG
//  fwiDecodeHuffmanStateInitAlloc_JPEG
//  fwiDecodeHuffmanStateFree_JPEG
//  fwiDecodeHuffman8x8_JPEG
//  fwiDecodeHuffman8x8_Direct_JPEG
//  fwiDecodeHuffman8x8_DCFirst_JPEG
//  fwiDecodeHuffman8x8_DCRefine_JPEG
//  fwiDecodeHuffman8x8_ACFirst_JPEG
//  fwiDecodeHuffman8x8_ACRefine_JPEG
//************************************************************************* 

#include "fwdev.h"
#include "fwJPEG.h"
#include "JPEG-HuffCodec.h"

using namespace OPT_LEVEL;

#if BUILD_NUM_AT_LEAST( 102 )

//extra data in the array to prevent table overrun
extern const Fw8u zigZagFwdOrder[80];// =
//{
//	0,   1,  8, 16,  9,  2,  3, 10, 
//	17, 24, 32, 25, 18, 11,  4,  5,
//	12, 19, 26, 33, 40, 48, 41, 34, 
//	27, 20, 13,  6,  7, 14, 21, 28,
//	35, 42, 49, 56, 57, 50, 43, 36, 
//	29, 22, 15, 23, 30, 37, 44, 51,
//	58, 59, 52, 45, 38, 31, 39, 46, 
//	53, 60, 61, 54, 47, 55, 62, 63,
//	63, 63, 63, 63, 63, 63, 63, 63,
//	63, 63, 63, 63, 63, 63, 63, 63
//};

//-----------------------------------------------------------------------
//This function creates raw Huffman tables with symbols statistics.  It 
//follows standards from annex k.2 at page 145
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiEncodeHuffmanRawTableInit_JPEG_8u)(const int pStatistics[256],
																		Fw8u *pListBits, Fw8u *pListVals)
{
	if (pStatistics == 0 || pListBits == 0 || pListVals == 0)
		return fwStsNullPtrErr;

	int codesize[257];		// code size of symbol V
	int others[257];		// Index to next symbol in chain of all 
	// symbols in current branch of code tree
	int freq[257];			// copy of pStatistics[]
	int v1, v2, v;
	int i, j, k;

	//The following algorithm follows JPEG standard annex k.2 at page 145

	//Initialization
	memset(codesize, 0, 1028);
	memset(others, -1, 1028); 
	memcpy(freq, pStatistics, 1024);//256<<2

	freq[256] = 1;		// to reserve one code point

	//generation of the list of lengths and values which are in accord with 
	//the rules for generating the Huffman code tables
	//Procedure to find Huffman code sizes
	//Maximum 256 cycles, we will complete the call
	for (j=0; j<257; j++) {
		//Find V1 for least value of FREQ(V1) > 0
		//selects the value with the largest value of V1 when
		//more than one V1 with the same frequency occurs
		v1 = -1;
		v = 0x7fffffff; //maximum signed 32-bit integer
		for (i = 0; i <= 256; i++) {
			if (freq[i] > 0 && freq[i] <= v) {
				v = freq[i];
				v1 = i;
			}
		}

		//Find V2 for next least value of FREQ(V2) > 0
		v2 = -1;
		v = 0x7fffffff;
		//i!= v1
		for (i = 0; i < v1; i++) {
			if (freq[i] > 0 && freq[i] <= v) {
				v = freq[i];
				v2 = i;
			}
		}
		for (i = v1+1; i <=256; i++) {
			if (freq[i] > 0 && freq[i] <= v) {
				v = freq[i];
				v2 = i;
			}
		}

		// V2 exists?
		if (v2 < 0) break;

		freq[v1] += freq[v2];
		freq[v2] = 0;

		codesize[v1]++;
		while (others[v1] !=-1 ) {
			v1 = others[v1];
			codesize[v1]++;
		}

		others[v1] = v2;	

		codesize[v2]++;
		while (others[v2] != -1) {
			v2 = others[v2];
			codesize[v2]++;
		}
	}

	//Procedure to find the number of codes of each size
	Fw8u bits[33];	        // number of codes with length i
	memset(bits, 0, 33);

	for (i = 0; i < 257; i++) {
		if (codesize[i]!=0) {
			// The JPEG standard assumes that the probabilities are large 
			//enough that code lengths greater than 32 bits never occur.
			bits[codesize[i]]++;
		}
	}

	//adjusting the BITS list so that no code is longer than 16 bits. Since 
	//symbols are paired for the longest Huffman code, the symbols are removed 
	//from this length category two at a time. The prefix for the pair (which is 
	//one bit shorter) is allocated to one of the pair; then (skip the BITS 
	//entry for that prefix length) a code word from the next shortest non-zero 
	//BITS entry is converted into a prefix for two code words one bit longer. 
	//After the BITS list is reduced to a maximum code length of 16 bits, the 
	//last step removes the reserved code point from the code length count.

	//Procedure for limiting code lengths to 16 bits
	for (i = 32; i > 16; i--) {
		while (bits[i] > 0) {
			j = i - 2;		
			while (bits[j] == 0) j--;

			bits[i] -= 2;		
			bits[i-1]++;		
			bits[j+1] += 2;		
			bits[j]--;		
		}
	}

	while (bits[i] == 0) i--;

	bits[i]--;

	// output symbol counts from 1 to 16
	memcpy(pListBits, &bits[1], 16);

	//Sorting of input values according to code size
	k = 0;
	for (i = 1; i < 33; i++) {
		for (j = 0; j < 256; j++) {
			if (codesize[j] == i) {
				pListVals[k] = (Fw8u) j;
				k++;
			}
		}
	}

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//This function returns the buffer size (in bytes) of FwiEncodeHuffmanSpec 
//structure.
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiEncodeHuffmanSpecGetBufSize_JPEG_8u)(int* size)
{
	//fixed data size
	*size = sizeof(FwiEncodeHuffmanSpec);

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//This function creates Huffman table of codes and code length for Encoder.
//It follows Annex C.1,C.2,C.3 from CCITT Rec. T.81(1992 E) page 50
//-----------------------------------------------------------------------
namespace OPT_LEVEL
    {
SYS_INLINE STATIC FwStatus MyFW_HuffmanSpecInit(const Fw8u *pListBits, const Fw8u *pListVals, 
								Fw16u ehufco[256], Fw16u ehufsi[256])
{
    int i, j, k, bits;
    Fw16u code=0;
    int val;
    k=0;
	
    memset(ehufsi, 0, 512);
	memset(ehufco, 0, 512);

    for(i=1;i<17;i++)
        {
        bits = pListBits[i-1];

        if (bits+k > 256) return fwStsJPEGHuffTableErr;

        for(j=0;j<bits;j++)
            {
            val = pListVals[k];
            ehufsi[val] = (Fw16u)i;
            ehufco[val] = code++;
            k++;
            }
        code<<=1;
        }
    return fwStsNoErr;
}
    }//namespace OPT_LEVEL
FwStatus PREFIX_OPT(OPT_PREFIX, fwiEncodeHuffmanSpecInit_JPEG_8u)(
	const Fw8u *pListBits, const Fw8u *pListVals,  FwiEncodeHuffmanSpec *pEncHuffSpec) 
{
	if (pListBits == 0 || pListVals==0 || pEncHuffSpec==0)
		return fwStsNullPtrErr;

	return MyFW_HuffmanSpecInit(pListBits, pListVals, pEncHuffSpec->symcode,
		pEncHuffSpec->symlen);
}

//-----------------------------------------------------------------------
//This function allocates memory and create huffman table for Encoder
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiEncodeHuffmanSpecInitAlloc_JPEG_8u)(
	const Fw8u *pListBits, const Fw8u *pListVals, FwiEncodeHuffmanSpec** pEncHuffSpec)
{
	int size;

	fwiEncodeHuffmanSpecGetBufSize_JPEG_8u(&size);
	*pEncHuffSpec = (FwiEncodeHuffmanSpec *) fwMalloc (size);

	return fwiEncodeHuffmanSpecInit_JPEG_8u(pListBits, pListVals, *pEncHuffSpec);
}

//-----------------------------------------------------------------------
//This function frees the memory allocated by EncodeHuffmanSpecInitAlloc
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiEncodeHuffmanSpecFree_JPEG_8u)(FwiEncodeHuffmanSpec *pEncHuffSpec)
{
	fwFree(pEncHuffSpec);

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//This function returns the buffer size (in bytes) of fwiEncodeHuffmanState 
//structure.
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiEncodeHuffmanStateGetBufSize_JPEG_8u)(int* size)
{
	//add some alignment requirement
	*size = sizeof(FwiEncodeHuffmanState) + 8;

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//This function initializes FwiEncodehuffmanState structure
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiEncodeHuffmanStateInit_JPEG_8u)(FwiEncodeHuffmanState *pEncHuffState)
{
	pEncHuffState->accbitnum = 0;
	pEncHuffState->accbuf = 0;
	pEncHuffState->EOBRUN = 0;
	pEncHuffState->BE     = 0;
	//1024 bits reserved for bit correction
	memset(pEncHuffState->cor_AC, 0, 1024);

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//This function allocates memory and initialize FwiEncodehuffmanState structure
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiEncodeHuffmanStateInitAlloc_JPEG_8u)(
	FwiEncodeHuffmanState** pEncHuffState)
{
	int size;

	fwiEncodeHuffmanStateGetBufSize_JPEG_8u(&size);
	*pEncHuffState = (FwiEncodeHuffmanState *) fwMalloc (size);

	return fwiEncodeHuffmanStateInit_JPEG_8u(*pEncHuffState);
}

//-----------------------------------------------------------------------
//This function free the memory allocated by EncodeHuffmanStateInitAlloc
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiEncodeHuffmanStateFree_JPEG_8u)(FwiEncodeHuffmanState *pEncHuffState)
{
	fwFree(pEncHuffState);

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//This function perform Huffman Baseline encoding of an 8*8 block using 
//pDcTable and pAcTable. The encoding process follows CCITT Rec. T.81
//Annex F.1.2. This function will only write full bytes to the output buffer.
//In the case of incomplete bits, the FwiEncodeHuffmanState structure will
//accmulate these bits. User can set the bFlashState to be 1 to force to add
//the accmulated bits for the last 8*8 block or restart encoded interval.
//-----------------------------------------------------------------------
//This internal function put the bits to destination buffer
//size 0 will do nothing
namespace OPT_LEVEL
    {

bool SYS_INLINE EncStuffbits (FwiEncodeHuffmanState * pEncHuffState, Fw16u code, int size, 
					Fw8u *pDst, int dstLenBytes, int *pDstCurrPos)

{
	//since size > 0, we have incoming data
	if ((*pDstCurrPos) >= dstLenBytes) {
		*pDstCurrPos += ((pEncHuffState->accbitnum + size+7)>>3);
		pEncHuffState->accbitnum = 0;
		return false;
	}
	//get rid of excessive code
	code &= ((1<<size)-1);
	int bitlen = pEncHuffState->accbitnum + size;
	if (bitlen < 8) {
		pEncHuffState->accbitnum = bitlen;
		pEncHuffState->accbuf = code ^ (pEncHuffState->accbuf << size);
		return true;
	}
	int rembit = bitlen & 0x7;
	pEncHuffState->accbitnum = rembit;
	Fw32u buf = code >> rembit;
	Fw32u accbuf = pEncHuffState->accbuf;
	pEncHuffState->accbuf = code ^ (buf <<rembit); 
	size-=rembit;

	bitlen = bitlen >> 3;
	buf = buf | (accbuf << size);
	Fw8u tempacc;

	if (bitlen ==1) {
		if (buf == 0xff) {
			//protect buffer overrun
			if ((*pDstCurrPos) > dstLenBytes-2) {
				*pDstCurrPos += 2;
				return false;
			}
			pDst[(*pDstCurrPos)++] = (Fw8u)buf;
			//a "0" byte is stuffed into the code string
			// if the byte value "0xff" is created.
			pDst[(*pDstCurrPos)++] = 0;
		} else {
			pDst[(*pDstCurrPos)++] = (Fw8u)buf;
		}
	} else { //bitlen ==2 
		tempacc = (Fw8u)(buf >> 8);
		if (tempacc == 0xff){
			//protect buffer overrun
			//at least one more byte is coming
			if ((*pDstCurrPos) > dstLenBytes-3) {
				*pDstCurrPos += 3;
				return false;
			}
			pDst[(*pDstCurrPos)++] = tempacc;
			pDst[(*pDstCurrPos)++] = 0;
		} else {
			//at least one more byte is coming
			if ((*pDstCurrPos) > dstLenBytes-2) {
				*pDstCurrPos += 2;
				return false;
			};
			pDst[(*pDstCurrPos)++] = tempacc;
		}

		tempacc = (Fw8u)(buf & 0xff);
		if (tempacc == 0xff){
			if ((*pDstCurrPos) > dstLenBytes-2) {
				*pDstCurrPos += 2;
				return false;
			};
			pDst[(*pDstCurrPos)++] = tempacc;
			pDst[(*pDstCurrPos)++] = 0;
		} else {
			pDst[(*pDstCurrPos)++] = tempacc;
		}
	}

	return true;
}

bool EncStateFlush (FwiEncodeHuffmanState * pEncHuffState, Fw8u *pDst, 
					 int dstLenBytes, int *pDstCurrPos)
{
	// padding incomplete bytes with 1-bits
	// no checking for return value
	EncStuffbits(pEncHuffState, 0x7F, 7, pDst, dstLenBytes, pDstCurrPos);

	//flush the state
	pEncHuffState->accbuf = 0;
	pEncHuffState->accbitnum = 0;

	return true;
}

//for JPEG use only
//For greater than 2^16, the answer will be 17
int SYS_INLINE LeadBit (int ssss) 
{
	if (ssss==0) return 0;

	int numbit=1;

	if (ssss > 0xff) {
		numbit += 8;
		ssss >>= 8;
	}

	if (ssss > 0xf) {
		numbit +=4;
		ssss >>= 4;
	} 

	if (ssss > 0x7) {
		numbit += 2;
		ssss >>= 2;
	}

	if (ssss > 0x3) {
		numbit +=1;
		ssss >>=1;
	}

	if (ssss > 0x1)
		numbit ++;

	return numbit;
}

}//namespace OPT_LEVEL

FwStatus PREFIX_OPT(OPT_PREFIX, fwiEncodeHuffman8x8_JPEG_16s1u_C1)(
	const Fw16s *pSrc, Fw8u *pDst, int dstLenBytes, int *pDstCurrPos, 
	Fw16s *pLastDC, const FwiEncodeHuffmanSpec *pDcTable, const 
	FwiEncodeHuffmanSpec *pAcTable, FwiEncodeHuffmanState *pEncHuffState, int bFlushState)
{
return EncodeHuffman8x8_JPEG_16s1u_C1(pSrc, pDst, dstLenBytes, pDstCurrPos, pLastDC, pDcTable, pAcTable, pEncHuffState, bFlushState);
}

//-----------------------------------------------------------------------
//This function directly performs an 8*8 block encoding for quatized DCT 
//coefficients with the DC and AC huffmann code table, but without the 
//internal structure to accumulate the bits. The encoding procedure will
//still follow CCITT Rec. T.81 Annex F.1.2.
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiEncodeHuffman8x8_Direct_JPEG_16s1u_C1)(
	const Fw16s *pSrc, Fw8u *pDst, int *pDstBitsLen, Fw16s *pLastDC,
	const FwiEncodeHuffmanSpec *pDcTable, const FwiEncodeHuffmanSpec *pAcTable) 
{
	FwiEncodeHuffmanState *pEncHuffState;
	int size, dstLenBytes=128; // just a large enough buffer for dstLenBytes
	FwStatus status;
	int pDstCurrPos=0;

	fwiEncodeHuffmanStateGetBufSize_JPEG_8u(&size);
	pEncHuffState = (FwiEncodeHuffmanState *) fwMalloc (size);

	status = fwiEncodeHuffmanStateInit_JPEG_8u(pEncHuffState);
	if (status != fwStsNoErr) return status;

	status = fwiEncodeHuffman8x8_JPEG_16s1u_C1(pSrc, pDst, dstLenBytes, &pDstCurrPos,
		pLastDC, pDcTable, pAcTable, pEncHuffState, 0);
	if (status != fwStsNoErr) return status;

	//Flush the state
	status = fwiEncodeHuffman8x8_JPEG_16s1u_C1(0, pDst, dstLenBytes, &pDstCurrPos,
		0, 0, 0, pEncHuffState, 1);

	*pDstBitsLen = pDstCurrPos <<3;

	fwFree(pEncHuffState);	

	return status;
}

//-----------------------------------------------------------------------
//This function performs the first scan for progressive encoding of the DC 
//coefficient from an 8*8 block. The encoding process follows CCITT Rec. T.81
//Annex G.1.2. This function will only write full bytes to the output buffer.
//In the case of incomplete bits, the FwiEncodeHuffmanState structure will
//accmulate these bits. User can set the bFlashState to be 1 to force to add
//the accmulated bits for the last 8*8 block or restart encoded interval.
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiEncodeHuffman8x8_DCFirst_JPEG_16s1u_C1)(
	const Fw16s *pSrc, Fw8u *pDst, int dstLenBytes, int *pDstCurrPos, 
	Fw16s *pLastDC, int Al, const FwiEncodeHuffmanSpec *pDcTable, 
	FwiEncodeHuffmanState *pEncHuffState, int bFlushState)
{
	if (pDst == 0 || pDstCurrPos==0 ||pEncHuffState == 0 ) 
		return fwStsNullPtrErr;

	if (dstLenBytes < 2 || *pDstCurrPos <0 || *pDstCurrPos > dstLenBytes || Al < 0)
		return fwStsBadArgErr;

	//When pSrc is 0 and bFlushstate is set, we flush the bits
	if (bFlushState) {
		EncStateFlush(pEncHuffState, pDst, dstLenBytes, pDstCurrPos);
		//final check range
		if (*pDstCurrPos > dstLenBytes) return fwStsSizeErr;
		return fwStsNoErr;
	} else {
		if (pSrc == 0)
			return fwStsNullPtrErr;
	}

	if (pLastDC == 0 || pDcTable ==0) return fwStsNullPtrErr;

	int diff, loworder;
	int ssss;

	loworder = ((int) (pSrc[0])>> Al);
	diff = loworder - *pLastDC;
	*pLastDC = (Fw16s) loworder;

	//The following procedure follows JPEG standard section G.1.2.1
	if (diff < 0) {
		loworder = diff-1; 
		diff  = - diff;
	} else loworder = diff;

	ssss = LeadBit(diff);
	if (ssss > 11) return fwStsJPEGOutOfBufErr;

	EncStuffbits(pEncHuffState, pDcTable->symcode[ssss], pDcTable->symlen[ssss],
		pDst, dstLenBytes, pDstCurrPos);

	if (ssss)	{
		EncStuffbits(pEncHuffState, (Fw16u)loworder, ssss, pDst, dstLenBytes, pDstCurrPos);
	}

	//final check range
	if (*pDstCurrPos > dstLenBytes) return fwStsSizeErr;

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//This function performs the subsequent scan for progressive encoding of the DC 
//coefficient from an 8*8 block. The encoding process follows CCITT Rec. T.81
//Annex G.1.2. This function will only write full bytes to the output buffer.
//In the case of incomplete bits, the FwiEncodeHuffmanState structure will
//accmulate these bits. User can set the bFlashState to be 1 to force to add
//the accmulated bits for the last 8*8 block or restart encoded interval.
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiEncodeHuffman8x8_DCRefine_JPEG_16s1u_C1)(
	const Fw16s *pSrc, Fw8u *pDst, int dstLenBytes, int *pDstCurrPos, int Al,
	FwiEncodeHuffmanState *pEncHuffState, int bFlushState)
{
	if (pDst == 0 || pDstCurrPos==0 || pEncHuffState ==0)
		return fwStsNullPtrErr;

	if (dstLenBytes < 2 || *pDstCurrPos <0 || *pDstCurrPos > dstLenBytes
		|| Al < 0)
		return fwStsBadArgErr;

	//When pSrc is 0 and bFlushstate is set, we flush the bits
	if (bFlushState) {
		EncStateFlush(pEncHuffState, pDst, dstLenBytes, pDstCurrPos);
		//final check range
		if (*pDstCurrPos > dstLenBytes) return fwStsSizeErr;
		return fwStsNoErr;
	} else {
		if (pSrc == 0)
			return fwStsNullPtrErr;
	}

	int diff;

	diff = pSrc[0];
	if (diff <0) diff= -diff;
	diff = diff >> Al;

	int ssss = LeadBit(diff);
	if (ssss > 11) return fwStsJPEGOutOfBufErr;

	//using successive approximation the least significant bits are appended to the 
	//compressed bit stream without compression or modification
	//This approach is to be confirmed
	EncStuffbits(pEncHuffState, (Fw16u) diff, 1, pDst, dstLenBytes, pDstCurrPos);

	//final check range
	if (*pDstCurrPos > dstLenBytes) return fwStsSizeErr;

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//This function performs the first scan for progressive encoding of the AC 
//coefficient from an 8*8 block. The encoding process follows CCITT Rec. T.81
//Annex G.1.2. This function will only write full bytes to the output buffer.
//In the case of incomplete bits, the FwiEncodeHuffmanState structure will
//accmulate these bits. User can set the bFlashState to be 1 to force to add
//the accmulated bits for the last 8*8 block or restart encoded interval.
//-----------------------------------------------------------------------
namespace OPT_LEVEL
    {
bool EncStuff_EOBRUN (FwiEncodeHuffmanState *pEncHuffState, Fw8u *pDst, 
					  int dstLenBytes, int *pDstCurrPos, const FwiEncodeHuffmanSpec *pAcTable)
{
	int EOBRUN, ssss;
	int I;

	EOBRUN = pEncHuffState->EOBRUN;

	ssss = LeadBit(EOBRUN);
	if (ssss > 14) return false;

	I = ssss <<4; //ssss*16
	EncStuffbits(pEncHuffState, pAcTable->symcode[I], pAcTable->symlen[I], 
		pDst, dstLenBytes, pDstCurrPos);

	if (ssss) {
		EncStuffbits(pEncHuffState, (Fw16u)pEncHuffState->EOBRUN, ssss, pDst, 
			dstLenBytes, pDstCurrPos);
	}

	pEncHuffState->EOBRUN = 0;

	return true;
}
    }//namespace OPT_LEVEL
FwStatus PREFIX_OPT(OPT_PREFIX, fwiEncodeHuffman8x8_ACFirst_JPEG_16s1u_C1)(
	const Fw16s *pSrc, Fw8u *pDst, int dstLenBytes, int *pDstCurrPos, 
	int Ss, int Se, int Al, const FwiEncodeHuffmanSpec *pAcTable, 
	FwiEncodeHuffmanState *pEncHuffState, int bFlushState)
{
	if (pDst == 0 || pDstCurrPos==0 || pEncHuffState == 0 || pAcTable ==0) 
		return fwStsNullPtrErr;

	if (dstLenBytes < 2 || *pDstCurrPos <0 || *pDstCurrPos > dstLenBytes || 
		Al <0 ||Ss < 1 ||Se > 63)
		return fwStsBadArgErr;

	//When pSrc is 0 and bFlushstate is set, we flush the bits
	if (bFlushState) {
		//Encode_EOBRUN
		if (pEncHuffState->EOBRUN > 0) {
			EncStuff_EOBRUN(pEncHuffState, pDst, dstLenBytes, pDstCurrPos, pAcTable);
		}

		if (pEncHuffState->accbitnum > 0) {
			EncStateFlush(pEncHuffState, pDst, dstLenBytes, pDstCurrPos);
		}

		//final check range
		if (*pDstCurrPos > dstLenBytes) return fwStsSizeErr;
		return fwStsNoErr;

	} else {
		if (pSrc == 0)
			return fwStsNullPtrErr;
	}

	int ZZ, k, loworder, ssss, I;
	int runlen; //runlen is the run length of zero coefficients

	// The following procedure follos JPEG standard G.1.2.2 and 
	// its flow charts G.3, G.4, G.5, and G.6 
	runlen = 0;	

	for (k = Ss; k <= Se; k++) {
		ZZ = pSrc[zigZagFwdOrder[k]];

		if (ZZ < 0) {
			ZZ = -ZZ;		
			ZZ >>= Al;	
			loworder= -ZZ -1;
		} else {
			ZZ >>= Al;		
			loworder = ZZ;
		}

		if (ZZ == 0) {
			runlen++;
			continue;
		}

		//Encode_EOBRUN
		if (pEncHuffState->EOBRUN > 0)
			EncStuff_EOBRUN(pEncHuffState, pDst, dstLenBytes, pDstCurrPos, pAcTable);

		//Encode_ZRL
		while (runlen >= 16) {
			EncStuffbits(pEncHuffState, pAcTable->symcode[0xF0], pAcTable->symlen[0xF0], 
				pDst, dstLenBytes, pDstCurrPos);
			runlen -= 16;
		}

		//Encode_R_ZZ
		ssss = LeadBit(ZZ);
		if (ssss > 10) return fwStsJPEGOutOfBufErr;

		I =(runlen << 4) + ssss;
		EncStuffbits(pEncHuffState, pAcTable->symcode[I], pAcTable->symlen[I], 
			pDst, dstLenBytes, pDstCurrPos);

		EncStuffbits(pEncHuffState, (Fw16u)loworder, ssss, pDst, dstLenBytes, pDstCurrPos);

		runlen = 0;
	}

	//ZZ(K)=0 and K=Se case, otherwise runlen=0
	if (runlen > 0) {
		pEncHuffState->EOBRUN++;		
		if (pEncHuffState->EOBRUN == 0x7FFF)
			EncStuff_EOBRUN(pEncHuffState, pDst, dstLenBytes, pDstCurrPos, pAcTable);	
	}

	//final check range
	if (*pDstCurrPos > dstLenBytes) return fwStsSizeErr;

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//This function performs the subsequent scan for progressive encoding of the AC 
//coefficient from an 8*8 block. The encoding process follows CCITT Rec. T.81
//Annex G.1.2. This function will only write full bytes to the output buffer.
//In the case of incomplete bits, the FwiEncodeHuffmanState structure will
//accmulate these bits. User can set the bFlashState to be 1 to force to add
//the accmulated bits for the last 8*8 block or restart encoded interval.
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiEncodeHuffman8x8_ACRefine_JPEG_16s1u_C1)(
	const Fw16s *pSrc, Fw8u *pDst, int dstLenBytes, int *pDstCurrPos, 
	int Ss, int Se, int Al, const FwiEncodeHuffmanSpec *pAcTable, 
	FwiEncodeHuffmanState *pEncHuffState, int bFlushState)
{
	if (pDst == 0 || pDstCurrPos==0 || pEncHuffState ==0 || pAcTable ==0)
		return fwStsNullPtrErr;

	if (dstLenBytes < 2 || *pDstCurrPos <0 || *pDstCurrPos > dstLenBytes || 
		Al <0 || Ss < 1 ||Se > 63)
		return fwStsBadArgErr;

	int i;

	//When pSrc is 0 and bFlushstate is set, we flush the bits
	if (bFlushState) {
		//Encode_EOBRUN
		if (pEncHuffState->EOBRUN > 0) {
			EncStuff_EOBRUN(pEncHuffState, pDst, dstLenBytes, pDstCurrPos, pAcTable);
		}

		//Append_BE_bit
		for (i=0; i<pEncHuffState->BE; i++) {
			EncStuffbits(pEncHuffState, (Fw16u)(pEncHuffState->cor_AC[i]), 1, pDst, 
				dstLenBytes, pDstCurrPos);
		}  
		pEncHuffState->BE = 0;

		if (pEncHuffState->accbitnum > 0) {
			EncStateFlush(pEncHuffState, pDst, dstLenBytes, pDstCurrPos);
		}

		return fwStsNoErr;
	} else {
		if (pSrc == 0)
			return fwStsNullPtrErr;
	}

	int runlen, k, I, EOB, BR, BE;
	Fw16s ZZ[64];
	Fw16u ZZsgn[64];

	// The following procedure follows JPEG standard G.1.2.3,
	// and its flow chars figure G.7, G.8 and G.9

	//To determine EOB, we must scan all data.
	EOB = 0;
	for (k=Ss;k<=Se;k++) {
		ZZ[k]=pSrc[zigZagFwdOrder[k]];
		if (ZZ[k] <0) {
			ZZsgn[k] = 0;
			ZZ[k] = -ZZ[k];
		} else {
			ZZsgn[k] = 1;
		}
		ZZ[k] >>= Al;

		if (ZZ[k] == 1) EOB=k;
	}

	runlen = 0;			
	BR = 0;			

	for (k = Ss; k <= Se; k++) {
		if (ZZ[k] == 0) {
			runlen++;
			//runlen > 0 will be handled at the end.
			continue;
		}

		while (runlen >= 16 && k < EOB) {
			//Encode_EOBRUN
			if (pEncHuffState->EOBRUN > 0) {
				EncStuff_EOBRUN(pEncHuffState, pDst, dstLenBytes, pDstCurrPos, pAcTable);
			}

			//Append_BE_bit
			BE = pEncHuffState->BE;
			for (i=0; i<BE; i++) {
				EncStuffbits(pEncHuffState, (Fw16u)(pEncHuffState->cor_AC[i]), 1, pDst, 
					dstLenBytes, pDstCurrPos);
			}  
			pEncHuffState->BE = 0;

			//Encode_ZRL
			EncStuffbits(pEncHuffState, pAcTable->symcode[0xF0], pAcTable->symlen[0xF0], 
				pDst, dstLenBytes, pDstCurrPos);
			runlen -= 16;

			//Append BR_bits
			for (i=0; i<BR; i++) {
				EncStuffbits(pEncHuffState, (Fw16u)(pEncHuffState->cor_AC[BE+i]), 1, pDst, 
					dstLenBytes, pDstCurrPos);
			}
			BR = 0;
		}

		if (ZZ[k] > 1) {
			//Append LSB of ZZ(K) to buffered bits
			pEncHuffState->cor_AC[pEncHuffState->BE+BR] = (Fw8u) (ZZ[k] & 0x1);
			BR++;
			//BR>0 will be handled with runlen > 0.
			continue;
		}

		//Encode_EOBRUN
		if (pEncHuffState->EOBRUN > 0) {
			EncStuff_EOBRUN(pEncHuffState, pDst, dstLenBytes, pDstCurrPos, pAcTable);
		}

		//Append_BE_bit
		BE = pEncHuffState->BE;
		for (i=0; i<BE; i++) {
			EncStuffbits(pEncHuffState, (Fw16u)(pEncHuffState->cor_AC[i]), 1, pDst, 
				dstLenBytes, pDstCurrPos);
		}  
		pEncHuffState->BE = 0;

		//Encode_R_ZZ(K) with ZZ(k)==1 case, so ssss=1 for sure
		I = (runlen << 4) + 1;
		EncStuffbits(pEncHuffState, pAcTable->symcode[I], pAcTable->symlen[I], 
			pDst, dstLenBytes, pDstCurrPos);
		//Append 1 bit for the newly non-zero-coefficient
		EncStuffbits(pEncHuffState, ZZsgn[k], 1, pDst, dstLenBytes, pDstCurrPos);
		runlen = 0;

		//Append BR_bits
		for (i=0; i<BR; i++) {
			EncStuffbits(pEncHuffState, (Fw16u)(pEncHuffState->cor_AC[BE+i]), 1, pDst, 
				dstLenBytes, pDstCurrPos);
		}
		BR = 0;

	}

	if (runlen > 0 || BR > 0) {	
		pEncHuffState->EOBRUN++;		
		pEncHuffState->BE += BR;

		//correction bit reserved to 1024 bits, we will leave one block free
		if (pEncHuffState->EOBRUN == 0x7FFF || pEncHuffState->BE > 960){
			//Encode_EOBRUN
			EncStuff_EOBRUN(pEncHuffState, pDst, dstLenBytes, pDstCurrPos, pAcTable);

			//Append_BE_bit
			BE = pEncHuffState->BE;
			for (i=0; i<BE; i++) {
				EncStuffbits(pEncHuffState, (Fw16u)(pEncHuffState->cor_AC[i]), 1, pDst, 
					dstLenBytes, pDstCurrPos);
			}  
			pEncHuffState->BE = 0;
		}
	}

	//final check range
	if (*pDstCurrPos > dstLenBytes) return fwStsSizeErr;

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//This functions computes the statistics for the baseline encoding.
//-----------------------------------------------------------------------
namespace OPT_LEVEL
    {
static int MSB_lookup[16]= {
	0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4 
};

int MSB_pos_16s(const Fw16s number) 
{
	Fw16u u16 = (((Fw16u) number) & 0x7fff);
	Fw16u u8;
	int pos=0;

	u8=(u16&0x7f00);
	if(u8) {
		u16 = u16 >> 8;
		pos+=8;
	}
	u8 = (u16 &0xf);
	if (u8) {
		u16 = u16>>4;
		pos+=4;
	}

	return pos+MSB_lookup[u16];
}
    }//namespace OPT_LEVEL
FwStatus PREFIX_OPT(OPT_PREFIX, fwiGetHuffmanStatistics8x8_JPEG_16s_C1)(
	const Fw16s *pSrc, int pDcStatistics[256], int pAcStatistics[256], Fw16s *pLastDC)
{
	if (pSrc==0 || pDcStatistics ==0 ||
		pAcStatistics ==0 || pLastDC ==0) 
		return fwStsNullPtrErr;

	//DC stat value
	pDcStatistics[MSB_pos_16s(*pLastDC)] ++;

	//AC Stat value
	//The corresponding use different value for zero value
	for (int i=0; i< 64; i++) {
		pAcStatistics[MSB_pos_16s(pSrc[i])] ++;
	}

	//Update last DC value
	*pLastDC=pSrc[0];

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//This functions computes the first scan statistics for DC coefficients in 
//progressivce encoding.
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiGetHuffmanStatistics8x8_DCFirst_JPEG_16s_C1)(
	const Fw16s *pSrc, int pDcStatistics[256], Fw16s *pLastDC, int Al)
{
	if (pSrc==0 || pDcStatistics ==0 || pLastDC ==0) 
		return fwStsNullPtrErr;

	if (Al <0) return fwStsBadArgErr;

	Fw16s diff, loworder, temp3;

	diff = (Fw16s)((pSrc[0])>> Al);

	if (diff <0) loworder=-diff;
	else loworder=diff;
	if (*pLastDC<0) temp3 = - (*pLastDC);
	else temp3 = (*pLastDC);

	*pLastDC = (Fw16s) diff;
	if (temp3 >= 2048+loworder) return fwStsJPEGDCTRangeErr;

	pDcStatistics[MSB_pos_16s(temp3)]++;

	return fwStsNoErr;
}

#if BUILD_NUM_AT_LEAST(9999)
//-----------------------------------------------------------------------
//This functions computes the first scan statistics for AC coefficients in 
//progressivce encoding.
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiGetHuffmanStatistics8x8_ACFirst_JPEG_16s_C1)(
	const Fw16s *pSrc, int pAcStatistics[256], int Ss, int Se, int Al, 
	FwiEncodeHuffmanState *pEncHuffState, int bFlushState)
{
	if (pSrc==0 || pAcStatistics ==0 || pEncHuffState ==0) 
		return fwStsNullPtrErr;

	if (Ss < 1 || Se<0 || Al <0 || Se >63) return fwStsBadArgErr;

	//TODO
	bFlushState;
	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//This functions computes the subsequent scan statistics for AC coefficients in 
//progressivce encoding.
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiGetHuffmanStatistics8x8_ACRefine_JPEG_16s_C1)(
	const Fw16s *pSrc, int pAcStatistics[256], int Ss, int Se, int Al, 
	FwiEncodeHuffmanState *pEncHuffState, int bFlushState)
{
	if (pSrc==0 || pAcStatistics ==0 || pEncHuffState ==0) 
		return fwStsNullPtrErr;

	if (Ss < 1 || Se<0 || Al <0 || Se > 63) return fwStsBadArgErr;

	//TODO
	bFlushState;
	return fwStsNoErr;
}
#endif

//Decoder definitions and functions
//EXTEND(V, T)
//Follows JPEG Standard Figure F.12, page 105
#ifndef __JPEGHUFF
#define __JPEGHUFF

#define DEC_EXTEND(V,T)  (V < (1<<(T-1)) ? (V + ((-1)<<T) + 1) : V)

#define GET_ACCBITS(pDecHuffState, s) \
	(((int) (pDecHuffState->accbuf >> (pDecHuffState->accbitnum -= (s)))) & ((1<<(s))-1))

#endif

struct DecodeHuffmanSpec 
{ 
	Fw8u  pListVals[256];				//Copy of pListVals from SpecInit
	Fw16u symcode[256];            // symbol code
	Fw16u symlen[256];             // symbol code length

	Fw16s mincode[18];             // smallest code value of specified length I
	Fw16s maxcode[18];             // largest  code value of specified length I 
	Fw16s valptr[18];              // index to the start of HUFFVAL decoded by code words of Length I 
};

struct DecodeHuffmanState 
{
	Fw8u  *pCurrSrc;               //Current source pointer position
	int srcLenBytes;                //Bytes left in the current source buffer
	Fw32u accbuf;                  //accumulated buffer for extraction
	int accbitnum;                  //bit number for accumulated buffer
	int EOBRUN;                     //EOB run length
	int marker;                     //JPEG marker
};

//-----------------------------------------------------------------------
//This function returns the buffer size (in bytes) of fwiDecodeHuffmanSpec 
//structure.
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiDecodeHuffmanSpecGetBufSize_JPEG_8u)(int* size)
{
	if (size ==0) return fwStsNullPtrErr;

	//add alignment requirement
	*size = sizeof(FwiDecodeHuffmanSpec) + 128;

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//This function creates Huffman table for decoder.
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiDecodeHuffmanSpecInit_JPEG_8u)(
	const Fw8u *pListBits, const Fw8u *pListVals, FwiDecodeHuffmanSpec *pDecHuffSpec)
{
	if (pListBits==0 || pListVals==0 || pDecHuffSpec==0)
		return fwStsNullPtrErr;

	Fw16u huffsize[257], huffcode[257], code;
	int i, j, k, si;
	Fw16s bits; 

	//Figure C.1 from CCITT Rec. T.81(1992 E) page 51
	//generation of table of Huffman code Sizes
	k=0;
	for (i=1; i<=16; i++) {
		bits = pListBits[i-1];
		//Protection for next for loop
		if (bits+k > 256) return fwStsJPEGHuffTableErr;

		for (j=1; j<=bits; j++) {
			huffsize[k]=(Fw16u)i;
			k++;
		}
	}

	huffsize[k]=0;

	//Figure C.2 from CCITT Rec. T.81(1992 E) page 52
	//generation of table of Huffman codes
	code=0;
	si=huffsize[0];

	//huffsize[k]==0 means the last k to exit the loop
	for (i=0; i<k;) {
		while (huffsize[i]==si) {
			huffcode[i++]=code++;
		}
		code <<=1;
		si++;
	}

	//Figure F.15 from CCITT Rec. T.81(1992 E) page 108
	//ordering procedure for decoding procedure code tables

	//set all codeless symbols to have code length 0
	memset(pDecHuffSpec->symlen, 0, 512);
	memset(pDecHuffSpec->symcode, 0, 512);
	memset(pDecHuffSpec->maxcode, -1, 36);//Fw16s type
	memset(pDecHuffSpec->mincode, 0, 36);//Fw16s type
	memset(pDecHuffSpec->valptr,  0, 36);//Fw16s type

	j=0;
	for (i=1;i<=16;i++) {
		bits = pListBits[i-1];
		if (bits != 0) {
			pDecHuffSpec->valptr[i] =(Fw16s)(j);
			pDecHuffSpec->mincode[i] = huffcode[j];
			j=j+bits-1;
			pDecHuffSpec->maxcode[i] = huffcode[j];
			j++;
		}
		//else maxcode to be -1
		//else pDecHuffSpec->maxcode[i] = -1;
	}

	k=0;
	for (j=1; j<=8; j++) {
		for (i=1;i<= pListBits[j-1];i++) {
			bits = (Fw16s)(huffcode[k] << (8-j));
			for (si=0;si < (1<<(8-j));si++) {
				pDecHuffSpec->symcode[bits]=(Fw16u)j;
				pDecHuffSpec->symlen[bits] = pListVals[k];
				bits++;
			}
			k++;
		}
	}

	memcpy(pDecHuffSpec->pListVals, pListVals, 256);

	//Set 16s max_value to maxcode[17] to prevent data corruption
	pDecHuffSpec->maxcode[17]= 0x7fff;

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//This function allocates memory and create huffman table for decoder
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiDecodeHuffmanSpecInitAlloc_JPEG_8u)(
	const Fw8u *pListBits, const Fw8u *pListVals, FwiDecodeHuffmanSpec** pDecHuffSpec)
{
	//Other parameters will be checked by fwiDecodeHuffmanSpecInit_JPEG_8u
	if (pDecHuffSpec==0) return fwStsNullPtrErr;

	int size;

	size = sizeof(DecodeHuffmanSpec) + 128;
	*pDecHuffSpec = (FwiDecodeHuffmanSpec *) fwMalloc (size);

	return fwiDecodeHuffmanSpecInit_JPEG_8u(pListBits, pListVals, *pDecHuffSpec);
}

//-----------------------------------------------------------------------
//This function frees the memory allocated by DecodeHuffmanSpecInitAlloc
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiDecodeHuffmanSpecFree_JPEG_8u)(FwiDecodeHuffmanSpec *pDecHuffSpec)
{
	if (pDecHuffSpec==0) return fwStsNullPtrErr;

	fwFree(pDecHuffSpec);

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//This function returns the buffer size (in bytes) of fwiDecodeHuffmanState 
//structure.
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiDecodeHuffmanStateGetBufSize_JPEG_8u)(int* size)
{
	if (size==0) return fwStsNullPtrErr;

	//add alignment requirement
	*size = sizeof(FwiDecodeHuffmanState) + 128;

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//This function initializes FwiDecodeHuffmanState structure
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiDecodeHuffmanStateInit_JPEG_8u)(FwiDecodeHuffmanState *pDecHuffState)
{
	if (pDecHuffState==0) return fwStsNullPtrErr;

	pDecHuffState->accbitnum = 0;
	pDecHuffState->srcLenBytes = 0;
	pDecHuffState->accbuf = 0;
	pDecHuffState->pCurrSrc = (unsigned char *)((unsigned char *)pDecHuffState + 
		sizeof (DecodeHuffmanState)) ;
	pDecHuffState->marker = 0;
	pDecHuffState->EOBRUN=0;

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//This function allocates memory and initialize FwiDecodeHuffmanState structure
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiDecodeHuffmanStateInitAlloc_JPEG_8u)(
	FwiDecodeHuffmanState** pDecHuffState)
{
	if (pDecHuffState==0) return fwStsNullPtrErr;

	int size;

	fwiDecodeHuffmanStateGetBufSize_JPEG_8u(&size);
	*pDecHuffState = (FwiDecodeHuffmanState *) fwMalloc (size);

	return fwiDecodeHuffmanStateInit_JPEG_8u(*pDecHuffState);
}

//-----------------------------------------------------------------------
//This function free the memory allocated by fwiDecodeHuffmanStateInitAlloc
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiDecodeHuffmanStateFree_JPEG_8u)(FwiDecodeHuffmanState *pDecHuffState)
{
	if (pDecHuffState==0) return fwStsNullPtrErr;

	fwFree(pDecHuffState);

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//Internal functions for helping decoders
//-----------------------------------------------------------------------
namespace OPT_LEVEL
    {
bool SYS_INLINE dec_receivebits (FwiDecodeHuffmanState * pDecHuffState, Fw32u accbuf, 
					  int accbitnum, int ssss)
{
	unsigned char  *pCurrSrc = pDecHuffState->pCurrSrc;
	int         srcLenBytes = pDecHuffState->srcLenBytes;
	int c;

	//Figure F.17 procedure for Receive (SSSS)
	if (pDecHuffState->marker == 0) {
		//read to full 32u bytes
		while (accbitnum <= 24) {
			if (srcLenBytes <= 0) break;
			srcLenBytes--;
			c =  *(pCurrSrc++);

			if (c == 0xFF) {
				do {
					srcLenBytes--;
					c =  *(pCurrSrc++);
				} while (c == 0xFF);

				if (c == 0) {
					c = 0xFF; 
				} else {
					pDecHuffState->marker = c;
					//prevent data corruption
					if (ssss > accbitnum) {
						accbuf <<= 25 - accbitnum;
						accbitnum = 25;
					}
					break;
				}
			}

			accbuf = (accbuf << 8) | c;
			accbitnum += 8;
		}
	} else {
		//prevent data corruption
		if (ssss > accbitnum) {
			accbuf <<= 25 - accbitnum;
			accbitnum = 25;
		}  
	}

	pDecHuffState->pCurrSrc    = pCurrSrc;
	pDecHuffState->srcLenBytes = srcLenBytes;
	pDecHuffState->accbuf      = accbuf;
	pDecHuffState->accbitnum   = accbitnum;

	return true;
}

int dec_huff (FwiDecodeHuffmanState * pDecHuffState, Fw32u accbuf, 
			  int accbitnum, const FwiDecodeHuffmanSpec *pTable, int nbits)
{
	Fw16s code;

	if (accbitnum < nbits) { 
		if (! dec_receivebits(pDecHuffState,accbuf,accbitnum,nbits)) 
			return -1; 
		accbitnum = pDecHuffState->accbitnum;
		accbuf	  = pDecHuffState->accbuf;
	}

	accbitnum -= nbits;
	code = (Fw16s)((accbuf >> accbitnum) & ((1<<nbits)-1));

	//Following JPEG standard Figure F.16
	while (code > pTable->maxcode[nbits]) {
		code <<= 1;

		if (accbitnum < 1) { 
			if (! dec_receivebits(pDecHuffState,accbuf,accbitnum,1)) 
				return -1; 
			accbitnum = pDecHuffState->accbitnum;
			accbuf	  = pDecHuffState->accbuf;
		}

		accbitnum--;
		code |= ((accbuf >> accbitnum) & 1);
		nbits++;
	}

	pDecHuffState->accbitnum = accbitnum;

	//To prevent corruption
	if (nbits > 16) return 0;	

	return pTable->pListVals[(code-pTable->mincode[nbits]+pTable->valptr[nbits])];
}

bool SYS_INLINE FW_HUFF_DECODE(int *result, FwiDecodeHuffmanState *pDecHuffState, 
					 const FwiDecodeHuffmanSpec *pTable)
{	
	int nextbit, look; 

	//Figure F.18 Procedure for fetching the next bit of compressed data
	if (pDecHuffState->accbitnum < 8) { 
		//get more bytes in
		if (! dec_receivebits(pDecHuffState,pDecHuffState->accbuf,pDecHuffState->accbitnum, 0)) {
			return false;
		} 
		if (pDecHuffState->accbitnum < 8) {
			nextbit = 1; 
			*result = dec_huff(pDecHuffState,pDecHuffState->accbuf,pDecHuffState->accbitnum,
				pTable,nextbit);
			if (*result < 0) return false;
			return true;
		} 
	} 

	look = (pDecHuffState->accbuf >> (pDecHuffState->accbitnum - 8)) & 0xff; 
	nextbit = pTable->symcode[look];

	if (nextbit != 0) { 
		pDecHuffState->accbitnum -= nextbit; 
		*result = pTable->symlen[look]; 
	} else { 
		nextbit = 9; 
		*result = dec_huff(pDecHuffState,pDecHuffState->accbuf, pDecHuffState->accbitnum,
			pTable,	nextbit);
		if (*result < 0) return false; 
	}

	return true;
}

}//namespace OPT_LEVEL
//-----------------------------------------------------------------------
//This function handles the Huffman Baseline decoding for a 8*8 block of the
//quantized DCT coefficients. The decoding procedure follows CCITT Rec. T.81
//section F.2.2
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiDecodeHuffman8x8_JPEG_1u16s_C1)(
	const Fw8u *pSrc, int srcLenBytes, int *pSrcCurrPos, Fw16s *pDst, 
	Fw16s *pLastDC, int *pMarker, const FwiDecodeHuffmanSpec *pDcTable, 
	const FwiDecodeHuffmanSpec *pAcTable, FwiDecodeHuffmanState *pDecHuffState)
{
	if (pSrc==0 || pSrcCurrPos==0 || pDst==0 || pLastDC ==0 ||
		pMarker==0 || pDcTable ==0 || pAcTable==0 ||
		pDecHuffState==0)
		return fwStsNullPtrErr;

	if (srcLenBytes == 0) {
		*pMarker = pDecHuffState->marker;
		return fwStsNoErr;
	}

	if (srcLenBytes <0 || *pSrcCurrPos >= srcLenBytes) 
		return fwStsSizeErr;

	int s, k, runlen;

	pDecHuffState->pCurrSrc = (unsigned char *)(pSrc+*pSrcCurrPos);
	pDecHuffState->srcLenBytes = srcLenBytes;
	pDecHuffState->marker   = *pMarker;

	//Follow JPEG standard F.2.2.1 to decode DC coefficient
	if (!FW_HUFF_DECODE(&s, pDecHuffState, pDcTable)) return fwStsJPEGOutOfBufErr;

	if (s) {
		if (pDecHuffState->accbitnum < s) {
			if (! dec_receivebits(pDecHuffState,pDecHuffState->accbuf,pDecHuffState->accbitnum,s)) 
				return fwStsJPEGOutOfBufErr;
		}
		runlen = GET_ACCBITS(pDecHuffState, s);
		s = DEC_EXTEND(runlen, s);
	}

	//pLastDC
	s += *pLastDC;
	*pLastDC = (Fw16s) s; 

	// clean pDst buffer since zero will be skipped
	memset(pDst, 0, 128);//pDst is 16s type

	pDst[0] = (Fw16s) s;

	// Follow JPEG standard F.2.2.2 to decode AC coefficient
	// Figure F.13
	for (k = 1; k < 64; k++) {

		//RS = DECODE
		if (!FW_HUFF_DECODE(&s, pDecHuffState, pAcTable)) return fwStsJPEGOutOfBufErr;

		runlen = s >> 4;
		s &= 0xf;

		if (s) {
			k += runlen;

			//Figure F.14
			//Decode_ZZ(K)
			if (pDecHuffState->accbitnum < s) {
				if (! dec_receivebits(pDecHuffState, pDecHuffState->accbuf, 
					pDecHuffState->accbitnum, s)) 
					return fwStsJPEGOutOfBufErr;
			}
			runlen = GET_ACCBITS(pDecHuffState, s);
			s = DEC_EXTEND(runlen, s);

			pDst[zigZagFwdOrder[k]] = (Fw16s) s;
		} else {
			if (runlen != 15)   break;
			k += 15;
		}
	}

	*pSrcCurrPos = (int)(pDecHuffState->pCurrSrc - pSrc);

	*pMarker = pDecHuffState->marker;

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//This function directly performs an 8*8 block encoding for quatized DCT 
//coefficients with the DC and AC huffmann code table, but without the 
//internal structure to accumulate the bits. The encoding procedure will
//still follow CCITT Rec. T.81 Annex F.2.2.
//If decoder detected a JPEG marker, it will stop and record the value at 
//pMarker. During function initialization, pLastDC, pMarker and 
//pNumValidPrefetchedBits should all set to zeroes.  After each restart 
//interval, pLastDC and pNumValidPrefetchedBits should set to zeroes. After
//each found marker has been processed, pMarker and pNumValidPrefetchedBits
//should set to zeroes.
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiDecodeHuffman8x8_Direct_JPEG_1u16s_C1)(
	const Fw8u *pSrc, int *pSrcBitsLen, Fw16s *pDst, Fw16s *pLastDC, int *pMarker,
	Fw32u *pPrefetchedBits, int *pNumValidPrefetchedBits, const FwiDecodeHuffmanSpec *pDcTable, 
	const FwiDecodeHuffmanSpec *pAcTable) 
{
	FwiDecodeHuffmanState *pDecHuffState;
	FwStatus status;
	int size, pSrcCurrPos, bytelength, srcLenBytes;

	//other parameters will be check by fwiDecodeHuffman8x8_JPEG_1u16s_C1.
	if (pSrc==0 || pSrcBitsLen==0 || pDst==0 || pPrefetchedBits==0
		||pNumValidPrefetchedBits==0) 
		return fwStsNullPtrErr;

	//Do nothing when src has 0 bit
	if (*pSrcBitsLen == 0) return fwStsNoErr;

	if (*pSrcBitsLen <0 || *pNumValidPrefetchedBits<0) 
		return fwStsSizeErr;

	bytelength = (*pNumValidPrefetchedBits)>>3;
	memcpy(pDst, pPrefetchedBits, bytelength);
	pSrcCurrPos=0;

	fwiDecodeHuffmanStateGetBufSize_JPEG_8u(&size);
	pDecHuffState = (FwiDecodeHuffmanState *)fwMalloc(size);

	status = fwiDecodeHuffmanStateInit_JPEG_8u(pDecHuffState);
	if (status != fwStsNoErr) return status;

	srcLenBytes = *pSrcBitsLen >>3;
	status = fwiDecodeHuffman8x8_JPEG_1u16s_C1(pSrc, srcLenBytes, &pSrcCurrPos, 
		pDst+(bytelength>>1), pLastDC, pMarker, pDcTable, pAcTable, pDecHuffState);

	*pMarker = pDecHuffState->marker;

	fwFree(pDecHuffState);	

	return status;
}

//-----------------------------------------------------------------------
//This function performs the first scan for progressive decoding of the DC 
//coefficient from an 8*8 block. The decoding process follows CCITT Rec. T.81
//Annex G.2. If decoder detected a JPEG marker, it will stop and record the 
//value at pMarker.
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiDecodeHuffman8x8_DCFirst_JPEG_1u16s_C1)(
	const Fw8u *pSrc, int srcLenBytes, int *pSrcCurrPos, Fw16s *pDst, Fw16s *pLastDC, 
	int *pMarker, int Al, const FwiDecodeHuffmanSpec *pDcTable, 
	FwiDecodeHuffmanState *pDecHuffState)
{
	if (pSrc==0 || pSrcCurrPos==0 || pDst==0 || pLastDC ==0 ||
		pMarker==0 || pDcTable ==0 || pDecHuffState==0)
		return fwStsNullPtrErr;

	if (srcLenBytes == 0) {
		*pMarker = pDecHuffState->marker;
		return fwStsNoErr;
	}

	if (srcLenBytes < 0 || *pSrcCurrPos >= srcLenBytes || Al <0) 
		return fwStsSizeErr;

	int s, runlen;

	pDecHuffState->pCurrSrc = (unsigned char *)(pSrc+*pSrcCurrPos);
	pDecHuffState->srcLenBytes = srcLenBytes;
	pDecHuffState->marker   = *pMarker;

	//Follow JPEG standard F.2.2.1 to decode DC coefficient
	if (!FW_HUFF_DECODE(&s, pDecHuffState, pDcTable)) return fwStsJPEGOutOfBufErr;

	if (s) {
		if (pDecHuffState->accbitnum < s) {
			if (! dec_receivebits(pDecHuffState,pDecHuffState->accbuf,pDecHuffState->accbitnum,s)) 
				return fwStsJPEGOutOfBufErr;
		}
		runlen = GET_ACCBITS(pDecHuffState, s);
		s = DEC_EXTEND(runlen, s);
	}
	s += *pLastDC;
	*pLastDC = (Fw16s) s;
	pDst[0] = (Fw16s) (s<<Al);

	*pSrcCurrPos = (int)(pDecHuffState->pCurrSrc - pSrc);
	*pMarker = pDecHuffState->marker;

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//This function performs the subsequent scan for progressive decoding of the DC 
//coefficient from an 8*8 block. The decoding process follows CCITT Rec. T.81
//Annex G.2. If decoder detected a JPEG marker, it will stop and record the 
//value at pMarker.
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiDecodeHuffman8x8_DCRefine_JPEG_1u16s_C1)(
	const Fw8u *pSrc, int srcLenBytes, int *pSrcCurrPos, Fw16s *pDst, 
	int *pMarker, int Al, FwiDecodeHuffmanState *pDecHuffState)
{
	if (pSrc==0 || pSrcCurrPos==0 || pDst==0 || 
		pMarker==0 || pDecHuffState==0)
		return fwStsNullPtrErr;

	if (srcLenBytes == 0) {
		*pMarker = pDecHuffState->marker;
		return fwStsNoErr;
	}

	if (srcLenBytes < 0 || *pSrcCurrPos >= srcLenBytes || Al <0) 
		return fwStsSizeErr;

	int p1Al = 1 << Al;

	if (pDecHuffState->accbitnum < 1) {
		if (! dec_receivebits(pDecHuffState,pDecHuffState->accbuf,pDecHuffState->accbitnum,1)) 
			return fwStsJPEGOutOfBufErr;
	}
	if (GET_ACCBITS(pDecHuffState, 1)) pDst[0] |= p1Al;

	*pSrcCurrPos = (int)(pDecHuffState->pCurrSrc - pSrc);
	*pMarker = pDecHuffState->marker;

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//This function performs progressive decoding of AC coefficients for an 
//8*8 block (first scan). The decoding procedure follows CCITT Rec. T.81
//Annex G.2. If a marker is detected during decoding, the functions writes
//the value to pMarker and stop decoding.
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiDecodeHuffman8x8_ACFirst_JPEG_1u16s_C1)(
	const Fw8u *pSrc, int srcLenBytes, int *pSrcCurrPos, Fw16s *pDst, 
	int *pMarker, int Ss, int Se, int Al, const FwiDecodeHuffmanSpec *pAcTable,
	FwiDecodeHuffmanState *pDecHuffState)
{
	if (pSrc==0 || pSrcCurrPos==0 || pDst==0 || pAcTable==0 ||
		pMarker==0 || pDecHuffState==0)
		return fwStsNullPtrErr;

	if (srcLenBytes == 0) {
		*pMarker = pDecHuffState->marker;
		return fwStsNoErr;
	}

	if (srcLenBytes < 0 || *pSrcCurrPos < 0 || *pSrcCurrPos >= srcLenBytes || 
		Al <0|| Ss < 1 ||Se > 63) 
		return fwStsSizeErr;

	int s, k, runlen;
	unsigned int EOBRUN;

	pDecHuffState->pCurrSrc = (unsigned char *)(pSrc+*pSrcCurrPos);
	pDecHuffState->srcLenBytes = srcLenBytes;
	pDecHuffState->marker   = *pMarker;
	EOBRUN = pDecHuffState->EOBRUN;

	if (EOBRUN == 0) {		
		for (k = Ss; k < Se; k++) {
			if (!FW_HUFF_DECODE(&s, pDecHuffState, pAcTable)) return fwStsJPEGOutOfBufErr;

			runlen = s >> 4;
			s &= 15;

			if (s) {
				k += runlen;
				if (pDecHuffState->accbitnum < s) {
					if (! dec_receivebits(pDecHuffState,pDecHuffState->accbuf,pDecHuffState->accbitnum,s)) 
						return fwStsJPEGOutOfBufErr;
				}
				runlen = GET_ACCBITS(pDecHuffState, s);
				s = DEC_EXTEND(runlen, s);
				pDst[zigZagFwdOrder[k]] = (Fw16s) (s<<Al);
			} else {
				if (runlen == 15) {	
					k += 15;		
				} else {		
					EOBRUN = 1 << runlen;
					if (runlen) {		

						if (pDecHuffState->accbitnum < runlen) {
							if (! dec_receivebits(pDecHuffState,pDecHuffState->accbuf,pDecHuffState->accbitnum, runlen)) 
								return fwStsJPEGOutOfBufErr;
						}
						runlen = GET_ACCBITS(pDecHuffState, runlen);
						EOBRUN += runlen;
					}

					EOBRUN--;		
					break;		
				}
			}
		}   
	}
	else EOBRUN--; //all zero case

	pDecHuffState->EOBRUN = EOBRUN;
	*pSrcCurrPos = (int)(pDecHuffState->pCurrSrc - pSrc);
	*pMarker = pDecHuffState->marker;

	return fwStsNoErr;
}

//-----------------------------------------------------------------------
//This function performs progressive decoding of AC coefficients for an 
//8*8 block (subsequent scan). The decoding procedure follows CCITT Rec. T.81
//Annex G.2. If a marker is detected during decoding, the functions writes
//the value to pMarker and stop decoding.
//-----------------------------------------------------------------------
FwStatus PREFIX_OPT(OPT_PREFIX, fwiDecodeHuffman8x8_ACRefine_JPEG_1u16s_C1)(
	const Fw8u *pSrc, int srcLenBytes, int *pSrcCurrPos, Fw16s *pDst, 
	int *pMarker, int Ss, int Se, int Al, const FwiDecodeHuffmanSpec *pAcTable,
	FwiDecodeHuffmanState *pDecHuffState)
{
	if (pSrc==0 || pSrcCurrPos==0 || pDst==0 || pAcTable==0 ||
		pMarker==0 || pDecHuffState==0)
		return fwStsNullPtrErr;

	if (srcLenBytes == 0) {
		*pMarker = pDecHuffState->marker;
		return fwStsNoErr;
	}

	if (srcLenBytes < 0 || *pSrcCurrPos < 0 || *pSrcCurrPos >= srcLenBytes || 
		Al <0 || Ss < 1 ||Se > 63) 
		return fwStsSizeErr;

	int i, s, k, runlen, pos;
	Fw16s *pDstZ;
	unsigned int EOBRUN = pDecHuffState->EOBRUN;
	int p1Al = 1 << Al;	
	int n1Al = (-1) << Al;	
	int nonZeroByte;
	int recovernz[64];

	//If suspended, we must undo the new nonzero coefficients in the block.
	nonZeroByte = 0;

	if (EOBRUN == 0) {
		for (k=Ss; k <= Se; k++) {
			if (!FW_HUFF_DECODE(&s, pDecHuffState, pAcTable)) {
				for (i=0;i<nonZeroByte;i++) 
					pDst[recovernz[i]] = 0;
				return fwStsJPEGDCTRangeErr;
			}

			runlen = s >> 4;
			s &= 15;
			if (s) {
				if (pDecHuffState->accbitnum < 1) {
					if (! dec_receivebits(pDecHuffState,pDecHuffState->accbuf,
						pDecHuffState->accbitnum,1)) {
							for (i=0;i<nonZeroByte;i++) 
								pDst[recovernz[i]] = 0;
							return fwStsJPEGDCTRangeErr;
					}
				}
				if (GET_ACCBITS(pDecHuffState, 1))
					s = p1Al;		
				else
					s = n1Al;		
			} else {
				if (runlen != 15) {
					EOBRUN = 1 << runlen;	
					if (runlen) {
						if (pDecHuffState->accbitnum < runlen) {
							if (! dec_receivebits(pDecHuffState,pDecHuffState->accbuf,
								pDecHuffState->accbitnum,runlen)) {
									for (i=0;i<nonZeroByte;i++) 
										pDst[recovernz[i]] = 0;
									return fwStsJPEGDCTRangeErr;
							}
						}
						runlen = GET_ACCBITS(pDecHuffState, runlen);
						EOBRUN += runlen;
					}
					break;		
				}
			}

			for (;k<Se;k++) {
				pDstZ = (Fw16s *)(pDst + zigZagFwdOrder[k]);
				if (*pDstZ != 0) {
					if (pDecHuffState->accbitnum < 1) {
						if (! dec_receivebits(pDecHuffState,pDecHuffState->accbuf,
							pDecHuffState->accbitnum,1)) {
								for (i=0;i<nonZeroByte;i++) 
									pDst[recovernz[i]] = 0;
								return fwStsJPEGDCTRangeErr;
						}
					}
					if (GET_ACCBITS(pDecHuffState, 1)) {
						if ((*pDstZ & p1Al) == 0) { 
							if (*pDstZ >= 0)
								*pDstZ = (Fw16s)(*pDstZ+p1Al);
							else
								*pDstZ = (Fw16s)(*pDstZ+n1Al);
						}
					}
				} else {
					runlen --;
					if (runlen < 0) break;		
				}
			}

			if (s) {
				pos = zigZagFwdOrder[k];
				pDst[pos] = (Fw16s) s;
				recovernz[nonZeroByte++] = pos;
			}
		}
	}

	if (EOBRUN > 0) {
		for (k=Ss; k <= Se; k++) {
			pDstZ = pDst + zigZagFwdOrder[k];
			if (*pDstZ != 0) {
				if (pDecHuffState->accbitnum < 1) {
					if (! dec_receivebits(pDecHuffState,pDecHuffState->accbuf,
						pDecHuffState->accbitnum,1)) {
							for (i=0;i<nonZeroByte;i++) 
								pDst[recovernz[i]] = 0;
							return fwStsJPEGDCTRangeErr;
					}
				}
				if (GET_ACCBITS(pDecHuffState, 1)) {
					if ((*pDstZ & p1Al) == 0) { 
						if (*pDstZ >= 0)
							*pDstZ = (Fw16s)(*pDstZ+p1Al);
						else
							*pDstZ = (Fw16s)(*pDstZ+n1Al);
					}
				}
			}
		}

		EOBRUN--;
	}

	pDecHuffState->EOBRUN = EOBRUN;
	*pSrcCurrPos = (int)(pDecHuffState->pCurrSrc - pSrc);
	*pMarker = pDecHuffState->marker;

	return fwStsNoErr;
}

#endif //BUILD_NUM_AT_LEAST

// Please do NOT remove the above line for CPP files that need to be multipass compiled
// OREFR 
