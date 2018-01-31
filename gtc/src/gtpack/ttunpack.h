/******************************************************************************
*
* project name:    TIGCC Tools Suite 
* file name:       ttunpack.h
* initial date:    14/08/2000
* author:          thomas.nussbaumer@gmx.net 
* description:     defines of errorcodes of decompression routine and its
*                  declaration
* $Id: ttunpack.h,v 1.2 2000/08/20 15:26:21 Thomas Nussbaumer Exp $
*
******************************************************************************/

#ifndef __TTUNPACK_H__
#define __TTUNPACK_H__

#define K_GAMMA 1
#define M_GAMMA 0

#define ERRPCK_OKAY             0
#define ERRPCK_NOESCFOUND     248
#define ERRPCK_ESCBITS        249  
#define ERRPCK_MAXGAMMA       250
#define ERRPCK_EXTRALZP       251
#define ERRPCK_NOMAGIC        252 
#define ERRPCK_OUTBUFOVERRUN  253
#define ERRPCK_LZPOSUNDERRUN  254

int _tt_Decompress(unsigned char *src, unsigned char *dest);
#define UnPack _tt_Decompress

#endif


//#############################################################################
//###################### NO MORE FAKES BEYOND THIS LINE #######################
//#############################################################################
//
//=============================================================================
// Revision History
//=============================================================================
//
// $Log: ttunpack.h,v $
// Revision 1.2  2000/08/20 15:26:21  Thomas Nussbaumer
// prefix of unpack routine (_tt_) corrected
//
// Revision 1.1  2000/08/14 22:49:57  Thomas Nussbaumer
// initial version
//
// 
