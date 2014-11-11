//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_psp.c
//
// Description: Reads a Paint Shop Pro file.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include "il_psp.h"
//#include "IL/il.h"
#ifndef IL_NO_PSP

const ILubyte PSPSignature[32] = {
	0x50, 0x61, 0x69, 0x6E, 0x74, 0x20, 0x53, 0x68, 0x6F, 0x70, 0x20, 0x50, 0x72, 0x6F, 0x20, 0x49,
	0x6D, 0x61, 0x67, 0x65, 0x20, 0x46, 0x69, 0x6C, 0x65, 0x0A, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00
};

const ILubyte GenAttHead[4] = {
	0x7E, 0x42, 0x4B, 0x00
};


struct PspLoadState {
	GENATT_CHUNK	AttChunk;
	PSPHEAD			Header;
	ILuint			NumChannels;
	ILubyte			**Channels; // = NULL;
	ILubyte			*Alpha; // = NULL;
	ILpal			Pal;
};


// Internal function used to get the Psp header from the current file.
ILuint iGetPspHead(SIO* io, PSPHEAD* header)
{
	return (ILuint) io->read(io, header, 1, sizeof(PSPHEAD));
}


// Internal function used to check if the HEADER is a valid Psp header.
ILboolean iCheckPsp(PSPHEAD* header)
{
	if (strcmp(header->FileSig, "Paint Shop Pro Image File\n\x1a"))
		return IL_FALSE;
	if (header->MajorVersion < 3 || header->MajorVersion > 5)
		return IL_FALSE;
	if (header->MinorVersion != 0)
		return IL_FALSE;

	return IL_TRUE;
}


// Internal function to get the header and check it.
ILboolean iIsValidPsp(SIO* io)
{
	PSPHEAD header;
	if (!iGetPspHead(io, &header))
		return IL_FALSE;
	io->seek(io, -(ILint)sizeof(PSPHEAD), IL_SEEK_CUR);

	return iCheckPsp(&header);
}


ILboolean ReadGenAttributes(PspLoadState* state, ILimage* image)
{
	BLOCKHEAD		AttHead;
	ILint			Padding;
	ILuint			ChunkLen;

	if (image->io.read(&image->io, &AttHead, sizeof(AttHead), 1) != 1)
		return IL_FALSE;
	UShort(&AttHead.BlockID);
	UInt(&AttHead.BlockLen);

	if (AttHead.HeadID[0] != 0x7E || AttHead.HeadID[1] != 0x42 ||
		AttHead.HeadID[2] != 0x4B || AttHead.HeadID[3] != 0x00) {
		il2SetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}
	if (AttHead.BlockID != PSP_IMAGE_BLOCK) {
		il2SetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	ChunkLen = GetLittleUInt(&image->io);
	if (state->Header.MajorVersion != 3)
		ChunkLen -= 4;
	ILuint toRead = IL_MIN(sizeof(state->AttChunk), ChunkLen);
	auto read = image->io.read(&image->io, &state->AttChunk, toRead, 1);
	if (read != 1)
		return IL_FALSE;

	// Can have new entries in newer versions of the spec (4.0).
	Padding = (ChunkLen) - sizeof(state->AttChunk);
	if (Padding > 0)
		image->io.seek(&image->io, Padding, IL_SEEK_CUR);

	// @TODO:  Anything but 24 not supported yet...
	if (state->AttChunk.BitDepth != 24 && state->AttChunk.BitDepth != 8) {
		il2SetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// @TODO;  Add support for compression...
	if (state->AttChunk.Compression != PSP_COMP_NONE && state->AttChunk.Compression != PSP_COMP_RLE) {
		il2SetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// @TODO: Check more things in the general attributes chunk here.

	return IL_TRUE;
}


ILboolean UncompRLE(ILubyte *CompData, ILubyte *Data, ILuint CompLen)
{
	ILubyte	Run, Colour;
	ILint	i, /*x, y,*/ Count/*, Total = 0*/;

	/*for (y = 0; y < AttChunk.Height; y++) {
		for (x = 0, Count = 0; x < AttChunk.Width; ) {
			Run = *CompData++;
			if (Run > 128) {
				Run -= 128;
				Colour = *CompData++;
				memset(Data, Colour, Run);
				Data += Run;
				Count += 2;
			}
			else {
				memcpy(Data, CompData, Run);
				CompData += Run;
				Data += Run;
				Count += Run;
			}
			x += Run;
		}

		Total += Count;

		if (Count % 4) {  // Has to be on a 4-byte boundary.
			CompData += (4 - (Count % 4)) % 4;
			Total += (4 - (Count % 4)) % 4;
		}

		if (Total >= CompLen)
			return IL_FALSE;
	}*/

	for (i = 0, Count = 0; i < (ILint)CompLen; ) {
		Run = *CompData++;
		i++;
		if (Run > 128) {
			Run -= 128;
			Colour = *CompData++;
			i++;
			memset(Data, Colour, Run);
		}
		else {
			memcpy(Data, CompData, Run);
			CompData += Run;
			i += Run;
		}
		Data += Run;
		Count += Run;
	}

	return IL_TRUE;
}


ILubyte *GetChannel(PspLoadState* state, ILimage* image)
{
	BLOCKHEAD		Block;
	CHANNEL_CHUNK	Channel;
	ILubyte			*CompData, *Data;
	ILuint			ChunkSize, Padding;

	if (image->io.read(&image->io, &Block, 1, sizeof(Block)) != sizeof(Block))
		return NULL;
	if (state->Header.MajorVersion == 3)
		Block.BlockLen = GetLittleUInt(&image->io);
	else
		UInt(&Block.BlockLen);

	if (Block.HeadID[0] != 0x7E || Block.HeadID[1] != 0x42 ||
		Block.HeadID[2] != 0x4B || Block.HeadID[3] != 0x00) {
			il2SetError(IL_ILLEGAL_FILE_VALUE);
			return NULL;
	}
	if (Block.BlockID != PSP_CHANNEL_BLOCK) {
		il2SetError(IL_ILLEGAL_FILE_VALUE);
		return NULL;
	}


	if (state->Header.MajorVersion >= 4) {
		ChunkSize = GetLittleUInt(&image->io);
		if (image->io.read(&image->io, &Channel, sizeof(Channel), 1) != 1)
			return NULL;

		Padding = (ChunkSize - 4) - sizeof(Channel);
		if (Padding > 0)
			image->io.seek(&image->io, Padding, IL_SEEK_CUR);
	}
	else {
		if (image->io.read(&image->io, &Channel, sizeof(Channel), 1) != 1)
			return NULL;
	}


	CompData = (ILubyte*)ialloc(Channel.CompLen);
	Data = (ILubyte*)ialloc(state->AttChunk.Width * state->AttChunk.Height);
	if (CompData == NULL || Data == NULL) {
		ifree(Data);
		ifree(CompData);
		return NULL;
	}

	if (image->io.read(&image->io, CompData, 1, Channel.CompLen) != Channel.CompLen) {
		ifree(CompData);
		ifree(Data);
		return NULL;
	}

	switch (state->AttChunk.Compression)
	{
		case PSP_COMP_NONE:
			ifree(Data);
			return CompData;
			break;

		case PSP_COMP_RLE:
			if (!UncompRLE(CompData, Data, Channel.CompLen)) {
				ifree(CompData);
				ifree(Data);
				return IL_FALSE;
			}
			break;

		default:
			ifree(CompData);
			ifree(Data);
			il2SetError(IL_INVALID_FILE_HEADER);
			return NULL;
	}

	ifree(CompData);

	return Data;
}


ILboolean ReadLayerBlock(PspLoadState* state, ILuint BlockLen, ILimage* image)
{
	BLOCKHEAD			Block;
	LAYERINFO_CHUNK		LayerInfo;
	LAYERBITMAP_CHUNK	Bitmap;
	ILuint				ChunkSize, Padding, i, j;
	ILushort			NumChars;

	BlockLen;

	// Layer sub-block header
	if (image->io.read(&image->io, &Block, 1, sizeof(Block)) != sizeof(Block))
		return IL_FALSE;
	if (state->Header.MajorVersion == 3)
		Block.BlockLen = GetLittleUInt(&image->io);
	else
		UInt(&Block.BlockLen);

	if (Block.HeadID[0] != 0x7E || Block.HeadID[1] != 0x42 ||
		Block.HeadID[2] != 0x4B || Block.HeadID[3] != 0x00) {
			return IL_FALSE;
	}
	if (Block.BlockID != PSP_LAYER_BLOCK)
		return IL_FALSE;


	if (state->Header.MajorVersion == 3) {
		image->io.seek(&image->io, 256, IL_SEEK_CUR);  // We don't care about the name of the layer.
		image->io.read(&image->io, &LayerInfo, sizeof(LayerInfo), 1);
		if (image->io.read(&image->io, &Bitmap, sizeof(Bitmap), 1) != 1)
			return IL_FALSE;
	}
	else {  // Header.MajorVersion >= 4
		ChunkSize = GetLittleUInt(&image->io);
		NumChars = GetLittleUShort(&image->io);
		image->io.seek(&image->io, NumChars, IL_SEEK_CUR);  // We don't care about the layer's name.

		ChunkSize -= (2 + 4 + NumChars);

		if (image->io.read(&image->io, &LayerInfo, IL_MIN(sizeof(LayerInfo), ChunkSize), 1) != 1)
			return IL_FALSE;

		// Can have new entries in newer versions of the spec (5.0).
		Padding = (ChunkSize) - sizeof(LayerInfo);
		if (Padding > 0)
			image->io.seek(&image->io, Padding, IL_SEEK_CUR);

		ChunkSize = GetLittleUInt(&image->io);
		if (image->io.read(&image->io, &Bitmap, sizeof(Bitmap), 1) != 1)
			return IL_FALSE;
		Padding = (ChunkSize - 4) - sizeof(Bitmap);
		if (Padding > 0)
			image->io.seek(&image->io, Padding, IL_SEEK_CUR);
	}


	state->Channels = (ILubyte**)ialloc(sizeof(ILubyte*) * Bitmap.NumChannels);
	if (state->Channels == NULL) {
		return IL_FALSE;
	}

	state->NumChannels = Bitmap.NumChannels;

	for (i = 0; i < state->NumChannels; i++) {
		state->Channels[i] = GetChannel(state, image);
		if (state->Channels[i] == NULL) {
			for (j = 0; j < i; j++)
				ifree(state->Channels[j]);
			return IL_FALSE;
		}
	}

	return IL_TRUE;
}


ILboolean ReadAlphaBlock(PspLoadState* state, ILuint BlockLen, ILimage* image)
{
	BLOCKHEAD		Block;
	ALPHAINFO_CHUNK	AlphaInfo;
	ALPHA_CHUNK		AlphaChunk;
	ILushort		NumAlpha, StringSize;
	ILuint			ChunkSize, Padding;

	if (state->Header.MajorVersion == 3) {
		NumAlpha = GetLittleUShort(&image->io);
	}
	else {
		ChunkSize = GetLittleUInt(&image->io);
		NumAlpha = GetLittleUShort(&image->io);
		Padding = (ChunkSize - 4 - 2);
		if (Padding > 0)
			image->io.seek(&image->io, Padding, IL_SEEK_CUR);
	}

	// Alpha channel header
	if (image->io.read(&image->io, &Block, 1, sizeof(Block)) != sizeof(Block))
		return IL_FALSE;
	if (state->Header.MajorVersion == 3)
		Block.BlockLen = GetLittleUInt(&image->io);
	else
		UInt(&Block.BlockLen);

	if (Block.HeadID[0] != 0x7E || Block.HeadID[1] != 0x42 ||
		Block.HeadID[2] != 0x4B || Block.HeadID[3] != 0x00) {
			return IL_FALSE;
	}
	if (Block.BlockID != PSP_ALPHA_CHANNEL_BLOCK)
		return IL_FALSE;


	if (state->Header.MajorVersion >= 4) {
		ChunkSize = GetLittleUInt(&image->io);
		StringSize = GetLittleUShort(&image->io);
		image->io.seek(&image->io, StringSize, IL_SEEK_CUR);
		if (image->io.read(&image->io, &AlphaInfo, sizeof(AlphaInfo), 1) != 1)
			return IL_FALSE;
		Padding = (ChunkSize - 4 - 2 - StringSize - sizeof(AlphaInfo));
		if (Padding > 0)
			image->io.seek(&image->io, Padding, IL_SEEK_CUR);

		ChunkSize = GetLittleUInt(&image->io);
		if (image->io.read(&image->io, &AlphaChunk, sizeof(AlphaChunk), 1) != 1)
			return IL_FALSE;
		Padding = (ChunkSize - 4 - sizeof(AlphaChunk));
		if (Padding > 0)
			image->io.seek(&image->io, Padding, IL_SEEK_CUR);
	}
	else {
		image->io.seek(&image->io, 256, IL_SEEK_CUR);
		image->io.read(&image->io, &AlphaInfo, sizeof(AlphaInfo), 1);
		if (image->io.read(&image->io, &AlphaChunk, sizeof(AlphaChunk), 1) != 1)
			return IL_FALSE;
	}


	/*Alpha = (ILubyte*)ialloc(AlphaInfo.AlphaRect.x2 * AlphaInfo.AlphaRect.y2);
	if (Alpha == NULL) {
		return IL_FALSE;
	}*/


	state->Alpha = GetChannel(state, image);
	if (state->Alpha == NULL)
		return IL_FALSE;

	return IL_TRUE;
}


ILboolean ReadPalette(PspLoadState* state, ILuint BlockLen, ILimage* image)
{
	ILuint ChunkSize, PalCount, Padding;

	if (state->Header.MajorVersion >= 4) {
		ChunkSize = GetLittleUInt(&image->io);
		PalCount = GetLittleUInt(&image->io);
		Padding = (ChunkSize - 4 - 4);
		if (Padding > 0)
			image->io.seek(&image->io, Padding, IL_SEEK_CUR);
	}
	else {
		PalCount = GetLittleUInt(&image->io);
	}

	if (state->Pal.use(PalCount, NULL, IL_PAL_BGRA32))
		return IL_FALSE;

	if (!state->Pal.readFromFile(&image->io))
		return IL_FALSE;

	return IL_TRUE;
}


ILboolean ParseChunks(PspLoadState* state, ILimage* image)
{
	BLOCKHEAD	Block;

	do {
		if (image->io.read(&image->io, &Block, 1, sizeof(Block)) != sizeof(Block)) {
			il2GetError();  // Get rid of the erroneous IL_FILE_READ_ERROR.
			return IL_TRUE;
		}
		if (state->Header.MajorVersion == 3)
			Block.BlockLen = GetLittleUInt(&image->io);
		else
			UInt(&Block.BlockLen);

		if (Block.HeadID[0] != 0x7E || Block.HeadID[1] != 0x42 ||
			Block.HeadID[2] != 0x4B || Block.HeadID[3] != 0x00) {
				return IL_TRUE;
		}
		UShort(&Block.BlockID);
		UInt(&Block.BlockLen);

		auto Pos = image->io.tell(&image->io);

		switch (Block.BlockID)
		{
			case PSP_LAYER_START_BLOCK:
				if (!ReadLayerBlock(state, Block.BlockLen, image))
					return IL_FALSE;
				break;

			case PSP_ALPHA_BANK_BLOCK:
				if (!ReadAlphaBlock(state, Block.BlockLen, image))
					return IL_FALSE;
				break;

			case PSP_COLOR_BLOCK:
				if (!ReadPalette(state, Block.BlockLen, image))
					return IL_FALSE;
				break;

			// Gets done in the next iseek, so this is now commented out.
			//default:
				//image->io.seek(&image->io, Block.BlockLen, IL_SEEK_CUR);
		}

		// Skip to next block just in case we didn't read the entire block.
		image->io.seek(&image->io, Pos + Block.BlockLen, IL_SEEK_SET);

		// @TODO: Do stuff here.

	} while (1);

	return IL_TRUE;
}


ILboolean AssembleImage(PspLoadState* state, ILimage* image)
{
	ILuint Size, i, j;

	Size = state->AttChunk.Width * state->AttChunk.Height;

	if (state->NumChannels == 1) {
		il2TexImage(image, state->AttChunk.Width, state->AttChunk.Height, 1, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, NULL);
		for (i = 0; i < Size; i++) {
			image->Data[i] = state->Channels[0][i];
		}

		if (state->Pal.hasPalette()) {
			image->Format = IL_COLOUR_INDEX;
			image->Pal = state->Pal;
		}
	}
	else {
		if (state->Alpha) {
			il2TexImage(image, state->AttChunk.Width, state->AttChunk.Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);
			for (i = 0, j = 0; i < Size; i++, j += 4) {
				image->Data[j  ] = state->Channels[0][i];
				image->Data[j+1] = state->Channels[1][i];
				image->Data[j+2] = state->Channels[2][i];
				image->Data[j+3] = state->Alpha[i];
			}
		}

		else if (state->NumChannels == 4) {

			il2TexImage(image, state->AttChunk.Width, state->AttChunk.Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);

			for (i = 0, j = 0; i < Size; i++, j += 4) {

				image->Data[j  ] = state->Channels[0][i];

				image->Data[j+1] = state->Channels[1][i];

				image->Data[j+2] = state->Channels[2][i];

				image->Data[j+3] = state->Channels[3][i];

			}

		}
		else if (state->NumChannels == 3) {
			il2TexImage(image, state->AttChunk.Width, state->AttChunk.Height, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL);
			for (i = 0, j = 0; i < Size; i++, j += 3) {
				image->Data[j  ] = state->Channels[0][i];
				image->Data[j+1] = state->Channels[1][i];
				image->Data[j+2] = state->Channels[2][i];
			}
		}
		else
			return IL_FALSE;
	}

	image->Origin = IL_ORIGIN_UPPER_LEFT;

	return IL_TRUE;
}


ILboolean Cleanup(PspLoadState* state)
{
	ILuint	i;

	if (state->Channels) {
		for (i = 0; i < state->NumChannels; i++) {
			ifree(state->Channels[i]);
		}
		ifree(state->Channels);
	}

	if (state->Alpha) {
		ifree(state->Alpha);
	}

	state->Channels = NULL;
	state->Alpha = NULL;
	state->Pal.clear();

	return IL_TRUE;
}


// Internal function used to load the PSP
ILboolean iLoadPspInternal(ILimage* image)
{
	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	PspLoadState state;
	state.Channels = NULL;
	state.Alpha = NULL;

	if (!iGetPspHead(&image->io, &state.Header))
		return IL_FALSE;
	if (!iCheckPsp(&state.Header)) {
		il2SetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!ReadGenAttributes(&state, image))
		return IL_FALSE;
	if (!ParseChunks(&state, image))
		return IL_FALSE;
	if (!AssembleImage(&state, image))
		return IL_FALSE;

	Cleanup(&state);
	return il2FixImage(image);
}

#endif//IL_NO_PSP
