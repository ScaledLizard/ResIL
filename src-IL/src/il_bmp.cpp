//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 2014 by Björn Ganster
//
// Filename: src-IL/src/il_bmp.c
//
// Description: Reads from and writes to a bitmap (.bmp) file.
//
//-----------------------------------------------------------------------------

#define IL_BMP_C

#include "il_internal.h"
#ifndef IL_NO_BMP
//#include "IL/il.h"
#include "il_bmp.h"
#include "il_endian.h"

#ifndef _WIN32
//#include <stdio.h>
#include <stdint.h>
typedef uint16_t WORD;
typedef uint32_t DWORD;
#undef CONST_RESTRICT
#define CONST_RESTRICT const
#undef putc
#endif

void GetShiftFromMask(const ILuint Mask, ILuint * CONST_RESTRICT ShiftLeft, ILuint * CONST_RESTRICT ShiftRight);


void flipHeaderEndians(BMPHEAD * const header)
{
	// @todo: untested endian conversion - I don't have a machine+OS that uses big endian
	#ifdef __BIG_ENDIAN__
	iSwapInt(Header->bfSize); //= GetLittleInt();
	iSwapUInt(Header->bfReserved); // = GetLittleUInt();
	iSwapInt(Header->bfDataOff); // = GetLittleInt();
	iSwapInt(Header->biSize); // = GetLittleInt();
	iSwapInt(Header->biWidth); // = GetLittleInt();
	iSwapInt(Header->biHeight); // = GetLittleInt();
	iSwapShort(Header->biPlanes); // = GetLittleShort();
	iSwapShort(Header->biBitCount); // = GetLittleShort();
	iSwapInt(Header->biCompression); // = GetLittleInt();
	iSwapInt(Header->biSizeImage); // = GetLittleInt();
	iSwapInt(Header->biXPelsPerMeter); // = GetLittleInt();
	iSwapInt(Header->biYPelsPerMeter); // = GetLittleInt();
	iSwapInt(Header->biClrUsed); // = GetLittleInt();
	iSwapInt(Header->biClrImportant); // = GetLittleInt();
	#endif
}

// Internal function used to get the .bmp header from the current file.
ILboolean iGetBmpHead(SIO* io, BMPHEAD * const header)
{
	ILint64 read = io->read(io, header, 1, sizeof(BMPHEAD));
	io->seek(io, -read, IL_SEEK_CUR);  // Go ahead and restore to previous state

	flipHeaderEndians(header);

	if (read == sizeof(BMPHEAD))
		return IL_TRUE;
	else
		return IL_FALSE;
}


ILboolean iGetOS2Head(SIO* io, OS2_HEAD * const Header)
{
	ILuint toRead = sizeof(OS2_HEAD);
	ILint64 read = io->read(io, Header, 1, toRead);
	io->seek(io, -read, IL_SEEK_CUR);

	if (read != toRead)
		return IL_FALSE;

	// @todo: untested endian conversion - I don't have a machine+OS that uses big endian
	#ifdef __BIG_ENDIAN__
	UShort(&Header->bfType);
	UInt(&Header->biSize);
	Short(&Header->xHotspot);
	Short(&Header->yHotspot);
	UInt(&Header->DataOff);
	UInt(&Header->cbFix);

	//2003-09-01 changed to UShort according to MSDN
	UShort(&Header->cx);
	UShort(&Header->cy);
	UShort(&Header->cPlanes);
	UShort(&Header->cBitCount);
	#endif

	return IL_TRUE;
}


// Internal function used to check if the HEADER is a valid .bmp header.
ILboolean iCheckBmp (const BMPHEAD * CONST_RESTRICT Header)
{
	//if ((Header->bfType != ('B'|('M'<<8))) || ((Header->biSize != 0x28) && (Header->biSize != 0x0C)))
	if (Header->bfType[0] != 'B' || Header->bfType[1] != 'M')
		return IL_FALSE;
	if (Header->biSize < 0x28)
		return IL_FALSE;
	if (Header->biHeight == 0 || Header->biWidth < 1)
		return IL_FALSE;
	if (Header->biPlanes > 1)  // Not sure...
		return IL_FALSE;
	if(Header->biCompression != 0 && Header->biCompression != 1 &&
		Header->biCompression != 2 && Header->biCompression != 3)
		return IL_FALSE;
	if(Header->biCompression == 3 && Header->biBitCount != 16 && Header->biBitCount != 32)
		return IL_FALSE;
	if (Header->biBitCount != 1 && Header->biBitCount != 4 && Header->biBitCount != 8 &&
		Header->biBitCount != 16 && Header->biBitCount != 24 && Header->biBitCount != 32)
		return IL_FALSE;
	return IL_TRUE;
}


ILboolean iCheckOS2 (const OS2_HEAD * CONST_RESTRICT Header)
{
	if ((Header->bfType != ('B'|('M'<<8))) || (Header->DataOff < 26) || (Header->cbFix < 12))
		return IL_FALSE;
	if (Header->cPlanes != 1)
		return IL_FALSE;
	if (Header->cx == 0 || Header->cy == 0)
		return IL_FALSE;
	if (Header->cBitCount != 1 && Header->cBitCount != 4 && Header->cBitCount != 8 &&
		 Header->cBitCount != 24)
		 return IL_FALSE;

	return IL_TRUE;
}


// Internal function to get the header and check it.
ILboolean iIsValidBmp(SIO* io)
{
	BMPHEAD		Head;
	OS2_HEAD	Os2Head;
	ILboolean	IsValid;

	iGetBmpHead(io, &Head);

	IsValid = iCheckBmp(&Head);
	if (!IsValid) {
		iGetOS2Head(io, &Os2Head);
		IsValid = iCheckOS2(&Os2Head);
	}
	return IsValid;
}


inline void GetShiftFromMask(const ILuint Mask, ILuint * CONST_RESTRICT ShiftLeft, ILuint * CONST_RESTRICT ShiftRight) {
	ILuint Temp, i;

	if( Mask == 0 ) {
		*ShiftLeft = *ShiftRight = 0;
		return;
	}

	Temp = Mask;
	for( i = 0; i < 32; i++, Temp >>= 1 ) {
		if( Temp & 1 )
			break;
	}
	*ShiftRight = i;

	// Temp is preserved, so use it again:
	for( i = 0; i < 8; i++, Temp >>= 1 ) {
		if( !(Temp & 1) )
			break;
	}
	*ShiftLeft = 8 - i;

	return;
}


ILboolean iGetOS2Bmp(ILimage* image, OS2_HEAD *Header)
{
	ILuint	PadSize, Read, i, j, k, c;
	ILubyte	ByteData;

	if (Header->cBitCount == 1) {
		if (!il2TexImage(image, Header->cx, Header->cy, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL)) {
			return IL_FALSE;
		}
		image->Origin = IL_ORIGIN_LOWER_LEFT;
		image->Pal.use(2, NULL, IL_PAL_BGR24);

		if (!image->Pal.readFromFile(&image->io))
			return IL_FALSE;

		PadSize = ((32 - (image->Width % 32)) / 8) % 4;  // Has to truncate.
		image->io.seek(&image->io, Header->DataOff, IL_SEEK_SET);

		for (j = 0; j < image->Height; j++) {
			Read = 0;
			for (i = 0; i < image->Width; ) {
				if (image->io.read(&image->io, &ByteData, 1, 1) != 1)
					return IL_FALSE;
				Read++;
				k = 128;
				for (c = 0; c < 8; c++) {
					image->Data[j * image->Width + i] =
						(!!(ByteData & k) == 1 ? 1 : 0);
					k >>= 1;
					if (++i >= image->Width)
						break;
				}
			}
			image->io.seek(&image->io, PadSize, IL_SEEK_CUR);
		}
		return IL_TRUE;
	}

	if (Header->cBitCount == 4) {
		if (!il2TexImage(image, Header->cx, Header->cy, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL)) {
			return IL_FALSE;
		}
		image->Origin = IL_ORIGIN_LOWER_LEFT;
		image->Pal.use(16, NULL, IL_PAL_BGR24);

		if (!image->Pal.readFromFile(&image->io))
			return IL_FALSE;

		PadSize = ((8 - (image->Width % 8)) / 2) % 4;  // Has to truncate
		image->io.seek(&image->io, Header->DataOff, IL_SEEK_SET);

		for (j = 0; j < image->Height; j++) {
			for (i = 0; i < image->Width; i++) {
				if (image->io.read(&image->io, &ByteData, 1, 1) != 1)
					return IL_FALSE;
				image->Data[j * image->Width + i] = ByteData >> 4;
				if (++i == image->Width)
					break;
				image->Data[j * image->Width + i] = ByteData & 0x0F;
			}
			image->io.seek(&image->io, PadSize, IL_SEEK_CUR);
		}

		return IL_TRUE;
	}

	if (Header->cBitCount == 8) {
		//added this line 2003-09-01...strange no-one noticed before...
		if (!il2TexImage(image, Header->cx, Header->cy, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL))
			return IL_FALSE;

		image->Pal.use(256, NULL, IL_PAL_BGR24);

		if (!image->Pal.readFromFile(&image->io))
			return IL_FALSE;
	}
	else { //has to be 24 bpp
		if (!il2TexImage(image, Header->cx, Header->cy, 1, 3, IL_BGR, IL_UNSIGNED_BYTE, NULL))
			return IL_FALSE;
	}
	image->Origin = IL_ORIGIN_LOWER_LEFT;

	image->io.seek(&image->io, Header->DataOff, IL_SEEK_SET);

	PadSize = (4 - (image->Bps % 4)) % 4;
	if (PadSize == 0) {
		if (image->io.read(&image->io, image->Data, 1, image->SizeOfData) != image->SizeOfData)
			return IL_FALSE;
	}
	else {
		for (i = 0; i < image->Height; i++) {
			if (image->io.read(&image->io, image->Data + i * image->Bps, 1, image->Bps) != image->Bps)
				return IL_FALSE;
			image->io.seek(&image->io, PadSize, IL_SEEK_CUR);
		}
	}

	return IL_TRUE;
}


ILboolean prepareBMP(ILimage* image, BMPHEAD * Header, ILubyte bpp, ILuint format)
{
	// Update the current image with the new dimensions
	if (!il2TexImage(image, Header->biWidth, Header->biHeight, 1, bpp, format, IL_UNSIGNED_BYTE, NULL)) 
	{
		return IL_FALSE;
	}

	// If the image height is negative, then the image is flipped
	//	(Normal is IL_ORIGIN_LOWER_LEFT)
	if (Header->biHeight < 0) {
		image->Origin = IL_ORIGIN_UPPER_LEFT;
	} else {
		image->Origin = IL_ORIGIN_LOWER_LEFT;
	}

	return IL_TRUE;
}


// Reads an uncompressed monochrome .bmp
ILboolean ilReadUncompBmp1(ILimage* image, BMPHEAD * Header)
{
	ILuint i, j, k, c, Read;
	ILubyte ByteData, PadSize, Padding[4];

	// Update the current image with the new dimensions
	if (!prepareBMP(image, Header, 1, IL_COLOUR_INDEX)) 
	{
		return IL_FALSE;
	}

	// Prepare palette
	if (!image->Pal.use(2, NULL, IL_PAL_BGR32))
		return IL_FALSE;

	// Read the palette
	image->io.seek(&image->io, sizeof(BMPHEAD), IL_SEEK_SET);
	if (!image->Pal.readFromFile(&image->io))
		return IL_FALSE;

	// Seek to the data from the "beginning" of the file
	image->io.seek(&image->io, Header->bfDataOff, IL_SEEK_SET);

	PadSize = ((32 - (image->Width % 32)) / 8) % 4;  // Has to truncate
	for (j = 0; j < image->Height; j++) {
		Read = 0;
		for (i = 0; i < image->Width; ) {
			if (image->io.read(&image->io, &ByteData, 1, 1) != 1) {
				return IL_FALSE;
			}
			Read++;
			k = 128;
			for (c = 0; c < 8; c++) {
				image->Data[j * image->Width + i] = 
					(!!(ByteData & k) == 1 ? 1 : 0);
				k >>= 1;
				if (++i >= image->Width)
					break;
			}
		}
		image->io.read(&image->io, Padding, 1, PadSize);
	}

	return IL_TRUE;
}


// Reads an uncompressed .bmp
ILboolean ilReadUncompBmp4(ILimage* image, BMPHEAD * Header)
{
	ILuint i, j;
	ILubyte ByteData, PadSize, Padding[4];

	// Update the current image with the new dimensions
	if (!prepareBMP(image, Header,  1, IL_COLOUR_INDEX))
	{
		return IL_FALSE;
	}

	// Prepare palette
	ILuint newColCount = Header->biClrUsed ? Header->biClrUsed : 256;
			
	if (Header->biBitCount == 4)  // biClrUsed is 0 for 4-bit bitmaps
		newColCount = 16;
	if (!image->Pal.use(newColCount, NULL, IL_PAL_BGR32))
		return IL_FALSE;

	// Read the palette
	image->io.seek(&image->io, sizeof(BMPHEAD), IL_SEEK_SET);
	if (!image->Pal.readFromFile(&image->io))
		return IL_FALSE;

	// Seek to the data from the "beginning" of the file
	image->io.seek(&image->io, Header->bfDataOff, IL_SEEK_SET);

	PadSize = ((8 - (image->Width % 8)) / 2) % 4;  // Has to truncate
	for (j = 0; j < image->Height; j++) {
		for (i = 0; i < image->Width; i++) {
			if (image->io.read(&image->io, &ByteData, 1, 1) != 1) {
				return IL_FALSE;
			}
			image->Data[j * image->Width + i] = ByteData >> 4;
			if (++i == image->Width)
				break;
			image->Data[j * image->Width + i] = ByteData & 0x0F;
		}
		image->io.read(&image->io, Padding, 1, PadSize);
	}

	return IL_TRUE;
}


// Reads an uncompressed .bmp
ILboolean ilReadUncompBmp8(ILimage* image, BMPHEAD * Header)
{
	// Update the current image with the new dimensions
	if (!prepareBMP(image, Header,  1, IL_COLOUR_INDEX))
	{
		return IL_FALSE;
	}

	// Prepare the palette
	ILuint newColCount = Header->biClrUsed ? Header->biClrUsed : 256;
			
	if (Header->biBitCount == 4)  // biClrUsed is 0 for 4-bit bitmaps
		newColCount = 16;
	if (!image->Pal.use(newColCount, NULL, IL_PAL_BGR32))
		return IL_FALSE;

	// Read the palette
	image->io.seek(&image->io, sizeof(BMPHEAD), IL_SEEK_SET);
	if (!image->Pal.readFromFile(&image->io))
		return IL_FALSE;

	// Seek to the data from the "beginning" of the file
	image->io.seek(&image->io, Header->bfDataOff, IL_SEEK_SET);

	ILuint PadSize = (4 - (image->Bps % 4)) % 4;
	if (PadSize == 0) {
		// If the data does not use padding, we can read it immediately
		if (image->io.read(&image->io, image->Data, 1, image->SizeOfPlane) != image->SizeOfPlane)
			return IL_FALSE;
	} else {	
		// Remove padding while reading
		for (ILuint i = 0; i < image->SizeOfPlane; i += image->Bps) {
			auto read = image->io.read(&image->io, image->Data + i, 1, image->Bps);
			if (read != image->Bps) {
				return IL_FALSE;
			}
			image->io.seek(&image->io, PadSize, IL_SEEK_CUR);
		}
	}

	return IL_TRUE;
}


ILboolean ilReadUncompBmp16(ILimage* image, BMPHEAD * Header)
{
	ILubyte PadSize = 0;
	ILuint rMask, gMask, bMask, rShiftR, gShiftR, bShiftR, rShiftL, gShiftL, bShiftL;
	ILuint inputBps = 2 * Header->biWidth;
	ILubyte* inputScanline = (ILubyte*) ialloc(inputBps);
	ILuint inputOffset = Header->bfDataOff;

	if ((Header->biWidth & 1) != 0) {
		PadSize = 2;
		inputBps += PadSize;
	}

	// Update the current image with the new dimensions, unpack to 24 bpp
	if (!prepareBMP(image, Header,  3, IL_BGR))
	{
		return IL_FALSE;
	}

	if (Header->biCompression == 3)
	{
		// If the bmp uses bitfields, we have to use them to decode the image
		ILuint readOffset = Header->biSize + 14;
		image->io.seek(&image->io, readOffset, IL_SEEK_SET); //seek to bitfield data
		image->io.read(&image->io, &rMask, 4, 1);
		image->io.read(&image->io, &gMask, 4, 1);
		image->io.read(&image->io, &bMask, 4, 1);
		UInt(&rMask);
		UInt(&gMask);
		UInt(&bMask);
		GetShiftFromMask(rMask, &rShiftL, &rShiftR);
		GetShiftFromMask(gMask, &gShiftL, &gShiftR);
		GetShiftFromMask(bMask, &bShiftL, &bShiftR);
	} else {
		// Default values if bitfields are not used
		rMask = 0x7C00;
		gMask = 0x03E0;
		bMask = 0x001F;
		rShiftR = 10;
		gShiftR = 5;
		bShiftR = 0;
		rShiftL = 3;
		gShiftL = 3;
		bShiftL = 3;
	}

	//@TODO: This may not be safe for Big Endian.
	// Read pixels
	for (ILuint y = 0; y < image->Height; y++) {
		image->io.seek(&image->io, inputOffset, IL_SEEK_SET);
		auto read = image->io.read(&image->io, inputScanline, 1, inputBps);
		if (read != inputBps) {
			return IL_FALSE;
		}
		inputOffset += inputBps;
		ILuint k = image->Bps * y;

		for(ILuint x = 0; x < image->Width; x++, k += 3) {
			WORD Read16 = ((WORD*) inputScanline)[x];
			image->Data[k] = ((Read16 & bMask) >> bShiftR) << bShiftL;
			image->Data[k + 1] = ((Read16 & gMask) >> gShiftR)  << gShiftL;
			image->Data[k + 2] = ((Read16 & rMask) >> rShiftR) << rShiftL;
		}
	}

	ifree(inputScanline);

	return IL_TRUE;
}


// Reads an uncompressed .bmp
ILboolean ilReadUncompBmp24(ILimage* image, BMPHEAD * Header)
{
	// Update the current image with the new dimensions
	if (!prepareBMP(image, Header,  3, IL_BGR))
	{
		return IL_FALSE;
	}

	// Seek to the data from the "beginning" of the file
	image->io.seek(&image->io, Header->bfDataOff, IL_SEEK_SET);

	// Read data
	ILubyte PadSize = (4 - (image->Bps % 4)) % 4;
	if (PadSize == 0) {
		// Read entire data with just one call
		if (image->io.read(&image->io, image->Data, 1, image->SizeOfPlane) != image->SizeOfPlane)
			return IL_FALSE;
	} else {	
		// Microsoft requires lines to be padded if the widths aren't multiples of 4
		for (ILuint i = 0; i < image->SizeOfPlane; i += image->Bps) {
			auto read = image->io.read(&image->io, image->Data + i, 1, image->Bps);
			if (read != image->Bps) {
				return IL_FALSE;
			}
			image->io.seek(&image->io, PadSize, IL_SEEK_CUR);
		}
	}

	return IL_TRUE;
}

// Reads an uncompressed 32-bpp .bmp
ILboolean ilReadUncompBmp32(ILimage* image, BMPHEAD * Header)
{
	// Update the current image with the new dimensions
	if (!prepareBMP(image, Header,  4, IL_BGRA))
	{
		return IL_FALSE;
	}

	// Read pixel data
	image->io.seek(&image->io, Header->bfDataOff, IL_SEEK_SET);
	auto read = image->io.read(&image->io, image->Data, 1, image->SizeOfPlane);

	// Convert data: ABGR to BGRA
	// @TODO: bitfields are not supported here yet ... would mean that at least one color channel 
	// could use more than 8 bits precision
	DWORD* start = (DWORD*) il2GetData(image);
	DWORD* stop = start + image->Width * image->Height;
	for (DWORD* pixel = start; pixel < stop; ++pixel)
		(*pixel) = (((*pixel) && 255) << 24) + ((*pixel) >> 8);

	if (read == image->SizeOfPlane)
	{
		return IL_TRUE;
	} else {
		return IL_FALSE;
	}
}

// Reads an uncompressed .bmp
ILboolean ilReadUncompBmp(ILimage* image, BMPHEAD * header)
{
	// A height of 0 is illegal
	if (header->biHeight == 0) {
		il2SetError(IL_ILLEGAL_FILE_VALUE);
		image->Pal.clear();
		return IL_FALSE;
	}

	switch (header->biBitCount)
	{
		case 1:
			return ilReadUncompBmp1(image, header);
		case 4:
			return ilReadUncompBmp4(image, header);
		case 8:
			return ilReadUncompBmp8(image, header);
		case 16:
			return ilReadUncompBmp16(image, header);
		case 24:
			return ilReadUncompBmp24(image, header);
		case 32:
			return ilReadUncompBmp32(image, header);
		default:
			il2SetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
	}
}


ILboolean ilReadRLE8Bmp(ILimage* image, BMPHEAD *Header)
{
	ILubyte	Bytes[2];
	size_t	offset = 0, count, endOfLine = Header->biWidth;

	// Update the current image with the new dimensions
	if (!il2TexImage(image, Header->biWidth, Header->biHeight, 1, 1, 0, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;

	image->Origin = IL_ORIGIN_LOWER_LEFT;

	// A height of 0 is illegal
	if (Header->biHeight == 0)
		return IL_FALSE;

	image->Format = IL_COLOUR_INDEX;
	ILuint newColCount = Header->biClrUsed;
	if (newColCount == 0)
		newColCount = 256;
	if (!image->Pal.use(newColCount, NULL, IL_PAL_BGR32))
		return IL_FALSE;

	// If the image height is negative, then the image is flipped
	//	(Normal is IL_ORIGIN_LOWER_LEFT)
	image->Origin = Header->biHeight < 0 ?
		 IL_ORIGIN_UPPER_LEFT : IL_ORIGIN_LOWER_LEFT;
	
	// Read the palette
	image->io.seek(&image->io, sizeof(BMPHEAD), IL_SEEK_SET);
	if (!image->Pal.readFromFile(&image->io))
		return IL_FALSE;

	// Seek to the data from the "beginning" of the file
	image->io.seek(&image->io, Header->bfDataOff, IL_SEEK_SET);

    while (offset < image->SizeOfData) {
		if (image->io.read(&image->io, Bytes, sizeof(Bytes), 1) != 1)
			return IL_FALSE;
		if (Bytes[0] == 0x00) {  // Escape sequence
			switch (Bytes[1])
			{
				case 0x00:  // End of line
					offset = endOfLine;
					endOfLine += image->Width;
					break;
				case 0x01:  // End of bitmap
					offset = image->SizeOfData;
					break;
				case 0x2:
					if (image->io.read(&image->io, Bytes, sizeof(Bytes), 1) != 1)
						return IL_FALSE;
					offset += Bytes[0] + Bytes[1] * image->Width;
					endOfLine += Bytes[1] * image->Width;
					break;
				default:
					count = IL_MIN(Bytes[1], image->SizeOfData-offset);
					if (image->io.read(&image->io, image->Data + offset, (ILuint)count, 1) != 1)
						return IL_FALSE;
					offset += count;
					if ((count & 1) == 1)  // Must be on a word boundary
						if (image->io.read(&image->io, Bytes, 1, 1) != 1)
							return IL_FALSE;
					break;
			}
		} else {
			count = IL_MIN (Bytes[0], image->SizeOfData-offset);
			memset(image->Data + offset, Bytes[1], count);
			offset += count;
		}
	}
	return IL_TRUE;
}


ILboolean ilReadRLE4Bmp(ILimage* image, BMPHEAD *Header)
{
	ILubyte	Bytes[2];
	ILuint	i;
	size_t	offset = 0, count, endOfLine = Header->biWidth;

	// Update the current image with the new dimensions
	if (!il2TexImage(image, Header->biWidth, Header->biHeight, 1, 1, 0, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;
	image->Origin = IL_ORIGIN_LOWER_LEFT;

	// A height of 0 is illegal
	if (Header->biHeight == 0) {
		il2SetError(IL_ILLEGAL_FILE_VALUE);
		return IL_FALSE;
	}

	image->Format = IL_COLOUR_INDEX;
	if (!image->Pal.use(16, NULL, IL_PAL_BGR32))
		return IL_FALSE;

	// If the image height is negative, then the image is flipped
	//	(Normal is IL_ORIGIN_LOWER_LEFT)
	image->Origin = Header->biHeight < 0 ?
		 IL_ORIGIN_UPPER_LEFT : IL_ORIGIN_LOWER_LEFT;

	// Read the palette
	image->io.seek(&image->io, sizeof(BMPHEAD), IL_SEEK_SET);

	if (!image->Pal.readFromFile(&image->io))
		return IL_FALSE;

	// Seek to the data from the "beginning" of the file
	image->io.seek(&image->io, Header->bfDataOff, IL_SEEK_SET);

	while (offset < image->SizeOfData) {
      int align;
		if (image->io.read(&image->io, &Bytes[0], sizeof(Bytes), 1) != 1)
			return IL_FALSE;
		if (Bytes[0] == 0x0) {				// Escape sequence
         switch (Bytes[1]) {
         case 0x0:	// End of line
            offset = endOfLine;
            endOfLine += image->Width;
            break;
         case 0x1:	// End of bitmap
            offset = image->SizeOfData;
            break;
         case 0x2:
				if (image->io.read(&image->io, &Bytes[0], sizeof(Bytes), 1) != 1)
					return IL_FALSE;
            offset += Bytes[0] + Bytes[1] * image->Width;
            endOfLine += Bytes[1] * image->Width;
            break;
         default:	  // Run of pixels
            count = IL_MIN (Bytes[1], image->SizeOfData-offset);

				for (i = 0; i < count; i++) {
					int byte;

					if ((i & 0x01) == 0) {
						if (image->io.read(&image->io, &Bytes[0], sizeof(Bytes[0]), 1) != 1)
							return IL_FALSE;
						byte = (Bytes[0] >> 4);
					}
					else
						byte = (Bytes[0] & 0x0F);
					image->Data[offset++] = byte;
				}

				align = Bytes[1] % 4;

				if (align == 1 || align == 2)	// Must be on a word boundary
					if (image->io.read(&image->io, &Bytes[0], sizeof(Bytes[0]), 1) != 1)
						return IL_FALSE;
			}
		} else {
         count = IL_MIN (Bytes[0], image->SizeOfData-offset);
         Bytes[0] = (Bytes[1] >> 4);
			Bytes[1] &= 0x0F;
			for (i = 0; i < count; i++)
				image->Data[offset++] = Bytes [i & 1];
		}
	}

   return IL_TRUE;
}


// Internal function used to load the .bmp.
ILboolean iLoadBitmapInternal(ILimage* image)
{
	BMPHEAD		Header;
	OS2_HEAD	Os2Head;
	ILboolean	bBitmap;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	iGetBmpHead(&image->io, &Header);
	if (!iCheckBmp(&Header)) {
		iGetOS2Head(&image->io, &Os2Head);
		if (!iCheckOS2(&Os2Head)) {
			il2SetError(IL_INVALID_FILE_HEADER);
			return IL_FALSE;
		}
		else {
			return iGetOS2Bmp(image, &Os2Head);
		}
	}

	// Don't know what to do if it has more than one plane...
	if (Header.biPlanes != 1) {
		il2SetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	switch (Header.biCompression)
	{
		case 0:	// No compression
		case 3:	// BITFIELDS compression is handled in 16 bit
						// and 32 bit code in ilReadUncompBmp()
			bBitmap = ilReadUncompBmp(image, &Header);
			break;
		case 1:  //	RLE 8-bit / pixel (BI_RLE8)
			bBitmap = ilReadRLE8Bmp(image, &Header);
			break;
		case 2:  // RLE 4-bit / pixel (BI_RLE4)
			bBitmap = ilReadRLE4Bmp(image, &Header);
			break;

		default:
			il2SetError(IL_INVALID_FILE_HEADER);
			return IL_FALSE;
	}

	if (!il2FixImage(image))
		return IL_FALSE;

	return bBitmap;
}


// Internal function used to save .bmp files
ILboolean iSaveBitmapInternal(ILimage* image)
{
	if (image == NULL)
		return IL_FALSE;

	BMPHEAD header;
	SIO * io = &image->io;
	//int compress_rle8 = ilGetInteger(IL_BMP_RLE);
	int compress_rle8 = IL_FALSE; // disabled BMP RLE compression. broken
	ILuint	i, PadSize, Padding = 0;
	ILimage	*TempImage = NULL;
	ILubyte	*TempData;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	// Fill out header. Some information, like file size, must be added later
	memset(&header, 0, sizeof(BMPHEAD));
	header.bfType[0] = 'B';
	header.bfType[1] = 'M';
	header.bfDataOff = sizeof(BMPHEAD);
	header.biSize = 40; // size of "second" header
	header.biWidth = image->Width;
	header.biHeight = image->Height;
	header.biPlanes = 1;
	header.biClrImportant = 1;

	ILpal TempPal;
	if( compress_rle8 == IL_TRUE ) {
		header.biCompression = 1; 
		TempImage = iConvertImage(image, IL_COLOR_INDEX, IL_UNSIGNED_BYTE);
		TempPal = iConvertPal(&TempImage->Pal, IL_PAL_BGR32);
	}

	if((image->Format == IL_LUMINANCE) && (!image->Pal.hasPalette()))
	{
		// For luminance images it is necessary to generate a grayscale BGR32
		//  color palette.  Could call iConvertImage(..., IL_COLOR_INDEX, ...)
		//  to generate an RGB24 palette, followed by iConvertPal(..., IL_PAL_BGR32),
		//  to convert the palette to BGR32, but it seemed faster to just
		//  explicitely generate the correct palette.
		image->Pal.use(256, NULL, IL_PAL_BGR32);
		
		// Generate grayscale palette
		for (i = 0; i < 256; i++)
		{
			image->Pal.setRGBA(i, i, i, i, 0);
		}
	}
	
	// Convert palette to BGR32, if necessary
	TempPal = image->Pal;
	if (image->Pal.hasPalette()) {
		// If the palette in .bmp format, write it directly
		if (image->Pal.getPalType() != IL_PAL_BGR32) {
			TempPal = iConvertPal(&image->Pal, IL_PAL_BGR32);
		}
		header.biClrUsed = il2GetInteger(IL_PALETTE_NUM_COLS);
	}
	
	if ((image->Format != IL_BGR) && (image->Format != IL_BGRA) && 
			(image->Format != IL_COLOUR_INDEX) && (image->Format != IL_LUMINANCE)) {
		if (image->Format == IL_RGBA) {
			TempImage = iConvertImage(image, IL_BGRA, IL_UNSIGNED_BYTE);
		} else {
			TempImage = iConvertImage(image, IL_BGR, IL_UNSIGNED_BYTE);
		}
		if (TempImage == NULL)
			return IL_FALSE;
	} else if (image->Bpc > 1) {
		TempImage = iConvertImage(image, image->Format, IL_UNSIGNED_BYTE);
		if (TempImage == NULL)
			return IL_FALSE;
	} else {
		TempImage = image;
	}

	if (TempImage->Origin != IL_ORIGIN_LOWER_LEFT) {
		TempData = iGetFlipped(TempImage);
		if (TempData == NULL) {
			ilCloseImage(TempImage);
			return IL_FALSE;
		}
	} else {
		TempData = TempImage->Data;
	}

	// Write header and palette, header will be updated later
	io->write(&header, 1, sizeof(BMPHEAD), io);
	if (TempPal.getPalSize() > 0) {
		header.biClrUsed = TempPal.getPalSize()/4;
		header.bfDataOff += TempPal.getPalSize();
		TempPal.writeToFile(io);
	}
	
	if( compress_rle8 == IL_TRUE ) {
		//@TODO compress and save
		ILubyte *Dest = (ILubyte*)ialloc((long)((double)TempImage->SizeOfPlane*130/127));
		header.bfSize = ilRleCompress(TempImage->Data,TempImage->Width,TempImage->Height,
						TempImage->Depth,TempImage->Bpp,Dest,IL_BMPCOMP,NULL);
		io->write(Dest, 1, header.biSize, io);
	} else {
		PadSize = (4 - (TempImage->Bps % 4)) % 4;
		// No padding, so write data directly
		if (PadSize == 0) {
			io->write(TempData, 1, TempImage->SizeOfPlane, io);
		} else {  // Odd width, so we must pad each line
			for (i = 0; i < TempImage->SizeOfPlane; i += TempImage->Bps) {
				io->write(TempData + i, 1, TempImage->Bps, io); // Write data
				io->write(&Padding, 1, PadSize, io); // Write pad byte(s)
			}
		}
		header.bfSize = (ILint) io->tell(io);
	}
	
	// Write updated header
	header.biBitCount = (TempImage->Bpp << 3);
	io->seek(io, 0, IL_SEEK_SET);
	flipHeaderEndians(&header);
	io->write(&header, 1, sizeof(BMPHEAD), io);
	io->seek(io, header.biSize, IL_SEEK_SET);

	//TempPal->clear(); // Destructor will be called when it goes off scope
	if (TempData != TempImage->Data)
		ifree(TempData);
	if (TempImage != image)
		ilCloseImage(TempImage);
	
	return IL_TRUE;
}

#endif//IL_NO_BMP
