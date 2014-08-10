//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_pix.c
//
// Description: Reads from an Alias | Wavefront .pix file.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_PIX
#include "il_manip.h"
#include "il_endian.h"


#ifdef _MSC_VER
#pragma pack(push, pix_struct, 1)
#endif
typedef struct PIXHEAD
{
	ILushort	Width;
	ILushort	Height;
	ILushort	OffX;
	ILushort	OffY;
	ILushort	Bpp;
} IL_PACKSTRUCT PIXHEAD;
#ifdef _MSC_VER
#pragma pack(pop, pix_struct)
#endif

// Internal function used to check if the HEADER is a valid Pix header.
ILboolean iCheckPix(PIXHEAD *Header)
{
	if (Header->Width == 0 || Header->Height == 0)
		return IL_FALSE;
	if (Header->Bpp != 24)
		return IL_FALSE;
	//if (Header->OffY != Header->Height)
	//	return IL_FALSE;

	return IL_TRUE;
}


// Internal function used to get the Pix header from the current file.
ILint iGetPixHead(SIO* io, PIXHEAD *Header)
{
	ILint read = (ILint) io->read(io, Header, 1, sizeof(PIXHEAD));

#ifdef __LITTLE_ENDIAN__
	iSwapUShort(&Header->Width);
	iSwapUShort(&Header->Height);
	iSwapUShort(&Header->OffX);
	iSwapUShort(&Header->OffY);
	iSwapUShort(&Header->Bpp);
#endif

	return read;
}


// Internal function to get the header and check it.
ILboolean iIsValidPix(SIO* io)
{
	PIXHEAD	Head;
	auto read = iGetPixHead(io, &Head);
	io->seek(io, -read, IL_SEEK_CUR);

	if (read == sizeof(Head))
		return iCheckPix(&Head);
	else
		return IL_FALSE;
}


// Internal function used to load the Pix.
ILboolean iLoadPixInternal(ILimage* image)
{
	PIXHEAD	Header;
	ILuint	i, j;
	ILubyte	ByteHead, Colour[3];

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!iGetPixHead(&image->io, &Header))
		return IL_FALSE;
	if (!iCheckPix(&Header)) {
		il2SetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!il2TexImage(image, Header.Width, Header.Height, 1, 3, IL_BGR, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;

	for (i = 0; i < image->SizeOfData; ) {
		ByteHead = image->io.getc(&image->io);
		if (image->io.read(&image->io, Colour, 1, 3) != 3)
			return IL_FALSE;
		for (j = 0; j < ByteHead; j++) {
			image->Data[i++] = Colour[0];
			image->Data[i++] = Colour[1];
			image->Data[i++] = Colour[2];
		}
	}

	image->Origin = IL_ORIGIN_UPPER_LEFT;

	return il2FixImage(image);
}

#endif//IL_NO_PIX
