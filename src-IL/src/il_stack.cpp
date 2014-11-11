//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 12/17/2008
//
// Filename: src-IL/src/il_stack.c
//
// Description: The main image stack
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#include "IL/il2.h"


// Global variables
static ILboolean IsInit = IL_FALSE;


ILAPI ILboolean ILAPIENTRY ilIsValidPal(ILpal *Palette)
{
	if (Palette == NULL)
		return IL_FALSE;
	if (!Palette->hasPalette())
		return IL_FALSE;
	switch (Palette->getPalType())
	{
		case IL_PAL_RGB24:
		case IL_PAL_RGB32:
		case IL_PAL_RGBA32:
		case IL_PAL_BGR24:
		case IL_PAL_BGR32:
		case IL_PAL_BGRA32:
			return IL_TRUE;
	}
	return IL_FALSE;
}


//! Closes Palette and frees all memory associated with it.
ILAPI void ILAPIENTRY ilClosePal(ILpal *Palette)
{
	if (ilIsValidPal(Palette))
		delete Palette;
}

// Create a number of sub images of a given type
// Type: IL_SUB_NEXT, IL_SUB_MIPMAP, IL_SUB_LAYER
ILuint ILAPIENTRY il2CreateSubImage(ILimage* image, ILenum Type, ILuint Num)
{
	ILimage	*SubImage = NULL;
	ILuint	Count = 1;  // Create one before we go in the loop.

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return 0;
	}
	if (Num == 0)  {
		return 0;
	}

	switch (Type)
	{
		case IL_SUB_NEXT:
			if (image->Next)
				ilCloseImage(image->Next);
			image->Next = il2NewImage(1, 1, 1, 1, 1);
			SubImage = image->Next;
			break;

		case IL_SUB_MIPMAP:
			if (image->Mipmaps)
				ilCloseImage(image->Mipmaps);
			image->Mipmaps = il2NewImage(1, 1, 1, 1, 1);
			SubImage = image->Mipmaps;
			break;

		case IL_SUB_LAYER:
			if (image->Layers)
				ilCloseImage(image->Layers);
			image->Layers = il2NewImage(1, 1, 1, 1, 1);
			SubImage = image->Layers;
			break;

		default:
			il2SetError(IL_INVALID_ENUM);
			return IL_FALSE;
	}

	if (SubImage == NULL) {
		return 0;
	}

	for (Count = 1; Count < Num; Count++) {
		SubImage->Next = il2NewImage(1, 1, 1, 1, 1);
		SubImage = SubImage->Next;
		if (SubImage == NULL)
			return Count;
	}

	return Count;
}


// ONLY call at startup.
void ILAPIENTRY il2Init()
{
	// if it is already initialized skip initialization
	if (IsInit == IL_TRUE ) 
		return;
	
	il2SetError(IL_NO_ERROR);
	IsInit = IL_TRUE;
	return;
}

