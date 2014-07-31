//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/include/il_jpeg.h
//
// Description: Jpeg (.jpg) functions
//
//-----------------------------------------------------------------------------

#ifndef JPEG_H
#define JPEG_H

#include "il_internal.h"

ILboolean iCheckJpg(ILubyte Header[2]);
ILboolean iIsValidJpg(void);

#ifndef _WIN32
#ifndef __cdecl
#define __cdecl __attribute__((__cdecl__))
#endif
#endif

#ifndef IL_USE_IJL
	ILboolean __cdecl iLoadJpegInternal(void);
	ILboolean iSaveJpegInternal(void);
#else
	ILboolean iLoadJpegInternal(ILconst_string FileName, ILvoid *Lump, ILuint Size);
	ILboolean iSaveJpegInternal(ILconst_string FileName, ILvoid *Lump, ILuint Size);
#endif

#endif//JPEG_H
