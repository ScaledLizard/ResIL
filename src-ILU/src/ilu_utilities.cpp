//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-ILU/src/ilu_utilities.c
//
// Description: Utility functions
//
//-----------------------------------------------------------------------------


#include "ilu_internal.h"
#include "IL/il2.h"


//! Retrieves information about the current bound image.
/*void ILAPIENTRY iluGetImageInfo(ILinfo *Info)
{
	iluCurImage = ilGetCurImage();
	if (iluCurImage == NULL || Info == NULL) {
		il2SetError(ILU_ILLEGAL_OPERATION);
		return;
	}

	Info->Id			= ilGetCurName();
	Info->Data			= ilGetData();
	Info->Width			= iluCurImage->Width;
	Info->Height		= iluCurImage->Height;
	Info->Depth			= iluCurImage->Depth;
	Info->Bpp			= iluCurImage->Bpp;
	Info->SizeOfData	= iluCurImage->SizeOfData;
	Info->Format		= iluCurImage->Format;
	Info->Type			= iluCurImage->Type;
	Info->Origin		= iluCurImage->Origin;
	Info->Palette		= iluCurImage->Pal.Palette;
	Info->PalType		= iluCurImage->Pal.PalType;
	Info->PalSize		= iluCurImage->Pal.PalSize;
	il2GetImageInteger(iluCurImage, IL_NUM_IMAGES,             
	                        (ILint*)&Info->NumNext);
	il2GetImageInteger(iluCurImage, IL_NUM_MIPMAPS, 
	                        (ILint*)&Info->NumMips);
	il2GetImageInteger(iluCurImage, IL_NUM_LAYERS, 
	                        (ILint*)&Info->NumLayers);
	
	return;
}
*/