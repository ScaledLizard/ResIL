//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_lif.c
//
// Description: Reads a Homeworld image.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_LIF
#include "IL/il2.h"


typedef struct LIF_HEAD
{
	char	Id[8];			//"Willy 7"
	ILuint	Version;		// Version Number (260)
	ILuint	Flags;			// Usually 50
	ILuint	Width;
	ILuint	Height;
	ILuint	PaletteCRC;		// CRC of palettes for fast comparison.
	ILuint	ImageCRC;		// CRC of the image.
	ILuint	PalOffset;		// Offset to the palette (not used).
	ILuint	TeamEffect0;	// Team effect offset 0
	ILuint	TeamEffect1;	// Team effect offset 1
} LIF_HEAD;


// Internal function used to get the Lif header from the current file.
ILboolean iGetLifHead(SIO * io, LIF_HEAD *Header)
{

	io->read(io, Header->Id, 1, 8);

	Header->Version = GetLittleUInt(io);

	Header->Flags = GetLittleUInt(io);

	Header->Width = GetLittleUInt(io);

	Header->Height = GetLittleUInt(io);

	Header->PaletteCRC = GetLittleUInt(io);

	Header->ImageCRC = GetLittleUInt(io);

	Header->PalOffset = GetLittleUInt(io);

	Header->TeamEffect0 = GetLittleUInt(io);

	Header->TeamEffect1 = GetLittleUInt(io);


	return IL_TRUE;
}


// Internal function used to check if the HEADER is a valid Lif header.
ILboolean iCheckLif(LIF_HEAD *Header)
{
	if (Header->Version != 260 || Header->Flags != 50)
		return IL_FALSE;
	if (strcmp(Header->Id, "Willy 7"))
		return IL_FALSE;
	return IL_TRUE;
}


// Internal function to get the header and check it.
ILboolean iIsValidLif(SIO * io)
{
	LIF_HEAD	Head;

	if (!iGetLifHead(io, &Head))
		return IL_FALSE;
	io->seek(io, -(ILint)sizeof(LIF_HEAD), IL_SEEK_CUR);

	return iCheckLif(&Head);
}


ILboolean iLoadLifInternal(ILimage* image)
{
	SIO * io = &image->io;
	LIF_HEAD	LifHead;
	ILuint		i;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	
	if (!iGetLifHead(io, &LifHead))
		return IL_FALSE;

	if (!il2TexImage(image, LifHead.Width, LifHead.Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	image->Origin = IL_ORIGIN_UPPER_LEFT;

	if (!image->Pal.use(256, NULL, IL_PAL_RGBA32))
		return IL_FALSE;

	if (io->read(io, image->Data, LifHead.Width * LifHead.Height, 1) != 1)
		return IL_FALSE;
	if (!image->Pal.readFromFile(io))
		return IL_FALSE;

	// Each data offset is offset by -1, so we add one.
	for (i = 0; i < image->SizeOfData; i++) {
		image->Data[i]++;
	}

	return il2FixImage(image);
}

#endif//IL_NO_LIF
