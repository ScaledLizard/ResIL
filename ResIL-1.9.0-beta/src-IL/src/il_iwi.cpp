//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/04/2009
//
// Filename: src-IL/src/il_iwi.c
//
// Description: Reads from an Infinity Ward Image (.iwi) file from Call of Duty.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_IWI
#include "il_dds.h"
#include "IL/il.h"

typedef struct IWIHEAD
{
	ILuint		Signature;
	ILubyte		Format;
	ILubyte		Flags;
	ILushort	Width;
	ILushort	Height;
	ILubyte unknown[18]; // @todo ???
} IWIHEAD;

#define IWI_ARGB8	0x01
#define IWI_RGB8	0x02
#define IWI_ARGB4	0x03
#define IWI_A8		0x04
#define IWI_JPG		0x07
#define IWI_DXT1	0x0B
#define IWI_DXT3	0x0C
#define IWI_DXT5	0x0D

ILboolean iCheckIwi(IWIHEAD *Header);
ILboolean iLoadIwiInternal(ILimage* image);
ILboolean IwiInitMipmaps(ILimage *BaseImage, ILuint *NumMips);
ILboolean IwiReadImage(ILimage *BaseImage, IWIHEAD *Header, ILuint NumMips);
ILenum IwiGetFormat(ILubyte Format, ILubyte *Bpp);


// Internal function used to get the IWI header from the current file.
/*ILboolean iGetIwiHead(SIO* io, IWIHEAD *Header)
{
	Header->Signature = GetLittleUInt(io);
	Header->Format = io->getc(io);
	Header->Flags = io->getc(io);  //@TODO: Find out what the flags mean.
	Header->Width = GetLittleUShort(io);
	Header->Height = GetLittleUShort(io);

	// @TODO: Find out what is in the rest of the header.
	io->seek(io, 18, IL_SEEK_CUR);

	return IL_TRUE;
}*/

ILint iGetIwiHead(SIO* io, IWIHEAD *Header)
{
	return (ILint) io->read(io, Header, 1, sizeof(IWIHEAD));
}


// Internal function to get the header and check it.
ILboolean iIsValidIwi(SIO* io)
{
	IWIHEAD		Header;
	auto		read = iGetIwiHead(io, &Header);
	io->seek(io, -read, IL_SEEK_CUR);

	if (read == sizeof(IWIHEAD))
		return iCheckIwi(&Header);
	else
		return IL_FALSE;
}


// Internal function used to check if the HEADER is a valid IWI header.
ILboolean iCheckIwi(IWIHEAD *Header)
{
	if (Header->Signature != 0x06695749 && Header->Signature != 0x05695749)  // 'IWi-' (version 6, and version 5 is the second).
		return IL_FALSE;
	if (Header->Width == 0 || Header->Height == 0)
		return IL_FALSE;
	// DXT images must have power-of-2 dimensions.
	if (Header->Format == IWI_DXT1 || Header->Format == IWI_DXT3 || Header->Format == IWI_DXT5)
		if (Header->Width != ilNextPower2(Header->Width) || Header->Height != ilNextPower2(Header->Height))
			return IL_FALSE;
	// 0x0B, 0x0C and 0x0D are DXT formats.
	if (Header->Format != IWI_ARGB4 && Header->Format != IWI_RGB8 && Header->Format != IWI_ARGB8 && Header->Format != IWI_A8 
		&& Header->Format != IWI_DXT1 && Header->Format != IWI_DXT3 && Header->Format != IWI_DXT5)
		return IL_FALSE;

	return IL_TRUE;
}


// Internal function used to load the IWI.
ILboolean iLoadIwiInternal(ILimage* image)
{
	IWIHEAD		Header;
	ILuint		NumMips = 0;
	ILboolean	HasMipmaps = IL_TRUE;
	ILenum		Format;
	ILubyte		Bpp;
	SIO* io = &image->io;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	// Read the header and check it.
	auto read = iGetIwiHead(io, &Header);
	if (read != sizeof(IWIHEAD))
		return IL_FALSE;
	if (!iCheckIwi(&Header)) {
		il2SetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// From a post by Pointy on http://iwnation.com/forums/index.php?showtopic=27903,
	//  flags ending with 0x3 have no mipmaps.
	HasMipmaps = ((Header.Flags & 0x03) == 0x03) ? IL_FALSE : IL_TRUE;

	// Create the image, then create the mipmaps, then finally read the image.
	Format = IwiGetFormat(Header.Format, &Bpp);
	if (!il2TexImage(image, Header.Width, Header.Height, 1, Bpp, Format, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;
	image->Origin = IL_ORIGIN_UPPER_LEFT;
	if (HasMipmaps)
		if (!IwiInitMipmaps(image, &NumMips))
			return IL_FALSE;
	if (!IwiReadImage(image, &Header, NumMips))
		return IL_FALSE;

	return il2FixImage(image);
}


// Helper function to convert IWI formats to DevIL formats and Bpp.
ILenum IwiGetFormat(ILubyte Format, ILubyte *Bpp)
{
	switch (Format)
	{
		case IWI_ARGB8:
			*Bpp = 4;
			return IL_BGRA;
		case IWI_RGB8:
			*Bpp = 3;
			return IL_BGR;
		case IWI_ARGB4:
			*Bpp = 4;
			return IL_BGRA;
		case IWI_A8:
			*Bpp = 1;
			return IL_ALPHA;
		case IWI_DXT1:
			*Bpp = 4;
			return IL_RGBA;
		case IWI_DXT3:
			*Bpp = 4;
			return IL_RGBA;
		case IWI_DXT5:
			*Bpp = 4;
			return IL_RGBA;
	}

	return 0;  // Will never reach this.
}


// Function to intialize the mipmaps and determine the number of mipmaps.
ILboolean IwiInitMipmaps(ILimage *BaseImage, ILuint *NumMips)
{
	ILimage	*Image;
	ILuint	Width, Height, Mipmap;

	Image = BaseImage;
	Width = BaseImage->Width;  Height = BaseImage->Height;
	Image->Origin = IL_ORIGIN_UPPER_LEFT;

	for (Mipmap = 0; Width != 1 && Height != 1; Mipmap++) {
		// 1 is the smallest dimension possible.
		Width = (Width >> 1) == 0 ? 1 : (Width >> 1);
		Height = (Height >> 1) == 0 ? 1 : (Height >> 1);

		Image->Mipmaps = ilNewImageFull(Width, Height, 1, BaseImage->Bpp, BaseImage->Format, BaseImage->Type, NULL);
		if (Image->Mipmaps == NULL)
			return IL_FALSE;
		Image = Image->Mipmaps;

		// ilNewImage does not set these.
		Image->Format = BaseImage->Format;
		Image->Type = BaseImage->Type;
		// The origin is in the upper left.
		Image->Origin = IL_ORIGIN_UPPER_LEFT;
	}

	*NumMips = Mipmap;
	return IL_TRUE;
}


ILboolean IwiReadImage(ILimage *BaseImage, IWIHEAD *Header, ILuint NumMips)
{
	ILimage	*Image;
	ILuint	SizeOfData;
	ILubyte	*CompData = NULL;
	ILint	i, j, k, m;
	SIO* io = &BaseImage->io;

	for (i = NumMips; i >= 0; i--) {
		Image = BaseImage;
		// Go to the ith mipmap level.
		//  The mipmaps go from smallest to the largest.
		for (j = 0; j < i; j++)
			Image = Image->Mipmaps;

		switch (Header->Format)
		{
			case IWI_ARGB8: // These are all
			case IWI_RGB8:  //  uncompressed data,
			case IWI_A8:    //  so just read it.
				if (io->read(io, Image->Data, 1, Image->SizeOfData) != Image->SizeOfData)
					return IL_FALSE;
				break;

			case IWI_ARGB4:  //@TODO: Find some test images for this.
				// Data is in ARGB4 format - 4 bits per component.
				SizeOfData = Image->Width * Image->Height * 2;
				CompData = (ILubyte*) ialloc(SizeOfData);  // Not really compressed - just in ARGB4 format.
				if (CompData == NULL)
					return IL_FALSE;
				if (io->read(io, CompData, 1, SizeOfData) != SizeOfData) {
					ifree(CompData);
					return IL_FALSE;
				}
				for (k = 0, m = 0; k < (ILint)Image->SizeOfData; k += 4, m += 2) {
					// @TODO: Double the image data into the low and high nibbles for a better range of values.
					Image->Data[k+0] = CompData[m] & 0xF0;
					Image->Data[k+1] = (CompData[m] & 0x0F) << 4;
					Image->Data[k+2] = CompData[m+1] & 0xF0;
					Image->Data[k+3] = (CompData[m+1] & 0x0F) << 4;
				}
				break;

			case IWI_DXT1:
				// DXT1 data has at least 8 bytes, even for one pixel.
				SizeOfData = IL_MAX(Image->Width * Image->Height / 2, 8);
				CompData = (ILubyte*) ialloc(SizeOfData);  // Gives a 6:1 compression ratio (or 8:1 for DXT1 with alpha)
				if (CompData == NULL)
					return IL_FALSE;
				if (io->read(io, CompData, 1, SizeOfData) != SizeOfData) {
					ifree(CompData);
					return IL_FALSE;
				}

				// Decompress the DXT1 data into Image (ith mipmap).
				if (!DecompressDXT1(Image, CompData)) {
					ifree(CompData);
					return IL_FALSE;
				}

				// Keep a copy of the DXTC data if the user wants it.
				if (ilGetInteger(IL_KEEP_DXTC_DATA) == IL_TRUE) {
					Image->DxtcSize = SizeOfData;
					Image->DxtcData = CompData;
					Image->DxtcFormat = IL_DXT1;
					CompData = NULL;
				}

				break;

			case IWI_DXT3:
				// DXT3 data has at least 16 bytes, even for one pixel.
				SizeOfData = IL_MAX(Image->Width * Image->Height, 16);
				CompData = (ILubyte*) ialloc(SizeOfData);  // Gives a 4:1 compression ratio
				if (CompData == NULL)
					return IL_FALSE;
				if (io->read(io, CompData, 1, SizeOfData) != SizeOfData) {
					ifree(CompData);
					return IL_FALSE;
				}

				// Decompress the DXT3 data into Image (ith mipmap).
				if (!DecompressDXT3(Image, CompData)) {
					ifree(CompData);
					return IL_FALSE;
				}
				break;

			case IWI_DXT5:
				// DXT5 data has at least 16 bytes, even for one pixel.
				SizeOfData = IL_MAX(Image->Width * Image->Height, 16);
				CompData = (ILubyte*) ialloc(SizeOfData);  // Gives a 4:1 compression ratio
				if (CompData == NULL)
					return IL_FALSE;
				if (io->read(io, CompData, 1, SizeOfData) != SizeOfData) {
					ifree(CompData);
					return IL_FALSE;
				}

				// Decompress the DXT5 data into Image (ith mipmap).
				if (!DecompressDXT5(Image, CompData)) {
					ifree(CompData);
					return IL_FALSE;
				}
				break;
		}
	
		ifree(CompData);
	}

	return IL_TRUE;
}


#endif//IL_NO_IWI
