//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 01/30/2009
//
// Filename: src-IL/src/il_size.c
//
// Description: Determines the size of output files for lump writing.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#include "IL/il2.h"


//! Fake seek function
ILint64 ILAPIENTRY iSizeSeek(SIO* io, ILint64 Offset, ILuint Mode)
{
	switch (Mode)
	{
		case IL_SEEK_SET:
			io->rwPos = Offset;
			if (io->rwPos > io->MaxPos)
				io->MaxPos = io->rwPos;
			break;

		case IL_SEEK_CUR:
			io->rwPos = io->rwPos + Offset;
			break;

		case IL_SEEK_END:
			io->rwPos = io->MaxPos + Offset;  // Offset should be negative in this case.
			break;

		default:
			il2SetError(IL_INTERNAL_ERROR);  // Should never happen!
			return -1;  // Error code
	}

	if (io->rwPos > io->MaxPos)
		io->MaxPos = io->rwPos;

	return 0;  // Code for success
}

ILint64 ILAPIENTRY iSizeTell(SIO* io)
{
	return io->rwPos;
}

ILint ILAPIENTRY iSizePutc(ILubyte Char, SIO* io)
{
	io->rwPos++;
	if (io->rwPos > io->MaxPos)
		io->MaxPos = io->rwPos;
	return Char;
}

ILint64 ILAPIENTRY iSizeWrite(const void *Buffer, ILuint Size, ILuint Number, SIO* io)
{
	io->rwPos += Size * Number;
	if (io->rwPos > io->MaxPos)
		io->MaxPos = io->rwPos;
	return Number;
}


// While it might be tempting to optimize this for uncompressed files, there are two reasons
// while this may not be a good idea:
// 1. With the current solution, changes in the file handler will be reflected immediately without additional 
//    effort in the size computation
// 2. The uncompressed file handlers are usually not a performance concern here, considering that no data
//    is actually written to a file

// Returns the size of the memory buffer needed to save the current image into this Type.
//  A return value of 0 is an error.
ILAPI ILint64	ILAPIENTRY il2DetermineSize(ILimage* imageExt, ILenum type)
{
	ILimage* image = (ILimage*) imageExt;
	image->io.MaxPos = 0;
	image->io.rwPos = 0;
	iSetOutputFake(&image->io);  // Sets iputc, iwrite, etc. to functions above.
	il2SaveFuncs(image, type);
	return image->io.MaxPos;
}

