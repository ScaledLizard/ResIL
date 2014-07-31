//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/05/2009
//
// Filename: src-IL/src/il_mp3.c
//
// MimeType: Reads from an MPEG-1 Audio Layer 3 (.mp3) file.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#ifndef IL_NO_MP3
#include "il_jpeg.h"


typedef struct MP3HEAD
{
	char	Signature[3];
	ILubyte	VersionMajor;
	ILubyte	VersionMinor;
	ILubyte	Flags;
	ILuint	Length;
} MP3HEAD;

#define MP3_NONE 0
#define MP3_JPG  1
#define MP3_PNG  2

ILboolean iLoadMp3Internal(ILimage* image);
ILboolean iIsValidMp3(void);
ILboolean iCheckMp3(MP3HEAD *Header);
ILboolean iLoadPngInternal(ILimage* image);


ILuint GetSynchInt(SIO* io)
{
	ILuint SynchInt;

	SynchInt = GetBigUInt(io);

	SynchInt = ((SynchInt & 0x7F000000) >> 3) | ((SynchInt & 0x7F0000) >> 2)
				| ((SynchInt & 0x7F00) >> 1) | (SynchInt & 0x7F);

	return SynchInt;
}


// Internal function used to get the MP3 header from the current file.
ILboolean iGetMp3Head(SIO* io, MP3HEAD *Header)
{
	if (io->read(io, Header->Signature, 3, 1) != 1)
		return IL_FALSE;
	Header->VersionMajor = io->getc(io);
	Header->VersionMinor = io->getc(io);
	Header->Flags = io->getc(io);
	Header->Length = GetSynchInt(io);

	return IL_TRUE;
}


// Internal function to get the header and check it.
ILboolean iIsValidMp3(SIO* io)
{
	MP3HEAD		Header;
	auto		Pos = io->tell(io);

	if (!iGetMp3Head(io, &Header))
		return IL_FALSE;
	// The length of the header varies, so we just go back to the original position.
	io->seek(io, Pos, IL_SEEK_CUR);

	return iCheckMp3(&Header);
}


// Internal function used to check if the HEADER is a valid MP3 header.
ILboolean iCheckMp3(MP3HEAD *Header)
{
	if (strncmp(Header->Signature, "ID3", 3))
		return IL_FALSE;
	if (Header->VersionMajor != 3 && Header->VersionMinor != 4)
		return IL_FALSE;

	return IL_TRUE;
}


ILuint iFindMp3Pic(SIO* io, MP3HEAD *Header)
{
	char	ID[4];
	ILuint	FrameSize;
	ILubyte	TextEncoding;
	ILubyte	MimeType[65], Description[65];
	ILubyte	PicType;
	ILuint	i;
	ILuint	Type = MP3_NONE;

	do {
		if (io->read(io, ID, 4, 1) != 1)
			return MP3_NONE;
		if (Header->VersionMajor == 3)
			FrameSize = GetBigUInt(io);
		else
			FrameSize = GetSynchInt(io);

		GetBigUShort(io);  // Skip the flags.

		//@TODO: Support multiple APIC entries in an mp3 file.
		if (!strncmp(ID, "APIC", 4)) {
			//@TODO: Use TextEncoding properly - UTF16 strings starting with FFFE or FEFF.
			TextEncoding = io->getc(io);
			// Get the MimeType (read until we hit 0).
			for (i = 0; i < 65; i++) {
				MimeType[i] = io->getc(io);
				if (MimeType[i] == 0)
					break;
			}
			// The MimeType must be terminated by 0 in the file by the specs.
			if (MimeType[i] != 0)
				return MP3_NONE;
			if (!strcmp((const char*)MimeType, "image/jpeg"))
				Type = MP3_JPG;
			else if (!strcmp((const char*)MimeType, "image/png"))
				Type = MP3_PNG;
			else
				Type = MP3_NONE;

			PicType = io->getc(io);  // Whether this is a cover, band logo, etc.

			// Skip the description.
			for (i = 0; i < 65; i++) {
				Description[i] = io->getc(io);
				if (Description[i] == 0)
					break;
			}
			if (Description[i] != 0)
				return MP3_NONE;
			return Type;
		}
		else {
			io->seek(io, FrameSize, IL_SEEK_CUR);
		}

		//if (!strncmp(MimeType, "
	} while (!io->eof(io) && io->tell(io) < Header->Length);

	return Type;
}


// Internal function used to load the MP3.
ILboolean iLoadMp3Internal(ILimage* image)
{
	MP3HEAD	Header;
	ILuint	Type;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!iGetMp3Head(&image->io, &Header))
		return IL_FALSE;
	if (!iCheckMp3(&Header))
		return IL_FALSE;
	Type = iFindMp3Pic(&image->io, &Header);
	
	switch (Type)
	{
#ifndef IL_NO_JPG
		case MP3_JPG:
			return iLoadJpegInternal(image);
#endif//IL_NO_JPG

#ifndef IL_NO_PNG
		case MP3_PNG:
			return iLoadPngInternal(image);
#endif//IL_NO_PNG

		// Either a picture was not found, or the MIME type was not recognized.
		default:
			il2SetError(IL_INVALID_FILE_HEADER);
	}

	return IL_FALSE;
}

#endif//IL_NO_MP3

