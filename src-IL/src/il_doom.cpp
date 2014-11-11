//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_doom.c
//
// Description: Reads Doom textures and flats
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_DOOM
#include "IL/il.h"
#include "IL/il2.h"
#include "il_manip.h"
#include "il_pal.h"
#include "il_doompal.h"


//
// READ A DOOM IMAGE
//

// From the DTE sources (mostly by Denton Woods with corrections by Randy Heit)
ILboolean iLoadDoomInternal(ILimage* image)
{
	ILshort	width, height, graphic_header[2], column_loop, row_loop;
	ILint	column_offset;
	ILubyte	post, topdelta, length;
	ILubyte	*NewData;
	ILuint	i;
	SIO * io = &image->io;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	auto first_pos = io->tell(io);  // Needed to go back to the offset table
	width = GetLittleShort(io);
	height = GetLittleShort(io);
	graphic_header[0] = GetLittleShort(io);  // Not even used
	graphic_header[1] = GetLittleShort(io);  // Not even used

	if (!il2TexImage(image, width, height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	image->Origin = IL_ORIGIN_UPPER_LEFT;

	if (!image->Pal.use(IL_DOOMPAL_SIZE/3, (ILubyte*) ilDefaultDoomPal, IL_PAL_RGB24))
		return IL_FALSE;

	// 247 is always the transparent colour (usually cyan)
	memset(image->Data, 247, image->SizeOfData);

	for (column_loop = 0; column_loop < width; column_loop++) {
		column_offset = GetLittleInt(io);
		auto pointer_position = io->tell(io);
		io->seek(io, first_pos + column_offset, IL_SEEK_SET);

		while (1) {
			if (io->read(io, &topdelta, 1, 1) != 1)
				return IL_FALSE;
			if (topdelta == 255)
				break;
			if (io->read(io, &length, 1, 1) != 1)
				return IL_FALSE;
			if (io->read(io, &post, 1, 1) != 1)
				return IL_FALSE; // Skip extra byte for scaling

			for (row_loop = 0; row_loop < length; row_loop++) {
				if (io->read(io, &post, 1, 1) != 1)
					return IL_FALSE;
				if (row_loop + topdelta < height)
					image->Data[(row_loop+topdelta) * width + column_loop] = post;
			}
			io->read(io, &post, 1, 1); // Skip extra scaling byte
		}

		io->seek(io, pointer_position, IL_SEEK_SET);
	}

	// Converts palette entry 247 (cyan) to transparent.
	if (ilGetBoolean(IL_CONV_PAL) == IL_TRUE) {
		NewData = (ILubyte*)ialloc(image->SizeOfData * 4);
		if (NewData == NULL) {
			return IL_FALSE;
		}

		for (i = 0; i < image->SizeOfData; i++) {
			ILubyte r, g, b, a;
			image->Pal.getRGBA(i, r, g, b, a);
			NewData[i * 4] = r;
			NewData[i * 4 + 1] = g;
			NewData[i * 4 + 2] = b;
			NewData[i * 4 + 3] = i != 247 ? 255 : 0;
		}

		if (!ilTexImage(image->Width, image->Height, image->Depth,
			4, IL_RGBA, image->Type, NewData)) 
		{
			ifree(NewData);
			return IL_FALSE;
		}
		image->Origin = IL_ORIGIN_UPPER_LEFT;
		ifree(NewData);
	}

	return il2FixImage(image);
}


//
// READ A DOOM FLAT
//

// Basically just ireads 4096 bytes and copies the palette
ILboolean iLoadDoomFlatInternal(ILimage* image)
{
	ILubyte	*NewData;
	ILuint	i;
	SIO* io = &image->io;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!ilTexImage(64, 64, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	image->Origin = IL_ORIGIN_UPPER_LEFT;

	image->Pal.use(IL_DOOMPAL_SIZE/3, (ILubyte*) ilDefaultDoomPal, IL_PAL_RGB24);

	if (io->read(io, image->Data, 1, 4096) != 4096)
		return IL_FALSE;

	if (ilGetBoolean(IL_CONV_PAL) == IL_TRUE) {
		NewData = (ILubyte*)ialloc(image->SizeOfData * 4);
		if (NewData == NULL) {
			return IL_FALSE;
		}

		for (i = 0; i < image->SizeOfData; i++) {
			ILubyte r, g, b, a;
			image->Pal.getRGBA(i, r, g, b, a);
			NewData[i * 4] = r;
			NewData[i * 4 + 1] = g;
			NewData[i * 4 + 2] = b;
			NewData[i * 4 + 3] = i != 247 ? 255 : 0;
		}

		if (!ilTexImage(image->Width, image->Height, image->Depth,
			4, IL_RGBA, image->Type, NewData)) {
			ifree(NewData);
			return IL_FALSE;
		}
		image->Origin = IL_ORIGIN_UPPER_LEFT;
		ifree(NewData);
	}

	return il2FixImage(image);
}


#endif
