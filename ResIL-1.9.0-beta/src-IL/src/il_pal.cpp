//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 02/14/2009
//
// Filename: src-IL/src/il_pal.c
//
// Description: Loads palettes from different file formats
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include "il_pal.h"
#include <string.h>
#include <ctype.h>
#include <limits.h>


//! Loads a palette from FileName into the current image's palette.
ILboolean ILAPIENTRY il2LoadPal(ILimage* image, ILconst_string FileName)
{
	FILE		*f;
	ILboolean	IsPsp;
	char		Head[8];

	if (FileName == NULL) {
		il2SetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (iCheckExtension(FileName, IL_TEXT("col"))) {
		return ilLoadColPal(image, FileName);
	}
	if (iCheckExtension(FileName, IL_TEXT("act"))) {
		return ilLoadActPal(image, FileName);
	}
	if (iCheckExtension(FileName, IL_TEXT("plt"))) {
		return ilLoadPltPal(image, FileName);
	}

#ifndef _UNICODE
	f = fopen(FileName, "rt");
#else
	f = _wfopen(FileName, L"rt");
#endif//_UNICODE
	if (f == NULL) {
		il2SetError(IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	fread(Head, 1, 8, f);
	if (strcmp(Head, "JASC-PAL") == 0)
		IsPsp = IL_TRUE;
	else
		IsPsp = IL_FALSE;

	fclose(f);

	if (IsPsp)
		return ilLoadJascPal(image, FileName);
	return ilLoadHaloPal(image, FileName);
}


//! Loads a Paint Shop Pro formatted palette (.pal) file.
ILboolean ilLoadJascPal(ILimage* image, ILconst_string FileName)
{
	FILE *PalFile;
	ILuint NumColours, i, c;
	ILubyte Buff[BUFFLEN];
	ILboolean Error = IL_FALSE;
	ILpal *Pal = &image->Pal;

	if (!iCheckExtension(FileName, IL_TEXT("pal"))) {
		il2SetError(IL_INVALID_EXTENSION);
		return IL_FALSE;
	}

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

#ifndef _UNICODE
	PalFile = fopen(FileName, "rt");
#else
	PalFile = _wfopen(FileName, L"rt");
#endif//_UNICODE
	if (PalFile == NULL) {
		il2SetError(IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	if (image->Pal.Palette && image->Pal.PalSize > 0 && image->Pal.PalType != IL_PAL_NONE) {
		ifree(image->Pal.Palette);
		image->Pal.Palette = NULL;
	}

	iFgetw(Buff, BUFFLEN, PalFile);
	if (strcmp((const char*)Buff, "JASC-PAL") != 0) {
		Error = IL_TRUE;
	}
	iFgetw(Buff, BUFFLEN, PalFile);
	if (strcmp((const char*)Buff, "0100")) {
		Error = IL_TRUE;
	}

	iFgetw(Buff, BUFFLEN, PalFile);
	NumColours = atoi((const char*)Buff);
	if (NumColours == 0 || Error) {
		il2SetError(IL_INVALID_FILE_HEADER);
		fclose(PalFile);
		return IL_FALSE;
	}
	
	Pal->PalSize = NumColours * PALBPP;
	Pal->PalType = IL_PAL_RGB24;
	Pal->Palette = (ILubyte*)ialloc(NumColours * PALBPP);
	if (Pal->Palette == NULL) {
		fclose(PalFile);
		return IL_FALSE;
	}

	for (i = 0; i < NumColours; i++) {
		for (c = 0; c < PALBPP; c++) {
			iFgetw(Buff, BUFFLEN, PalFile);
			Pal->Palette[i * PALBPP + c] = atoi((const char*)Buff);
		}
	}

	fclose(PalFile);

	return IL_TRUE;
}


// File Get Word
//	MaxLen must be greater than 1, because the trailing NULL is always stored.
char *iFgetw(ILubyte *Buff, ILint MaxLen, FILE *File)
{
	ILint Temp;
	ILint i;

	if (Buff == NULL || File == NULL || MaxLen < 2) {
		il2SetError(IL_INVALID_PARAM);
		return NULL;
	}

	for (i = 0; i < MaxLen - 1; i++) {
		Temp = fgetc(File);
		if (Temp == '\n' || Temp == '\0' || Temp == IL_EOF || feof(File)) {
			break;			
		}

		if (Temp == ' ') {
			while (Temp == ' ') {  // Just to get rid of any extra spaces
				Temp = fgetc(File);
			}
			fseek(File, -1, IL_SEEK_CUR);  // Go back one
			break;
		}

		if (!isprint(Temp)) {  // Skips any non-printing characters
			while (!isprint(Temp)) {
				Temp = fgetc(File);
			}
			fseek(File, -1, IL_SEEK_CUR);
			break;
		}

		Buff[i] = Temp;
	}

	Buff[i] = '\0';
	return (char *)Buff;
}


ILboolean ILAPIENTRY ilSavePal(ILimage* image, ILconst_string FileName)
{
	ILstring Ext = iGetExtension(FileName);

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

#ifndef _UNICODE
	if (FileName == NULL || strlen(FileName) < 1 || Ext == NULL) {
#else
	if (FileName == NULL || wcslen(FileName) < 1 || Ext == NULL) {
#endif//_UNICODE
		il2SetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (!image->Pal.Palette || !image->Pal.PalSize || image->Pal.PalType == IL_PAL_NONE) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!iStrCmp(Ext, IL_TEXT("pal"))) {
		return ilSaveJascPal(image, FileName);
	}

	il2SetError(IL_INVALID_EXTENSION);
	return IL_FALSE;
}


//! Saves a Paint Shop Pro formatted palette (.pal) file.
ILboolean ilSaveJascPal(ILimage* image, ILconst_string FileName)
{
	FILE	*PalFile;
	ILuint	i, PalBpp, NumCols = il2GetInteger(IL_PALETTE_NUM_COLS);
	ILubyte	*CurPal;

	if (image == NULL || NumCols == 0 || NumCols > 256) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	
#ifndef _UNICODE
	if (FileName == NULL || strlen(FileName) < 5) {
#else
	if (FileName == NULL || wcslen(FileName) < 5) {
#endif//_UNICODE
		il2SetError(IL_INVALID_VALUE);
		return IL_FALSE;
	}

	if (!iCheckExtension(FileName, IL_TEXT("pal"))) {
		il2SetError(IL_INVALID_EXTENSION);
		return IL_FALSE;
	}

	if (il2GetBoolean(IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			il2SetError(IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	// Create a copy of the current palette and convert it to RGB24 format.
	CurPal = image->Pal.Palette;
	image->Pal.Palette = (ILubyte*)ialloc(image->Pal.PalSize);
	if (!image->Pal.Palette) {
		image->Pal.Palette = CurPal;
		return IL_FALSE;
	}

	memcpy(image->Pal.Palette, CurPal, image->Pal.PalSize);
	if (!il2ConvertPal(image, IL_PAL_RGB24)) {
		ifree(image->Pal.Palette);
		image->Pal.Palette = CurPal;
		return IL_FALSE;
	}

#ifndef _UNICODE
	PalFile = fopen(FileName, "wt");
#else
	PalFile = _wfopen(FileName, L"wt");
#endif//_UNICODE
	if (!PalFile) {
		il2SetError(IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	// Header needed on all .pal files
	fputs("JASC-PAL\n0100\n256\n", PalFile);

	PalBpp = ilGetBppPal(image->Pal.PalType);
	for (i = 0; i < image->Pal.PalSize; i += PalBpp) {
		fprintf(PalFile, "%d %d %d\n",
			image->Pal.Palette[i], image->Pal.Palette[i+1], image->Pal.Palette[i+2]);
	}

	NumCols = 256 - NumCols;
	for (i = 0; i < NumCols; i++) {
		fprintf(PalFile, "0 0 0\n");
	}

	ifree(image->Pal.Palette);
	image->Pal.Palette = CurPal;

	fclose(PalFile);

	return IL_TRUE;
}


//! Loads a Halo formatted palette (.pal) file.
ILboolean ilLoadHaloPal(ILimage* image, ILconst_string FileName)
{
	HALOHEAD	HaloHead;
	ILushort	*TempPal;
	ILuint		i, Size;
	SIO * io = &image->io;

	if (!iCheckExtension(FileName, IL_TEXT("pal"))) {
		il2SetError(IL_INVALID_EXTENSION);
		return IL_FALSE;
	}

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	io->handle = io->openReadOnly(FileName);
	if (io->handle == NULL) {
		il2SetError(IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	if (io->read(io, &HaloHead, sizeof(HALOHEAD), 1) != 1)
		return IL_FALSE;

	if (HaloHead.Id != 'A' + ('H' << 8) || HaloHead.Version != 0xe3) {
		io->close(io);
		il2SetError(IL_ILLEGAL_FILE_VALUE);
		return IL_FALSE;
	}

	Size = (HaloHead.MaxIndex + 1) * 3;
	TempPal = (ILushort*)ialloc(Size * sizeof(ILushort));
	if (TempPal == NULL) {
		io->close(io);
		return IL_FALSE;
	}

	if (io->read(io, TempPal, sizeof(ILushort), Size) != Size) {
		io->close(io);
		ifree(TempPal);
		return IL_FALSE;
	}

	if (image->Pal.Palette && image->Pal.PalSize > 0 && image->Pal.PalType != IL_PAL_NONE) {
		ifree(image->Pal.Palette);
		image->Pal.Palette = NULL;
	}
	image->Pal.PalType = IL_PAL_RGB24;
	image->Pal.PalSize = Size;
	image->Pal.Palette = (ILubyte*)ialloc(image->Pal.PalSize);
	if (image->Pal.Palette == NULL) {
		io->close(io);
		return IL_FALSE;
	}

	for (i = 0; i < image->Pal.PalSize; i++, TempPal++) {
		image->Pal.Palette[i] = (ILubyte)*TempPal;
	}
	TempPal -= image->Pal.PalSize;
	ifree(TempPal);

	io->close(io);

	return IL_TRUE;
}


// Hasn't been tested
//	@TODO: Test the thing!

//! Loads a .col palette file
ILboolean ilLoadColPal(ILimage* image, ILconst_string FileName)
{
	ILuint		RealFileSize, FileSize;
	ILushort	Version;
	ILHANDLE	ColFile;

	if (!iCheckExtension(FileName, IL_TEXT("col"))) {
		il2SetError(IL_INVALID_EXTENSION);
		return IL_FALSE;
	}

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	ColFile = image->io.openReadOnly(FileName);
	if (ColFile == NULL) {
		il2SetError(IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	if (image->Pal.Palette && image->Pal.PalSize > 0 && image->Pal.PalType != IL_PAL_NONE) {
		ifree(image->Pal.Palette);
		image->Pal.Palette = NULL;
	}

	image->io.seek(&image->io, 0, IL_SEEK_END);
	RealFileSize = ftell((FILE*)ColFile);
	image->io.seek(&image->io, 0, IL_SEEK_SET);

	if (RealFileSize > 768) {  // has a header
		fread(&FileSize, 4, 1, (FILE*)ColFile);
		if ((FileSize - 8) % 3 != 0) {  // check to make sure an even multiple of 3!
			image->io.close(&image->io);
			il2SetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
		}
		if (image->io.read(&image->io, &Version, 2, 1) != 1) {
			image->io.close(&image->io);
			return IL_FALSE;
		}
		if (Version != 0xB123) {
			image->io.close(&image->io);
			il2SetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
		}
		if (image->io.read(&image->io, &Version, 2, 1) != 1) {
			image->io.close(&image->io);
			return IL_FALSE;
		}
		if (Version != 0) {
			image->io.close(&image->io);
			il2SetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
		}
	}

	image->Pal.Palette = (ILubyte*)ialloc(768);
	if (image->Pal.Palette == NULL) {
		image->io.close(&image->io);
		return IL_FALSE;
	}

	if (image->io.read(&image->io, image->Pal.Palette, 1, 768) != 768) {
		image->io.close(&image->io);
		ifree(image->Pal.Palette);
		image->Pal.Palette = NULL;
		return IL_FALSE;
	}

	image->Pal.PalSize = 768;
	image->Pal.PalType = IL_PAL_RGB24;

	image->io.close(&image->io);

	return IL_TRUE;
}


//! Loads an .act palette file.
ILboolean ilLoadActPal(ILimage* image, ILconst_string FileName)
{
	ILHANDLE	ActFile;

	if (!iCheckExtension(FileName, IL_TEXT("act"))) {
		il2SetError(IL_INVALID_EXTENSION);
		return IL_FALSE;
	}

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	ActFile = image->io.openReadOnly(FileName);
	if (ActFile == NULL) {
		il2SetError(IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	if (image->Pal.Palette && image->Pal.PalSize > 0 && image->Pal.PalType != IL_PAL_NONE) {
		ifree(image->Pal.Palette);
		image->Pal.Palette = NULL;
	}

	image->Pal.PalType = IL_PAL_RGB24;
	image->Pal.PalSize = 768;
	image->Pal.Palette = (ILubyte*)ialloc(768);
	if (!image->Pal.Palette) {
		image->io.close(&image->io);
		return IL_FALSE;
	}

	if (image->io.read(&image->io, image->Pal.Palette, 1, 768) != 768) {
		image->io.close(&image->io);
		return IL_FALSE;
	}

	image->io.close(&image->io);

	return IL_TRUE;
}


//! Loads an .plt palette file.
ILboolean ilLoadPltPal(ILimage* image, ILconst_string FileName)
{
	ILHANDLE	PltFile;

	if (!iCheckExtension(FileName, IL_TEXT("plt"))) {
		il2SetError(IL_INVALID_EXTENSION);
		return IL_FALSE;
	}

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	PltFile = image->io.openReadOnly(FileName);
	if (PltFile == NULL) {
		il2SetError(IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	if (image->Pal.Palette && image->Pal.PalSize > 0 && image->Pal.PalType != IL_PAL_NONE) {
		ifree(image->Pal.Palette);
		image->Pal.Palette = NULL;
	}

	image->Pal.PalSize = GetLittleUInt(&image->io);
	if (image->Pal.PalSize == 0) {
		il2SetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}
	image->Pal.PalType = IL_PAL_RGB24;
	image->Pal.Palette = (ILubyte*)ialloc(image->Pal.PalSize);
	if (!image->Pal.Palette) {
		image->io.close(&image->io);
		return IL_FALSE;
	}

	if (image->io.read(&image->io, image->Pal.Palette, image->Pal.PalSize, 1) != 1) {
		ifree(image->Pal.Palette);
		image->Pal.Palette = NULL;
		image->io.close(&image->io);
		return IL_FALSE;
	}

	image->io.close(&image->io);

	return IL_TRUE;
}


// Assumes that Dest has nothing in it.
ILboolean iCopyPalette(ILpal *Dest, ILpal *Src)
{
	if (Src->Palette == NULL || Src->PalSize == 0)
		return IL_FALSE;

	Dest->Palette = (ILubyte*)ialloc(Src->PalSize);
	if (Dest->Palette == NULL)
		return IL_FALSE;

	memcpy(Dest->Palette, Src->Palette, Src->PalSize);

	Dest->PalSize = Src->PalSize;
	Dest->PalType = Src->PalType;

	return IL_TRUE;
}


ILAPI ILpal* ILAPIENTRY iCopyPal(ILimage* image)
{
	ILpal *Pal;

	if (image == NULL || image->Pal.Palette == NULL ||
		image->Pal.PalSize == 0 || image->Pal.PalType == IL_PAL_NONE) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return NULL;
	}

	Pal = (ILpal*)ialloc(sizeof(ILpal));
	if (Pal == NULL) {
		return NULL;
	}
	if (!iCopyPalette(Pal, &image->Pal)) {
		ifree(Pal);
		return NULL;
	}

	return Pal;
}


// Converts the palette to the DestFormat format.
ILAPI ILpal* ILAPIENTRY iConvertPal(ILpal *Pal, ILenum DestFormat)
{
	ILpal	*NewPal = NULL;
	ILuint	i, j, NewPalSize;

	// Checks to see if the current image is valid and has a palette
	if (Pal == NULL || Pal->PalSize == 0 || Pal->Palette == NULL || Pal->PalType == IL_PAL_NONE) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return NULL;
	}

	/*if (Pal->PalType == DestFormat) {
		return NULL;
	}*/

	NewPal = (ILpal*)ialloc(sizeof(ILpal));
	if (NewPal == NULL) {
		return NULL;
	}
	NewPal->PalSize = Pal->PalSize;
	NewPal->PalType = Pal->PalType;

	switch (DestFormat)
	{
		case IL_PAL_RGB24:
		case IL_PAL_BGR24:
			switch (Pal->PalType)
			{
				case IL_PAL_RGB24:
					NewPal->Palette = (ILubyte*)ialloc(Pal->PalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if (DestFormat == IL_PAL_BGR24) {
						j = ilGetBppPal(Pal->PalType);
						for (i = 0; i < Pal->PalSize; i += j) {
							NewPal->Palette[i] = Pal->Palette[i+2];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i];
						}
					}
					else {
						memcpy(NewPal->Palette, Pal->Palette, Pal->PalSize);
					}
					NewPal->PalType = DestFormat;
					break;

				case IL_PAL_BGR24:
					NewPal->Palette = (ILubyte*)ialloc(Pal->PalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if (DestFormat == IL_PAL_RGB24) {
						j = ilGetBppPal(Pal->PalType);
						for (i = 0; i < Pal->PalSize; i += j) {
							NewPal->Palette[i] = Pal->Palette[i+2];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i];
						}
					}
					else {
						memcpy(NewPal->Palette, Pal->Palette, Pal->PalSize);
					}
					NewPal->PalType = DestFormat;
					break;

				case IL_PAL_BGR32:
				case IL_PAL_BGRA32:
					NewPalSize = (ILuint)((ILfloat)Pal->PalSize * 3.0f / 4.0f);
					NewPal->Palette = (ILubyte*)ialloc(NewPalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if (DestFormat == IL_PAL_RGB24) {
						for (i = 0, j = 0; i < Pal->PalSize; i += 4, j += 3) {
							NewPal->Palette[j]   = Pal->Palette[i+2];
							NewPal->Palette[j+1] = Pal->Palette[i+1];
							NewPal->Palette[j+2] = Pal->Palette[i];
						}
					}
					else {
						for (i = 0, j = 0; i < Pal->PalSize; i += 4, j += 3) {
							NewPal->Palette[j]   = Pal->Palette[i];
							NewPal->Palette[j+1] = Pal->Palette[i+1];
							NewPal->Palette[j+2] = Pal->Palette[i+2];
						}
					}
					NewPal->PalSize = NewPalSize;
					NewPal->PalType = DestFormat;
					break;

				case IL_PAL_RGB32:
				case IL_PAL_RGBA32:
					NewPalSize = (ILuint)((ILfloat)Pal->PalSize * 3.0f / 4.0f);
					NewPal->Palette = (ILubyte*)ialloc(NewPalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if (DestFormat == IL_PAL_RGB24) {
						for (i = 0, j = 0; i < Pal->PalSize; i += 4, j += 3) {
							NewPal->Palette[j]   = Pal->Palette[i];
							NewPal->Palette[j+1] = Pal->Palette[i+1];
							NewPal->Palette[j+2] = Pal->Palette[i+2];
						}
					}
					else {
						for (i = 0, j = 0; i < Pal->PalSize; i += 4, j += 3) {
							NewPal->Palette[j]   = Pal->Palette[i+2];
							NewPal->Palette[j+1] = Pal->Palette[i+1];
							NewPal->Palette[j+2] = Pal->Palette[i];
						}
					}
					NewPal->PalSize = NewPalSize;
					NewPal->PalType = DestFormat;
					break;

				default:
					il2SetError(IL_INVALID_PARAM);
					return NULL;
			}
			break;

		case IL_PAL_RGB32:
		case IL_PAL_RGBA32:
		case IL_PAL_BGR32:
		case IL_PAL_BGRA32:
			switch (Pal->PalType)
			{
				case IL_PAL_RGB24:
				case IL_PAL_BGR24:
					NewPalSize = Pal->PalSize * 4 / 3;
					NewPal->Palette = (ILubyte*)ialloc(NewPalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if ((Pal->PalType == IL_PAL_BGR24 && (DestFormat == IL_PAL_RGB32 || DestFormat == IL_PAL_RGBA32)) ||
						(Pal->PalType == IL_PAL_RGB24 && (DestFormat == IL_PAL_BGR32 || DestFormat == IL_PAL_BGRA32))) {
							for (i = 0, j = 0; i < Pal->PalSize; i += 3, j += 4) {
								NewPal->Palette[j]   = Pal->Palette[i+2];
								NewPal->Palette[j+1] = Pal->Palette[i+1];
								NewPal->Palette[j+2] = Pal->Palette[i];
								NewPal->Palette[j+3] = 255;
							}
					}
					else {
						for (i = 0, j = 0; i < Pal->PalSize; i += 3, j += 4) {
							NewPal->Palette[j]   = Pal->Palette[i];
							NewPal->Palette[j+1] = Pal->Palette[i+1];
							NewPal->Palette[j+2] = Pal->Palette[i+2];
							NewPal->Palette[j+3] = 255;
						}
					}
					NewPal->PalSize = NewPalSize;
					NewPal->PalType = DestFormat;
					break;

				case IL_PAL_RGB32:
					NewPal->Palette = (ILubyte*)ialloc(Pal->PalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;

					if (DestFormat == IL_PAL_BGR32 || DestFormat == IL_PAL_BGRA32) {
						for (i = 0; i < Pal->PalSize; i += 4) {
							NewPal->Palette[i]   = Pal->Palette[i+2];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i];
							NewPal->Palette[i+3] = 255;
						}
					}
					else {
						for (i = 0; i < Pal->PalSize; i += 4) {
							NewPal->Palette[i]   = Pal->Palette[i];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i+2];
							NewPal->Palette[i+3] = 255;
						}
					}
					NewPal->PalType = DestFormat;
					break;

				case IL_PAL_RGBA32:
					NewPal->Palette = (ILubyte*)ialloc(Pal->PalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if (DestFormat == IL_PAL_BGR32 || DestFormat == IL_PAL_BGRA32) {
						for (i = 0; i < Pal->PalSize; i += 4) {
							NewPal->Palette[i]   = Pal->Palette[i+2];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i];
							NewPal->Palette[i+3] = Pal->Palette[i+3];
						}
					}
					else {
						memcpy(NewPal->Palette, Pal->Palette, Pal->PalSize);
					}
					NewPal->PalType = DestFormat;
					break;
				
				case IL_PAL_BGR32:
					NewPal->Palette = (ILubyte*)ialloc(Pal->PalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if (DestFormat == IL_PAL_RGB32 || DestFormat == IL_PAL_RGBA32) {
						for (i = 0; i < Pal->PalSize; i += 4) {
							NewPal->Palette[i]   = Pal->Palette[i+2];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i];
							NewPal->Palette[i+3] = 255;
						}
					}
					else {
						for (i = 0; i < Pal->PalSize; i += 4) {
							NewPal->Palette[i]   = Pal->Palette[i];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i+2];
							NewPal->Palette[i+3] = 255;
						}
					}
					NewPal->PalType = DestFormat;
					break;

				case IL_PAL_BGRA32:
					NewPal->Palette = (ILubyte*)ialloc(Pal->PalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if (DestFormat == IL_PAL_RGB32 || DestFormat == IL_PAL_RGBA32) {
						for (i = 0; i < Pal->PalSize; i += 4) {
							NewPal->Palette[i]   = Pal->Palette[i+2];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i];
							NewPal->Palette[i+3] = Pal->Palette[i+3];
						}
					}
					else {
						memcpy(NewPal->Palette, Pal->Palette, Pal->PalSize);
					}
					NewPal->PalType = DestFormat;
					break;

				default:
					il2SetError(IL_INVALID_PARAM);
					return NULL;
			}
			break;


		default:
			il2SetError(IL_INVALID_PARAM);
			return NULL;
	}

	NewPal->PalType = DestFormat;

	return NewPal;

alloc_error:
	ifree(NewPal);
	return NULL;
}


//! Converts the current image to the DestFormat format.
ILboolean ILAPIENTRY il2ConvertPal(ILimage* image, ILenum DestFormat)
{
	ILpal *Pal;

	if (image == NULL || image->Pal.Palette == NULL ||
		image->Pal.PalSize == 0 || image->Pal.PalType == IL_PAL_NONE) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Pal = iConvertPal(&image->Pal, DestFormat);
	if (Pal == NULL)
		return IL_FALSE;

	ifree(image->Pal.Palette);
	image->Pal.PalSize = Pal->PalSize;
	image->Pal.PalType = Pal->PalType;

	image->Pal.Palette = (ILubyte*)ialloc(Pal->PalSize);
	if (image->Pal.Palette == NULL) {
		return IL_FALSE;
	}
	memcpy(image->Pal.Palette, Pal->Palette, Pal->PalSize);

	ifree(Pal->Palette);
	ifree(Pal);
	
	return IL_TRUE;
}


// Sets the current palette for an image
ILAPI void ILAPIENTRY il2SetPal(ILimage* image, ILpal *Pal)
{
	if (image->Pal.Palette && image->Pal.PalSize && image->Pal.PalType != IL_PAL_NONE) {
		ifree(image->Pal.Palette);
	}

	if (Pal->Palette && Pal->PalSize && Pal->PalType != IL_PAL_NONE) {
		image->Pal.Palette = (ILubyte*)ialloc(Pal->PalSize);
		if (image->Pal.Palette == NULL)
			return;
		memcpy(image->Pal.Palette, Pal->Palette, Pal->PalSize);
		image->Pal.PalSize = Pal->PalSize;
		image->Pal.PalType = Pal->PalType;
	}
	else {
		image->Pal.Palette = NULL;
		image->Pal.PalSize = 0;
		image->Pal.PalType = IL_PAL_NONE;
	}

	return;
}

// Global variable
ILuint CurSort = 0;

typedef struct COL_CUBE
{
	ILubyte	Min[3];
	ILubyte	Val[3];
	ILubyte	Max[3];
} COL_CUBE;

int sort_func(void *e1, void *e2)
{
	return ((COL_CUBE*)e1)->Val[CurSort] - ((COL_CUBE*)e2)->Val[CurSort];
}


ILboolean ILAPIENTRY ilApplyPal(ILimage* image, ILconst_string FileName)
{
	ILimage		Image, *CurImage = image;
	ILubyte		*NewData;
	ILuint		*PalInfo, NumColours, NumPix, MaxDist, DistEntry=0, i, j;
	ILint		Dist1, Dist2, Dist3;
	ILboolean	Same;
	ILenum		Origin;
//	COL_CUBE	*Cubes;

    if( image == NULL || (image->Format != IL_BYTE || image->Format != IL_UNSIGNED_BYTE) ) {
    	il2SetError(IL_ILLEGAL_OPERATION);
        return IL_FALSE;
    }

	NewData = (ILubyte*)ialloc(image->Width * image->Height * image->Depth);
	if (NewData == NULL) {
		return IL_FALSE;
	}

	image = &Image;
	imemclear(&Image, sizeof(ILimage));
	// IL_PAL_RGB24, because we don't want to make parts transparent that shouldn't be.
	if (!il2LoadPal(image, FileName) || !il2ConvertPal(image, IL_PAL_RGB24)) {
		ifree(NewData);
		image = CurImage;
		return IL_FALSE;
	}

	NumColours = Image.Pal.PalSize / 3;  // RGB24 is 3 bytes per entry.
	PalInfo = (ILuint*)ialloc(NumColours * sizeof(ILuint));
	if (PalInfo == NULL) {
		ifree(NewData);
		image = CurImage;
		return IL_FALSE;
	}

	NumPix = CurImage->SizeOfData / ilGetBppFormat(CurImage->Format);
	switch (CurImage->Format)
	{
		case IL_COLOUR_INDEX:
			image = CurImage;
			if (!il2ConvertPal(image, IL_PAL_RGB24)) {
				ifree(NewData);
				ifree(PalInfo);
				return IL_FALSE;
			}

			NumPix = image->Pal.PalSize / ilGetBppPal(image->Pal.PalType);
			for (i = 0; i < NumPix; i++) {
				for (j = 0; j < Image.Pal.PalSize; j += 3) {
					// No need to perform a sqrt.
					Dist1 = (ILint)image->Pal.Palette[i] - (ILint)Image.Pal.Palette[j];
					Dist2 = (ILint)image->Pal.Palette[i+1] - (ILint)Image.Pal.Palette[j+1];
					Dist3 = (ILint)image->Pal.Palette[i+2] - (ILint)Image.Pal.Palette[j+2];
					PalInfo[j / 3] = Dist1 * Dist1 + Dist2 * Dist2 + Dist3 * Dist3;
				}
				MaxDist = UINT_MAX;
				DistEntry = 0;
				for (j = 0; j < NumColours; j++) {
					if (PalInfo[j] < MaxDist) {
						DistEntry = j;
						MaxDist = PalInfo[j];
					}
				}
				image->Pal.Palette[i] = DistEntry;
			}

			for (i = 0; i < image->SizeOfData; i++) {
				NewData[i] = image->Pal.Palette[image->Data[i]];
			}
			break;
		case IL_RGB:
		case IL_RGBA:
			/*Cube = (COL_CUBE*)ialloc(NumColours * sizeof(COL_CUBE));
			// @TODO:  Check if ialloc failed here!
			for (i = 0; i < NumColours; i++)
				memcpy(&Cubes[i].Val, Image.Pal.Palette[i * 3], 3);
			for (j = 0; j < 3; j++) {
				qsort(Cubes, NumColours, sizeof(COL_CUBE), sort_func);
				Cubes[0].Min = 0;
				Cubes[NumColours-1] = UCHAR_MAX;
				NumColours--;
				for (i = 1; i < NumColours; i++) {
					Cubes[i].Min[CurSort] = Cubes[i-1].Val[CurSort] + 1;
					Cubes[i-1].Max[CurSort] = Cubes[i].Val[CurSort] - 1;
				}
				CurSort++;
				NumColours++;
			}*/
			for (i = 0; i < CurImage->SizeOfData; i += CurImage->Bpp) {
				Same = IL_TRUE;
				if (i != 0) {
					for (j = 0; j < CurImage->Bpp; j++) {
						if (CurImage->Data[i-CurImage->Bpp+j] != CurImage->Data[i+j]) {
							Same = IL_FALSE;
							break;
						}
					}
				}
				if (Same) {
					NewData[i / CurImage->Bpp] = DistEntry;
					continue;
				}
				for (j = 0; j < Image.Pal.PalSize; j += 3) {
					// No need to perform a sqrt.
					Dist1 = (ILint)CurImage->Data[i] - (ILint)Image.Pal.Palette[j];
					Dist2 = (ILint)CurImage->Data[i+1] - (ILint)Image.Pal.Palette[j+1];
					Dist3 = (ILint)CurImage->Data[i+2] - (ILint)Image.Pal.Palette[j+2];
					PalInfo[j / 3] = Dist1 * Dist1 + Dist2 * Dist2 + Dist3 * Dist3;
				}
				MaxDist = UINT_MAX;
				DistEntry = 0;
				for (j = 0; j < NumColours; j++) {
					if (PalInfo[j] < MaxDist) {
						DistEntry = j;
						MaxDist = PalInfo[j];
					}
				}
				NewData[i / CurImage->Bpp] = DistEntry;
			}

			break;

		case IL_BGR:
		case IL_BGRA:
			for (i = 0; i < CurImage->SizeOfData; i += CurImage->Bpp) {
				for (j = 0; j < NumColours; j++) {
					// No need to perform a sqrt.
					PalInfo[j] = ((ILint)CurImage->Data[i+2] - (ILint)Image.Pal.Palette[j * 3]) *
						((ILint)CurImage->Data[i+2] - (ILint)Image.Pal.Palette[j * 3]) + 
						((ILint)CurImage->Data[i+1] - (ILint)Image.Pal.Palette[j * 3 + 1]) *
						((ILint)CurImage->Data[i+1] - (ILint)Image.Pal.Palette[j * 3 + 1]) +
						((ILint)CurImage->Data[i] - (ILint)Image.Pal.Palette[j * 3 + 2]) *
						((ILint)CurImage->Data[i] - (ILint)Image.Pal.Palette[j * 3 + 2]);
				}
				MaxDist = UINT_MAX;
				DistEntry = 0;
				for (j = 0; j < NumColours; j++) {
					if (PalInfo[j] < MaxDist) {
						DistEntry = j;
						MaxDist = PalInfo[j];
					}
				}
				NewData[i / CurImage->Bpp] = DistEntry;
			}

			break;

		case IL_LUMINANCE:
		case IL_LUMINANCE_ALPHA:
			for (i = 0; i < CurImage->SizeOfData; i += CurImage->Bpp ) {
				for (j = 0; j < NumColours; j++) {
					// No need to perform a sqrt.
					PalInfo[j] = ((ILuint)CurImage->Data[i] - (ILuint)Image.Pal.Palette[j * 3]) *
						((ILuint)CurImage->Data[i] - (ILuint)Image.Pal.Palette[j * 3]) + 
						((ILuint)CurImage->Data[i] - (ILuint)Image.Pal.Palette[j * 3 + 1]) *
						((ILuint)CurImage->Data[i] - (ILuint)Image.Pal.Palette[j * 3 + 1]) +
						((ILuint)CurImage->Data[i] - (ILuint)Image.Pal.Palette[j * 3 + 2]) *
						((ILuint)CurImage->Data[i] - (ILuint)Image.Pal.Palette[j * 3 + 2]);
				}
				MaxDist = UINT_MAX;
				DistEntry = 0;
				for (j = 0; j < NumColours; j++) {
					if (PalInfo[j] < MaxDist) {
						DistEntry = j;
						MaxDist = PalInfo[j];
					}
				}
				NewData[i] = DistEntry;
			}

			break;

		default:  // Should be no other!
			il2SetError(IL_INTERNAL_ERROR);
			return IL_FALSE;
	}

	image = CurImage;
	Origin = image->Origin;
	if (!il2TexImage(image, image->Width, image->Height, image->Depth, 1,
		IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NewData)) {
		ifree(Image.Pal.Palette);
		ifree(PalInfo);
		ifree(NewData);
		return IL_FALSE;
	}
	image->Origin = Origin;

	image->Pal.Palette = Image.Pal.Palette;
	image->Pal.PalSize = Image.Pal.PalSize;
	image->Pal.PalType = Image.Pal.PalType;
	ifree(PalInfo);
	ifree(NewData);

	return IL_TRUE;
}
