//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 02/12/2009
//
// Filename: src-IL/src/il_ftx.c
//
// Description: Reads from a Heavy Metal: FAKK2 (.ftx) file.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#ifndef IL_NO_FTX
#include "IL/il2.h"


// Internal function used to load the FTX.
ILboolean iLoadFtxInternal(ILimage* image)
{
	ILuint Width, Height, HasAlpha;
	SIO * io = &image->io;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Width = GetLittleUInt(io);
	Height = GetLittleUInt(io);
	HasAlpha = GetLittleUInt(io);  // Kind of backwards from what I would think...

	//@TODO: Right now, it appears that all images are in RGBA format.  See if I can find specs otherwise
	//  or images that load incorrectly like this.
	//if (HasAlpha == 0) {  // RGBA format
		if (!il2TexImage(image, Width, Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL))
			return IL_FALSE;
	//}
	//else if (HasAlpha == 1) {  // RGB format
	//	if (!ilTexImage(Width, Height, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL))
	//		return IL_FALSE;
	//}
	//else {  // Unknown format
	//	il2SetError(IL_INVALID_FILE_HEADER);
	//	return IL_FALSE;
	//}

	// The origin will always be in the upper left.
	image->Origin = IL_ORIGIN_UPPER_LEFT;

	// All we have to do for this format is read the raw, uncompressed data.
	if (io->read(io, image->Data, 1, image->SizeOfData) != image->SizeOfData)
		return IL_FALSE;

	return il2FixImage(image);
}

#endif//IL_NO_FTX

