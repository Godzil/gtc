/******************************************************************************
*
* original project name:    TIGCC Tools Suite 
* file name:		    packhead.h
* initial date:		    14/08/2000
* authors:		    thomas.nussbaumer@gmx.net 
*			    Paul Froissart
* description:              header definition of compressed data
*
* $Id: packhead.h,v 1.3 2000/08/20 15:24:28 Thomas Nussbaumer Exp $
*
******************************************************************************/

#ifndef __PACKHEAD_H__
#define __PACKHEAD_H__

#ifndef EVEN_LZ
#define MAGIC_CHAR1     'P'
#define MAGIC_CHAR2     'k'
#else
#define MAGIC_CHAR1     'P'
#define MAGIC_CHAR2     'x'
#endif

#define MAX_RLE_ENTRIES 31

#define COMPACT
//#define OLD_HDR

#ifdef OLD_HDR
// size = 8 bytes
typedef struct {
    unsigned char origsize_hi; // original size lowbyte
    unsigned char origsize_lo; // original size highbyte
//    unsigned char magic1;      // must be equal to MAGIC_CHAR1
//    unsigned char magic2;      // must be equal to MAGIC_CHAR2
//    unsigned char compsize_lo; // compressed size lowbyte
//    unsigned char compsize_hi; // compressed size lowbyte
    unsigned char esc1;        // escape >> (8-escBits)
//    unsigned char notused3;
//    unsigned char notused4;
    unsigned char esc2;        // escBits
    unsigned char gamma1;      // maxGamma + 1
    unsigned char gamma2;      // (1<<maxGamma)
    unsigned char extralz;     // extraLZPosBits
//    unsigned char notused1;
//    unsigned char notused2;
    unsigned char rleentries;  // rleUsed
} PackedHeader;
#else
// size = 12 bytes
typedef struct {
    char magic[4];
    unsigned char origsize_hi; // original size lowbyte
    unsigned char origsize_lo; // original size highbyte
    unsigned char compsize_hi; // compressed size lowbyte
    unsigned char compsize_lo; // compressed size highbyte
    unsigned char esc2;        // escBits
    unsigned char esc1;        // escape >> (8-escBits)
    unsigned char extralz;     // extraLZPosBits
    unsigned char rleentries;  // rleUsed
} PackedHeader;
#endif


#define GetUnPackedSize(p)  (unsigned int)((p)->origsize_lo | ((p)->origsize_hi << 8))
#define IsPacked(p)         ((p)->magic1 == MAGIC_CHAR1 && (p)->magic2 == MAGIC_CHAR2)


typedef struct { 
    unsigned char value[MAX_RLE_ENTRIES];
} RLEEntries;


#endif


//#############################################################################
//###################### NO MORE FAKES BEYOND THIS LINE #######################
//#############################################################################
//
//=============================================================================
// Revision History
//=============================================================================
//
// $Log: packhead.h,v $
// Revision 1.4  2005/08/04 21:27:43  Paul Froissart
// adapted for XPak
//
// Revision 1.3  2000/08/20 15:24:28  Thomas Nussbaumer
// macros to get unpacked size and to check if packed added
//
// Revision 1.2  2000/08/16 23:08:55  Thomas Nussbaumer
// magic characters changed to TP ... t(igcc tools) p(acked file)
//
// Revision 1.1  2000/08/14 22:49:57  Thomas Nussbaumer
// initial version
//
// 
