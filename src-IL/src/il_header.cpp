//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 10/10/2006
//
// Filename: src-IL/src/il_header.c
//
// Description: Generates a C-style header file for the current image.
//
//-----------------------------------------------------------------------------

#ifndef IL_NO_CHEAD

#include "il_internal.h"

// Just a guess...let's see what's purty!
#define MAX_LINE_WIDTH 14

//! Generates a C-style header file for the current image.
ILboolean ilSaveCHeader(ILimage* image, char *InternalName)
{
	FILE		*HeadFile = NULL;
	ILuint		i = 0, j;
	ILimage		*TempImage;
	const char	*Name;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Name = iGetString(IL_CHEAD_HEADER_STRING);
	if (Name == NULL)
		Name = InternalName;

	if (image->Bpc > 1) {
		TempImage = iConvertImage(image, image->Format, IL_UNSIGNED_BYTE);
		if (TempImage == NULL)
           return IL_FALSE;
	} else {
		TempImage = image;
	}

	FILE* file = (FILE*) image->io.handle;

	fprintf(HeadFile, "//#include <il/il.h>\n");
	fprintf(HeadFile, "// C Image Header:\n\n\n");
	fprintf(HeadFile, "// IMAGE_BPP is in bytes per pixel, *not* bits\n");
	fprintf(HeadFile, "#define IMAGE_BPP %d\n",image->Bpp);
	fprintf(HeadFile, "#define IMAGE_WIDTH   %d\n", image->Width);
	fprintf(HeadFile, "#define IMAGE_HEIGHT  %d\n", image->Height);	
	fprintf(HeadFile, "#define IMAGE_DEPTH   %d\n\n\n", image->Depth);
	fprintf(HeadFile, "#define IMAGE_TYPE    0x%X\n", image->Type);
	fprintf(HeadFile, "#define IMAGE_FORMAT  0x%X\n\n\n", image->Format);
	fprintf(HeadFile, "ILubyte %s[] = {\n", Name);
        

	for (; i < TempImage->SizeOfData; i += MAX_LINE_WIDTH) {
		fprintf(HeadFile, "\t");
		for (j = 0; j < MAX_LINE_WIDTH; j++) {
			if (i + j >= TempImage->SizeOfData - 1) {
				fprintf(HeadFile, "%4d", TempImage->Data[i+j]);
				break;
			}
			else
				fprintf(HeadFile, "%4d,", TempImage->Data[i+j]);
		}
		fprintf(HeadFile, "\n");
	}
	if (TempImage != image)
		ilCloseImage(TempImage);

	fprintf(HeadFile, "};\n");


	if (image->Pal.hasPalette()) {
		fprintf(HeadFile, "\n\n");
		fprintf(HeadFile, "#define IMAGE_PALSIZE %u\n\n", image->Pal.getPalSize());
		fprintf(HeadFile, "#define IMAGE_PALTYPE 0x%X\n\n", image->Pal.getPalType());
        fprintf(HeadFile, "ILubyte %sPal[] = {\n", Name);
		for (i = 0; i < image->Pal.getPalSize(); i += MAX_LINE_WIDTH) {
			fprintf(HeadFile, "\t");
			for (j = 0; j < MAX_LINE_WIDTH; j++) {
				if (i + j >= image->Pal.getPalSize() - 1) {
					fprintf(HeadFile, " %4d", image->Pal.getPalette()[i+j]);
					break;
				}
				else
					fprintf(HeadFile, " %4d,", image->Pal.getPalette()[i+j]);
			}
			fprintf(HeadFile, "\n");
		}

		fprintf(HeadFile, "};\n");
	}
	fclose(HeadFile);
	return IL_TRUE;
}



#endif//IL_NO_CHEAD
