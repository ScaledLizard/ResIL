//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_pic.c
//
// Description: Softimage Pic (.pic) functions
//	Lots of this code is taken from Paul Bourke's Softimage Pic code at
//	http://local.wasp.uwa.edu.au/~pbourke/dataformats/softimagepic/
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#ifndef IL_NO_PIC
#include "il_pic.h"
#include "il_manip.h"
#include <string.h>


// Internal function used to get the .pic header from the current file.
ILint iGetPicHead(SIO* io, PIC_HEAD *Header)
{
	ILint read = (ILint) io->read(io, Header, 1, sizeof(Header));

	#ifdef __LITTLE_ENDIAN__
	iSwapInt(&Header->Magic);
	iSwapFloat(&Header->Version);
	iSwapShort(&Header->Width);
	iSwapShort(&Header->Height);
	iSwapFloat(&Header->Ratio);
	iSwapShort(&Header->Fields);
	iSwapShort(&Header->Padding);
	#endif

	return read;
}


// Internal function used to check if the header is a valid .pic header.
ILboolean iCheckPic(PIC_HEAD *Header)
{
	if (Header->Magic != 0x5380F634)
		return IL_FALSE;
	if (strncmp((const char*)Header->Id, "PICT", 4))
		return IL_FALSE;
	if (Header->Width == 0)
		return IL_FALSE;
	if (Header->Height == 0)
		return IL_FALSE;

	return IL_TRUE;
}


// Internal function to get the header and check it.
ILboolean iIsValidPic(SIO* io)
{
	PIC_HEAD	Head;

	auto read = iGetPicHead(io, &Head);
	io->seek(io, -read, IL_SEEK_CUR);  // Go ahead and restore to previous state
	if (read == sizeof(Head))
		return iCheckPic(&Head);
	else
		return IL_FALSE;
}


ILboolean channelReadMixed(SIO* io, ILubyte *scan, ILint width, ILint noCol, ILint *off, ILint bytes)
{
	ILint	count;
	int		i, j, k;
	ILubyte	col[4];

	for(i = 0; i < width; i += count) {
		if (io->eof(io))
			return IL_FALSE;

		count = io->getc(io);
		if (count == IL_EOF)
			return IL_FALSE;

		if (count >= 128) {  // Repeated sequence
			if (count == 128) {  // Long run
				count = GetBigUShort(io);
				if (io->eof(io)) {
					il2SetError(IL_FILE_READ_ERROR);
					return IL_FALSE;
				}
			}
			else
				count -= 127;
			
			// We've run past...
			if ((i + count) > width) {
				//fprintf(stderr, "ERROR: FF_PIC_load(): Overrun scanline (Repeat) [%d + %d > %d] (NC=%d)\n", i, count, width, noCol);
				il2SetError(IL_ILLEGAL_FILE_VALUE);
				return IL_FALSE;
			}

			for (j = 0; j < noCol; j++)
				if (io->read(io, &col[j], 1, 1) != 1) {
					il2SetError(IL_FILE_READ_ERROR);
					return IL_FALSE;
				}

			for (k = 0; k < count; k++, scan += bytes) {
				for (j = 0; j < noCol; j++)
					scan[off[j]] = col[j];
			}
		} else {				// Raw sequence
			count++;
			if ((i + count) > width) {
				//fprintf(stderr, "ERROR: FF_PIC_load(): Overrun scanline (Raw) [%d + %d > %d] (NC=%d)\n", i, count, width, noCol);
				il2SetError(IL_ILLEGAL_FILE_VALUE);
				return IL_FALSE;
			}
			
			for (k = count; k > 0; k--, scan += bytes) {
				for (j = 0; j < noCol; j++)
					if (io->read(io, &scan[off[j]], 1, 1) != 1) {
						il2SetError(IL_FILE_READ_ERROR);
						return IL_FALSE;
					}
			}
		}
	}

	return IL_TRUE;
}


ILboolean channelReadRaw(SIO* io, ILubyte *scan, ILint width, ILint noCol, ILint *off, ILint bytes)
{
	ILint i, j;

	for (i = 0; i < width; i++) {
		if (io->eof(io))
			return IL_FALSE;
		for (j = 0; j < noCol; j++)
			if (io->read(io, &scan[off[j]], 1, 1) != 1)
				return IL_FALSE;
		scan += bytes;
	}
	return IL_TRUE;
}


ILboolean channelReadPure(SIO* io, ILubyte *scan, ILint width, ILint noCol, ILint *off, ILint bytes)
{
	ILubyte		col[4];
	ILint		count;
	int			i, j, k;

	for (i = width; i > 0; ) {
		count = io->getc(io);
		if (count == IL_EOF)
			return IL_FALSE;
		if (count > width)
			count = width;
		i -= count;
		
		if (io->eof(io))
			return IL_FALSE;
		
		for (j = 0; j < noCol; j++)
			if (io->read(io, &col[j], 1, 1) != 1)
				return IL_FALSE;
		
		for (k = 0; k < count; k++, scan += bytes) {
			for(j = 0; j < noCol; j++)
				scan[off[j] + k] = col[j];
		}
	}
	return IL_TRUE;
}


ILuint readScanline(SIO* io, ILubyte *scan, ILint width, CHANNEL *channel, ILint bytes)
{
	ILint		noCol;
	ILint		off[4];
	ILuint		status=0;

	while (channel) {
		noCol = 0;
		if(channel->Chan & PIC_RED_CHANNEL) {
			off[noCol] = 0;
			noCol++;
		}
		if(channel->Chan & PIC_GREEN_CHANNEL) {
			off[noCol] = 1;
			noCol++;
		}
		if(channel->Chan & PIC_BLUE_CHANNEL) {
			off[noCol] = 2;
			noCol++;
		}
		if(channel->Chan & PIC_ALPHA_CHANNEL) {
			off[noCol] = 3;
			noCol++;
			//@TODO: Find out if this is possible.
			if (bytes == 3)  // Alpha channel in a 24-bit image.  Do not know what to do with this.
				return 0;
		}

		switch(channel->Type & 0x0F)
		{
			case PIC_UNCOMPRESSED:
				status = channelReadRaw(io, scan, width, noCol, off, bytes);
				break;
			case PIC_PURE_RUN_LENGTH:
				status = channelReadPure(io, scan, width, noCol, off, bytes);
				break;
			case PIC_MIXED_RUN_LENGTH:
				status = channelReadMixed(io, scan, width, noCol, off, bytes);
				break;
		}
		if (!status)
			break;

		channel = (CHANNEL*)channel->Next;
	}
	return status;
}

ILboolean readScanlines(SIO* io, ILuint *image, ILint width, ILint height, CHANNEL *channel, ILuint alpha)
{
	ILint	i;
	ILuint	*scan;

	(void)alpha;
	
	for (i = height - 1; i >= 0; i--) {
		scan = image + i * width;

		if (!readScanline(io, (ILubyte *)scan, width, channel, alpha ? 4 : 3)) {
			il2SetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
		}
	}

	return IL_TRUE;
}


// Internal function used to load the .pic
ILboolean iLoadPicInternal(ILimage* image)
{
	ILuint		Alpha = IL_FALSE;
	ILubyte		Chained;
	CHANNEL		*Channel = NULL, *Channels = NULL, *Prev;
	PIC_HEAD	Header;
	ILboolean	Read;
	SIO* io = &image->io;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!iGetPicHead(io, &Header))
		return IL_FALSE;
	if (!iCheckPic(&Header)) {
		il2SetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// Read channels
	do {
		if (Channel == NULL) {
			Channel = Channels = (CHANNEL*)ialloc(sizeof(CHANNEL));
			if (Channels == NULL)
				return IL_FALSE;
		}
		else {
			Channels->Next = (CHANNEL*)ialloc(sizeof(CHANNEL));
			if (Channels->Next == NULL) {
				// Clean up the list before erroring out.
				while (Channel) {
					Prev = Channel;
					Channel = (CHANNEL*)Channel->Next;
					ifree(Prev);
				}
				return IL_FALSE;
			}
			Channels = (CHANNEL*)Channels->Next;
		}
		Channels->Next = NULL;

		Chained = io->getc(io);
		Channels->Size = io->getc(io);
		Channels->Type = io->getc(io);
		Channels->Chan = io->getc(io);
		if (io->eof(io)) {
			Read = IL_FALSE;
			goto finish;
		}
		
		// See if we have an alpha channel in there
		if (Channels->Chan & PIC_ALPHA_CHANNEL)
			Alpha = IL_TRUE;
		
	} while (Chained);

	if (Alpha) {  // Has an alpha channel
		if (!il2TexImage(image, Header.Width, Header.Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL)) {
			Read = IL_FALSE;
			goto finish;  // Have to destroy Channels first.
		}
	}
	else {  // No alpha channel
		if (!il2TexImage(image, Header.Width, Header.Height, 1, 3, IL_RGBA, IL_UNSIGNED_BYTE, NULL)) {
			Read = IL_FALSE;
			goto finish;  // Have to destroy Channels first.
		}
	}
	image->Origin = IL_ORIGIN_LOWER_LEFT;

	Read = readScanlines(io, (ILuint*)image->Data, Header.Width, Header.Height, Channel, Alpha);

finish:
	// Destroy channels
	while (Channel) {
		Prev = Channel;
		Channel = (CHANNEL*)Channel->Next;
		ifree(Prev);
	}

	if (Read == IL_FALSE)
		return IL_FALSE;

	return il2FixImage(image);
}


#endif//IL_NO_PIC

