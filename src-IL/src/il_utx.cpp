//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_utx.cpp
//
// Description: Reads from an Unreal and Unreal Tournament Texture (.utx) file.
//				Specifications can be found at
//				http://wiki.beyondunreal.com/Legacy:Package_File_Format.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#ifndef IL_NO_UTX
//#include "IL/il.h"
#include "il_utx.h"


ILint64 GetUtxHead(SIO* io, UTXHEADER &Header)
{
	return io->read(io, &Header, 1, sizeof(Header));
}


ILboolean CheckUtxHead(UTXHEADER &Header)
{
	// This signature signifies a UTX file.
	if (Header.Signature != 0x9E2A83C1)
		return IL_FALSE;
	// Unreal uses 61-63, and Unreal Tournament uses 67-69.
	if ((Header.Version < 61 || Header.Version > 69))
		return IL_FALSE;
	return IL_TRUE;
}


// Gets a name variable from the file.  Keep in mind that the return value must be freed.
string GetUtxName(SIO* io, UTXHEADER &Header)
{
#define NAME_MAX_LEN 256  //@TODO: Figure out if these can possibly be longer.
	char	Name[NAME_MAX_LEN];
	ILubyte	Length = 0;

	// New style (Unreal Tournament) name.  This has a byte at the beginning telling
	//  how long the string is (plus terminating 0), followed by the terminating 0. 
	if (Header.Version >= 64) {
		Length = io->getc(io);
		if (io->read(io, Name, Length, 1) != 1)
			return "";
		if (Name[Length-1] != 0)
			return "";
		return string(Name);
	}

	// Old style (Unreal) name.  This string length is unknown, but it is terminated
	//  by a 0.
	do {
		Name[Length++] = io->getc(io);
	} while (!io->eof(io) && Name[Length-1] != 0 && Length < NAME_MAX_LEN);

	// Never reached the terminating 0.
	if (Length == NAME_MAX_LEN && Name[Length-1] != 0)
		return "";

	return string(Name);
#undef NAME_MAX_LEN
}


bool GetUtxNameTable(SIO* io, vector <UTXENTRYNAME> &NameEntries, UTXHEADER &Header)
{
	ILuint	NumRead;

	// Go to the name table.
	io->seek(io, Header.NameOffset, IL_SEEK_SET);

	NameEntries.resize(Header.NameCount);

	// Read in the name table.
	for (NumRead = 0; NumRead < Header.NameCount; NumRead++) {
		NameEntries[NumRead].Name = GetUtxName(io, Header);
		if (NameEntries[NumRead].Name == "")
			break;
		NameEntries[NumRead].Flags = GetLittleUInt(io);
	}

	// Did not read all of the entries (most likely GetUtxName failed).
	if (NumRead < Header.NameCount) {
		il2SetError(IL_INVALID_FILE_HEADER);
		return false;
	}

	return true;
}


// This following code is from http://wiki.beyondunreal.com/Legacy:Package_File_Format/Data_Details.
/// <summary>Reads a compact integer from the FileReader.
/// Bytes read differs, so do not make assumptions about
/// physical data being read from the stream. (If you have
/// to, get the difference of FileReader.BaseStream.Position
/// before and after this is executed.)</summary>
/// <returns>An "uncompacted" signed integer.</returns>
/// <remarks>FileReader is a System.IO.BinaryReader mapped
/// to a file. Also, there may be better ways to implement
/// this, but this is fast, and it works.</remarks>
ILint UtxReadCompactInteger(SIO* io)
{
        int output = 0;
        ILboolean sign = IL_FALSE;
		int i;
		ILubyte x;
        for(i = 0; i < 5; i++)
        {
                x = io->getc(io);
                // First byte
                if(i == 0)
                {
                        // Bit: X0000000
                        if((x & 0x80) > 0)
                                sign = IL_TRUE;
                        // Bits: 00XXXXXX
                        output |= (x & 0x3F);
                        // Bit: 0X000000
                        if((x & 0x40) == 0)
                                break;
                }
                // Last byte
                else if(i == 4)
                {
                        // Bits: 000XXXXX -- the 0 bits are ignored
                        // (hits the 32 bit boundary)
                        output |= (x & 0x1F) << (6 + (3 * 7));
                }
                // Middle bytes
                else
                {
                        // Bits: 0XXXXXXX
                        output |= (x & 0x7F) << (6 + ((i - 1) * 7));
                        // Bit: X0000000
                        if((x & 0x80) == 0)
                                break;
                }
        }
        // multiply by negative one here, since the first 6+ bits could be 0
        if (sign)
                output *= -1;
        return output;
}


void ChangeObjectReference(ILint *ObjRef, ILboolean *IsImported)
{
	if (*ObjRef < 0) {
		*IsImported = IL_TRUE;
		*ObjRef = -*ObjRef - 1;
	}
	else if (*ObjRef > 0) {
		*IsImported = IL_FALSE;
		*ObjRef = *ObjRef - 1;  // This is an object reference, so we have to do this conversion.
	}
	else {
		*ObjRef = -1;  // "NULL" pointer
	}

	return;
}


bool GetUtxExportTable(SIO* io, vector <UTXEXPORTTABLE> &ExportTable, UTXHEADER &Header)
{
	ILuint i;

	// Go to the name table.
	io->seek(io, Header.ExportOffset, IL_SEEK_SET);

	// Create ExportCount elements in our array.
	ExportTable.resize(Header.ExportCount);

	for (i = 0; i < Header.ExportCount; i++) {
		ExportTable[i].Class = UtxReadCompactInteger(io);
		ExportTable[i].Super = UtxReadCompactInteger(io);
		ExportTable[i].Group = GetLittleUInt(io);
		ExportTable[i].ObjectName = UtxReadCompactInteger(io);
		ExportTable[i].ObjectFlags = GetLittleUInt(io);
		ExportTable[i].SerialSize = UtxReadCompactInteger(io);
		ExportTable[i].SerialOffset = UtxReadCompactInteger(io);

		ChangeObjectReference(&ExportTable[i].Class, &ExportTable[i].ClassImported);
		ChangeObjectReference(&ExportTable[i].Super, &ExportTable[i].SuperImported);
		ChangeObjectReference(&ExportTable[i].Group, &ExportTable[i].GroupImported);
	}

	return true;
}


bool GetUtxImportTable(SIO* io, vector <UTXIMPORTTABLE> &ImportTable, UTXHEADER &Header)
{
	ILuint i;

	// Go to the name table.
	io->seek(io, Header.ImportOffset, IL_SEEK_SET);

	// Allocate the name table.
	ImportTable.resize(Header.ImportCount);

	for (i = 0; i < Header.ImportCount; i++) {
		ImportTable[i].ClassPackage = UtxReadCompactInteger(io);
		ImportTable[i].ClassName = UtxReadCompactInteger(io);
		ImportTable[i].Package = GetLittleUInt(io);
		ImportTable[i].ObjectName = UtxReadCompactInteger(io);

		ChangeObjectReference(&ImportTable[i].Package, &ImportTable[i].PackageImported);
	}

	return true;
}


/*void UtxDestroyPalettes(UTXPALETTE *Palettes, ILuint NumPal)
{
	ILuint i;
	for (i = 0; i < NumPal; i++) {
		//ifree(Palettes[i].Name);
		ifree(Palettes[i].Pal);
	}
	ifree(Palettes);
}*/


ILenum UtxFormatToDevIL(ILuint Format)
{
	switch (Format)
	{
		case UTX_P8:
			return IL_COLOR_INDEX;
		case UTX_DXT1:
			return IL_RGBA;
	}
	return IL_BGRA;  // Should never reach here.
}


ILuint UtxFormatToBpp(ILuint Format)
{
	switch (Format)
	{
		case UTX_P8:
			return 1;
		case UTX_DXT1:
			return 4;
	}
	return 4;  // Should never reach here.
}


// Internal function used to load the UTX.
ILboolean iLoadUtxInternal(ILimage* baseImage)
{
	UTXHEADER		Header;
	vector <UTXENTRYNAME> NameEntries;
	vector <UTXEXPORTTABLE> ExportTable;
	vector <UTXIMPORTTABLE> ImportTable;
	vector <UTXPALETTE> Palettes;
	ILuint		NumPal = 0, i, j = 0;
	ILint		Name;
	ILubyte		Type;
	ILint		Val;
	ILint		Size;
	ILint		Width, Height, PalEntry;
	ILboolean	BaseCreated = IL_FALSE, HasPal;
	ILint		Format;
	ILubyte		*CompData = NULL;
	SIO * io = &baseImage->io;

	if (baseImage == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!GetUtxHead(&baseImage->io, Header))
		return IL_FALSE;
	if (!CheckUtxHead(Header))
		return IL_FALSE;

	// We grab the name table.
	if (!GetUtxNameTable(io, NameEntries, Header))
		return IL_FALSE;
	// Then we get the export table.
	if (!GetUtxExportTable(io, ExportTable, Header))
		return IL_FALSE;
	// Then the last table is the import table.
	if (!GetUtxImportTable(io, ImportTable, Header))
		return IL_FALSE;

	// Find the number of palettes in the export table.
	for (i = 0; i < Header.ExportCount; i++) {
		if (NameEntries[ImportTable[ExportTable[i].Class].ObjectName].Name == "Palette")
			NumPal++;
	}
	Palettes.resize(NumPal);

	// Read in all of the palettes.
	NumPal = 0;
	for (i = 0; i < Header.ExportCount; i++) {
		if (NameEntries[ImportTable[ExportTable[i].Class].ObjectName].Name == "Palette") {
			Palettes[NumPal].Name = i;
			io->seek(io, ExportTable[NumPal].SerialOffset, IL_SEEK_SET);
			Name = io->getc(io);  // Skip the 2.  @TODO: Can there be more in front of the palettes?
			Palettes[NumPal].Count = UtxReadCompactInteger(io);
			Palettes[NumPal].Pal = new ILubyte[Palettes[NumPal].Count * 4];
			if (io->read(io, Palettes[NumPal].Pal, Palettes[NumPal].Count * 4, 1) != 1)
				return IL_FALSE;
			NumPal++;
		}
	}

	ILimage* currImage = NULL;
	for (i = 0; i < Header.ExportCount; i++) {
		// Find textures in the file.
		if (NameEntries[ImportTable[ExportTable[i].Class].ObjectName].Name == "Texture") {
			io->seek(io, ExportTable[i].SerialOffset, IL_SEEK_SET);
			Width = -1;  Height = -1;  PalEntry = NumPal;  HasPal = IL_FALSE;  Format = -1;

			do {
				// Always starts with a comptact integer that gives us an entry into the name table.
				Name = UtxReadCompactInteger(io);
				if (NameEntries[Name].Name == "None")
					break;
				Type = io->getc(io);
				Size = (Type & 0x70) >> 4;

				if (Type == 0xA2)
					io->getc(io);  // Byte is 1 here...

				switch (Type & 0x0F)
				{
					case 1:  // Get a single byte.
						Val = io->getc(io);
						break;

					case 2:  // Get unsigned integer.
						Val = GetLittleUInt(io);
						break;

					case 3:  // Boolean value is in the info byte.
						io->getc(io);
						break;

					case 4:  // Skip flaots for right now.
						GetLittleFloat(io);
						break;

					case 5:
					case 6:  // Get a compact integer - an object reference.
						Val = UtxReadCompactInteger(io);
						Val--;
						break;

					case 10:
						Val = io->getc(io);
						switch (Size)
						{
							case 0:
								io->seek(io, 1, IL_SEEK_CUR);
								break;
							case 1:
								io->seek(io, 2, IL_SEEK_CUR);
								break;
							case 2:
								io->seek(io, 4, IL_SEEK_CUR);
								break;
							case 3:
								io->seek(io, 12, IL_SEEK_CUR);
								break;
						}
						break;

					default:  // Uhm...
						break;
				}

				//@TODO: What should we do if Name >= Header.NameCount?
				if ((ILuint)Name < Header.NameCount) {  // Don't want to go past the end of our array.
					if (NameEntries[Name].Name == "Palette") {
						// If it has references to more than one palette, just use the first one.
						if (HasPal == IL_FALSE) {
							// We go through the palette list here to match names.
							for (PalEntry = 0; (ILuint)PalEntry < NumPal; PalEntry++) {
								if (Val == Palettes[PalEntry].Name) {
									HasPal = IL_TRUE;
									break;
								}
							}
						}
					}
					if (NameEntries[Name].Name == "Format")  // Not required for P8 images but can be present.
						if (Format == -1)
							Format = Val;
					if (NameEntries[Name].Name == "USize")  // Width of the image
						if (Width == -1)
							Width = Val;
					if (NameEntries[Name].Name == "VSize")  // Height of the image
						if (Height == -1)
							Height = Val;
				}
			} while (!io->eof(io));

			// If the format property is not present, it is a paletted (P8) image.
			if (Format == -1)
				Format = UTX_P8;
			// Just checks for everything being proper.  If the format is P8, we check to make sure that a palette was found.
			if (Width == -1 || Height == -1 || (PalEntry == NumPal && Format != UTX_DXT1) || (Format != UTX_P8 && Format != UTX_DXT1))
				return IL_FALSE;
			if (BaseCreated == IL_FALSE) {
				BaseCreated = IL_TRUE;
				il2TexImage(baseImage, Width, Height, 1, UtxFormatToBpp(Format), UtxFormatToDevIL(Format), IL_UNSIGNED_BYTE, NULL);
				currImage = baseImage;
			}
			else {
				currImage->Next = ilNewImageFull(Width, Height, 1, UtxFormatToBpp(Format), UtxFormatToDevIL(Format), IL_UNSIGNED_BYTE, NULL);
				if (currImage->Next == NULL)
					return IL_FALSE;
				currImage = currImage->Next;
			}

			// Skip the mipmap count, width offset and mipmap size entries.
			//@TODO: Implement mipmaps.
			io->seek(io, 5, IL_SEEK_CUR);
			UtxReadCompactInteger(io);

			switch (Format)
			{
				case UTX_P8:
					currImage->Pal.use(Palettes[PalEntry].Count, Palettes[PalEntry].Pal, IL_PAL_RGBA32);

					if (io->read(io, currImage->Data, currImage->SizeOfData, 1) != 1)
						return IL_FALSE;
					break;

				case UTX_DXT1:
					currImage->DxtcSize = IL_MAX(currImage->Width * currImage->Height / 2, 8);
					CompData = (ILubyte*)ialloc(currImage->DxtcSize);
					if (CompData == NULL)
						return IL_FALSE;

					if (io->read(io, CompData, currImage->DxtcSize, 1) != 1) {
						ifree(CompData);
						return IL_FALSE;
					}
					// Keep a copy of the DXTC data if the user wants it.
					if (il2GetInteger(IL_KEEP_DXTC_DATA) == IL_TRUE) {
						currImage->DxtcData = CompData;
						currImage->DxtcFormat = IL_DXT1;
						CompData = NULL;
					}
					if (DecompressDXT1(currImage, CompData) == IL_FALSE) {
						ifree(CompData);
						return IL_FALSE;
					}
					ifree(CompData);
					break;
			}
			currImage->Origin = IL_ORIGIN_UPPER_LEFT;
		}
	}

	return il2FixImage(baseImage);
}

#endif//IL_NO_UTX

