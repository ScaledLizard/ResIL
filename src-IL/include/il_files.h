//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 01/04/2009
//
// Filename: src-IL/include/il_files.h
//
// Description: File handling for DevIL
//
//-----------------------------------------------------------------------------

#ifndef FILES_H
#define FILES_H

#if defined (__FILES_C)
#define __FILES_EXTERN
#else
#define __FILES_EXTERN extern
#endif
#include <IL/il2.h>

// Functions to set file or lump for reading/writing
__FILES_EXTERN void				iSetInputFile(SIO* io, ILHANDLE File);
__FILES_EXTERN void				iSetInputLump(SIO* io, const void *Lump, ILuint Size);
__FILES_EXTERN void				iSetOutputFile(SIO* io, ILHANDLE File);
__FILES_EXTERN void				iSetOutputLump(SIO* io, void *Lump, ILuint Size);
__FILES_EXTERN void				iSetOutputFake(SIO* io);
 
__FILES_EXTERN ILHANDLE			ILAPIENTRY iGetFile(ILimage* image);
__FILES_EXTERN const ILubyte*	ILAPIENTRY iGetLump(ILimage* image);

__FILES_EXTERN ILuint			ILAPIENTRY ilprintf(SIO* io, const char *, ...);
__FILES_EXTERN void				ipad(SIO* io, ILuint NumZeros);

#endif//FILES_H
