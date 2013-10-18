/*
    Description: This file contains all the markers used in JPEG standard
    Source     : JPEG standard ISO/IEC 10918-1 : 1993(E) Table B.1 - Marker Code Segments
    Author     : Korsa
    CopyRight  : Korsa And Ramesh
*/

#ifndef MARKERS_H
#define MARKERS_H

typedef enum
    {
    // non-differential Huffman coding
    SOF0  = 0xC0, // Baseline DCT
    SOF1  = 0xC1, // Extended sequential DCT
    SOF2  = 0xC2, // Progressive DCT
    SOF3  = 0xC3, // Lossless(sequential)
    
    // differential Huffman coding
    SOF5  = 0xC5, // Differential sequential DCT 
    SOF6  = 0xC6, // Differential Progressive DCT
    SOF7  = 0xC7, // Differential lossless(sequential)
           
    // non-differential arithematic coding
    JPG   = 0xC8, // Reserved for JPEG extensions
    SOF9  = 0xC9, // Extended sequential DCT
    SOFA  = 0xCA, // Progressive DCT
    SOFB  = 0xCB, // Lossless(sequential)
    
    // differential arithematic coding
    SOFD  = 0xCD, // Differntial sequential DCT
    SOFE  = 0xCE, // Differential progressive DCT
    SOFF  = 0xCF, // Differential lossless(sequential)
               
    // Huffman table specifications
    DHT   = 0xC4, // Define Huffman Table
    
    // Arithematic coding conditioning specification
    DAC   = 0xCC, // Define Arithematic coding conditioning(s)
    
    // Restart Interval Termination
    RST0  = 0xD0, // restart with modulo 8 count 'm'
    RST1  = 0xD1,
    RST2  = 0xD2,
    RST3  = 0xD3,
    RST4  = 0xD4,
    RST5  = 0xD5,
    RST6  = 0xD6,
    RST7  = 0xD7,
    
    // Other markers
    SOI   = 0xD8, // Start Of Image
    EOI   = 0xD9, // End Of Image
    SOS   = 0xDA, // Start of Scan
    DQT   = 0xDB, // Define Quantisation Table(s)
    DNL   = 0xDC, // Define Number of Lines
    DRI   = 0xDD, // Define Restart Interval
    DHP   = 0xDE, // Define Hierarchical Progression
    EXP   = 0xDF, // Expand Reference Component(s)

    APP0  = 0xE0, // Reserved for Application segments JFIF
    APP1  = 0xE1,
    APP2  = 0xE2,
    APP3  = 0xE3,
    APP4  = 0xE4,
    APP5  = 0xE5,
    APP6  = 0xE6,
    APP7  = 0xE7,
    APP8  = 0xE8,
    APP9  = 0xE9,
    APPA  = 0xEA,
    APPB  = 0xEB,
    APPC  = 0xEC,
    APPD  = 0xED,
    APPE  = 0xEE,

    JPG0  = 0xF0,// Reserved for JPEG Extensions
    JPG1  = 0xF1,
    JPG2  = 0xF2,
    JPG3  = 0xF3,
    JPG4  = 0xF4,
    JPG5  = 0xF5,
    JPG6  = 0xF6,
    JPG7  = 0xF7,
    JPG8  = 0xF8,
    JPG9  = 0xF9,
    JPGA  = 0xFA,
    JPGB  = 0xFB,
    JPGC  = 0xFC,
    JPGD  = 0xFD,

    COM   = 0xFE, // Comment
              
    // Reserved markers
    TEM   = 0x01, // for Temporary private use in arithematic coding 
    RES   = 0x02  // Reserved from 0x02 upto 0xBF
    }MARKER;

#endif

/*
                                            Modification history:
                                            ---------------------
        Date             Author            Changes Made
        ------------------------------------------------------------------------------------------
        10th Mar 2007    Korsa             created the file and entered all the JPEG markers 

*/