//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_wal.c
//
// Description: Loads a Quake .wal texture.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_WAL
#include "il_manip.h"
#include "il_q2pal.h"


typedef struct WALHEAD
{
	ILbyte	FileName[32];	// Image name
	ILuint	Width;			// Width of first image
	ILuint	Height;			// Height of first image
	ILuint	Offsets[4];		// Offsets to image data
	ILbyte	AnimName[32];	// Name of next frame
	ILuint	Flags;			// ??
	ILuint	Contents;		// ??
	ILuint	Value;			// ??
} WALHEAD;

ILboolean iLoadWalInternal(ILimage* image)
{
	WALHEAD	Header;
	ILimage	*Mipmaps[3], *CurImage;
	ILuint	i, NewW, NewH;
	SIO* io = &image->io;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	CurImage = image;


	// Read header
	io->read(io, &Header.FileName, 1, 32);
	Header.Width = GetLittleUInt(io);
	Header.Height = GetLittleUInt(io);

	for (i = 0; i < 4; i++)
		Header.Offsets[i] = GetLittleUInt(io);

	io->read(io, Header.AnimName, 1, 32);
	Header.Flags = GetLittleUInt(io);
	Header.Contents = GetLittleUInt(io);
	Header.Value = GetLittleUInt(io);

	if (!il2TexImage(CurImage, Header.Width, Header.Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;

	for (i = 0; i < 3; i++) {
		Mipmaps[i] = (ILimage*)icalloc(sizeof(ILimage), 1);
		if (Mipmaps[i] == NULL)
			goto cleanup_error;
		Mipmaps[i]->Pal.Palette = (ILubyte*)ialloc(768);
		if (Mipmaps[i]->Pal.Palette == NULL)
			goto cleanup_error;
		memcpy(Mipmaps[i]->Pal.Palette, ilDefaultQ2Pal, 768);
		Mipmaps[i]->Pal.PalType = IL_PAL_RGB24;
	}

	NewW = Header.Width;
	NewH = Header.Height;
	for (i = 0; i < 3; i++) {
		NewW /= 2;
		NewH /= 2;
		image = Mipmaps[i];
		if (!il2TexImage(CurImage, NewW, NewH, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL))
			goto cleanup_error;
		// Don't set until now so ilTexImage won't get rid of the palette.
		Mipmaps[i]->Pal.PalSize = 768;
		Mipmaps[i]->Origin = IL_ORIGIN_UPPER_LEFT;
	}

	image = CurImage;
	ilCloseImage(image->Mipmaps);
	image->Mipmaps = Mipmaps[0];
	Mipmaps[0]->Mipmaps = Mipmaps[1];
	Mipmaps[1]->Mipmaps = Mipmaps[2];

	image->Origin = IL_ORIGIN_UPPER_LEFT;

	if (image->Pal.Palette && image->Pal.PalSize && image->Pal.PalType != IL_PAL_NONE)
		ifree(image->Pal.Palette);
	image->Pal.Palette = (ILubyte*)ialloc(768);
	if (image->Pal.Palette == NULL)
		goto cleanup_error;

	image->Pal.PalSize = 768;
	image->Pal.PalType = IL_PAL_RGB24;
	memcpy(image->Pal.Palette, ilDefaultQ2Pal, 768);

	io->seek(io, Header.Offsets[0], IL_SEEK_SET);
	if (io->read(io, image->Data, Header.Width * Header.Height, 1) != 1)
		goto cleanup_error;

	for (i = 0; i < 3; i++) {
		io->seek(io, Header.Offsets[i+1], IL_SEEK_SET);
		if (io->read(io, Mipmaps[i]->Data, Mipmaps[i]->Width * Mipmaps[i]->Height, 1) != 1)
			goto cleanup_error;
	}

	// Fixes all images, even mipmaps.
	return il2FixImage(image);

cleanup_error:
	for (i = 0; i < 3; i++) {
		ilCloseImage(Mipmaps[i]);
	}
	return IL_FALSE;
}


#endif//IL_NO_WAL
