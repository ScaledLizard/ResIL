//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 08/14/2004
//
// Filename: src-IL/src/il_bits.c
//
// Description: Implements a file class that reads/writes bits directly.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include "il_bits.h"


// Converts a FILE to a BITFILE.
BITFILE *bfile(SIO * io)
{
	BITFILE *ToReturn = NULL;

	if (io != NULL) {
		ToReturn = (BITFILE*)ialloc(sizeof(BITFILE));
		if (ToReturn != NULL) {
			ToReturn->BitPos = io->tell(io) << 3;
			ToReturn->ByteBitOff = 8;
			ToReturn->Buff = 0;
			ToReturn->io = io;
		}
	}

	return ToReturn;
}


// Closes an open BITFILE and frees memory for it.
ILint bclose(BITFILE *BitFile)
{
	if (BitFile == NULL)
		return IL_EOF;

	// Removed 01-26-2008.  The file will get closed later by
	//  the calling function.
	//icloser(BitFile->File);
	ifree(BitFile);

	return 0;
}


// Returns the current bit position of a BITFILE.
ILint64 btell(BITFILE *BitFile)
{
	return BitFile->BitPos;
}


// Seeks in a BITFILE just like fseek for FILE.
ILint bseek(BITFILE *BitFile, ILuint Offset, ILuint Mode)
{
	ILint64 KeepPos, Len;

	if (BitFile == NULL)
		return 1;

	SIO * io = BitFile->io;
	switch (Mode)
	{
		case IL_SEEK_SET:
			if (!io->seek(io, Offset >> 3, Mode)) {
				BitFile->BitPos = Offset;
				BitFile->ByteBitOff = BitFile->BitPos % 8;
			}
			break;
		case IL_SEEK_CUR:
			if (!io->seek(io, Offset >> 3, Mode)) {
				BitFile->BitPos += Offset;
				BitFile->ByteBitOff = BitFile->BitPos % 8;
			}
			break;
		case IL_SEEK_END:
			KeepPos = io->tell(io);
			io->seek(io, 0, IL_SEEK_END);
			Len = io->tell(io);
			io->seek(io, 0, IL_SEEK_SET);

			if (!io->seek(io, Offset >> 3, Mode)) {
				BitFile->BitPos = (Len << 3) + Offset;
				BitFile->ByteBitOff = BitFile->BitPos % 8;
			}

			break;

		default:
			return 1;
	}

	return 0;
}


// hehe, "bread".  It reads data into Buffer from the BITFILE, just like fread for FILE.
ILint bread(void *Buffer, ILuint Size, ILuint Number, BITFILE *BitFile)
{
	// Note that this function is somewhat useless: In binary image
	// formats, there are some pad bits after each scanline. This
	// function does not take that into account, so you must use bseek to
	// skip the calculated value of bits.

	ILuint	BuffPos = 0, Count = Size * Number;
	SIO * io = BitFile->io;

	while (BuffPos < Count) {
		if (BitFile->ByteBitOff < 0 || BitFile->ByteBitOff > 7) {
			BitFile->ByteBitOff = 7;
			if (io->read(io, &BitFile->Buff, 1, 1) != 1)  // Reached eof or error...
				return BuffPos;
		}

		*((ILubyte*)(Buffer) + BuffPos) = (ILubyte)!!(BitFile->Buff & (1 << BitFile->ByteBitOff));

		BuffPos++;
		BitFile->ByteBitOff--;
	}

	return BuffPos;
}


// Reads bits and puts the first bit in the file as the highest in the return value.
ILuint breadVal(ILuint NumBits, BITFILE *BitFile)
{
	ILuint	BuffPos = 0;
	ILuint	Buffer = 0;
	SIO * io = BitFile->io;

	// Only returning up to 32 bits at one time
	if (NumBits > 32) {
		il2SetError(IL_INTERNAL_ERROR);
		return 0;
	}

	while (BuffPos < NumBits) {
		Buffer <<= 1;
		if (BitFile->ByteBitOff < 0 || BitFile->ByteBitOff > 7) {
			BitFile->ByteBitOff = 7;
			if (io->read(io, &BitFile->Buff, 1, 1) != 1)  // Reached eof or error...
				return BuffPos;
		}

		Buffer = Buffer + (ILubyte)!!(BitFile->Buff & (1 << BitFile->ByteBitOff));

		BuffPos++;
		BitFile->ByteBitOff--;
	}

	return BuffPos;
}

