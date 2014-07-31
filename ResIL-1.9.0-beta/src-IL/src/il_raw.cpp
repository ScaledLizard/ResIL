//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_raw.c
//
// Description: "Raw" file functions
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_RAW
#include "IL/il.h"


// Internal function to load a raw image
ILboolean iLoadRawInternal(ILimage* image)
{
	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}


	image->Width = GetLittleUInt(&image->io);

	image->Height = GetLittleUInt(&image->io);

	image->Depth = GetLittleUInt(&image->io);

	image->Bpp = (ILubyte)image->io.getc(&image->io);

	if (image->io.read(&image->io, &image->Bpc, 1, 1) != 1)
		return IL_FALSE;

	if (!il2TexImage(image, image->Width, image->Height, image->Depth, image->Bpp, 0, ilGetTypeBpc(image->Bpc), NULL)) {
		return IL_FALSE;
	}
	image->Origin = IL_ORIGIN_LOWER_LEFT;

	// Tries to read the correct amount of data
	if (image->io.read(&image->io, image->Data, 1, image->SizeOfData) < image->SizeOfData)
		return IL_FALSE;

	if (ilIsEnabled(IL_ORIGIN_SET)) {
		image->Origin = ilGetInteger(IL_ORIGIN_MODE);
	}
	else {
		image->Origin = IL_ORIGIN_UPPER_LEFT;
	}

	if (image->Bpp == 1)
		image->Format = IL_LUMINANCE;
	else if (image->Bpp == 3)
		image->Format = IL_RGB;
	else  // 4
		image->Format = IL_RGBA;

	return il2FixImage(image);
}


// Internal function used to load the raw data.
ILboolean iSaveRawInternal(ILimage* image)
{
	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	SaveLittleUInt(&image->io, image->Width);
	SaveLittleUInt(&image->io, image->Height);
	SaveLittleUInt(&image->io, image->Depth);
	image->io.putc(image->Bpp, &image->io);
	image->io.putc(image->Bpc, &image->io);
	image->io.write(image->Data, 1, image->SizeOfData, &image->io);

	return IL_TRUE;
}


#endif//IL_NO_RAW
