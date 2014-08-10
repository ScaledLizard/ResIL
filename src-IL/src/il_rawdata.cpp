//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/rawdata.c
//
// Description: "Raw" file functions
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
//#ifndef IL_NO_DATA
#include "il_manip.h"


ILboolean iLoadDataInternal(ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp);


// Internal function to load a raw data image
ILboolean iLoadDataInternal(ILimage* image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp)
{
	if (image == NULL || ((Bpp != 1) && (Bpp != 3) && (Bpp != 4))) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!il2TexImage(image, Width, Height, Depth, Bpp, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	image->Origin = IL_ORIGIN_UPPER_LEFT;

	// Tries to read the correct amount of data
	if (image->io.read(&image->io, image->Data, Width * Height * Depth * Bpp, 1) != 1)
		return IL_FALSE;

	if (image->Bpp == 1)
		image->Format = IL_LUMINANCE;
	else if (image->Bpp == 3)
		image->Format = IL_RGB;
	else  // 4
		image->Format = IL_RGBA;

	return il2FixImage(image);
}


//! Save the current image to FileName as raw data
ILboolean ILAPIENTRY ilSaveData(ILimage* image, ILconst_string FileName)
{
	ILHANDLE DataFile;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	DataFile = image->io.openReadOnly(FileName);
	if (DataFile == NULL) {
		il2SetError(IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	image->io.write(image->Data, 1, image->SizeOfData, &image->io);
	image->io.close(&image->io);

	return IL_TRUE;
}


//#endif//IL_NO_DATA
