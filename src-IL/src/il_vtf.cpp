//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_vtf.c
//
// Description: Reads from and writes to a Valve Texture Format (.vtf) file.
//                These are used in Valve's Source games.  VTF specs available
//                from http://developer.valvesoftware.com/wiki/VTF.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_VTF
#include "il_vtf.h"
#include "il_dds.h"


// Internal function used to get the VTF header from the current file.
ILboolean iGetVtfHead(SIO* io, VTFHEAD *Header)
{
	auto read = io->read(io, Header, 1, sizeof(*Header));

	// @todo: untested endian conversion - I don't have a machine+OS that uses big endian
	#ifdef __BIG_ENDIAN__
	iSwapUInt(Header->Version[0]);
	iSwapUInt(Header->Version[1]);
	iSwapUInt(Header->HeaderSize);
	iSwapUShort(Header->Width);
	iSwapUShort(Header->Height);
	iSwapUInt(Header->Flags);
	iSwapUShort(Header->Frames);
	iSwapUShort(Header->FirstFrame);
	iSwapFloat(Header->Reflectivity[0]);
	iSwapFloat(Header->Reflectivity[1]);
	iSwapFloat(Header->Reflectivity[2]);
	iSwapFloat(Header->BumpmapScale);
	iSwapUInt(Header->HighResImageFormat);
	iSwapInt(Header->LowResImageFormat);
	#endif

	//@TODO: This is a hack for the moment.
	if (Header->HeaderSize == 64) {
		if (Header->Depth == 0)
			Header->Depth = 1;
	}

	if (read == sizeof(VTFHEAD))
		return IL_TRUE;
	else
		return IL_FALSE;
}


// Internal function to get the header and check it.
ILboolean iIsValidVtf(SIO* io)
{
	VTFHEAD Header;

	ILint64 oldReadPos = io->tell(io);
	ILboolean gotHeader = iGetVtfHead(io, &Header);
	io->seek(io, oldReadPos, IL_SEEK_SET);
	
	if (gotHeader)
		return iCheckVtf(&Header);
	else
		return IL_FALSE;
}


//@TODO: Add more checks.
// Should we check for Frames, MipmapCount and Depth != 0?

// Internal function used to check if the HEADER is a valid VTF header.
ILboolean iCheckVtf(VTFHEAD *Header)
{
	// The file signature is "VTF\0".
	if ((Header->Signature[0] != 'V') || (Header->Signature[1] != 'T') || (Header->Signature[2] != 'F')
		|| (Header->Signature[3] != 0))
		return IL_FALSE;
	// Are there other versions available yet?
	if (Header->Version[0] != 7)
		return IL_FALSE;
	// We have 7.0 through 7.4 as of 12/27/2008.
	if (Header->Version[1] > 4)
		return IL_FALSE;
	// May change in future version of the specifications.
	//  80 is through version 7.2, and 96/104 are through 7.4.
	//  This must be 16-byte aligned, but something is outputting headers with 104.
	if ((Header->HeaderSize != 80) && (Header->HeaderSize != 96) && (Header->HeaderSize != 104)
		&& (Header->HeaderSize != 64))
		return IL_FALSE;

	// 0 is an invalid dimension
	if (Header->Width == 0 || Header->Height == 0)
		return IL_FALSE;
	// Width and Height must be powers of 2.
	if ((ilNextPower2(Header->Width) != Header->Width) || (ilNextPower2(Header->Height) != Header->Height))
		return IL_FALSE;
	// It looks like width and height of zero are valid - i.e. no low res image.
	if (Header->LowResImageWidth != 0 && Header->LowResImageHeight != 0) {
		if ((ilNextPower2(Header->LowResImageWidth) != Header->LowResImageWidth)
			|| (ilNextPower2(Header->LowResImageHeight) != Header->LowResImageHeight))
			return IL_FALSE;
	}
	// In addition, the LowResImage has to have dimensions no greater than 16.
	if ((Header->LowResImageWidth > 16) || (Header->LowResImageHeight > 16))
		//|| (Header->LowResImageWidth == 0) || (Header->LowResImageHeight == 0))
		// It looks like width and height of zero are valid.
		return IL_FALSE;
	// And the LowResImage has to have dimensions less than or equal to the main image.
	if ((Header->LowResImageWidth > Header->Width) || (Header->LowResImageHeight > Header->Height))
		return IL_FALSE;
	// The LowResImage must be in DXT1 format, or if it does not exist, it is denoted by all bits set.
	if (Header->LowResImageFormat != IMAGE_FORMAT_DXT1 && Header->LowResImageFormat != 0xFFFFFFFF)
		return IL_FALSE;
	
	return IL_TRUE;
}


// Internal function used to load the VTF.
ILboolean iLoadVtfInternal(ILimage* baseImage)
{
	ILboolean	bVtf = IL_TRUE;
	ILimage		*curImage = baseImage;
	ILenum		Format, Type;
	ILuint		SizeOfData, Channels, k;
	ILubyte		*CompData = NULL, SwapVal, *Data16Bit, *Temp, NumFaces;
	VTFHEAD		Head;
	SIO * io = &baseImage->io;

	if (baseImage == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	
	if (!iGetVtfHead(io, &Head))
		return IL_FALSE;
	if (!iCheckVtf(&Head)) {
		il2SetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (Head.Flags & TEXTUREFLAGS_ENVMAP) {
		// If we have a spherical map in addition to the 6 sides of the cubemap, FirstFrame's bits
		//  are all set.  The only place I found out about this was VTFLib.
		NumFaces = Head.FirstFrame == 0xFFFF ? 6 : 7;
	}
	else
		NumFaces = 1;  // This is not an environmental map.

	// Skip the low resolution image.  This is just a thumbnail.
	//  The block size is 8, and the compression ratio is 6:1.
	SizeOfData = IL_MAX(Head.LowResImageWidth * Head.LowResImageHeight / 2, 8);
	if (Head.LowResImageWidth == 0 && Head.LowResImageHeight == 0)
		SizeOfData = 0;  // No low resolution image present.
	io->seek(io, SizeOfData, IL_SEEK_CUR);

	//@TODO: Make this a helper function that set channels, bpc and format.
	switch (Head.HighResImageFormat)
	{
		case IMAGE_FORMAT_DXT1:  //@TODO: Should we make DXT1 channels = 3?
		case IMAGE_FORMAT_DXT1_ONEBITALPHA:
		case IMAGE_FORMAT_DXT3:
		case IMAGE_FORMAT_DXT5:
			Channels = 4;
			Format = IL_RGBA;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_BGR888:
		case IMAGE_FORMAT_BGR888_BLUESCREEN:
			Channels = 3;
			Format = IL_BGR;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_BGRA8888:
			Channels = 4;
			Format = IL_BGRA;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_BGRX8888:
			Channels = 3;
			Format = IL_BGR;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_RGB888:
		case IMAGE_FORMAT_RGB888_BLUESCREEN:
			Channels = 3;
			Format = IL_RGB;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_RGBA8888:
			Channels = 4;
			Format = IL_RGBA;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_RGBA16161616:  // 16-bit shorts
			Channels = 4;
			Format = IL_RGBA;
			Type = IL_UNSIGNED_SHORT;
			break;
		case IMAGE_FORMAT_RGBA16161616F:  // 16-bit floats
			Channels = 4;
			Format = IL_RGBA;
			Type = IL_FLOAT;
			break;
		case IMAGE_FORMAT_I8:  // 8-bit luminance data
			Channels = 1;
			Format = IL_LUMINANCE;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_IA88:  // 8-bit luminance and alpha data
			Channels = 2;
			Format = IL_LUMINANCE_ALPHA;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_A8:  // 8-bit alpha data
			Channels = 1;
			Format = IL_ALPHA;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_ARGB8888:
			Channels = 4;
			Format = IL_BGRA;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_ABGR8888:
			Channels = 4;
			Format = IL_RGBA;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_RGB565:
			Channels = 3;
			Format = IL_RGB;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_BGR565:
			Channels = 3;
			Format = IL_BGR;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_BGRA5551:
			Channels = 4;
			Format = IL_BGRA;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_BGRX5551:  // Unused alpha channel
			Channels = 3;
			Format = IL_BGR;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_BGRA4444:
			Channels = 4;
			Format = IL_BGRA;
			Type = IL_UNSIGNED_BYTE;
			break;

		default:
			il2SetError(IL_FORMAT_NOT_SUPPORTED);
			return IL_FALSE;
	}

	if (!il2TexImage(baseImage, Head.Width, Head.Height, Head.Depth, Channels, Format, Type, NULL))
		return IL_FALSE;
	// The origin should be in the upper left.
	baseImage->Origin = IL_ORIGIN_UPPER_LEFT;
	// Create any mipmaps.
	VtfInitFacesMipmaps(baseImage, NumFaces, &Head);

	// Create our animation chain
	ILimage * frame = baseImage;
	for (ILint Frame = 1; Frame < Head.Frames; Frame++) {
		frame->Next = ilNewImageFull(Head.Width, Head.Height, Head.Depth, Channels, Format, Type, NULL);
		if (frame->Next == NULL)
			return IL_FALSE;
		frame = frame->Next;
		// The origin should be in the upper left.
		frame->Origin = IL_ORIGIN_UPPER_LEFT;

		// Create our faces and mipmaps for each frame.
		VtfInitFacesMipmaps(frame, NumFaces, &Head);
	}

	// We want to put the smallest mipmap at the end, but it is first in the file, so we count backwards.
	for (ILint Mipmap = Head.MipmapCount - 1; Mipmap >= 0; Mipmap--) {
		ILimage* mipmap = il2GetMipmap(baseImage, Mipmap);
		// Frames are in the normal order.
		for (ILint Frame = 0; Frame < Head.Frames; Frame++) {
			ILimage* frameImage = il2GetFrame(mipmap, Frame);
			// @TODO: Cubemap faces are always in the same order?
			for (ILint Face = 0; Face < NumFaces; Face++) {
				ILimage* face = il2GetFace(mipmap, Face);

				switch (Head.HighResImageFormat)
				{
					// DXT1 compression
					case IMAGE_FORMAT_DXT1:
					case IMAGE_FORMAT_DXT1_ONEBITALPHA:
						// The block size is 8.
						SizeOfData = IL_MAX(face->Width * face->Height * face->Depth / 2, 8);
						CompData = (ILubyte*)ialloc(SizeOfData);  // Gives a 6:1 compression ratio (or 8:1 for DXT1 with alpha)
						if (CompData == NULL)
							return IL_FALSE;
						io->read(io, CompData, 1, SizeOfData);
						// Keep a copy of the DXTC data if the user wants it.
						if (il2GetInteger(IL_KEEP_DXTC_DATA) == IL_TRUE) {
							face->DxtcSize = SizeOfData;
							face->DxtcData = CompData;
							face->DxtcFormat = IL_DXT5;
							CompData = NULL;
						}
						bVtf = DecompressDXT1(face, CompData);
						break;

					// DXT3 compression
					case IMAGE_FORMAT_DXT3:
						// The block size is 16.
						SizeOfData = IL_MAX(face->Width * face->Height * face->Depth, 16);
						CompData = (ILubyte*)ialloc(SizeOfData);  // Gives a 4:1 compression ratio
						if (CompData == NULL)
							return IL_FALSE;
						io->read(io, CompData, 1, SizeOfData);
						// Keep a copy of the DXTC data if the user wants it.
						if (il2GetInteger(IL_KEEP_DXTC_DATA) == IL_TRUE) {
							face->DxtcSize = SizeOfData;
							face->DxtcData = CompData;
							face->DxtcFormat = IL_DXT3;
							CompData = NULL;
						}
						bVtf = DecompressDXT3(face, CompData);
						break;

					// DXT5 compression
					case IMAGE_FORMAT_DXT5:
						// The block size is 16.
						SizeOfData = IL_MAX(face->Width * face->Height * face->Depth, 16);
						CompData = (ILubyte*)ialloc(SizeOfData);  // Gives a 4:1 compression ratio
						if (CompData == NULL)
							return IL_FALSE;
						io->read(io, CompData, 1, SizeOfData);
						// Keep a copy of the DXTC data if the user wants it.
						if (il2GetInteger(IL_KEEP_DXTC_DATA) == IL_TRUE) {
							face->DxtcSize = SizeOfData;
							face->DxtcData = CompData;
							face->DxtcFormat = IL_DXT5;
							CompData = NULL;
						}
						bVtf = DecompressDXT5(face, CompData);
						break;

					// Uncompressed BGR(A) data (24-bit and 32-bit)
					case IMAGE_FORMAT_BGR888:
					case IMAGE_FORMAT_BGRA8888:
					// Uncompressed RGB(A) data (24-bit and 32-bit)
					case IMAGE_FORMAT_RGB888:
					case IMAGE_FORMAT_RGBA8888:
					// Uncompressed 16-bit shorts
					case IMAGE_FORMAT_RGBA16161616:
					// Luminance data only
					case IMAGE_FORMAT_I8:
					// Luminance and alpha data
					case IMAGE_FORMAT_IA88:
					// Alpha data only
					case IMAGE_FORMAT_A8:
					// We will ignore the part about the bluescreen right now.
					//   I could not find any information about it.
					case IMAGE_FORMAT_RGB888_BLUESCREEN:
					case IMAGE_FORMAT_BGR888_BLUESCREEN:
						// Just copy the data over - no compression.
						if (io->read(io, face->Data, 1, face->SizeOfData) != face->SizeOfData)
							bVtf = IL_FALSE;
						else
							bVtf = IL_TRUE;
						break;

					// Uncompressed 24-bit data with an unused alpha channel (we discard it)
					case IMAGE_FORMAT_BGRX8888:
						SizeOfData = face->Width * face->Height * face->Depth * 3;
						Temp = CompData = (ILubyte*)ialloc(SizeOfData / 3 * 4);  // Not compressed data
						if (CompData == NULL)
							return IL_FALSE;
						if (io->read(io, CompData, 1, SizeOfData / 3 * 4) != SizeOfData / 3 * 4) {
							bVtf = IL_FALSE;
							break;
						}
						for (k = 0; k < SizeOfData; k += 3) {
							face->Data[k]   = Temp[0];
							face->Data[k+1] = Temp[1];
							face->Data[k+2] = Temp[2];
							Temp += 4;
						}

						break;

					// Uncompressed 16-bit floats (must be converted to 32-bit)
					case IMAGE_FORMAT_RGBA16161616F:
						SizeOfData = face->Width * face->Height * face->Depth * face->Bpp * 2;
						CompData = (ILubyte*)ialloc(SizeOfData);  // Not compressed data
						if (CompData == NULL)
							return IL_FALSE;
						if (io->read(io, CompData, 1, SizeOfData) != SizeOfData) {
							bVtf = IL_FALSE;
							break;
						}
						bVtf = iConvFloat16ToFloat32((ILuint*)face->Data, (ILushort*)CompData, SizeOfData / 2);
						break;

					// Uncompressed 32-bit ARGB and ABGR data.  DevIL does not handle this
					//   internally, so we have to swap values.
					case IMAGE_FORMAT_ARGB8888:
					case IMAGE_FORMAT_ABGR8888:
						if (io->read(io, face->Data, 1, face->SizeOfData) != face->SizeOfData) {
							bVtf = IL_FALSE;
							break;
						}
						else {
							bVtf = IL_TRUE;
						}
						// Swap the data
						for (k = 0; k < face->SizeOfData; k += 4) {
							SwapVal = face->Data[k];
							face->Data[k]   = face->Data[k+3];
							face->Data[k+3] = SwapVal;
							SwapVal = face->Data[k+1];
							face->Data[k+1] = face->Data[k+2];
							face->Data[k+2] = SwapVal;
						}
						break;

					// Uncompressed 16-bit RGB and BGR data.  We have to expand this to 24-bit, since
					//   DevIL does not handle this internally.
					//   The data is in the file as: gggbbbbb rrrrrrggg
					case IMAGE_FORMAT_RGB565:
					case IMAGE_FORMAT_BGR565:
						SizeOfData = face->Width * face->Height * face->Depth * 2;
						Data16Bit = CompData = (ILubyte*)ialloc(SizeOfData);  // Not compressed data
						if (CompData == NULL)
							return IL_FALSE;
						if (io->read(io, CompData, 1, SizeOfData) != SizeOfData) {
							bVtf = IL_FALSE;
							break;
						}
						for (k = 0; k < face->SizeOfData; k += 3) {
							face->Data[k]   =  (Data16Bit[0] & 0x1F) << 3;
							face->Data[k+1] = ((Data16Bit[1] & 0x07) << 5) | ((Data16Bit[0] & 0xE0) >> 3);
							face->Data[k+2] =   Data16Bit[1] & 0xF8;
							Data16Bit += 2;
						}
						break;

					// Uncompressed 16-bit BGRA data (1-bit alpha).  We have to expand this to 32-bit,
					//   since DevIL does not handle this internally.
					//   Something seems strange with this one, but this is how VTFEdit outputs.
					//   The data is in the file as: gggbbbbb arrrrrgg
					case IMAGE_FORMAT_BGRA5551:
						SizeOfData = face->Width * face->Height * face->Depth * 2;
						Data16Bit = CompData = (ILubyte*)ialloc(SizeOfData);  // Not compressed data
						if (CompData == NULL)
							return IL_FALSE;
						if (io->read(io, CompData, 1, SizeOfData) != SizeOfData) {
							bVtf = IL_FALSE;
							break;
						}
						for (k = 0; k < face->SizeOfData; k += 4) {
							face->Data[k]   =  (Data16Bit[0] & 0x1F) << 3;
							face->Data[k+1] = ((Data16Bit[0] & 0xE0) >> 2) | ((Data16Bit[1] & 0x03) << 6);
							face->Data[k+2] =  (Data16Bit[1] & 0x7C) << 1;
							// 1-bit alpha is either off or on.
							face->Data[k+3] = ((Data16Bit[0] & 0x80) == 0x80) ? 0xFF : 0x00;
							Data16Bit += 2;
						}
						break;

					// Same as above, but the alpha channel is unused.
					case IMAGE_FORMAT_BGRX5551:
						SizeOfData = face->Width * face->Height * face->Depth * 2;
						Data16Bit = CompData = (ILubyte*)ialloc(SizeOfData);  // Not compressed data
						if (io->read(io, CompData, 1, SizeOfData) != SizeOfData) {
							bVtf = IL_FALSE;
							break;
						}
						for (k = 0; k < face->SizeOfData; k += 3) {
							face->Data[k]   =  (Data16Bit[0] & 0x1F) << 3;
							face->Data[k+1] = ((Data16Bit[0] & 0xE0) >> 2) | ((Data16Bit[1] & 0x03) << 6);
							face->Data[k+2] =  (Data16Bit[1] & 0x7C) << 1;
							Data16Bit += 2;
						}
						break;

					// Data is reduced to a 4-bits per channel format.
					case IMAGE_FORMAT_BGRA4444:
						SizeOfData = face->Width * face->Height * face->Depth * 4;
						Temp = CompData = (ILubyte*)ialloc(SizeOfData / 2);  // Not compressed data
						if (CompData == NULL)
							return IL_FALSE;
						if (io->read(io, CompData, 1, SizeOfData / 2) != SizeOfData / 2) {
							bVtf = IL_FALSE;
							break;
						}
						for (k = 0; k < SizeOfData; k += 4) {
							// We double the data here.
							face->Data[k]   = (Temp[0] & 0x0F) << 4 | (Temp[0] & 0x0F);
							face->Data[k+1] = (Temp[0] & 0xF0) >> 4 | (Temp[0] & 0xF0);
							face->Data[k+2] = (Temp[1] & 0x0F) << 4 | (Temp[1] & 0x0F);
							face->Data[k+3] = (Temp[1] & 0xF0) >> 4 | (Temp[1] & 0xF0);
							Temp += 2;
						}
						break;
				}

				ifree(CompData);
				CompData = NULL;
				if (bVtf == IL_FALSE)  //@TODO: Do we need to do any cleanup here?
					return IL_FALSE;
			}
		}
	}

	return il2FixImage(baseImage);
}


ILuint GetFaceFlag(ILuint FaceNum)
{
	switch (FaceNum)
	{
		case 0:
			return IL_CUBEMAP_POSITIVEX;
		case 1:
			return IL_CUBEMAP_NEGATIVEX;
		case 2:
			return IL_CUBEMAP_POSITIVEY;
		case 3:
			return IL_CUBEMAP_NEGATIVEY;
		case 4:
			return IL_CUBEMAP_POSITIVEZ;
		case 5:
			return IL_CUBEMAP_NEGATIVEZ;
		case 6:
			return IL_SPHEREMAP;
	}

	return IL_SPHEREMAP;  // Should never reach here!
}


ILboolean VtfInitFacesMipmaps(ILimage *BaseImage, ILuint NumFaces, VTFHEAD *Header)
{
	ILimage	*Image;
	ILuint	Face;

	// Initialize mipmaps under the base image.
	VtfInitMipmaps(BaseImage, Header);
	Image = BaseImage;

	// We have an environment map.
	if (NumFaces != 1) {
		Image->CubeFlags = IL_CUBEMAP_POSITIVEX;
	}

	for (Face = 1; Face < NumFaces; Face++) {
		Image->Faces = ilNewImageFull(Image->Width, Image->Height, Image->Depth, Image->Bpp, Image->Format, Image->Type, NULL);
		if (Image->Faces == NULL)
			return IL_FALSE;
		Image = Image->Faces;

		// The origin should be in the upper left.
		Image->Origin = IL_ORIGIN_UPPER_LEFT;
		// Set the flags that tell which face this is.
		Image->CubeFlags = GetFaceFlag(Face);

		// Now we can initialize the mipmaps under each face.
		VtfInitMipmaps(Image, Header);
	}

	return IL_TRUE;
}


ILboolean VtfInitMipmaps(ILimage *BaseImage, VTFHEAD *Header)
{
	ILimage	*Image;
	ILuint	Width, Height, Depth, Mipmap;

	Image = BaseImage;
	Width = BaseImage->Width;  Height = BaseImage->Height;  Depth = BaseImage->Depth;

	for (Mipmap = 1; Mipmap < Header->MipmapCount; Mipmap++) {
		// 1 is the smallest dimension possible.
		Width = (Width >> 1) == 0 ? 1 : (Width >> 1);
		Height = (Height >> 1) == 0 ? 1 : (Height >> 1);
		Depth = (Depth >> 1) == 0 ? 1 : (Depth >> 1);

		Image->Mipmaps = ilNewImageFull(Width, Height, Depth, BaseImage->Bpp, BaseImage->Format, BaseImage->Type, NULL);
		if (Image->Mipmaps == NULL)
			return IL_FALSE;
		Image = Image->Mipmaps;

		// ilNewImage does not set these.
		Image->Format = BaseImage->Format;
		Image->Type = BaseImage->Type;
		// The origin should be in the upper left.
		Image->Origin = IL_ORIGIN_UPPER_LEFT;
	}

	return IL_TRUE;
}


// Internal function used to save the Vtf.
ILboolean iSaveVtfInternal(ILimage* image)
{
	ILimage	*TempImage = image;
	ILubyte	*TempData, *CompData;
	ILuint	Format, i, CompSize;
	ILenum	Compression;
	SIO* io = &image->io;

	// Find out if the user has specified to use DXT compression.
	Compression = il2GetInteger(IL_VTF_COMP);

	//@TODO: Other formats
	if (Compression == IL_DXT_NO_COMP) {
		switch (TempImage->Format)
		{
			case IL_RGB:
				Format = IMAGE_FORMAT_RGB888;
				break;
			case IL_RGBA:
				Format = IMAGE_FORMAT_RGBA8888;
				break;
			case IL_BGR:
				Format = IMAGE_FORMAT_BGR888;
				break;
			case IL_BGRA:
				Format = IMAGE_FORMAT_BGRA8888;
				break;
			case IL_LUMINANCE:
				Format = IMAGE_FORMAT_I8;
				break;
			case IL_ALPHA:
				Format = IMAGE_FORMAT_A8;
				break;
			case IL_LUMINANCE_ALPHA:
				Format = IMAGE_FORMAT_IA88;
				break;
			//case IL_COLOUR_INDEX:
			default:
				Format = IMAGE_FORMAT_BGRA8888;
				TempImage = iConvertImage(image, IL_BGRA, IL_UNSIGNED_BYTE);
				if (TempImage == NULL)
					return IL_FALSE;
		}

		//@TODO: When we have the half format available internally, also use IMAGE_FORMAT_RGBA16161616F.
		if (TempImage->Format == IL_RGBA && TempImage->Type == IL_UNSIGNED_SHORT) {
			Format = IMAGE_FORMAT_RGBA16161616;
		}
		else if (TempImage->Type != IL_UNSIGNED_BYTE) {  //@TODO: Any possibility for shorts, etc. to be used?
			TempImage = iConvertImage(image, Format, IL_UNSIGNED_BYTE);
			if (TempImage == NULL)
				return IL_FALSE;
		}
	}
	else {  // We are using DXT compression.
		switch (Compression)
		{
			case IL_DXT1:
				Format = IMAGE_FORMAT_DXT1_ONEBITALPHA;//IMAGE_FORMAT_DXT1;
				break;
			case IL_DXT3:
				Format = IMAGE_FORMAT_DXT3;
				break;
			case IL_DXT5:
				Format = IMAGE_FORMAT_DXT5;
				break;
			default:  // Should never reach this point.
				il2SetError(IL_INTERNAL_ERROR);
				Format = IMAGE_FORMAT_DXT5;
		}
	}

	if (TempImage->Origin != IL_ORIGIN_UPPER_LEFT) {
		TempData = iGetFlipped(TempImage);
		if (TempData == NULL) {
			ilCloseImage(TempImage);
			return IL_FALSE;
		}
	} else {
		TempData = TempImage->Data;
	}

	// @todo: use VTFHEAD structure for writing
	// Write the file signature.
	io->write("VTF", 1, 4, io);
	// Write the file version - currently using 7.2 specs.
	SaveLittleUInt(io, 7);
	SaveLittleUInt(io, 2);
	// Write the header size.
	SaveLittleUInt(io, 80);
	// Now we write the width and height of the image.
	SaveLittleUShort(io, TempImage->Width);
	SaveLittleUShort(io, TempImage->Height);
	//@TODO: This is supposed to be the flags used.  What should we use here?  Let users specify?
	SaveLittleUInt(io, 0);
	// Number of frames in the animation. - @TODO: Change to use animations.
	SaveLittleUShort(io, 1);
	// First frame in the animation
	SaveLittleUShort(io, 0);
	// Padding
	SaveLittleUInt(io, 0);
	// Reflectivity (3 floats) - @TODO: Use what values?  User specified?
	SaveLittleFloat(io, 0.0f);
	SaveLittleFloat(io, 0.0f);
	SaveLittleFloat(io, 0.0f);
	// Padding
	SaveLittleUInt(io, 0);
	// Bumpmap scale
	SaveLittleFloat(io, 0.0f);
	// Image format
	SaveLittleUInt(io, Format);
	// Mipmap count - @TODO: Use mipmaps
	io->putc(1, io);
	// Low resolution image format - @TODO: Create low resolution image.
	SaveLittleUInt(io, 0xFFFFFFFF);
	// Low resolution image width and height
	io->putc(0, io);
	io->putc(0, io);
	// Depth of the image - @TODO: Support for volumetric images.
	SaveLittleUShort(io, 1);

	// Write final padding for the header (out to 80 bytes).
	for (i = 0; i < 15; i++) {
		io->putc(0, io);
	}

	if (Compression == IL_DXT_NO_COMP) {
		// We just write the image data directly.
		if (io->write(TempImage->Data, TempImage->SizeOfData, 1, io) != 1)
			return IL_FALSE;
	}
	else {  // Do DXT compression here and write.
		// We have to find out how much we are writing first.
		CompSize = il2GetDXTCData(image, NULL, 0, Compression);
		if (CompSize == 0) {
			il2SetError(IL_INTERNAL_ERROR);
			if (TempData != TempImage->Data)
				ifree(TempData);
			return IL_FALSE;
		}
		CompData = (ILubyte*)ialloc(CompSize);
		if (CompData == NULL) {
			if (TempData != TempImage->Data)
				ifree(TempData);
			return IL_FALSE;
		}

		// DXT compress the data.
		CompSize = ilGetDXTCData(CompData, CompSize, Compression);
		if (CompSize == 0) {
			il2SetError(IL_INTERNAL_ERROR);
			if (TempData != TempImage->Data)
				ifree(TempData);
			return IL_FALSE;
		}
		// Finally write the data.
		if (io->write(CompData, CompSize, 1, io) != 1) {
			ifree(CompData);
			if (TempData != TempImage->Data)
				ifree(TempData);
			return IL_FALSE;
		}
	}

	if (TempData != TempImage->Data)
		ifree(TempData);
	if (TempImage != image)
		ilCloseImage(TempImage);

	return IL_TRUE;
}


#endif//IL_NO_VTF
