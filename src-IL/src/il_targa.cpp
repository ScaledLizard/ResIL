//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_targa.c
//
// Description: Reads from and writes to a Targa (.tga) file.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_TGA
#include "il_targa.h"
#include <string.h>
#include "il_manip.h"
#include "il_bits.h"
#include "il_endian.h"

#ifdef DJGPP
#include <dos.h>
#endif

ILint64 iGetTgaHead(SIO* io, TARGAHEAD *Header)
{
	return io->read(io, Header, 1, sizeof(TARGAHEAD));
}

// Internal function used to check if the HEADER is a valid Targa header.
ILboolean iCheckTarga(TARGAHEAD *Header)
{
	if (Header->Width == 0 || Header->Height == 0)
		return IL_FALSE;
	if (Header->Bpp != 8 && Header->Bpp != 15 && Header->Bpp != 16 
		&& Header->Bpp != 24 && Header->Bpp != 32)
		return IL_FALSE;
	if (Header->ImageDesc & BIT_4)	// Supposed to be set to 0
		return IL_FALSE;
	
	// check type (added 20040218)
	if (Header->ImageType   != TGA_NO_DATA
	   && Header->ImageType != TGA_COLMAP_UNCOMP
	   && Header->ImageType != TGA_UNMAP_UNCOMP
	   && Header->ImageType != TGA_BW_UNCOMP
	   && Header->ImageType != TGA_COLMAP_COMP
	   && Header->ImageType != TGA_UNMAP_COMP
	   && Header->ImageType != TGA_BW_COMP)
		return IL_FALSE;
	
	// Doesn't work well with the bitshift so change it.
	if (Header->Bpp == 15)
		Header->Bpp = 16;
	
	return IL_TRUE;
}


// Internal function to get the header and check it.
ILboolean iIsValidTarga(SIO* io)
{
	TARGAHEAD	Head;
	auto read = iGetTgaHead(io, &Head);
	io->seek(io, -read, IL_SEEK_CUR);

	if (read == sizeof(Head)) 
		return iCheckTarga(&Head);
	else
		return IL_FALSE;
}


ILboolean iUncompressTgaData(ILimage *image)
{
	ILuint	BytesRead = 0, Size, RunLen, i, ToRead;
	ILubyte Header, Color[4];
	ILint	c;
	
	Size = image->Width * image->Height * image->Depth * image->Bpp;
	
	while (BytesRead < Size) {
		Header = (ILubyte)image->io.getc(&image->io);
		if (Header & BIT_7) {
			ClearBits(Header, BIT_7);
			if (image->io.read(&image->io, Color, 1, image->Bpp) != image->Bpp) {
				return IL_FALSE;
			}
			RunLen = (Header+1) * image->Bpp;
			for (i = 0; i < RunLen; i += image->Bpp) {
				// Read the color in, but we check to make sure that we do not go past the end of the image.
				for (c = 0; c < image->Bpp && BytesRead+i+c < Size; c++) {
					image->Data[BytesRead+i+c] = Color[c];
				}
			}
			BytesRead += RunLen;
		}
		else {
			RunLen = (Header+1) * image->Bpp;
			// We have to check that we do not go past the end of the image data.
			if (BytesRead + RunLen > Size)
				ToRead = Size - BytesRead;
			else
				ToRead = RunLen;
			if (image->io.read(&image->io, image->Data + BytesRead, 1, ToRead) != ToRead) {
				return IL_FALSE;
			}
			BytesRead += RunLen;

			if (BytesRead + RunLen > Size)
				image->io.seek(&image->io, RunLen - ToRead, IL_SEEK_CUR);
		}
	}
	
	return IL_TRUE;
}


ILboolean iReadColMapTga(ILimage* image, TARGAHEAD *Header)
{
	char		ID[255];
	ILushort	Pixel;
	
	if (image->io.read(&image->io, ID, 1, Header->IDLen) != Header->IDLen)
		return IL_FALSE;
	
	if (!il2TexImage(image, Header->Width, Header->Height, 1, (ILubyte)(Header->Bpp >> 3), 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	image->Format = IL_COLOUR_INDEX;
	
	switch (Header->ColMapEntSize)
	{
		case 16:
			image->Pal.use(Header->ColMapLen, NULL, IL_PAL_BGRA32);
			break;
		case 24:
			image->Pal.use(Header->ColMapLen, NULL, IL_PAL_BGR24);
			break;
		case 32:
			image->Pal.use(Header->ColMapLen, NULL, IL_PAL_BGRA32);
			break;
		default:
			// Should *never* reach here
			il2SetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
	}
	
	// Do we need to do something with FirstEntry?	Like maybe:
	//	image->io.read(&image->io, Image->Pal + Targa->FirstEntry, 1, Image->Pal.PalSize);  ??
	if (Header->ColMapEntSize != 16)
	{
		//if (image->io.read(&image->io, image->Pal.Palette, 1, image->Pal.PalSize) != image->Pal.PalSize)
			return IL_FALSE;
	}
	else {
		// 16 bit palette, so we have to break it up. Bit order: A1 R5 G5 B5
		for (ILshort i = 0; i < Header->ColMapLen; ++i)
		{
			Pixel = GetBigUShort(&image->io);
			if (image->io.eof(&image->io))
				return IL_FALSE;
			ILubyte b = (Pixel & 0xFC00) >> 7;
			ILubyte g = (Pixel & 0x03E0) >> 2;
			ILubyte r = (Pixel & 0x001F) << 3;
			ILubyte a = (Pixel & 0x8000) >> 12;
			image->Pal.setRGBA(i, r, g, b, a);
		}
	}
	
	if (Header->ImageType == TGA_COLMAP_COMP)
	{
		if (!iUncompressTgaData(image))
		{
			return IL_FALSE;
		}
	}
	else
	{
		if (image->io.read(&image->io, image->Data, 1, image->SizeOfData) != image->SizeOfData)
		{
			return IL_FALSE;
		}
	}
	
	return IL_TRUE;
}


// Pretty damn unoptimized
ILboolean i16BitTarga(ILimage *image)
{
	ILushort	*Temp1;
	ILubyte 	*Data, *Temp2;
	ILuint		x, PixSize = image->Width * image->Height;
	
	Data = (ILubyte*)ialloc(image->Width * image->Height * 3);
	Temp1 = (ILushort*)image->Data;
	Temp2 = Data;
	
	if (Data == NULL)
		return IL_FALSE;
	
	for (x = 0; x < PixSize; x++) {
		*Temp2++ = (*Temp1 & 0x001F) << 3;	// Blue
		*Temp2++ = (*Temp1 & 0x03E0) >> 2;	// Green
		*Temp2++ = (*Temp1 & 0x7C00) >> 7;	// Red
		
		Temp1++;
		
		
		/*s = *Temp;
		s = SwapShort(s);
		a = !!(s & BIT_15);
		
		s = s << 1;
		
		//if (a) {
		SetBits(s, BIT_0);
		//}
		
		//SetBits(s, BIT_15);
		
		*Temp++ = s;*/
	}
	
	if (!il2TexImage(image, image->Width, image->Height, 1, 3, IL_BGR, IL_UNSIGNED_BYTE, Data)) {
		ifree(Data);
		return IL_FALSE;
	}
	
	ifree(Data);
	
	return IL_TRUE;
}


ILboolean iReadUnmapTga(ILimage* image, TARGAHEAD *Header)
{
	ILubyte Bpp;
	char	ID[255];
	
	if (image->io.read(&image->io, ID, 1, Header->IDLen) != Header->IDLen)
		return IL_FALSE;
	
	/*if (Header->Bpp == 16)
		Bpp = 3;
	else*/
	Bpp = (ILubyte)(Header->Bpp >> 3);
	
	if (!il2TexImage(image, Header->Width, Header->Height, 1, Bpp, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	
	switch (image->Bpp)
	{
		case 1:
			image->Format = IL_COLOUR_INDEX;  // wtf?  How is this possible?
			break;
		case 2:  // 16-bit is not supported directly!
				 //image->Format = IL_RGB5_A1;
			/*image->Format = IL_RGBA;
			image->Type = IL_UNSIGNED_SHORT_5_5_5_1_EXT;*/
			//image->Type = IL_UNSIGNED_SHORT_5_6_5_REV;
			
			// Remove?
			//ilCloseImage(image);
			//il2SetError(IL_FORMAT_NOT_SUPPORTED);
			//return IL_FALSE;
			
			/*image->Bpp = 4;
			image->Format = IL_BGRA;
			image->Type = IL_UNSIGNED_SHORT_1_5_5_5_REV;*/
			
			image->Format = IL_BGR;
			
			break;
		case 3:
			image->Format = IL_BGR;
			break;
		case 4:
			image->Format = IL_BGRA;
			break;
		default:
			il2SetError(IL_INVALID_VALUE);
			return IL_FALSE;
	}
	
	
	// @TODO:  Determine this:
	// We assume that no palette is present, but it's possible...
	//	Should we mess with it or not?
	
	
	if (Header->ImageType == TGA_UNMAP_COMP) {
		if (!iUncompressTgaData(image)) {
			return IL_FALSE;
		}
	}
	else {
		if (image->io.read(&image->io, image->Data, 1, image->SizeOfData) != image->SizeOfData) {
			return IL_FALSE;
		}
	}
	
	// Go ahead and expand it to 24-bit.
	if (Header->Bpp == 16) {
		if (!i16BitTarga(image))
			return IL_FALSE;
		return IL_TRUE;
	}
	
	return IL_TRUE;
}


ILboolean iReadBwTga(ILimage* image, TARGAHEAD *Header)
{
	char ID[255];
	
	if (image->io.read(&image->io, ID, 1, Header->IDLen) != Header->IDLen)
		return IL_FALSE;
	
	// We assume that no palette is present, but it's possible...
	//	Should we mess with it or not?
	
	if (!il2TexImage(image, Header->Width, Header->Height, 1, (ILubyte)(Header->Bpp >> 3), IL_LUMINANCE, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	
	if (Header->ImageType == TGA_BW_COMP) {
		if (!iUncompressTgaData(image)) {
			return IL_FALSE;
		}
	}
	else {
		if (image->io.read(&image->io, image->Data, 1, image->SizeOfData) != image->SizeOfData) {
			return IL_FALSE;
		}
	}
	
	return IL_TRUE;
}


// Internal function used to load the Targa.
ILboolean iLoadTargaInternal(ILimage* image)
{
	TARGAHEAD	Header;
	ILboolean	bTarga;
	ILenum		iOrigin;
	
	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	
	if (!iGetTgaHead(&image->io, &Header))
		return IL_FALSE;
	if (!iCheckTarga(&Header)) {
		il2SetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}
	
	switch (Header.ImageType)
	{
		case TGA_NO_DATA:
			il2SetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
		case TGA_COLMAP_UNCOMP:
		case TGA_COLMAP_COMP:
			bTarga = iReadColMapTga(image, &Header);
			break;
		case TGA_UNMAP_UNCOMP:
		case TGA_UNMAP_COMP:
			bTarga = iReadUnmapTga(image, &Header);
			break;
		case TGA_BW_UNCOMP:
		case TGA_BW_COMP:
			bTarga = iReadBwTga(image, &Header);
			break;
		default:
			il2SetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
	}

	if (bTarga==IL_FALSE)
		return IL_FALSE;
	
	// @JASON Extra Code to manipulate the image depending on
	// the Image Descriptor's origin bits.
	iOrigin = Header.ImageDesc & IMAGEDESC_ORIGIN_MASK;
	
	switch (iOrigin)
	{
		case IMAGEDESC_TOPLEFT:
			image->Origin = IL_ORIGIN_UPPER_LEFT;
			break;
			
		case IMAGEDESC_TOPRIGHT:
			image->Origin = IL_ORIGIN_UPPER_LEFT;
			iMirror(image);
			break;
			
		case IMAGEDESC_BOTLEFT:
			image->Origin = IL_ORIGIN_LOWER_LEFT;
			break;
			
		case IMAGEDESC_BOTRIGHT:
			image->Origin = IL_ORIGIN_LOWER_LEFT;
			iMirror(image);
			break;
	}
	
	return il2FixImage(image);
}


//changed name to iGetDateTime on 20031221 to fix bug 830196
void iGetDateTime(ILuint *Month, ILuint *Day, ILuint *Yr, ILuint *Hr, ILuint *Min, ILuint *Sec)
{
#ifdef DJGPP
	struct date day;
	struct time curtime;
	
	gettime(&curtime);
	getdate(&day);
	
	*Month = day.da_mon;
	*Day = day.da_day;
	*Yr = day.da_year;
	
	*Hr = curtime.ti_hour;
	*Min = curtime.ti_min;
	*Sec = curtime.ti_sec;
	
	return;
#else
	
#ifdef _WIN32
	SYSTEMTIME Time;
	
	GetSystemTime(&Time);
	
	*Month = Time.wMonth;
	*Day = Time.wDay;
	*Yr = Time.wYear;
	
	*Hr = Time.wHour;
	*Min = Time.wMinute;
	*Sec = Time.wSecond;
	
	return;
#else
	
	*Month = 0;
	*Day = 0;
	*Yr = 0;
	
	*Hr = 0;
	*Min = 0;
	*Sec = 0;
	
	return;
#endif
#endif
}


// Internal function used to save the Targa.
// @todo: write header in one read() call
ILboolean iSaveTargaInternal(ILimage* image)
{
	const char	*ID = iGetString(IL_TGA_ID_STRING);
	const char	*AuthName = iGetString(IL_TGA_AUTHNAME_STRING);
	const char	*AuthComment = iGetString(IL_TGA_AUTHCOMMENT_STRING);
	ILubyte 	IDLen = 0, UsePal, Type, PalEntSize;
	ILshort 	ColMapStart = 0, PalSize;
	ILubyte		Temp;
	ILenum		Format;
	ILboolean	Compress;
	ILuint		RleLen;
	ILubyte 	*Rle;
	ILpal		TempPal;
	ILimage 	*TempImage = NULL;
	ILuint		ExtOffset, i;
	char		*Footer = "TRUEVISION-XFILE.\0";
	char		*idString = "Developer's Image Library (DevIL)";
	ILuint		Day, Month, Year, Hour, Minute, Second;
	char		*TempData;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	
	if (iGetInt(IL_TGA_RLE) == IL_TRUE)
		Compress = IL_TRUE;
	else
		Compress = IL_FALSE;
	
	if (ID)
		IDLen = (ILubyte)ilCharStrLen(ID);
	
	UsePal = image->Pal.hasPalette();
	
	image->io.write(&IDLen, sizeof(ILubyte), 1, &image->io);
	image->io.write(&UsePal, sizeof(ILubyte), 1, &image->io);

	Format = image->Format;
	switch (Format) {
		case IL_COLOUR_INDEX:
			if (Compress)
				Type = 9;
			else
				Type = 1;
			break;
		case IL_BGR:
		case IL_BGRA:
			if (Compress)
				Type = 10;
			else
				Type = 2;
			break;
		case IL_RGB:
		case IL_RGBA:
			ilSwapColours();
			if (Compress)
				Type = 10;
			else
				Type = 2;
			break;
		case IL_LUMINANCE:
			if (Compress)
				Type = 11;
			else
				Type = 3;
			break;
		default:
			// Should convert the types here...
			il2SetError(IL_INVALID_VALUE);
			ifree(ID);
			ifree(AuthName);
			ifree(AuthComment);
			return IL_FALSE;
	}
	
	image->io.write(&Type, sizeof(ILubyte), 1, &image->io);
	SaveLittleShort(&image->io, ColMapStart);
	
	switch (image->Pal.getPalType())
	{
		case IL_PAL_NONE:
			PalSize = 0;
			PalEntSize = 0;
			break;
		case IL_PAL_BGR24:
			PalSize = (ILshort)(image->Pal.getPalSize() / 3);
			PalEntSize = 24;
			TempPal = image->Pal;
			break;
			
		case IL_PAL_RGB24:
		case IL_PAL_RGB32:
		case IL_PAL_RGBA32:
		case IL_PAL_BGR32:
		case IL_PAL_BGRA32:
			TempPal = iConvertPal(&image->Pal, IL_PAL_BGR24);
			PalSize = (ILshort)(TempPal.getNumCols());
			PalEntSize = 24;
			break;
		default:
			il2SetError(IL_INVALID_VALUE);
			ifree(ID);
			ifree(AuthName);
			ifree(AuthComment);
			PalSize = 0;
			PalEntSize = 0;
			return IL_FALSE;
	}
	SaveLittleShort(&image->io, PalSize);
	image->io.write(&PalEntSize, sizeof(ILubyte), 1, &image->io);
	
	if (image->Bpc > 1) {
		TempImage = iConvertImage(image, image->Format, IL_UNSIGNED_BYTE);
		if (TempImage == NULL) {
			ifree(ID);
			ifree(AuthName);
			ifree(AuthComment);
			return IL_FALSE;
		}
	}
	else {
		TempImage = image;
	}
	
	if (TempImage->Origin != IL_ORIGIN_LOWER_LEFT)
		TempData = (char*)iGetFlipped(TempImage);
	else
		TempData = (char*)TempImage->Data;
	
	// Write out the origin stuff.
	Temp = 0;
	image->io.write(&Temp, sizeof(ILshort), 1, &image->io);
	image->io.write(&Temp, sizeof(ILshort), 1, &image->io);
	
	Temp = image->Bpp << 3;  // Changes to bits per pixel
	SaveLittleUShort(&image->io, (ILushort)image->Width);
	SaveLittleUShort(&image->io, (ILushort)image->Height);
	image->io.write(&Temp, sizeof(ILubyte), 1, &image->io);
	
	// Still don't know what exactly this is for...
	Temp = 0;
	image->io.write(&Temp, sizeof(ILubyte), 1, &image->io);
	image->io.write(ID, sizeof(char), IDLen, &image->io);
	ifree(ID);
	//iwrite(ID, sizeof(ILbyte), IDLen - sizeof(ILuint));
	//iwrite(&image->Depth, sizeof(ILuint), 1);
	
	// Write out the colormap
	if (UsePal)
		TempPal.writeToFile(&image->io);
		//image->io.write(TempPal->Palette, sizeof(ILubyte), TempPal->PalSize, &image->io);
	// else do nothing
	
	if (!Compress)
		image->io.write(TempData, sizeof(ILubyte), TempImage->SizeOfData, &image->io);
	else {
		Rle = (ILubyte*)ialloc(TempImage->SizeOfData + TempImage->SizeOfData / 2 + 1);	// max
		if (Rle == NULL) {
			ifree(AuthName);
			ifree(AuthComment);
			return IL_FALSE;
		}
		RleLen = ilRleCompress((unsigned char*)TempData, TempImage->Width, TempImage->Height,
		                       TempImage->Depth, TempImage->Bpp, Rle, IL_TGACOMP, NULL);
		
		image->io.write(Rle, 1, RleLen, &image->io);
		ifree(Rle);
	}
	
	// Write the extension area.
	ExtOffset = (ILuint) image->io.tell(&image->io);
	SaveLittleUShort(&image->io, 495);	// Number of bytes in the extension area (TGA 2.0 spec)
	image->io.write(AuthName, 1, ilCharStrLen(AuthName), &image->io);
	ipad(&image->io, 41 - ilCharStrLen(AuthName));
	image->io.write(AuthComment, 1, ilCharStrLen(AuthComment), &image->io);
	ipad(&image->io, 324 - ilCharStrLen(AuthComment));
	ifree(AuthName);
	ifree(AuthComment);
	
	// Write time/date
	iGetDateTime(&Month, &Day, &Year, &Hour, &Minute, &Second);
	SaveLittleUShort(&image->io, (ILushort)Month);
	SaveLittleUShort(&image->io, (ILushort)Day);
	SaveLittleUShort(&image->io, (ILushort)Year);
	SaveLittleUShort(&image->io, (ILushort)Hour);
	SaveLittleUShort(&image->io, (ILushort)Minute);
	SaveLittleUShort(&image->io, (ILushort)Second);
	
	for (i = 0; i < 6; i++) {  // Time created
		SaveLittleUShort(&image->io, 0);
	}
	for (i = 0; i < 41; i++) {	// Job name/ID
		image->io.putc(0, &image->io);
	}
	for (i = 0; i < 3; i++) {  // Job time
		SaveLittleUShort(&image->io, 0);
	}
	
	image->io.write(idString, 1, ilCharStrLen(idString), &image->io);	// Software ID
	for (i = 0; i < 41 - ilCharStrLen(idString); i++) {
		image->io.putc(0, &image->io);
	}
	SaveLittleUShort(&image->io, IL_VERSION);  // Software version
	image->io.putc(' ', &image->io);  // Release letter (not beta anymore, so use a space)
	
	SaveLittleUInt(&image->io, 0);	// Key colour
	SaveLittleUInt(&image->io, 0);	// Pixel aspect ratio
	SaveLittleUInt(&image->io, 0);	// Gamma correction offset
	SaveLittleUInt(&image->io, 0);	// Colour correction offset
	SaveLittleUInt(&image->io, 0);	// Postage stamp offset
	SaveLittleUInt(&image->io, 0);	// Scan line offset
	image->io.putc(3, &image->io);  // Attributes type
	
	// Write the footer.
	SaveLittleUInt(&image->io, ExtOffset);	// No extension area
	SaveLittleUInt(&image->io, 0);	// No developer directory
	image->io.write(Footer, 1, ilCharStrLen(Footer), &image->io);
	
	if (TempImage->Origin != IL_ORIGIN_LOWER_LEFT) {
		ifree(TempData);
	}
	if (Format == IL_RGB || Format == IL_RGBA) {
		ilSwapColours();
	}
	
	if (TempImage != image)
		ilCloseImage(TempImage);

	return IL_TRUE;
}


#endif//IL_NO_TGA
