//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 02/16/2009
//
// Filename: src-IL/src/il_texture.c
//
// Description: Reads from a Medieval II: Total War	(by Creative Assembly)
//				Texture (.texture) file.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#ifndef IL_NO_TEXTURE


//! Reads from a memory "lump" that contains a TEXTURE
ILboolean ilLoadTextureL(ILimage* image, const void *Lump, ILuint Size)
{
	iSetInputLump(&image->io, Lump, Size);
	// From http://forums.totalwar.org/vb/showthread.php?t=70886, all that needs to be done
	//  is to strip out the first 48 bytes, and then it is DDS data.
	image->io.seek(&image->io, 48, IL_SEEK_CUR);
	return iLoadDdsInternal(image);
}

#endif//IL_NO_TEXTURE

