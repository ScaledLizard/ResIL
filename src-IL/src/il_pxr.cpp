//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/26/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_pxr.c
//
// Description: Reads from a Pxrar (.pxr) file.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_PXR
#include "il_manip.h"
#include "il_endian.h"

#ifdef _MSC_VER
#pragma pack(push, pxr_struct, 1)
#endif
typedef struct PIXHEAD
{
	ILushort	Signature;
	ILubyte		Reserved1[413];
	ILushort	Height;
	ILushort	Width;
	ILubyte		Reserved2[4];
	ILubyte		BppInfo;
	ILubyte		Reserved3[598];
} IL_PACKSTRUCT PIXHEAD;
#ifdef _MSC_VER
#pragma pack(pop, pxr_struct)
#endif


// Internal function used to load the Pxr.
ILboolean iLoadPxrInternal(ILimage* image)
{
	ILushort	Width, Height;
	ILubyte		Bpp;
	SIO * io = &image->io;
	Width = sizeof(PIXHEAD);

	io->seek(io, 416, IL_SEEK_SET);
	Height = GetLittleUShort(io);
	Width = GetLittleUShort(io);
	io->seek(io, 424, IL_SEEK_SET);
	Bpp = (ILubyte)io->getc(io);

	switch (Bpp)
	{
		case 0x08:
			il2TexImage(image, Width, Height, 1, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, NULL);
			break;
		case 0x0E:
			il2TexImage(image, Width, Height, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL);
			break;
		case 0x0F:
			il2TexImage(image, Width, Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);
			break;
		default:
			il2SetError(IL_INVALID_FILE_HEADER);
			return IL_FALSE;
	}

	io->seek(io, 1024, IL_SEEK_SET);
	io->read(io, image->Data, 1, image->SizeOfData);
	image->Origin = IL_ORIGIN_UPPER_LEFT;

	return IL_TRUE;
}


#endif//IL_NO_PXR
