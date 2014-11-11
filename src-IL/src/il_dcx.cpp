//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_dcx.c
//
// Description: Reads from a .dcx file.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_DCX
#include "il_manip.h"


#ifdef _WIN32
#pragma pack(push, packed_struct, 1)
#endif
typedef struct DCXHEAD
{
	ILubyte		Manufacturer;
	ILubyte		Version;
	ILubyte		Encoding;
	ILubyte		Bpp;
	ILushort	Xmin, Ymin, Xmax, Ymax;
	ILushort	HDpi;
	ILushort	VDpi;
	ILubyte		ColMap[48];
	ILubyte		Reserved;
	ILubyte		NumPlanes;
	ILushort	Bps;
	ILushort	PaletteInfo;
	ILushort	HScreenSize;
	ILushort	VScreenSize;
	ILubyte		Filler[54];
} IL_PACKSTRUCT DCXHEAD;
#ifdef _WIN32
#pragma pack(pop, packed_struct)
#endif


// Internal function obtain the .dcx header from the current file.
ILboolean iGetDcxHead(SIO* io, DCXHEAD *Head)
{
	Head->Xmin = GetLittleUShort(io);
	Head->Ymin = GetLittleUShort(io);
	Head->Xmax = GetLittleUShort(io);
	Head->Ymax = GetLittleUShort(io);
	Head->HDpi = GetLittleUShort(io);
	Head->VDpi = GetLittleUShort(io);
	Head->Bps = GetLittleUShort(io);
	Head->PaletteInfo = GetLittleUShort(io);
	Head->HScreenSize = GetLittleUShort(io);
	Head->VScreenSize = GetLittleUShort(io);

	return IL_TRUE;
}


// Internal function to get the header and check it.
ILboolean iIsValidDcx(SIO* io)
{
	ILuint Signature;

	if (io->read(io, &Signature, 1, 4) != 4)
		return IL_FALSE;
	io->seek(io, -4, IL_SEEK_CUR);

	return (Signature == 987654321);
}


// Internal function used to check if the HEADER is a valid .dcx header.
// Should we also do a check on Header->Bpp?
ILboolean iCheckDcx(DCXHEAD *Header)
{
	ILuint	Test, i;

	// There are other versions, but I am not supporting them as of yet.
	//	Got rid of the Reserved check, because I've seen some .dcx files with invalid values in it.
	if (Header->Manufacturer != 10 || Header->Version != 5 || Header->Encoding != 1/* || Header->Reserved != 0*/)
		return IL_FALSE;

	// See if the padding size is correct
	Test = Header->Xmax - Header->Xmin + 1;
	/*if (Header->Bpp >= 8) {
		if (Test & 1) {
			if (Header->Bps != Test + 1)
				return IL_FALSE;
		}
		else {
			if (Header->Bps != Test)  // No padding
				return IL_FALSE;
		}
	}*/

	for (i = 0; i < 54; i++) {
		if (Header->Filler[i] != 0)
			return IL_FALSE;
	}

	return IL_TRUE;
}


ILimage *iUncompressDcxSmall(ILimage* image, DCXHEAD *Header)
{
	ILuint	i = 0, j, k, c, d, x, y, Bps;
	ILubyte	HeadByte, Colour, Data = 0, *ScanLine = NULL;
	ILimage	*Image;
	SIO* io = &image->io;

	Image = il2NewImage(Header->Xmax - Header->Xmin + 1, Header->Ymax - Header->Ymin + 1, 1, Header->NumPlanes, 1);
	if (Image == NULL)
		return NULL;

	/*if (!ilTexImage(Header->Xmax - Header->Xmin + 1, Header->Ymax - Header->Ymin + 1, 1, 1, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}*/
	Image->Origin = IL_ORIGIN_UPPER_LEFT;

	switch (Header->NumPlanes)
	{
		case 1:
			Image->Format = IL_LUMINANCE;
			break;
		case 4:
			Image->Format = IL_COLOUR_INDEX;
			break;
		default:
			il2SetError(IL_ILLEGAL_FILE_VALUE);
			ilCloseImage(Image);
			return NULL;
	}

	if (Header->NumPlanes == 1) {
		for (j = 0; j < Image->Height; j++) {
			i = 0;
			while (i < Image->Width) {
				if (io->read(io, &HeadByte, 1, 1) != 1)
					goto file_read_error;
				if (HeadByte >= 192) {
					HeadByte -= 192;
					if (io->read(io, &Data, 1, 1) != 1)
						goto file_read_error;

					for (c = 0; c < HeadByte; c++) {
						k = 128;
						for (d = 0; d < 8 && i < Image->Width; d++) {
							Image->Data[j * Image->Width + i++] = (!!(Data & k) == 1 ? 255 : 0);
							k >>= 1;
						}
					}
				}
				else {
					k = 128;
					for (c = 0; c < 8 && i < Image->Width; c++) {
						Image->Data[j * Image->Width + i++] = (!!(HeadByte & k) == 1 ? 255 : 0);
						k >>= 1;
					}
				}
			}
			if (Data != 0)
				io->getc(io);  // Skip pad byte if last byte not a 0
		}
	}
	else {   // 4-bit images
		Bps = Header->Bps * Header->NumPlanes * 2;
		if (!Image->Pal.use(16, NULL, IL_PAL_RGB24)) {
			ifree(ScanLine);
			ilCloseImage(Image);
			return NULL;
		}
		ScanLine = (ILubyte*)ialloc(Bps);
		if (ScanLine == NULL) {
			ifree(ScanLine);
			ilCloseImage(Image);
			return NULL;
		}

		memcpy(Image->Pal.getPalette(), Header->ColMap, 16 * 3);
		imemclear(Image->Data, Image->SizeOfData);  // Since we do a += later.

		for (y = 0; y < Image->Height; y++) {
			for (c = 0; c < Header->NumPlanes; c++) {
				x = 0;
				while (x < Bps) {
					if (io->read(io, &HeadByte, 1, 1) != 1)
						goto file_read_error;
					if ((HeadByte & 0xC0) == 0xC0) {
						HeadByte &= 0x3F;
						if (io->read(io, &Colour, 1, 1) != 1)
							goto file_read_error;
						for (i = 0; i < HeadByte; i++) {
							k = 128;
							for (j = 0; j < 8; j++) {
								ScanLine[x++] = !!(Colour & k);
								k >>= 1;
							}
						}
					}
					else {
						k = 128;
						for (j = 0; j < 8; j++) {
							ScanLine[x++] = !!(HeadByte & k);
							k >>= 1;
						}
					}
				}

				for (x = 0; x < Image->Width; x++) {  // 'Cleverly' ignores the pad bytes. ;)
					Image->Data[y * Image->Width + x] += ScanLine[x] << c;
				}
			}
		}
		ifree(ScanLine);
	}

	return Image;

file_read_error:
	ifree(ScanLine);
	ilCloseImage(Image);
	return NULL;
}


// Internal function to uncompress the .dcx (all .dcx files are rle compressed)
ILimage *iUncompressDcx(ILimage* image, DCXHEAD *Header)
{
	ILubyte		ByteHead, Colour, *ScanLine = NULL /* Only one plane */;
	ILuint		c, i, x, y;//, Read = 0;
	ILimage		*Image = NULL;
	SIO* io = &image->io;

	if (Header->Bpp < 8) {
		/*il2SetError(IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;*/
		return iUncompressDcxSmall(image, Header);
	}

	Image = il2NewImage(Header->Xmax - Header->Xmin + 1, Header->Ymax - Header->Ymin + 1, 1, Header->NumPlanes, 1);
	if (Image == NULL)
		return NULL;
	/*if (!ilTexImage(Header->Xmax - Header->Xmin + 1, Header->Ymax - Header->Ymin + 1, 1, Header->NumPlanes, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}*/
	Image->Origin = IL_ORIGIN_UPPER_LEFT;

	ScanLine = (ILubyte*)ialloc(Header->Bps);
	if (ScanLine == NULL)
		goto dcx_error;

	switch (Image->Bpp)
	{
		case 1:
			Image->Format = IL_COLOUR_INDEX;
			if (!Image->Pal.use(256, NULL, IL_PAL_RGB24))
				goto dcx_error;
			break;
		//case 2:  // No 16-bit images in the dcx format!
		case 3:
			Image->Format = IL_RGB;
			Image->Pal.clear();
			break;
		case 4:
			Image->Format = IL_RGBA;
			Image->Pal.clear();
			break;

		default:
			il2SetError(IL_ILLEGAL_FILE_VALUE);
			goto dcx_error;
	}

	//TODO: because the .pcx-code was broken this
	//code is probably broken, too
	for (y = 0; y < Image->Height; y++) {
		for (c = 0; c < Image->Bpp; c++) {
			x = 0;
			while (x < Header->Bps) {
				if (io->read(io, &ByteHead, 1, 1) != 1) {
					goto dcx_error;
				}
				if ((ByteHead & 0xC0) == 0xC0) {
					ByteHead &= 0x3F;
					if (io->read(io, &Colour, 1, 1) != 1) {
						goto dcx_error;
					}
					for (i = 0; i < ByteHead; i++) {
						ScanLine[x++] = Colour;
					}
				}
				else {
					ScanLine[x++] = ByteHead;
				}
			}

			for (x = 0; x < Image->Width; x++) {  // 'Cleverly' ignores the pad bytes ;)
				Image->Data[y * Image->Bps + x * Image->Bpp + c] = ScanLine[x];
			}
		}
	}

	ifree(ScanLine);

	// Read in the palette
	if (Image->Bpp == 1) {
		ByteHead = io->getc(io);	// the value 12, because it signals there's a palette for some reason...
							//	We should do a check to make certain it's 12...
		if (ByteHead != 12)
			io->seek(io, -1, IL_SEEK_CUR);
		if (!Image->Pal.readFromFile(io)) {
			ilCloseImage(Image);
			return NULL;
		}
	}

	return Image;

dcx_error:
	ifree(ScanLine);
	ilCloseImage(Image);
	return NULL;
}


// Internal function used to load the .dcx.
ILboolean iLoadDcxInternal(ILimage* image)
{
	DCXHEAD	Header;
	ILuint	Signature, i, Entries[1024], Num = 0;
	ILimage	*Image, *Base;
	SIO* io = &image->io;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!iIsValidDcx(io))
		return IL_FALSE;
	io->read(io, &Signature, 1, 4);

	do {
		if (io->read(io, &Entries[Num], 1, 4) != 4)
			return IL_FALSE;
		Num++;
	} while (Entries[Num-1] != 0);

	for (i = 0; i < Num; i++) {
		io->seek(io, Entries[i], IL_SEEK_SET);
		iGetDcxHead(io, &Header);
		/*if (!iCheckDcx(&Header)) {
			il2SetError(IL_INVALID_FILE_HEADER);
			return IL_FALSE;
		}*/

		Image = iUncompressDcx(image, &Header);
		if (Image == NULL)
			return IL_FALSE;

		if (i == 0) {
			il2TexImage(Image, Image->Width, Image->Height, 1, Image->Bpp, Image->Format, Image->Type, Image->Data);
			Base = image;
			Base->Origin = IL_ORIGIN_UPPER_LEFT;
			ilCloseImage(Image);
		}
		else {
			image->Next = Image;
			image = image->Next;
		}
	}

	return il2FixImage(image);
}


#endif//IL_NO_DCX
