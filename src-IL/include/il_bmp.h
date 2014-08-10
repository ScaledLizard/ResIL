//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 09/01/2003 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_bmp.h
//
// Description: Reads and writes to a bitmap (.bmp) file.
//
//-----------------------------------------------------------------------------


#ifndef BMP_H
#define BMP_H

#include "il_internal.h"

#ifdef _WIN32
	#pragma pack(push, bmp_struct, 1)
#endif

// Microsoft legislated that these are two headers, but putting them into one 
// struct allows to read the whole thing in just one read call
typedef struct BMPHEAD {
	ILbyte		bfType[2];
	uint32_t		bfSize;
	uint32_t		bfReserved;
	uint32_t		bfDataOff;
	uint32_t		biSize;
	uint32_t		biWidth;
	uint32_t		biHeight;
	uint16_t		biPlanes; // Must be 1
	uint16_t		biBitCount; // Bits per pixel
	uint32_t		biCompression; // 0: none, 1: RLE8, 2: RLE4, more...
	uint32_t		biSizeImage;
	uint32_t		biXPelsPerMeter;
	uint32_t		biYPelsPerMeter;
	uint32_t		biClrUsed;
	uint32_t		biClrImportant;
} IL_PACKSTRUCT BMPHEAD;

typedef struct OS2_HEAD
{
	// Bitmap file header.
	ILushort	bfType;
	ILuint		biSize;
	ILshort		xHotspot;
	ILshort		yHotspot;
	ILuint		DataOff;

	// Bitmap core header.
	ILuint		cbFix;
	//2003-09-01: changed cx, cy to ushort according to MSDN
	ILushort		cx;
	ILushort		cy;
	ILushort	cPlanes;
	ILushort	cBitCount;
} IL_PACKSTRUCT OS2_HEAD;
#ifdef _WIN32
	#pragma pack(pop, bmp_struct)
#endif

#endif//BMP_H
