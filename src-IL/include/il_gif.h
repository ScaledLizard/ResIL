//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 02/14/2009
//
// Filename: src-IL/include/il_gif.h
//
// Description: Reads from a Graphics Interchange Format (.gif) file.
//
//-----------------------------------------------------------------------------

#ifndef GIF_H
#define GIF_H

#include "il_internal.h"

#ifdef _WIN32
	#pragma pack(push, gif_struct, 1)
#endif
typedef struct GIFHEAD
{
	char		Sig[6];
	ILushort	Width;
	ILushort	Height;
	ILubyte		ColourInfo;
	ILubyte		Background;
	ILubyte		Aspect;
} IL_PACKSTRUCT GIFHEAD;

typedef struct IMAGEDESC
{
	ILubyte		Separator;
	ILushort	OffX;
	ILushort	OffY;
	ILushort	Width;
	ILushort	Height;
	ILubyte		ImageInfo;
} IL_PACKSTRUCT IMAGEDESC;

typedef struct GFXCONTROL
{
	ILubyte ExtensionBlockSignature;
	ILubyte GraphicsControlLabel;
	ILubyte Size;
	ILubyte Packed;
	ILushort	Delay;
	ILubyte Transparent;
	ILubyte Terminator;
} IL_PACKSTRUCT GFXCONTROL;
#ifdef _WIN32
	#pragma pack(pop, gif_struct)
#endif

#endif//GIF_H
