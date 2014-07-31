//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 02/26/2009
//
// Filename: src-IL/src/il_dpx.c
//
// Description: Reads from a Digital Picture Exchange (.dpx) file.
//				Specifications for this format were	found at
//				http://www.cineon.com/ff_draft.php and
//				http://www.fileformat.info/format/dpx/.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#ifndef IL_NO_DPX
#include "il_dpx.h"
#include "il_bits.h"


ILboolean DpxGetFileInfo(SIO* io, DPX_FILE_INFO *FileInfo)
{
	FileInfo->MagicNum = GetBigUInt(io);
	FileInfo->Offset = GetBigUInt(io);
	io->read(io, FileInfo->Vers, 8, 1);
	FileInfo->FileSize = GetBigUInt(io);
	FileInfo->DittoKey = GetBigUInt(io);
	FileInfo->GenHdrSize = GetBigUInt(io);
	FileInfo->IndHdrSize = GetBigUInt(io);
	FileInfo->UserDataSize = GetBigUInt(io);
	io->read(io, FileInfo->FileName, 100, 1);
	io->read(io, FileInfo->CreateTime, 24, 1);
	io->read(io, FileInfo->Creator, 100, 1);
	io->read(io, FileInfo->Project, 200, 1);
	if (io->read(io, FileInfo->Copyright, 200, 1) != 1)
		return IL_FALSE;
	FileInfo->Key = GetBigUInt(io);
	io->seek(io, 104, IL_SEEK_CUR);  // Skip reserved section.

	return IL_TRUE;
}


ILboolean GetImageElement(SIO* io, DPX_IMAGE_ELEMENT *ImageElement)
{
	ImageElement->DataSign = GetBigUInt(io);
	ImageElement->RefLowData = GetBigUInt(io);
	io->read(io, &ImageElement->RefLowQuantity, 1, 4);
	ImageElement->RefHighData = GetBigUInt(io);
	io->read(io, &ImageElement->RefHighQuantity, 1, 4);
	ImageElement->Descriptor = io->getc(io);
	ImageElement->Transfer = io->getc(io);
	ImageElement->Colorimetric = io->getc(io);
	ImageElement->BitSize = io->getc(io);
	ImageElement->Packing = GetBigUShort(io);
	ImageElement->Encoding = GetBigUShort(io);
	ImageElement->DataOffset = GetBigUInt(io);
	ImageElement->EolPadding = GetBigUInt(io);
	ImageElement->EoImagePadding = GetBigUInt(io);
	if (io->read(io, ImageElement->Description, 32, 1) != 1)
		return IL_FALSE;

	return IL_TRUE;
}


ILboolean DpxGetImageInfo(SIO* io, DPX_IMAGE_INFO *ImageInfo)
{
	ILuint i;

	ImageInfo->Orientation = GetBigUShort(io);
	ImageInfo->NumElements = GetBigUShort(io);
	ImageInfo->Width = GetBigUInt(io);
	ImageInfo->Height = GetBigUInt(io);

	for (i = 0; i < 8; i++) {
		GetImageElement(io, &ImageInfo->ImageElement[i]);
	}

	io->seek(io, 52, IL_SEEK_CUR);  // Skip padding bytes.

	return IL_TRUE;
}


ILboolean DpxGetImageOrient(SIO* io, DPX_IMAGE_ORIENT *ImageOrient)
{
	ImageOrient->XOffset = GetBigUInt(io);
	ImageOrient->YOffset = GetBigUInt(io);
	io->read(io, &ImageOrient->XCenter, 4, 1);
	io->read(io, &ImageOrient->YCenter, 4, 1);
	ImageOrient->XOrigSize = GetBigUInt(io);
	ImageOrient->YOrigSize = GetBigUInt(io);
	io->read(io, ImageOrient->FileName, 100, 1);
	io->read(io, ImageOrient->CreationTime, 24, 1);
	io->read(io, ImageOrient->InputDev, 32, 1);
	if (io->read(io, ImageOrient->InputSerial, 32, 1) != 1)
		return IL_FALSE;
	ImageOrient->Border[0] = GetBigUShort(io);
	ImageOrient->Border[1] = GetBigUShort(io);
	ImageOrient->Border[2] = GetBigUShort(io);
	ImageOrient->Border[3] = GetBigUShort(io);
	ImageOrient->PixelAspect[0] = GetBigUInt(io);
	ImageOrient->PixelAspect[1] = GetBigUInt(io);
	io->seek(io, 28, IL_SEEK_CUR);  // Skip reserved bytes.

	return IL_TRUE;
}


// Internal function used to load the DPX.
ILboolean iLoadDpxInternal(ILimage* image)
{
	DPX_FILE_INFO		FileInfo;
	DPX_IMAGE_INFO		ImageInfo;
	DPX_IMAGE_ORIENT	ImageOrient;
//	BITFILE		*File;
	ILuint		i, NumElements, CurElem = 0;
	ILushort	Val, *ShortData;
	ILubyte		Data[8];
	ILenum		Format = 0;
	ILubyte		NumChans = 0;
	SIO* io = &image->io;


	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	
	if (!DpxGetFileInfo(io, &FileInfo))
		return IL_FALSE;
	if (!DpxGetImageInfo(io, &ImageInfo))
		return IL_FALSE;
	if (!DpxGetImageOrient(io, &ImageOrient))
		return IL_FALSE;

	io->seek(io, ImageInfo.ImageElement[CurElem].DataOffset, IL_SEEK_SET);

//@TODO: Deal with different origins!

	switch (ImageInfo.ImageElement[CurElem].Descriptor)
	{
		case 6:  // Luminance data
			Format = IL_LUMINANCE;
			NumChans = 1;
			break;
		case 50:  // RGB data
			Format = IL_RGB;
			NumChans = 3;
			break;
		case 51:  // RGBA data
			Format = IL_RGBA;
			NumChans = 4;
			break;
		default:
			il2SetError(IL_FORMAT_NOT_SUPPORTED);
			return IL_FALSE;
	}

	// These are all on nice word boundaries.
	switch (ImageInfo.ImageElement[CurElem].BitSize)
	{
		case 8:
		case 16:
		case 32:
			if (!il2TexImage(image, ImageInfo.Width, ImageInfo.Height, 1, NumChans, Format, IL_UNSIGNED_BYTE, NULL))
				return IL_FALSE;
			image->Origin = IL_ORIGIN_UPPER_LEFT;
			if (io->read(io, image->Data, image->SizeOfData, 1) != 1)
				return IL_FALSE;
			goto finish;
	}

	// The rest of these do not end on word boundaries.
	if (ImageInfo.ImageElement[CurElem].Packing == 1) {
		// Here we have it padded out to a word boundary, so the extra bits are filler.
		switch (ImageInfo.ImageElement[CurElem].BitSize)
		{
			case 10:
				//@TODO: Support other formats!
				/*if (Format != IL_RGB) {
					il2SetError(IL_FORMAT_NOT_SUPPORTED);
					return IL_FALSE;
				}*/
				switch (Format)
				{
					case IL_LUMINANCE:
						if (!il2TexImage(image, ImageInfo.Width, ImageInfo.Height, 1, 1, IL_LUMINANCE, IL_UNSIGNED_SHORT, NULL))
							return IL_FALSE;
						image->Origin = IL_ORIGIN_UPPER_LEFT;
						ShortData = (ILushort*)image->Data;
						NumElements = image->SizeOfData / 2;

						for (i = 0; i < NumElements;) {
							io->read(io, Data, 1, 2);
							Val = ((Data[0] << 2) + ((Data[1] & 0xC0) >> 6)) << 6;  // Use the first 10 bits of the word-aligned data.
							ShortData[i++] = Val | ((Val & 0x3F0) >> 4);  // Fill in the lower 6 bits with a copy of the higher bits.
						}
						break;

					case IL_RGB:
						if (!il2TexImage(image, ImageInfo.Width, ImageInfo.Height, 1, 3, IL_RGB, IL_UNSIGNED_SHORT, NULL))
							return IL_FALSE;
						image->Origin = IL_ORIGIN_UPPER_LEFT;
						ShortData = (ILushort*)image->Data;
						NumElements = image->SizeOfData / 2;

						for (i = 0; i < NumElements;) {
							io->read(io, Data, 1, 4);
							Val = ((Data[0] << 2) + ((Data[1] & 0xC0) >> 6)) << 6;  // Use the first 10 bits of the word-aligned data.
							ShortData[i++] = Val | ((Val & 0x3F0) >> 4);  // Fill in the lower 6 bits with a copy of the higher bits.
							Val = (((Data[1] & 0x3F) << 4) + ((Data[2] & 0xF0) >> 4)) << 6;  // Use the next 10 bits.
							ShortData[i++] = Val | ((Val & 0x3F0) >> 4);  // Same fill
							Val = (((Data[2] & 0x0F) << 6) + ((Data[3] & 0xFC) >> 2)) << 6;  // And finally use the last 10 bits (ignores the last 2 bits).
							ShortData[i++] = Val | ((Val & 0x3F0) >> 4);  // Same fill
						}
						break;

					case IL_RGBA:  // Is this even a possibility?  There is a ton of wasted space here!
						if (!il2TexImage(image, ImageInfo.Width, ImageInfo.Height, 1, 4, IL_RGBA, IL_UNSIGNED_SHORT, NULL))
							return IL_FALSE;
						image->Origin = IL_ORIGIN_UPPER_LEFT;
						ShortData = (ILushort*)image->Data;
						NumElements = image->SizeOfData / 2;

						for (i = 0; i < NumElements;) {
							io->read(io, Data, 1, 8);
							Val = (Data[0] << 2) + ((Data[1] & 0xC0) >> 6);  // Use the first 10 bits of the word-aligned data.
							ShortData[i++] = (Val << 6) | ((Val & 0x3F0) >> 4);  // Fill in the lower 6 bits with a copy of the higher bits.
							Val = ((Data[1] & 0x3F) << 4) + ((Data[2] & 0xF0) >> 4);  // Use the next 10 bits.
							ShortData[i++] = (Val << 6) | ((Val & 0x3F0) >> 4);  // Same fill
							Val = ((Data[2] & 0x0F) << 6) + ((Data[3] & 0xFC) >> 2);  // Use the next 10 bits.
							ShortData[i++] = (Val << 6) | ((Val & 0x3F0) >> 4);  // Same fill
							Val = ((Data[3] & 0x03) << 8) + Data[4];  // And finally use the last 10 relevant bits (skips 3 whole bytes worth of padding!).
							ShortData[i++] = (Val << 6) | ((Val & 0x3F0) >> 4);  // Last fill
						}
						break;
				}
				break;

			//case 1:
			//case 12:
			default:
				il2SetError(IL_FORMAT_NOT_SUPPORTED);
				return IL_FALSE;
		}
	}
	else if (ImageInfo.ImageElement[0].Packing == 0) {
		// Here we have the data packed so that it is never aligned on a word boundary.
		/*File = bfile(iGetFile());
		if (File == NULL)
			return IL_FALSE;  //@TODO: Error?
		ShortData = (ILushort*)image->Data;
		NumElements = image->SizeOfData / 2;
		for (i = 0; i < NumElements; i++) {
			//bread(&Val, 1, 10, File);
			Val = breadVal(10, File);
			ShortData[i] = (Val << 6) | (Val >> 4);
		}
		bclose(File);*/

		il2SetError(IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;
	}
	else {
		il2SetError(IL_ILLEGAL_FILE_VALUE);
		return IL_FALSE;  //@TODO: Take care of this in an iCheckDpx* function.
	}

finish:
	return il2FixImage(image);
}

#endif//IL_NO_DPX

