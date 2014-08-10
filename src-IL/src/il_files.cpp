//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 01/04/2009
//
// Filename: src-IL/src/il_files.c
//
// Description: File handling for DevIL
//
//-----------------------------------------------------------------------------


#define __FILES_C
#include "il_internal.h"
#include "IL/il2.h"
#include <stdarg.h>


// Lump read functions
ILboolean	ILAPIENTRY iEofLump(SIO* io);
ILint		ILAPIENTRY iGetcLump(SIO* io);
ILint64		ILAPIENTRY iReadLump(SIO* io, void *Buffer, const ILuint Size, const ILuint Number);
ILint64		ILAPIENTRY iSeekLump(SIO* io, ILint64 Offset, ILuint Mode);
ILint64		ILAPIENTRY iTellLump(SIO* io);
ILint		ILAPIENTRY iPutcLump(ILubyte Char, SIO* io);
ILint64		ILAPIENTRY iWriteLump(const void *Buffer, ILuint Size, ILuint Number, SIO* io);

// "Fake" size functions, used to determine the size of write lumps
// Definitions are in il_size.cpp
ILint64		ILAPIENTRY iSizeSeek(SIO* io, ILint64 Offset, ILuint Mode);
ILint64		ILAPIENTRY iSizeTell(SIO* io);
ILint		ILAPIENTRY iSizePutc(ILubyte Char, SIO* io);
ILint64		ILAPIENTRY iSizeWrite(const void *Buffer, ILuint Size, ILuint Number, SIO* io);

// Next 7 functions are the default read functions

ILHANDLE ILAPIENTRY iDefaultOpenR(ILconst_string FileName)
{
#ifndef _UNICODE
	return (ILHANDLE)fopen((char*)FileName, "rb");
#else
	// Windows has a different function, _wfopen, to open UTF16 files,
	//  whereas Linux just uses fopen for its UTF8 files.
	#ifdef _WIN32
		return (ILHANDLE)_wfopen(FileName, L"rb");
	#else
		return (ILHANDLE)fopen((char*)FileName, "rb");
	#endif
#endif//UNICODE
}


void ILAPIENTRY iDefaultCloseR(SIO* io)
{
	fclose((FILE*)(io->handle));
	return;
}


ILboolean ILAPIENTRY iDefaultEof(SIO* io)
{
	// Find out the filesize for checking for the end of file
	auto OrigPos = io->tell(io);
	io->seek(io, 0, IL_SEEK_END);
	auto FileSize = io->tell(io);
	io->seek(io, OrigPos, IL_SEEK_SET);

	if (io->tell(io) >= FileSize)
		return IL_TRUE;
	return IL_FALSE;
}


ILint ILAPIENTRY iDefaultGetc(SIO* io)
{
	ILint Val;

	Val = 0;
	if (io->read(io, &Val, 1, 1) != 1)
		return IL_EOF;
	return Val;
}


ILint64 ILAPIENTRY iDefaultRead(SIO* io, void *Buffer, ILuint Size, ILuint Number)
{
	return (ILint)fread(Buffer, Size, Number, (FILE*) (io->handle));
}


ILint64 ILAPIENTRY iDefaultSeek(SIO* io, ILint64 Offset, ILuint Mode)
{
	return fseek((FILE*)(io->handle), Offset, Mode);
}


ILint64 ILAPIENTRY iDefaultTell(SIO* io)
{
	return ftell((FILE*)(io->handle));
}


ILHANDLE ILAPIENTRY iDefaultOpenW(ILconst_string FileName)
{
#ifndef _UNICODE
	return (ILHANDLE)fopen((char*)FileName, "wb");
#else
	// Windows has a different function, _wfopen, to open UTF16 files,
	//  whereas Linux just uses fopen.
	#ifdef _WIN32
		return (ILHANDLE)_wfopen(FileName, L"wb");
	#else
		return (ILHANDLE)fopen((char*)FileName, "wb");
	#endif
#endif//UNICODE
}


void ILAPIENTRY iDefaultCloseW(SIO* io)
{
	fclose((FILE*)(io->handle));
	return;
}


ILint ILAPIENTRY iDefaultPutc(ILubyte Char, SIO* io)
{
	return fputc(Char, (FILE*)(io->handle));
}


ILint64 ILAPIENTRY iDefaultWrite(const void *Buffer, ILuint Size, ILuint Number, SIO* io)
{
	return (ILint)fwrite(Buffer, Size, Number, (FILE*)(io->handle));
}


// Allows you to override the default file-reading functions
void ILAPIENTRY iSetRead(SIO* io, fOpenProc aOpen, fCloseProc aClose, fEofProc aEof, fGetcProc aGetc, 
	fReadProc aRead, fSeekProc aSeek, fTellProc aTell)
{
	if (io != NULL) {
		memset(io, 0, sizeof(SIO));
		io->openReadOnly    = aOpen;
		io->close   = aClose;
		io->eof   = aEof;
		io->getc  = aGetc;
		io->read  = aRead;
		io->seek = aSeek;
		io->tell = aTell;
	}

	return;
}

// Allows you to override the default file-reading functions
void ILAPIENTRY il2SetRead(ILimage* imageExt, fOpenProc aOpen, fCloseProc aClose, fEofProc aEof, fGetcProc aGetc, 
	fReadProc aRead, fSeekProc aSeek, fTellProc aTell)
{
	ILimage* image = (ILimage*) imageExt;
	if (image != NULL)
		iSetRead(&image->io, aOpen, aClose, aEof, aGetc, aRead, aSeek, aTell);
}


// Reset io to use the default file system functions for reading data
void ILAPIENTRY il2ResetRead(ILimage* image)
{
	il2SetRead((ILimage*) image, iDefaultOpenR, iDefaultCloseR, iDefaultEof, iDefaultGetc, 
				iDefaultRead, iDefaultSeek, iDefaultTell);
	return;
}

// Allows you to override the default file-writing functions
void ILAPIENTRY iSetWrite(SIO* io, fOpenProc Open, fCloseProc Close, fPutcProc Putc, fSeekProc Seek, 
	fTellProc Tell, fWriteProc Write)
{
	if (io != NULL) {
		memset(io, 0, sizeof(SIO));
		io->openWrite = Open;
		io->close = Close;
		io->putc  = Putc;
		io->write = Write;
		io->seek  = Seek;
		io->tell  = Tell;
	}
}

// Allows you to override the default file-writing functions
void ILAPIENTRY il2SetWrite(ILimage* aImageExt, fOpenProc aOpen, fCloseProc aClose, fPutcProc aPutc, fSeekProc aSeek, 
	fTellProc aTell, fWriteProc aWrite)
{
	ILimage* image = (ILimage*) aImageExt;
	if (image != NULL)
		iSetWrite(&image->io, aOpen, aClose, aPutc, aSeek, aTell, aWrite);
}

// Restore file-based i/o functions
void ILAPIENTRY il2ResetWrite(ILimage* aImageExt)
{
	ILimage* image = (ILimage*) aImageExt;
	il2SetWrite(image, iDefaultOpenW, iDefaultCloseW, iDefaultPutc,
				iDefaultSeek, iDefaultTell, iDefaultWrite);
	return;
}


// Tells DevIL that we're reading from a file, not a lump
void iSetInputFile(SIO* io, ILHANDLE File)
{
	if (io != NULL) {
		iSetRead(io, NULL, NULL, iDefaultEof, iDefaultGetc, iDefaultRead, iDefaultSeek, iDefaultTell);
		io->handle = File;
		io->ReadFileStart = io->tell(io);
	}
}


// Tells DevIL that we're reading from a lump, not a file
void iSetInputLump(SIO* io, const void *Lump, ILuint Size)
{
	if (io != NULL) {
		iSetRead(io, NULL, NULL, iEofLump, iGetcLump, iReadLump, iSeekLump, iTellLump);
		io->lump = Lump;
		io->rwPos = 0;
		io->lumpSize = Size;
	}
}


// Tells DevIL that we're writing to a file, not a lump
void iSetOutputFile(SIO* io, ILHANDLE File)
{
	if (io != NULL) {
		iSetWrite(io, NULL, NULL, iDefaultPutc, iDefaultSeek, iDefaultTell, iDefaultWrite);
		io->handle = File;
	}
}


// This is only called by ilDetermineSize.  Associates iputc, etc. with
//  "fake" writing functions in il_size.c.
void iSetOutputFake(SIO* io)
{
	iSetWrite(io, NULL, NULL, iSizePutc, iSizeSeek, iSizeTell, iSizeWrite);
}


// Tells DevIL that we're writing to a lump, not a file
void iSetOutputLump(SIO* io, void *Lump, ILuint Size)
{
	// In this case, ilDetermineSize is currently trying to determine the
	// output buffer size.  It already has the write functions it needs.
	if (Lump != NULL && io != NULL) {
		iSetWrite(io, NULL, NULL, iPutcLump, iSeekLump, iTellLump, iWriteLump);
		io->lump = Lump;
		io->lumpSize = Size;
	}
}

// Get current read/write position for a lump
ILuint64 ILAPIENTRY il2GetLumpPos(ILimage* aImageExt)
{
	ILimage* image = (ILimage*) aImageExt;
	if (image != NULL)
		if (image->io.lump != 0)
			return image->io.rwPos;
	return 0;
}


ILuint ILAPIENTRY ilprintf(SIO* io, const char *Line, ...)
{
	char	Buffer[2048];  // Hope this is large enough
	va_list	VaLine;
	ILuint	i;

	va_start(VaLine, Line);
	vsprintf(Buffer, Line, VaLine);
	va_end(VaLine);

	i = ilCharStrLen(Buffer);
	io->write(Buffer, 1, i, io);

	return i;
}


// To pad zeros where needed...
void ipad(SIO* io, ILuint NumZeros)
{
	ILuint i = 0;
	for (; i < NumZeros; i++)
		io->putc(0, io);
	return;
}


//
// The rest of the functions following in this file are quite
//	self-explanatory, except where commented.
//

// Next 12 functions are the default write functions

ILboolean ILAPIENTRY iEofFile(SIO* io)
{
	return feof((FILE*)(io->handle));
}


ILboolean ILAPIENTRY iEofLump(SIO* io)
{
	if (io->lumpSize != 0)
		return (io->rwPos >= io->lumpSize);
	return IL_FALSE;
}


ILint ILAPIENTRY iGetcLump(SIO* io)
{
	// If ReadLumpSize is 0, don't even check to see if we've gone past the bounds.
	if (io->lumpSize > 0) {
		if (io->rwPos + 1 > io->lumpSize) {
			io->rwPos--;
			il2SetError(IL_FILE_READ_ERROR);
			return IL_EOF;
		}
	}

	return *((ILubyte*)io->lump + io->rwPos++);
}


ILint64 ILAPIENTRY iReadLump(SIO* io, void *Buffer, const ILuint Size, const ILuint Number)
{
	ILint64 i, ByteSize = IL_MIN( Size*Number, io->lumpSize-io->rwPos);

	for (i = 0; i < ByteSize; i++) {
		*((ILubyte*)Buffer + i) = *((ILubyte*)io->lump + io->rwPos + i);
		if (io->lumpSize > 0) {  // ReadLumpSize is too large to care about apparently
			if (io->rwPos + i > io->lumpSize) {
				io->rwPos += i;
				if (i != Number)
					il2SetError(IL_FILE_READ_ERROR);
				return i;
			}
		}
	}

	io->rwPos += i;
	if (Size != 0)
		i /= Size;
	if (i != Number)
		il2SetError(IL_FILE_READ_ERROR);
	return i;
}


ILint64 ILAPIENTRY iSeekFile(SIO* io, ILint64 Offset, ILuint Mode)
{
	if (Mode == IL_SEEK_SET)
		Offset += io->ReadFileStart;  // This allows us to use IL_SEEK_SET in the middle of a file.
	return fseek((FILE*) io->handle, Offset, Mode);
}


// Returns 1 on error, 0 on success
ILint64 ILAPIENTRY iSeekLump(SIO* io, ILint64 Offset, ILuint Mode)
{
	switch (Mode)
	{
		case IL_SEEK_SET:
			if (Offset > (ILint)io->lumpSize)
				return 1;
			io->rwPos = Offset;
			break;

		case IL_SEEK_CUR:
			if (io->rwPos + Offset > io->lumpSize || io->rwPos+Offset < 0)
				return 1;
			io->rwPos += Offset;
			break;

		case IL_SEEK_END:
			if (Offset > 0)
				return 1;
			// Should we use >= instead?
			if (abs(Offset) > (ILint)io->lumpSize)  // If ReadLumpSize == 0, too bad
				return 1;
			io->rwPos = io->lumpSize + Offset;
			break;

		default:
			return 1;
	}

	return 0;
}


ILHANDLE ILAPIENTRY iGetFile(ILimage* image)
{
	if (image != NULL)
		return image->io.handle;
	else
		return 0;
}


const ILubyte* ILAPIENTRY iGetLump(ILimage* image) 
{
	if (image != NULL)
		return (ILubyte*)image->io.lump;
	else
		return NULL;
}


// Next 4 functions are the default write functions

ILint ILAPIENTRY iPutcFile(ILubyte Char, SIO* io)
{
	return putc(Char, (FILE*)io->handle);
}


ILint ILAPIENTRY iPutcLump(ILubyte Char, SIO* io)
{
	if (io->rwPos >= io->lumpSize || io->lump == NULL)
		return IL_EOF;

	((ILubyte*)(io->lump))[io->rwPos] = Char;
	++io->rwPos;
	return Char;
}


ILint64 ILAPIENTRY iWriteFile(const void *Buffer, ILuint Size, ILuint Number, SIO* io)
{
	ILint64 NumWritten;
	NumWritten = fwrite(Buffer, Size, Number, (FILE*) io->handle);
	if (NumWritten != Number) {
		il2SetError(IL_FILE_WRITE_ERROR);
		return 0;
	}
	return NumWritten;
}


ILint64 ILAPIENTRY iWriteLump(const void *Buffer, ILuint Size, ILuint Number, SIO* io)
{
	if (Buffer == NULL || Size == 0 || io->rwPos >= io->lumpSize || io->lump == NULL)
		return 0;

	ILubyte* inP = (ILubyte*) Buffer;
	ILubyte* outP = &((ILubyte*) io->lump)[io->rwPos];
	size_t toCopy = Size * Number;

	if (io->rwPos + toCopy > io->lumpSize)
		toCopy = io->lumpSize - io->rwPos;

	memcpy(outP, inP, toCopy);
	io->rwPos += toCopy;

	return toCopy;
}


ILuint ILAPIENTRY iTellFile(SIO* io)
{
	return ftell((FILE*) (io->handle));
}


ILint64 ILAPIENTRY iTellLump(SIO* io)
{
	return io->rwPos;
}

