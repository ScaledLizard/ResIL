//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 02/14/2009
//
// Filename: src-IL/src/il_dicom.c
//
// Description: Reads from a Digital Imaging and Communications in Medicine
//				(DICOM) file.  Specifications can be found at 
//                http://en.wikipedia.org/wiki/Dicom.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_DICOM


typedef struct DICOMHEAD
{
	ILubyte		Signature[4];
	ILuint		Version;
	ILuint		Width;
	ILuint		Height;
	ILuint		Depth;
	ILuint		Samples;
	ILuint		BitsAllocated;
	ILuint		BitsStored;
	ILuint		DataLen;
	ILboolean	BigEndian;
	ILenum		Encoding;

	// For DevIL use only
	ILenum	Format;
	ILenum	Type;
} DICOMHEAD;

ILboolean	iIsValidDicom(void);
ILboolean	iLoadDicomInternal(void);


ILuint GetGroupNum(SIO * io, DICOMHEAD *Header)
{
	ILushort GroupNum;

	io->read(io, &GroupNum, 1, 2);
	// The 0x02 group is always little endian.
	if (GroupNum == 0x02) {
		UShort(&GroupNum);
		return GroupNum;
	}
	// Now we have to swizzle it if it is not 0x02.
	if (Header->BigEndian)
		BigUShort(&GroupNum);
	else
		UShort(&GroupNum);

	return GroupNum;
}


ILuint GetShort(SIO * io, DICOMHEAD *Header, ILushort GroupNum)
{
	ILushort Num;

	io->read(io, &Num, 1, 2);
	// The 0x02 group is always little endian.
	if (GroupNum == 0x02) {
		UShort(&Num);
		return Num;
	}
	// Now we have to swizzle it if it is not 0x02.
	if (Header->BigEndian)
		BigUShort(&Num);
	else
		UShort(&Num);

	return Num;
}


ILuint GetInt(SIO * io, DICOMHEAD *Header, ILushort GroupNum)
{
	ILuint Num;

	io->read(io, &Num, 1, 4);
	// The 0x02 group is always little endian.
	if (GroupNum == 0x02) {
		UInt(&Num);
		return Num;
	}
	// Now we have to swizzle it if it is not 0x02.
	if (Header->BigEndian)
		BigUInt(&Num);
	else
		UInt(&Num);

	return Num;
}


ILfloat GetFloat(SIO * io, DICOMHEAD *Header, ILushort GroupNum)
{
	ILfloat Num;

	io->read(io, &Num, 1, 4);
	// The 0x02 group is always little endian.
	if (GroupNum == 0x02) {
		Float(&Num);
		return Num;
	}
	// Now we have to swizzle it if it is not 0x02.
	if (Header->BigEndian)
		BigFloat(&Num);
	else
		Float(&Num);

	return Num;
}


ILboolean GetNumericValue(SIO * io, DICOMHEAD *Header, ILushort GroupNum, ILuint *Number)
{
	ILubyte		VR1, VR2;
	ILushort	ValLen;

	// 2 byte character string telling what type this element is ('OB', 'UI', etc.)
	VR1 = io->getc(io);
	VR2 = io->getc(io);

	if (VR1 == 'U' && VR2 == 'S') {  // Unsigned short
		ValLen = GetShort(io, Header, GroupNum);//GetLittleUShort();
		if (ValLen != 2)  // Must always be 2 for short ('US')
			return IL_FALSE;
		*((ILushort*)Number) = GetShort(io, Header, GroupNum);//GetLittleUShort();
		return IL_TRUE;
	}
	if (VR1 == 'U' && VR2 == 'L') {  // Unsigned long
		ValLen = GetInt(io, Header, GroupNum);//GetLittleUInt();
		if (ValLen != 4)  // Must always be 4 for long ('UL')
			return IL_FALSE;
		*Number = GetInt(io, Header, GroupNum);
		return IL_TRUE;
	}
	if (VR1 == 'S' && VR2 == 'S') {  // Signed short
		ValLen = GetShort(io, Header, GroupNum);
		if (ValLen != 2)  // Must always be 2 for short ('US')
			return IL_FALSE;
		*((ILshort*)Number) = GetShort(io, Header, GroupNum);
		return IL_TRUE;
	}
	if (VR1 == 'S' && VR2 == 'L') {  // Signed long
		ValLen = GetInt(io, Header, GroupNum);
		if (ValLen != 4)  // Must always be 4 for long ('UL')
			return IL_FALSE;
		*((ILint*)Number) = GetInt(io, Header, GroupNum);
		return IL_TRUE;
	}

	return IL_FALSE;
}


ILboolean GetUID(SIO * io, ILubyte *UID)
{
	ILubyte		VR1, VR2;
	ILushort	ValLen;

	// 2 byte character string telling what type this element is ('OB', 'UI', etc.)
	VR1 = io->getc(io);
	VR2 = io->getc(io);

	if (VR1 != 'U' || VR2 != 'I')  // 'UI' == UID
		return IL_FALSE;

	ValLen = GetLittleUShort(io);
	if (io->read(io, UID, ValLen, 1) != 1)
		return IL_FALSE;
	UID[64] = 0;  // Just to make sure that our string is terminated.

	return IL_TRUE;
}


// Internal function used to get the DICOM header from the current file.
ILboolean SkipElement(SIO * io, DICOMHEAD *Header, ILushort GroupNum, ILushort ElementNum)
{
	ILubyte	VR1, VR2;
	ILuint	ValLen;

	// 2 byte character string telling what type this element is ('OB', 'UI', etc.)
	VR1 = io->getc(io);
	VR2 = io->getc(io);

	if ((VR1 == 'O' && VR2 == 'B') || (VR1 == 'O' && VR2 == 'W') || (VR1 == 'O' && VR2 == 'F') ||
		(VR1 == 'S' && VR2 == 'Q') || (VR1 == 'U' && VR2 == 'T') || (VR1 == 'U' && VR2 == 'N')) {
		// These all have a different format than the other formats, since they can be up to 32 bits long.
		GetLittleUShort(io);  // Values reserved, we do not care about them.
		ValLen = GetInt(io, Header, GroupNum);//GetLittleUInt();  // Length of the rest of the element
		if (ValLen % 2)  // This length must be even, according to the specs.
			return IL_FALSE;
		if (ElementNum != 0x00)  // Element numbers of 0 tell the size of the full group, so we do not skip this.
								 //  @TODO: We could use this to skip groups that we do not care about.
			if (io->seek(io, ValLen, IL_SEEK_CUR))
				return IL_FALSE;
	}
	else {
		// These have a length of 16 bits.
		ValLen = GetShort(io, Header, GroupNum);//GetLittleUShort();
		//if (ValLen % 2)  // This length must be even, according to the specs.
		//	ValLen++;  // Add the extra byte to seek.
		//if (ElementNum != 0x00)  // Element numbers of 0 tell the size of the full group, so we do not skip this.
								 //  @TODO: We could use this to skip groups that we do not care about.
			if (io->seek(io, ValLen, IL_SEEK_CUR))
				return IL_FALSE;
	}

	return IL_TRUE;
}


ILboolean iGetDicomHead(SIO* io, DICOMHEAD *Header)
{
	ILushort	GroupNum, ElementNum;
	ILboolean	ReachedData = IL_FALSE;
	ILubyte		Var2, UID[65];

	// Signature should be "DICM" at position 128.
	io->seek(io, 128, IL_SEEK_SET);
	if (io->read(io, Header->Signature, 1, 4) != 4)
		return IL_FALSE;

//@TODO: What about the case when we are reading an image with Big Endian data?

	do {
		GroupNum = GetGroupNum(io, Header);
		ElementNum = GetShort(io, Header, GroupNum);;

		switch (GroupNum)
		{
			case 0x02:
				switch (ElementNum)
				{
					/*case 0x01:  // Version number
						if (!GetNumericValue(&Header->Version))
							return IL_FALSE;
						if (Header->Version != 0x0100)
							return IL_FALSE;
						break;*/

					case 0x10:
						//@TODO: Look at pg. 60 of 07_05pu.pdf (PS 3.5) for more UIDs.
						if (!GetUID(io, UID))
							return IL_FALSE;
						if (!strncmp((const char*)UID, "1.2.840.10008.1.2.2", 64))  // Explicit big endian
							Header->BigEndian = IL_TRUE;
						else if (!strncmp((const char*)UID, "1.2.840.10008.1.2.1", 64))  // Explicit little endian
							Header->BigEndian = IL_FALSE;
						else if (!strncmp((const char*)UID, "1.2.840.10008.1.2", 64))  // Implicit little endian
							Header->BigEndian = IL_FALSE;
						else 
							return IL_FALSE;  // Unrecognized UID.
						break;

					default:
						if (!SkipElement(io, Header, GroupNum, ElementNum))  // We do not understand this entry, so we just skip it.
							return IL_FALSE;
				}
				break;

			case 0x28:
				switch (ElementNum)
				{
					case 0x02:  // Samples per pixel
						if (!GetNumericValue(io, Header, GroupNum, &Header->Samples))
							return IL_FALSE;
						break;

					case 0x08:  // Number of frames, or depth
						if (!GetNumericValue(io, Header, GroupNum, &Header->Depth))
							return IL_FALSE;
						break;

					case 0x10:  // The number of rows
						if (!GetNumericValue(io, Header, GroupNum, &Header->Height))
							return IL_FALSE;
						break;

					case 0x11:  // The number of columns
						if (!GetNumericValue(io, Header, GroupNum, &Header->Width))
							return IL_FALSE;
						break;

					case 0x100:  // Bits allocated per sample
						if (!GetNumericValue(io, Header, GroupNum, &Header->BitsAllocated))
							return IL_FALSE;
						break;

					case 0x101:  // Bits stored per sample - Do we really need this information?
						if (!GetNumericValue(io, Header, GroupNum, &Header->BitsStored))
							return IL_FALSE;
						break;

					default:
						if (!SkipElement(io, Header, GroupNum, ElementNum))  // We do not understand this entry, so we just skip it.
							return IL_FALSE;
				}
				break;

			case 0x7FE0:
				switch (ElementNum)
				{
					case 0x10:  // This element is the actual pixel data.  We are done with the header here.
						if (io->getc(io) != 'O')  // @TODO: Can we assume that this is always 'O'?
							return IL_FALSE;
						Var2 = io->getc(io);
						if (Var2 != 'B' && Var2 != 'W' && Var2 != 'F')  // 'OB', 'OW' and 'OF' accepted for this element.
							return IL_FALSE;
						GetLittleUShort(io);  // Skip the 2 reserved bytes.
						Header->DataLen = GetInt(io, Header, GroupNum);//GetLittleUInt();
						ReachedData = IL_TRUE;
						break;
					default:
						if (!SkipElement(io, Header, GroupNum, ElementNum))  // We do not understand this entry, so we just skip it.
							return IL_FALSE;
				}
				break;

			default:
				if (!SkipElement(io, Header, GroupNum, ElementNum))  // We do not understand this entry, so we just skip it.
					return IL_FALSE;
		}
	} while (!io->eof(io) && !ReachedData);

	if (io->eof(io))
		return IL_FALSE;

	// Some DICOM images do not have the depth (number of frames) field.
	if (Header->Depth == 0)
		Header->Depth = 1;

	switch (Header->BitsAllocated)
	{
		case 8:
			Header->Type = IL_UNSIGNED_BYTE;
			break;
		case 16:
			Header->Type = IL_UNSIGNED_SHORT;
			break;
		case 32:
			Header->Type = IL_FLOAT;  //@TODO: Is this ever an integer?
			break;
		default:  //@TODO: Any other types we can deal with?
			return IL_FALSE;
	}

	// Cannot handle more than 4 channels in an image.
	if (Header->Samples > 4)
		return IL_FALSE;
	Header->Format = ilGetFormatBpp(Header->Samples);

	return IL_TRUE;
}

// Internal function used to check if the HEADER is a valid DICOM header.
ILboolean iCheckDicom(DICOMHEAD *Header)
{
	// Always has the signature "DICM" at position 0x80.
	if (strncmp((const char*)Header->Signature, "DICM", 4))
		return IL_FALSE;
	// Does not make sense to have any dimension = 0.
	if (Header->Width == 0 || Header->Height == 0 || Header->Depth == 0)
		return IL_FALSE;
	// We can only deal with images that have byte-aligned data.
	//@TODO: Take care of any others?
	if (Header->BitsAllocated % 8)
		return IL_FALSE;
	// Check for an invalid format being set (or not set at all).
	if (ilGetBppFormat(Header->Format) == 0)
		return IL_FALSE;
	// Check for an invalid type being set (or not set at all).
	if (ilGetBpcType(Header->Type) == 0)
		return IL_FALSE;
	return IL_TRUE;
}


// Internal function to get the header and check it.
ILboolean iIsValidDicom(SIO* io)
{
	DICOMHEAD Header;
	auto Pos = io->tell(io);

	// Clear the header to all 0s to make checks later easier.
	memset(&Header, 0, sizeof(DICOMHEAD));
	ILboolean gotHeader = iGetDicomHead(io, &Header);

	// The length of the header varies, so we just go back to the original position.
	io->seek(io, Pos, IL_SEEK_CUR);

	if (gotHeader)
		return iCheckDicom(&Header);
	else
		return IL_FALSE;
}


// Internal function used to load the DICOM.
ILboolean iLoadDicomInternal(ILimage* image)
{
	DICOMHEAD	Header;
	ILuint		i;
	ILushort	TempS, *ShortPtr;
	ILfloat		TempF, *FloatPtr;
	ILboolean	Swizzle = IL_FALSE;
	SIO * io = &image->io;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	// Clear the header to all 0s to make checks later easier.
	memset(&Header, 0, sizeof(DICOMHEAD));
	if (!iGetDicomHead(io, &Header)) {
		il2SetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}
	if (!iCheckDicom(&Header))
		return IL_FALSE;

	if (!il2TexImage(image, Header.Width, Header.Height, Header.Depth, ilGetBppFormat(Header.Format), Header.Format, Header.Type, NULL))
		return IL_FALSE;
	//@TODO: Find out if the origin is always in the upper left.
	image->Origin = IL_ORIGIN_UPPER_LEFT;
	// Header.DataLen may be larger than SizeOfData, since it has to be padded with a NULL if it is not an even length,
	//   so we just test to make sure it is at least large enough.
	//@TODO: Do this check before ilTexImage call.
	if (Header.DataLen < image->SizeOfData) {
		il2SetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// We may have to swap the order of the data.
#ifdef __BIG_ENDIAN__
			if (!Header.BigEndian) {
				if (Header.Format == IL_RGB)
					Header.Format = IL_BGR;
				else if (Header.Format == IL_RGBA)
					Swizzle = IL_TRUE;
			}
#else  // Little endian
			if (Header.BigEndian) {
				if (Header.Format == IL_RGB)
					Header.Format = IL_BGR;
				if (Header.Format == IL_RGBA)
					Swizzle = IL_TRUE;
			}
#endif

	switch (Header.Type)
	{
		case IL_UNSIGNED_BYTE:
			if (io->read(io, image->Data, image->SizeOfData, 1) != 1)
				return IL_FALSE;

			// Swizzle the data from ABGR to RGBA.
			if (Swizzle) {
				for (i = 0; i < image->SizeOfData; i += 4) {
					iSwapUInt((ILuint*)(image->Data + i));
				}
			}
			break;

		case IL_UNSIGNED_SHORT:
			for (i = 0; i < image->SizeOfData; i += 2) {
				*((ILushort*)(image->Data + i)) = GetShort(io, &Header, 0);//GetLittleUShort();
			}

			// Swizzle the data from ABGR to RGBA.
			if (Swizzle) {
				ShortPtr = (ILushort*)image->Data;
				for (i = 0; i < image->SizeOfData / 2; i += 4) {
					TempS = ShortPtr[i];
					ShortPtr[i] = ShortPtr[i+3];
					ShortPtr[i+3] = TempS;
				}
			}
			break;

		case IL_FLOAT:
			for (i = 0; i < image->SizeOfData; i += 4) {
				*((ILfloat*)(image->Data + i)) = GetFloat(io, &Header, 0);//GetLittleFloat();
			}

			// Swizzle the data from ABGR to RGBA.
			if (Swizzle) {
				FloatPtr = (ILfloat*)image->Data;
				for (i = 0; i < image->SizeOfData / 4; i += 4) {
					TempF = FloatPtr[i];
					FloatPtr[i] = FloatPtr[i+3];
					FloatPtr[i+3] = TempF;
				}
			}
			break;
	}

	return il2FixImage(image);
}



#endif//IL_NO_DICOM
