//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_sgi.c
//
// Description: Reads and writes Silicon Graphics Inc. (.sgi) files.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#ifndef IL_NO_SGI
#include "il_sgi.h"
#include "il_manip.h"
#include <limits.h>
#include <stdint.h>

typedef struct iSgiHeader
{
	ILshort		MagicNum;	// IRIS image file magic number
	ILbyte		Storage;	// Storage format
	ILbyte		Bpc;		// Number of bytes per pixel channel
	ILushort	Dim;		// Number of dimensions
							//  1: single channel, 1 row with XSize pixels
							//  2: single channel, XSize*YSize pixels
							//  3: ZSize channels, XSize*YSize pixels
	
	ILushort	XSize;		// X size in pixels
	ILushort	YSize;		// Y size in pixels
	ILushort	ZSize;		// Number of channels
	ILint		PixMin;		// Minimum pixel value
	ILint		PixMax;		// Maximum pixel value
	ILint		Dummy1;		// Ignored
	ILbyte		Name[80];	// Image name
	ILint		ColMap;		// Colormap ID
	ILbyte		Dummy[404];	// Ignored
} IL_PACKSTRUCT iSgiHeader;

// Sgi format #define's
#define SGI_VERBATIM		0
#define SGI_RLE				1
#define SGI_MAGICNUM		474

// Sgi colormap types
#define SGI_COLMAP_NORMAL	0
#define SGI_COLMAP_DITHERED	1
#define SGI_COLMAP_SCREEN	2
#define SGI_COLMAP_COLMAP	3


// Just an internal convenience function for reading SGI files
ILboolean iNewSgi(ILimage*  image, iSgiHeader *Head)
{
	if (!il2TexImage(image, Head->XSize, Head->YSize, Head->Bpc, (ILubyte)Head->ZSize, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	image->Origin = IL_ORIGIN_LOWER_LEFT;

	switch (Head->ZSize)
	{
		case 1:
			image->Format = IL_LUMINANCE;
			break;
		/*case 2:
			image->Format = IL_LUMINANCE_ALPHA; 
			break;*/
		case 3:
			image->Format = IL_RGB;
			break;
		case 4:
			image->Format = IL_RGBA;
			break;
		default:
			il2SetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
	}

	switch (Head->Bpc)
	{
		case 1:
			if (Head->PixMin < 0)
				image->Type = IL_BYTE;
			else
				image->Type = IL_UNSIGNED_BYTE;
			break;
		case 2:
			if (Head->PixMin < 0)
				image->Type = IL_SHORT;
			else
				image->Type = IL_UNSIGNED_SHORT;
			break;
		default:
			il2SetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
	}

	image->Origin = IL_ORIGIN_LOWER_LEFT;

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

// Internal function used to get the .sgi header from the current file.
ILint iGetSgiHead(SIO* io, iSgiHeader *Header)
{
	ILint read = (ILint) io->read(io, Header, 1, sizeof(iSgiHeader));

	#ifdef __LITTLE_ENDIAN__
	iSwapShort(&Header->MagicNum);
	iSwapUShort(&Header->Dim);
	iSwapUShort(&Header->XSize);
	iSwapUShort(&Header->YSize);
	iSwapUShort(&Header->ZSize);
	iSwapInt(&Header->PixMin);
	iSwapInt(&Header->PixMax);
	iSwapInt(&Header->Dummy1);
	iSwapInt(&Header->ColMap);
	#endif

	return read;
}

/*----------------------------------------------------------------------------*/

/* Internal function used to check if the HEADER is a valid .sgi header. */
ILboolean iCheckSgi(iSgiHeader *Header)
{
	if (Header->MagicNum != SGI_MAGICNUM)
		return IL_FALSE;
	if (Header->Storage != SGI_RLE && Header->Storage != SGI_VERBATIM)
		return IL_FALSE;
	if (Header->Bpc == 0 || Header->Dim == 0)
		return IL_FALSE;
	if (Header->XSize == 0 || Header->YSize == 0 || Header->ZSize == 0)
		return IL_FALSE;

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

/* Internal function to get the header and check it. */
ILboolean iIsValidSgi(SIO* io)
{
	iSgiHeader	Head;
	ILint read = iGetSgiHead(io, &Head);
	io->seek(io, -read, IL_SEEK_CUR);  // Restore previous file position

	if (read == sizeof(Head))
		return iCheckSgi(&Head);
	else
		return IL_FALSE;
}

/*----------------------------------------------------------------------------*/

// Much easier to read - just assemble from planes, no decompression
ILboolean iReadNonRleSgi(ILimage* image, iSgiHeader *Head)
{
	ILuint		i, c;
	ILboolean	Cache = IL_FALSE;

	if (!iNewSgi(image, Head)) {
		return IL_FALSE;
	}

	for (c = 0; c < image->Bpp; c++) {
		for (i = c; i < image->SizeOfData; i += image->Bpp) {
			if (image->io.read(&image->io, image->Data + i, 1, 1) != 1) {
				return IL_FALSE;
			}
		}
	}

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

void sgiSwitchData(ILubyte *Data, ILuint SizeOfData)
{	
	ILubyte	Temp;
	ILuint	i;
	#ifdef ALTIVEC_GCC
		i = 0;
		union {
			vector unsigned char vec;
			vector unsigned int load;
		}inversion_vector;

		inversion_vector.load  = (vector unsigned int)\
			{0x01000302,0x05040706,0x09080B0A,0x0D0C0F0E};
		while( i <= SizeOfData-16 ) {
			vector unsigned char data = vec_ld(i,Data);
			vec_perm(data,data,inversion_vector.vec);
			vec_st(data,i,Data);
			i+=16;
		}
		SizeOfData -= i;
	#endif
	for (i = 0; i < SizeOfData; i += 2) {
		Temp = Data[i];
		Data[i] = Data[i+1];
		Data[i+1] = Temp;
	}
	return;
}

/*----------------------------------------------------------------------------*/

ILint iGetScanLine(ILimage* image, ILubyte *ScanLine, iSgiHeader *Head, ILuint Length)
{
	ILushort Pixel, Count;  // For current pixel
	ILuint	 BppRead = 0, CurPos = 0, Bps = Head->XSize * Head->Bpc;

	while (BppRead < Length && CurPos < Bps)
	{
		Pixel = 0;
		if (image->io.read(&image->io, &Pixel, Head->Bpc, 1) != 1)
			return -1;
		
#ifndef __LITTLE_ENDIAN__
		iSwapUShort(&Pixel);
#endif

		if (!(Count = (Pixel & 0x7f)))  // If 0, line ends
			return CurPos;
		if (Pixel & 0x80) {  // If top bit set, then it is a "run"
			if (image->io.read(&image->io, ScanLine, Head->Bpc, Count) != Count)
				return -1;
			BppRead += Head->Bpc * Count + Head->Bpc;
			ScanLine += Head->Bpc * Count;
			CurPos += Head->Bpc * Count;
		}
		else {
			if (image->io.read(&image->io, &Pixel, Head->Bpc, 1) != 1)
				return -1;
#ifndef __LITTLE_ENDIAN__
			iSwapUShort(&Pixel);
#endif
			if (Head->Bpc == 1) {
				while (Count--) {
					*ScanLine = (ILubyte)Pixel;
					ScanLine++;
					CurPos++;
				}
			}
			else {
				while (Count--) {
					*(ILushort*)ScanLine = Pixel;
					ScanLine += 2;
					CurPos += 2;
				}
			}
			BppRead += Head->Bpc + Head->Bpc;
		}
	}

	return CurPos;
}

/*----------------------------------------------------------------------------*/

ILboolean iReadRleSgi(ILimage* image, iSgiHeader *Head)
{
	#ifdef __LITTLE_ENDIAN__
	ILuint ixTable;
	#endif
	ILuint 		ChanInt = 0;
	ILuint  	ixPlane, ixHeight,ixPixel, RleOff, RleLen;
	ILuint		*OffTable=NULL, *LenTable=NULL, TableSize, Cur;
	ILubyte		**TempData=NULL;

	if (!iNewSgi(image, Head))
		return IL_FALSE;

	TableSize = Head->YSize * Head->ZSize;
	OffTable = (ILuint*)ialloc(TableSize * sizeof(ILuint));
	LenTable = (ILuint*)ialloc(TableSize * sizeof(ILuint));
	if (OffTable == NULL || LenTable == NULL)
		goto cleanup_error;
	if (image->io.read(&image->io, OffTable, TableSize * sizeof(ILuint), 1) != 1)
		goto cleanup_error;
	if (image->io.read(&image->io, LenTable, TableSize * sizeof(ILuint), 1) != 1)
		goto cleanup_error;

#ifdef __LITTLE_ENDIAN__
	// Fix the offset/len table (it's big endian format)
	for (ixTable = 0; ixTable < TableSize; ixTable++) {
		iSwapUInt(OffTable + ixTable);
		iSwapUInt(LenTable + ixTable);
	}
#endif //__LITTLE_ENDIAN__

	// We have to create a temporary buffer for the image, because SGI
	//	images are plane-separated.
	TempData = (ILubyte**)ialloc(Head->ZSize * sizeof(ILubyte*));
	if (TempData == NULL)
		goto cleanup_error;
	imemclear(TempData, Head->ZSize * sizeof(ILubyte*));  // Just in case ialloc fails then cleanup_error.
	for (ixPlane = 0; ixPlane < Head->ZSize; ixPlane++) {
		TempData[ixPlane] = (ILubyte*)ialloc(Head->XSize * Head->YSize * Head->Bpc);
		if (TempData[ixPlane] == NULL)
			goto cleanup_error;
	}

	// Read the Planes into the temporary memory
	for (ixPlane = 0; ixPlane < Head->ZSize; ixPlane++) {
		for (ixHeight = 0, Cur = 0;	ixHeight < Head->YSize;
			ixHeight++, Cur += Head->XSize * Head->Bpc) {

			RleOff = OffTable[ixHeight + ixPlane * Head->YSize];
			RleLen = LenTable[ixHeight + ixPlane * Head->YSize];
			
			// Seeks to the offset table position
			image->io.seek(&image->io, RleOff, IL_SEEK_SET);
			if (iGetScanLine(image, (TempData[ixPlane]) + (ixHeight * Head->XSize * Head->Bpc),
				Head, RleLen) != Head->XSize * Head->Bpc) {
					il2SetError(IL_ILLEGAL_FILE_VALUE);
					goto cleanup_error;
			}
		}
	}

	// DW: Removed on 05/25/2002.
	/*// Check if an alphaplane exists and invert it
	if (Head->ZSize == 4) {
		for (ixPixel=0; (ILint)ixPixel<Head->XSize * Head->YSize; ixPixel++) {
 			TempData[3][ixPixel] = TempData[3][ixPixel] ^ 255;
 		}	
	}*/
	
	// Assemble the image from its planes
	for (ixPixel = 0; ixPixel < image->SizeOfData;
		ixPixel += Head->ZSize * Head->Bpc, ChanInt += Head->Bpc) {
		for (ixPlane = 0; (ILint)ixPlane < Head->ZSize * Head->Bpc;	ixPlane += Head->Bpc) {
			image->Data[ixPixel + ixPlane] = TempData[ixPlane][ChanInt];
			if (Head->Bpc == 2)
				image->Data[ixPixel + ixPlane + 1] = TempData[ixPlane][ChanInt + 1];
		}
	}

	#ifdef __LITTLE_ENDIAN__
	if (Head->Bpc == 2)
		sgiSwitchData(image->Data, image->SizeOfData);
	#endif

	ifree(OffTable);
	ifree(LenTable);

	for (ixPlane = 0; ixPlane < Head->ZSize; ixPlane++) {
		ifree(TempData[ixPlane]);
	}
	ifree(TempData);

	return IL_TRUE;

cleanup_error:
	ifree(OffTable);
	ifree(LenTable);
	if (TempData) {
		for (ixPlane = 0; ixPlane < Head->ZSize; ixPlane++) {
			ifree(TempData[ixPlane]);
		}
		ifree(TempData);
	}

	return IL_FALSE;
}

/*----------------------------------------------------------------------------*/

/* Internal function used to load the SGI image */
ILboolean iLoadSgiInternal(ILimage* image)
{
	iSgiHeader	Header;
	ILboolean	bSgi;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!iGetSgiHead(&image->io, &Header))
		return IL_FALSE;
	if (!iCheckSgi(&Header)) {
		il2SetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// Bugfix for #1060946.
	//  The ZSize should never really be 2 by the specifications.  Some
	//  application is outputting these, and it looks like the ZSize
	//  should really be 1.
	if (Header.ZSize == 2)
		Header.ZSize = 1;
	
	if (Header.Storage == SGI_RLE) {  // RLE
		bSgi = iReadRleSgi(image, &Header);
	}
	else {  // Non-RLE  //(Header.Storage == SGI_VERBATIM)
		bSgi = iReadNonRleSgi(image, &Header);
	}

	if (!bSgi)
		return IL_FALSE;
	return il2FixImage(image);
}

/*----------------------------------------------------------------------------*/

ILenum DetermineSgiType(ILimage* image, ILenum Type)
{
	if (Type > IL_UNSIGNED_SHORT) {
		if (image->Type == IL_INT)
			return IL_SHORT;
		return IL_UNSIGNED_SHORT;
	}
	return Type;
}

/*----------------------------------------------------------------------------*/

ILboolean iSaveRleSgi(ILimage* image, ILubyte *Data, ILuint w, ILuint h, ILuint numChannels,
		ILuint bps)
{
	//works only for sgi files with only 1 bpc

	ILuint	c, i, y, j;
	ILubyte	*ScanLine = NULL, *CompLine = NULL;
	ILuint	*StartTable = NULL, *LenTable = NULL;

	ScanLine = (ILubyte*)ialloc(w);
	CompLine = (ILubyte*)ialloc(w * 2 + 1);  // Absolute worst case.
	StartTable = (ILuint*)ialloc(h * numChannels * sizeof(ILuint));
	LenTable = (ILuint*)icalloc(h * numChannels, sizeof(ILuint));
	if (!ScanLine || !CompLine || !StartTable || !LenTable) {
		ifree(ScanLine);
		ifree(CompLine);
		ifree(StartTable);
		ifree(LenTable);
		return IL_FALSE;
	}

	// These just contain dummy values at this point.
	auto TableOff = image->io.tell(&image->io);
	image->io.write(StartTable, sizeof(ILuint), h * numChannels, &image->io);
	image->io.write(LenTable, sizeof(ILuint), h * numChannels, &image->io);

	auto DataOff = image->io.tell(&image->io);
	for (c = 0; c < numChannels; c++) {
		for (y = 0; y < h; y++) {
			i = y * bps + c;
			for (j = 0; j < w; j++, i += numChannels) {
				ScanLine[j] = Data[i];
			}

			ilRleCompressLine(ScanLine, w, 1, CompLine, LenTable + h * c + y, IL_SGICOMP);
			image->io.write(CompLine, 1, *(LenTable + h * c + y), &image->io);
		}
	}

	image->io.seek(&image->io, TableOff, IL_SEEK_SET);

	j = h * numChannels;
	ILboolean success = IL_TRUE;
	y = 0;
	while (y < j && success) {
		if (DataOff < UINT32_MAX)
			StartTable[y] = (ILuint) DataOff;
		else
			success = IL_FALSE;
		DataOff += LenTable[y];
#ifdef __LITTLE_ENDIAN__
		iSwapUInt(&StartTable[y]);
 		iSwapUInt(&LenTable[y]);
#endif
		++y;
	}

	if (success) {
		image->io.write(StartTable, sizeof(ILuint), h * numChannels, &image->io);
		image->io.write(LenTable, sizeof(ILuint), h * numChannels, &image->io);
	}

	ifree(ScanLine);
	ifree(CompLine);
	ifree(StartTable);
	ifree(LenTable);

	return success;
}

/*----------------------------------------------------------------------------*/

// Rle does NOT work yet.

// Internal function used to save the Sgi.
// @todo: write header in one call
ILboolean iSaveSgiInternal(ILimage* image)
{
	ILuint		i, c;
	ILboolean	Compress;
	ILimage		*Temp = image;
	ILubyte		*TempData;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (image->Format != IL_LUMINANCE
	    //while the sgi spec doesn't directly forbid rgb files with 2
	    //channels, they are quite uncommon and most apps don't support
	    //them. so convert lum_a images to rgba before writing.
	    //&& image->Format != IL_LUMINANCE_ALPHA
	    && image->Format != IL_RGB
	    && image->Format != IL_RGBA) {
		if (image->Format == IL_BGRA || image->Format == IL_LUMINANCE_ALPHA)
			Temp = iConvertImage(image, IL_RGBA, DetermineSgiType(image, image->Type));
		else
			Temp = iConvertImage(image, IL_RGB, DetermineSgiType(image, image->Type));
	}
	else if (image->Type > IL_UNSIGNED_SHORT) {
		Temp = iConvertImage(image, image->Format, DetermineSgiType(image, image->Type));
	}
	
	//compression of images with 2 bytes per channel doesn't work yet
	Compress = iGetInt(IL_SGI_RLE) && Temp->Bpc == 1;

	if (Temp == NULL)
		return IL_FALSE;

	SaveBigUShort(&image->io, SGI_MAGICNUM);  // 'Magic' number
	if (Compress)
		image->io.putc(1, &image->io);
	else
		image->io.putc(0, &image->io);

	if (Temp->Type == IL_UNSIGNED_BYTE)
		image->io.putc(1, &image->io);
	else if (Temp->Type == IL_UNSIGNED_SHORT)
		image->io.putc(2, &image->io);
	// Need to error here if not one of the two...

	if (Temp->Format == IL_LUMINANCE || Temp->Format == IL_COLOUR_INDEX)
		SaveBigUShort(&image->io, 2);
	else
		SaveBigUShort(&image->io, 3);

	SaveBigUShort(&image->io, (ILushort)Temp->Width);
	SaveBigUShort(&image->io, (ILushort)Temp->Height);
	SaveBigUShort(&image->io, (ILushort)Temp->Bpp);

	switch (Temp->Type)
	{
		case IL_BYTE:
			SaveBigInt(&image->io, SCHAR_MIN);	// Minimum pixel value
			SaveBigInt(&image->io, SCHAR_MAX);	// Maximum pixel value
			break;
		case IL_UNSIGNED_BYTE:
			SaveBigInt(&image->io, 0);			// Minimum pixel value
			SaveBigInt(&image->io, UCHAR_MAX);	// Maximum pixel value
			break;
		case IL_SHORT:
			SaveBigInt(&image->io, SHRT_MIN);	// Minimum pixel value
			SaveBigInt(&image->io, SHRT_MAX);	// Maximum pixel value
			break;
		case IL_UNSIGNED_SHORT:
			SaveBigInt(&image->io, 0);			// Minimum pixel value
			SaveBigInt(&image->io, USHRT_MAX);	// Maximum pixel value
			break;
	}

	SaveBigInt(&image->io, 0);  // Dummy value

	for (i = 0; i < 80; i++) {
		image->io.putc(0, &image->io);
	}

	SaveBigUInt(&image->io, 0);  // Colormap

	// Padding
	for (i = 0; i < 101; i++) {
		SaveLittleInt(&image->io, 0);
	}


	if (image->Origin == IL_ORIGIN_UPPER_LEFT) {
		TempData = iGetFlipped(Temp);
		if (TempData == NULL) {
			if (Temp!= image)
				ilCloseImage(Temp);
			return IL_FALSE;
		}
	}
	else {
		TempData = Temp->Data;
	}


	if (!Compress) {
		for (c = 0; c < Temp->Bpp; c++) {
			for (i = c; i < Temp->SizeOfData; i += Temp->Bpp) {
				image->io.putc(TempData[i], &image->io);  // Have to save each colour plane separately.
			}
		}
	}
	else {
		iSaveRleSgi(image, TempData, Temp->Width, Temp->Height, Temp->Bpp, Temp->Bps);
	}


	if (TempData != Temp->Data)
		ifree(TempData);
	if (Temp != image)
		ilCloseImage(Temp);

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

#endif//IL_NO_SGI
