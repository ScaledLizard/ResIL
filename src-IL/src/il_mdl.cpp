//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_mdl.c
//
// Description: Reads a Half-Life model file (.mdl).
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_MDL
#include "il_mdl.h"
#include "IL/il2.h"
#include "IL/il.h"


// Internal function to get the header and check it.
ILboolean iIsValidMdl(SIO * io)
{
	ILuint Id, Version;

	Id = GetLittleUInt(io);
	Version = GetLittleUInt(io);
	io->seek(io, -8, IL_SEEK_CUR);  // Restore to previous position.

	// 0x54534449 == "IDST"
	if (Id != 0x54534449 || Version != 10)
		return IL_FALSE;
	return IL_TRUE;
}


ILboolean iLoadMdlInternal(ILimage* image)
{
	ILuint		Id, Version, NumTex, TexOff, TexDataOff, ImageNum;
	ILubyte		*TempPal;
	TEX_HEAD	TexHead;
	ILimage		*BaseImage=NULL;
	ILboolean	BaseCreated = IL_FALSE;
	SIO * io = &image->io;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Id = GetLittleUInt(io);
	Version = GetLittleUInt(io);

	// 0x54534449 == "IDST"
	if (Id != 0x54534449 || Version != 10) {
		il2SetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// Skips the actual model header.
	io->seek(io, 172, IL_SEEK_CUR);

	NumTex = GetLittleUInt(io);
	TexOff = GetLittleUInt(io);
	TexDataOff = GetLittleUInt(io);

	if (NumTex == 0 || TexOff == 0 || TexDataOff == 0) {
		il2SetError(IL_ILLEGAL_FILE_VALUE);
		return IL_FALSE;
	}

	io->seek(io, TexOff, IL_SEEK_SET);

	for (ImageNum = 0; ImageNum < NumTex; ImageNum++) {
		if (io->read(io, TexHead.Name, 1, 64) != 64)
			return IL_FALSE;
		TexHead.Flags = GetLittleUInt(io);
		TexHead.Width = GetLittleUInt(io);
		TexHead.Height = GetLittleUInt(io);
		TexHead.Offset = GetLittleUInt(io);
		ILint64 Position = io->tell(io);

		if (TexHead.Offset == 0) {
			il2SetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
		}

		if (!BaseCreated) {
			il2TexImage(image, TexHead.Width, TexHead.Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL);
			image->Origin = IL_ORIGIN_LOWER_LEFT;
			BaseCreated = IL_TRUE;
			BaseImage = image;
		}
		else {
			image = image->Next;
			image->Format = IL_COLOUR_INDEX;
			image->Type = IL_UNSIGNED_BYTE;
		}

		TempPal	= (ILubyte*)ialloc(768);
		if (TempPal == NULL) {
			image = BaseImage;
			return IL_FALSE;
		}
		image->Pal.use(256, NULL, IL_PAL_RGB24);

		io->seek(io, TexHead.Offset, IL_SEEK_SET);
		if (io->read(io, image->Data, TexHead.Width * TexHead.Height, 1) != 1)
			return IL_FALSE;
		if (!image->Pal.readFromFile(io))
			return IL_FALSE;

		if (ilGetBoolean(IL_CONV_PAL) == IL_TRUE) {
			il2ConvertImage(image, IL_RGB, IL_UNSIGNED_BYTE);
		}

		io->seek(io, Position, IL_SEEK_SET);
	}

	return il2FixImage(image);
}

#endif//IL_NO_MDL
