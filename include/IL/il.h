//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: IL/il.h
//
// Description: The main include file for DevIL
//
//-----------------------------------------------------------------------------

// Doxygen comment
/*! \file il.h
    The main include file for DevIL
*/

#ifndef __il_h_
#ifndef __IL_H__

#define __il_h_
#define __IL_H__

#ifdef IL__2__h
#pragma message("Mixed usage of ResIL API 1 and 2 found")
#endif


#include "il_constants.h"

#include <limits.h>
#ifdef _UNICODE
	#ifndef _WIN32_WCE
		#include <wchar.h>
	#endif
	//if we use a define instead of a typedef,
	//ILconst_string works as intended
	#define ILchar wchar_t
	#define ILstring wchar_t*
	#define ILconst_string  wchar_t const *
#else
	//if we use a define instead of a typedef,
	//ILconst_string works as intended
	#define ILchar char
	#define ILstring char*
	#define ILconst_string char const *

	#define PathCharMod "%s"
#endif //_UNICODE

# if defined __GNUC__ && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 0))
// __attribute__((deprecated)) is supported by GCC 3.1 and later.
#  define DEPRECATED(D) D __attribute__((deprecated))
# elif defined _MSC_VER && _MSC_VER >= 1300
// __declspec(deprecated) is supported by MSVC 7.0 and later.
#  define DEPRECATED(D) __declspec(deprecated) D
# else
#  define DEPRECATED (D) D
# endif

typedef unsigned int   ILenum;
typedef unsigned char  ILboolean;
typedef unsigned int   ILbitfield;
typedef signed char    ILbyte;
typedef signed short   ILshort;
typedef int     	   ILint;
typedef size_t         ILsizei;
typedef unsigned char  ILubyte;
typedef unsigned short ILushort;
typedef unsigned int   ILuint;
typedef float          ILfloat;
typedef float          ILclampf;
typedef double         ILdouble;
typedef double         ILclampd;

#ifdef _MSC_VER
typedef __int64          ILint64;
typedef unsigned __int64 ILuint64;
#else
typedef long long int          ILint64;
typedef long long unsigned int ILuint64;
#endif

// Callback functions for file reading and writing
struct SIO;
typedef void* ILHANDLE;
typedef void      (ILAPIENTRY *fCloseProc)(SIO*);
typedef ILboolean (ILAPIENTRY *fEofProc)   (SIO*);
typedef ILint     (ILAPIENTRY *fGetcProc)  (SIO*);
typedef ILHANDLE  (ILAPIENTRY *fOpenProc) (ILconst_string);
typedef ILint64     (ILAPIENTRY *fReadProc)  (SIO*, void*, ILuint, ILuint);
typedef ILint64     (ILAPIENTRY *fSeekProc) (SIO*, ILint64, ILuint);
typedef ILint64     (ILAPIENTRY *fTellProc) (SIO*);
typedef ILint    (ILAPIENTRY *fPutcProc)  (ILubyte, SIO*);
typedef ILint64    (ILAPIENTRY *fWriteProc) (const void*, ILuint, ILuint, SIO*);

// Callback functions for allocation and deallocation
typedef void* (ILAPIENTRY *mAlloc)(const ILsizei);
typedef void  (ILAPIENTRY *mFree) (const void* CONST_RESTRICT);

// Registered format procedures
typedef ILenum (ILAPIENTRY *IL_LOADPROC)(ILconst_string);
typedef ILenum (ILAPIENTRY *IL_SAVEPROC)(ILconst_string);


// ImageLib Functions
ILAPI ILboolean ILAPIENTRY ilActiveFace(ILuint Number);
ILAPI ILboolean ILAPIENTRY ilActiveImage(ILuint Number);
ILAPI ILboolean ILAPIENTRY ilActiveLayer(ILuint Number);
ILAPI ILboolean ILAPIENTRY ilActiveMipmap(ILuint Number);
ILAPI ILboolean ILAPIENTRY ilApplyPal(ILconst_string FileName);
ILAPI ILboolean ILAPIENTRY ilApplyProfile(ILstring InProfile, ILstring OutProfile);
ILAPI void		ILAPIENTRY ilBindImage(ILuint Image);
ILAPI ILboolean ILAPIENTRY ilBlit(ILuint Source, ILint DestX, ILint DestY, ILint DestZ, ILuint SrcX, ILuint SrcY, ILuint SrcZ, ILuint Width, ILuint Height, ILuint Depth);
ILAPI ILboolean ILAPIENTRY ilClampNTSC(void);
ILAPI void		ILAPIENTRY ilClearColour(ILclampf Red, ILclampf Green, ILclampf Blue, ILclampf Alpha);
ILAPI ILboolean ILAPIENTRY ilClearImage(void);
ILAPI ILuint    ILAPIENTRY ilCloneCurImage(void);
ILAPI ILubyte*	ILAPIENTRY ilCompressDXT(ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILenum DXTCFormat, ILuint *DXTCSize);
ILAPI ILboolean ILAPIENTRY ilCompressFunc(ILenum Mode);
ILAPI ILboolean ILAPIENTRY ilConvertImage(ILenum DestFormat, ILenum DestType);
ILAPI ILboolean ILAPIENTRY ilConvertPal(ILenum DestFormat);
ILAPI ILboolean ILAPIENTRY ilCopyImage(ILuint Src);
ILAPI ILuint    ILAPIENTRY ilCopyPixels(ILuint XOff, ILuint YOff, ILuint ZOff, ILuint Width, ILuint Height, ILuint Depth, ILenum Format, ILenum Type, void *Data);
ILAPI ILuint    ILAPIENTRY ilCreateSubImage(ILenum Type, ILuint Num);
ILAPI ILboolean ILAPIENTRY ilDefaultImage();
ILAPI void		ILAPIENTRY ilDeleteImage(const ILuint Num);
ILAPI void      ILAPIENTRY ilDeleteImages(ILsizei Num, const ILuint *Images);
ILAPI ILint64	ILAPIENTRY ilDetermineSize(ILenum Type);
ILAPI ILenum	ILAPIENTRY ilDetermineType(ILconst_string FileName);
ILAPI ILenum	ILAPIENTRY ilDetermineTypeFuncs();
ILAPI ILenum	ILAPIENTRY ilDetermineTypeF(ILHANDLE File);
ILAPI ILenum	ILAPIENTRY ilDetermineTypeL(const void *Lump, ILuint Size);
ILAPI ILboolean ILAPIENTRY ilDisable(ILenum Mode);
ILAPI ILboolean ILAPIENTRY ilDxtcDataToImage(void);
ILAPI ILboolean ILAPIENTRY ilEnable(ILenum Mode);
ILAPI void		ILAPIENTRY ilFlipSurfaceDxtcData(void);
ILAPI ILboolean ILAPIENTRY ilFormatFunc(ILenum Mode);
ILAPI void	    ILAPIENTRY ilGenImages(ILsizei Num, ILuint *Images);
ILAPI ILuint	ILAPIENTRY ilGenImage(void);
ILAPI ILubyte*  ILAPIENTRY ilGetAlpha(ILenum Type);
ILAPI ILboolean ILAPIENTRY ilGetBoolean(ILenum Mode);
ILAPI void      ILAPIENTRY ilGetBooleanv(ILenum Mode, ILboolean *Param);
ILAPI ILubyte*  ILAPIENTRY ilGetData(void);
ILAPI ILuint    ILAPIENTRY ilGetDXTCData(void *Buffer, ILuint BufferSize, ILenum DXTCFormat);
ILAPI ILenum    ILAPIENTRY ilGetError(void);
ILAPI void ILAPIENTRY ilGetImageInteger(ILenum Mode, ILint *Param);
ILAPI ILint     ILAPIENTRY ilGetInteger(ILenum Mode);
ILAPI void      ILAPIENTRY ilGetIntegerv(ILenum Mode, ILint *Param);
ILAPI ILuint64    ILAPIENTRY ilGetLumpPos(void);
ILAPI ILubyte*  ILAPIENTRY ilGetPalette(void);
ILAPI ILconst_string  ILAPIENTRY ilGetString(ILenum StringName);
ILAPI void      ILAPIENTRY ilHint(ILenum Target, ILenum Mode);
ILAPI ILboolean	ILAPIENTRY ilInvertSurfaceDxtcDataAlpha(void);
ILAPI void      ILAPIENTRY ilInit(void);
ILAPI ILboolean ILAPIENTRY ilImageToDxtcData(ILenum Format);
ILAPI ILboolean ILAPIENTRY ilIsDisabled(ILenum Mode);
ILAPI ILboolean ILAPIENTRY ilIsEnabled(ILenum Mode);
ILAPI ILboolean ILAPIENTRY ilIsImage(ILuint Image);
ILAPI ILboolean ILAPIENTRY ilIsValid(ILenum Type, ILconst_string FileName);
ILAPI ILboolean ILAPIENTRY ilIsValidF(ILenum Type, ILHANDLE File);
ILAPI ILboolean ILAPIENTRY ilIsValidL(ILenum Type, void *Lump, ILuint Size);
ILAPI void      ILAPIENTRY ilKeyColour(ILclampf Red, ILclampf Green, ILclampf Blue, ILclampf Alpha);
ILAPI ILboolean ILAPIENTRY ilLoad(ILenum Type, ILconst_string FileName);
ILAPI ILboolean ILAPIENTRY ilLoadF(ILenum Type, ILHANDLE File);
ILAPI ILboolean ILAPIENTRY ilLoadFuncs(ILenum Type);
ILAPI ILboolean ILAPIENTRY ilLoadImage(ILconst_string FileName);
ILAPI ILboolean ILAPIENTRY ilLoadL(ILenum Type, const void *Lump, ILuint Size);
ILAPI ILboolean ILAPIENTRY ilLoadPal(ILconst_string FileName);
ILAPI void      ILAPIENTRY ilModAlpha(ILdouble AlphaValue);
ILAPI ILboolean ILAPIENTRY ilOriginFunc(ILenum Mode);
ILAPI ILboolean ILAPIENTRY ilOverlayImage(ILuint Source, ILint XCoord, ILint YCoord, ILint ZCoord);
ILAPI void      ILAPIENTRY ilPopAttrib(void);
ILAPI void      ILAPIENTRY ilPushAttrib(ILuint Bits);
ILAPI void      ILAPIENTRY ilRegisterFormat(ILenum Format);
ILAPI ILboolean ILAPIENTRY ilRegisterLoad(ILconst_string Ext, IL_LOADPROC Load);
ILAPI ILboolean ILAPIENTRY ilRegisterMipNum(ILuint Num);
ILAPI ILboolean ILAPIENTRY ilRegisterNumFaces(ILuint Num);
ILAPI ILboolean ILAPIENTRY ilRegisterNumImages(ILuint Num);
ILAPI void      ILAPIENTRY ilRegisterOrigin(ILenum Origin);
ILAPI void      ILAPIENTRY ilRegisterPal(void *Pal, ILuint Size, ILenum Type);
ILAPI ILboolean ILAPIENTRY ilRegisterSave(ILconst_string Ext, IL_SAVEPROC Save);
ILAPI void      ILAPIENTRY ilRegisterType(ILenum Type);
ILAPI ILboolean ILAPIENTRY ilRemoveLoad(ILconst_string Ext);
ILAPI ILboolean ILAPIENTRY ilRemoveSave(ILconst_string Ext);
ILAPI void      ILAPIENTRY ilResetMemory(void); // Deprecated

// Reset read, write procedures
// @todo: Setting read calls currently zeroes the write calls, and vice versa - is this correct?
ILAPI void      ILAPIENTRY ilResetRead(void);
ILAPI void      ILAPIENTRY ilResetWrite(void);

ILAPI ILboolean ILAPIENTRY ilSave(ILenum Type, ILconst_string FileName);
ILAPI ILuint    ILAPIENTRY ilSaveF(ILenum Type, ILHANDLE File);
ILAPI ILboolean ILAPIENTRY ilSaveFuncs(ILenum type);
ILAPI ILboolean ILAPIENTRY ilSaveImage(ILconst_string FileName);
ILAPI ILint64    ILAPIENTRY ilSaveL(ILenum Type, void *Lump, ILuint Size);
ILAPI ILboolean ILAPIENTRY ilSavePal(ILconst_string FileName);
ILAPI ILboolean ILAPIENTRY ilSetAlpha(ILdouble AlphaValue);
ILAPI ILboolean ILAPIENTRY ilSetData(void *Data);
ILAPI ILboolean ILAPIENTRY ilSetDuration(ILuint Duration);
ILAPI void ILAPIENTRY ilSetImageInteger(ILenum Mode, ILint Param);
ILAPI void      ILAPIENTRY ilSetInteger(ILenum Mode, ILint Param);
ILAPI void      ILAPIENTRY ilSetMemory(mAlloc, mFree);
ILAPI void      ILAPIENTRY ilSetPixels(ILint XOff, ILint YOff, ILint ZOff, 
	ILuint Width, ILuint Height, ILuint Depth, ILenum Format, ILenum Type, void *Data);

// Set read functions - currently not useful, because it is not possible to read files using these functions
// Only ilLoad* functions can be used to load images, and these will override the settings defined here,
// Therefore, unfortunately, this function is currently useful only within ResIL
ILAPI void      ILAPIENTRY ilSetRead(fOpenProc, fCloseProc, fEofProc, fGetcProc, fReadProc, fSeekProc, fTellProc);

ILAPI void      ILAPIENTRY ilSetString(ILenum Mode, const char *String);

// Set write functions - currently not useful, because it is not possible to write files using these functions
// Only ilSave* functions can be used to write images, and these will override the settings defined here,
// Therefore, unfortunately, this function is currently useful only within ResIL
ILAPI void      ILAPIENTRY ilSetWrite(fOpenProc, fCloseProc, fPutcProc, fSeekProc, fTellProc, fWriteProc);

ILAPI void      ILAPIENTRY ilShutDown(void);
ILAPI ILboolean ILAPIENTRY ilSurfaceToDxtcData(ILenum Format);
ILAPI ILboolean ILAPIENTRY ilTexImage(ILuint Width, ILuint Height, ILuint Depth, ILubyte NumChannels, ILenum Format, ILenum Type, void *Data);
ILAPI ILboolean ILAPIENTRY ilTexImageDxtc(ILint w, ILint h, ILint d, ILenum DxtFormat, const ILubyte* data);
ILAPI ILenum    ILAPIENTRY ilTypeFromExt(ILconst_string FileName);
ILAPI ILboolean ILAPIENTRY ilTypeFunc(ILenum Mode);
ILAPI ILboolean ILAPIENTRY ilLoadData(ILconst_string FileName, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp);
ILAPI ILboolean ILAPIENTRY ilLoadDataF(ILHANDLE File, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp);
ILAPI ILboolean ILAPIENTRY ilLoadDataL(void *Lump, ILuint Size, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp);
ILAPI ILboolean ILAPIENTRY ilSaveData(ILconst_string FileName);

// For all those weirdos that spell "colour" without the 'u'.
#define ilClearColor	ilClearColour
#define ilKeyColor      ilKeyColour

#endif // __IL_H__
#endif // __il_h__
