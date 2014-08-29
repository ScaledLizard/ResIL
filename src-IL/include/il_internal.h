//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/01/2009
//
// Filename: src-IL/include/il_internal.h
//
// Description: Internal stuff for DevIL
//
//-----------------------------------------------------------------------------
#ifndef INTERNAL_H
#define INTERNAL_H


// Local headers
/*#if (defined(_WIN32) || defined(_WIN64)) && !defined(HAVE_CONFIG_H)
	#define HAVE_CONFIG_H
#endif*/
#ifdef HAVE_CONFIG_H //if we use autotools, we have HAVE_CONFIG_H defined and we have to look for it like that
	#include <config.h>
#else // If we do not use autotools, we have to point to (possibly different) config.h than in the opposite case
	#include <IL/config.h>
#endif

// Standard headers
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <IL/il2.h>
#include <IL/devil_internal_exports.h>
#include "il_files.h"
#include "il_endian.h"

// Thread variables
#ifdef _MSC_VER
#define IL_THREAD_VAR __declspec(thread)
#else
#define IL_THREAD_VAR __thread
#endif

#ifndef _WIN32
	// The Microsoft HD Photo Device Porting Kit has not been ported to anything other
	//  than Windows yet, so we disable this if Windows is not the current platform.
	#define IL_NO_WDP
#endif//_WIN32

// If we do not want support for game image formats, this define removes them all.
#ifdef IL_NO_GAMES
	#define IL_NO_BLP
	#define IL_NO_DOOM
	#define IL_NO_FTX
	#define IL_NO_IWI
	#define IL_NO_LIF
	#define IL_NO_MDL
	#define IL_NO_ROT
	#define IL_NO_TPL
	#define IL_NO_UTX
	#define IL_NO_VTF
	#define IL_NO_WAL
#endif//IL_NO_GAMES

// If we want to compile without support for formats supported by external libraries,
//  this define will remove them all.
#ifdef IL_NO_EXTLIBS
	#define IL_NO_EXR
	#define IL_NO_JP2
	#define IL_NO_JPG
	#define IL_NO_LCMS
	#define IL_NO_MNG
	#define IL_NO_PNG
	#define IL_NO_TIF
	#define IL_NO_WDP
	#undef IL_USE_DXTC_NVIDIA
	#undef IL_USE_DXTC_SQUISH
#endif//IL_NO_EXTLIBS

// Windows-specific
#ifdef _WIN32
	#ifdef _MSC_VER
		#if _MSC_VER > 1000
			#pragma once
			#pragma intrinsic(memcpy)
			#pragma intrinsic(memset)
			#pragma intrinsic(strcmp)
			#pragma intrinsic(strlen)
			#pragma intrinsic(strcpy)
			
			#if _MSC_VER >= 1300
				#pragma warning(disable : 4996)  // MSVC++ 8/9 deprecation warnings
			#endif
		#endif // _MSC_VER > 1000
	#endif
	#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
	#include <windows.h>
#endif//_WIN32

#ifdef _UNICODE
	#define IL_TEXT(s) L##s
	#ifndef _WIN32  // At least in Linux, fopen works fine, and wcsicmp is not defined.
		#define wcsicmp wcsncasecmp
		#define _wcsicmp wcsncasecmp
		#define _wfopen fopen
	#endif
	#define iStrCpy wcscpy
#else
	#define IL_TEXT(s) (s)
	#define iStrCpy strcpy
#endif

#ifdef IL_INLINE_ASM
	#if (defined (_MSC_VER) && defined(_WIN32))  // MSVC++ only
		#define USE_WIN32_ASM
	#endif

	#ifdef _WIN64
		#undef USE_WIN32_ASM
	//@TODO: Windows 64 compiler cannot use inline ASM, so we need to
	//  generate some MASM code at some point.
	#endif

	#ifdef _WIN32_WCE  // Cannot use our inline ASM in Windows Mobile.
		#undef USE_WIN32_ASM
	#endif
#endif


// Global struct for storing image data - defined in il_internal.cpp
extern ILimage *iCurImage;

#define BIT_0	0x00000001
#define BIT_1	0x00000002
#define BIT_2	0x00000004
#define BIT_3	0x00000008
#define BIT_4	0x00000010
#define BIT_5	0x00000020
#define BIT_6	0x00000040
#define BIT_7	0x00000080
#define BIT_8	0x00000100
#define BIT_9	0x00000200
#define BIT_10	0x00000400
#define BIT_11	0x00000800
#define BIT_12	0x00001000
#define BIT_13	0x00002000
#define BIT_14	0x00004000
#define BIT_15	0x00008000
#define BIT_16	0x00010000
#define BIT_17	0x00020000
#define BIT_18	0x00040000
#define BIT_19	0x00080000
#define BIT_20	0x00100000
#define BIT_21	0x00200000
#define BIT_22	0x00400000
#define BIT_23	0x00800000
#define BIT_24	0x01000000
#define BIT_25	0x02000000
#define BIT_26	0x04000000
#define BIT_27	0x08000000
#define BIT_28	0x10000000
#define BIT_29	0x20000000
#define BIT_30	0x40000000
#define BIT_31	0x80000000
#define NUL '\0'  // Easier to type and ?portable?
#if !_WIN32 || _WIN32_WCE
	int stricmp(const char *src1, const char *src2);
	int strnicmp(const char *src1, const char *src2, size_t max);
#endif//_WIN32
#ifdef _WIN32_WCE
	char *strdup(const char *src);
#endif
int iStrCmp(ILconst_string src1, ILconst_string src2);

// File pointer-based io functions
ILHANDLE ILAPIENTRY iDefaultOpenR(ILconst_string FileName);
void ILAPIENTRY iDefaultCloseR(SIO* io);
ILboolean ILAPIENTRY iDefaultEof(SIO* io);
ILint64 ILAPIENTRY iDefaultSeek(SIO* io, ILint64 Offset, ILuint Mode);
ILint64 ILAPIENTRY iDefaultTell(SIO* io);
ILHANDLE ILAPIENTRY iDefaultOpenW(ILconst_string FileName);
void ILAPIENTRY iDefaultCloseW(SIO* io);
ILHANDLE	ILAPIENTRY iDefaultOpen(ILconst_string FileName);
void ILAPIENTRY iDefaultClose(SIO* io);
ILint	ILAPIENTRY iDefaultGetc(SIO* io);
ILint64 ILAPIENTRY iDefaultRead(SIO* io, void *Buffer, ILuint Size, ILuint Number);
ILint	ILAPIENTRY iDefaultSeekR(SIO* io, ILint64 Offset, ILint Mode);
ILint	ILAPIENTRY iDefaultSeekW(SIO* io, ILint Offset, ILint Mode);
ILint	ILAPIENTRY iDefaultTellR(SIO* io);
ILint	ILAPIENTRY iDefaultTellW(SIO* io);
ILint	ILAPIENTRY iDefaultPutc(ILubyte Char, SIO* io);
ILint64 ILAPIENTRY iDefaultWrite(const void *Buffer, ILuint Size, ILuint Number, SIO* io);


//
// Some math functions
//
// A fast integer squareroot, completely accurate for x < 289.
// Taken from http://atoms.org.uk/sqrt/
// There is also a version that is accurate for all integers
// < 2^31, if we should need it
int iSqrt(int x);

//
// Useful miscellaneous functions
//
ILboolean	iCheckExtension(ILconst_string Arg, ILconst_string Ext);
ILbyte*		iFgets(ILimage* image, char *buffer, ILuint maxlen);
ILboolean	iFileExists(ILconst_string FileName);
ILstring	iGetExtension(ILconst_string FileName);
ILstring	ilStrDup(ILconst_string Str);
ILuint		ilStrLen(ILconst_string Str);
ILuint		ilCharStrLen(const char *Str);

// Miscellaneous functions
void					ilDefaultStates(void);
ILenum					iGetHint(ILenum Target);
ILint					iGetInt(ILenum Mode);
void					ilRemoveRegistered(void);
ILAPI void ILAPIENTRY	ilSetCurImage(ILimage *Image);
ILboolean ilFlipImage(ILimage* image);


ILAPI ILenum ILAPIENTRY iDetermineTypeFuncs(SIO* ioExt);

// Store set of read io functions in a struct
void ILAPIENTRY iSetRead(SIO* io, fOpenProc aOpen, fCloseProc aClose, fEofProc aEof, fGetcProc aGetc, 
	fReadProc aRead, fSeekProc aSeek, fTellProc aTell);

//
// Rle compression
//
#define		IL_TGACOMP 0x01
#define		IL_PCXCOMP 0x02
#define		IL_SGICOMP 0x03
#define     IL_BMPCOMP 0x04
ILboolean	ilRleCompressLine(ILubyte *ScanLine, ILuint Width, ILubyte Bpp, ILubyte *Dest, ILuint *DestWidth, ILenum CompressMode);
ILuint		ilRleCompress(ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILubyte *Dest, ILenum CompressMode, ILuint *ScanTable);
void		iSetImage0(void);

// DXTC compression
ILuint			ilNVidiaCompressDXTFile(ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILenum DxtType);
ILAPI ILubyte*	ILAPIENTRY ilNVidiaCompressDXT(ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILenum DxtFormat, ILuint *DxtSize);
ILAPI ILubyte*	ILAPIENTRY ilSquishCompressDXT(ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILenum DxtFormat, ILuint *DxtSize);

// Conversion functions
ILAPI ILboolean	ilAddAlpha(void);
ILAPI ILboolean	ilAddAlphaKey(ILimage *Image);
ILAPI ILboolean	iFastConvert(ILimage *Image, ILenum DestFormat);
//ILboolean	ilFixCur(void);
//ILboolean	ilFixImage(void);
ILAPI ILboolean	ilRemoveAlpha(void);
ILAPI ILboolean	ilSwapColours(void);
// Palette functions
ILboolean	iCopyPalette(ILpal *Dest, ILpal *Src);
// Miscellaneous functions
char*		iGetString(ILenum StringName);  // Internal version of ilGetString

//
// Image loading/saving functions
//
ILboolean iIsValidBlp(SIO* io);
ILboolean iLoadBlpInternal(ILimage * Image);
ILboolean ilLoadBlp(ILconst_string FileName);

ILboolean iIsValidBmp(SIO* io);
ILboolean iLoadBitmapInternal(ILimage* image);
ILboolean iSaveBitmapInternal(ILimage* image);

ILboolean ilSaveCHeader(ILimage* image, char *InternalName);

ILboolean iLoadCutInternal(ILimage* image);

ILboolean ilIsValidDcx(ILconst_string FileName);
ILboolean ilIsValidDcxF(ILHANDLE File);
ILboolean ilIsValidDcxL(const void *Lump, ILuint Size);
ILboolean ilLoadDcx(ILconst_string FileName);
ILboolean ilLoadDcxF(ILHANDLE File);
ILboolean ilLoadDcxL(const void *Lump, ILuint Size);

ILboolean iIsValidDds(SIO* io);
ILboolean iLoadDdsInternal(ILimage* image);
ILboolean iSaveDdsInternal(ILimage* curImage);

ILboolean iIsValidDicom(SIO* io);
ILboolean iLoadDicomInternal(ILimage* image);

ILboolean iLoadDoomInternal(ILimage* image);
ILboolean iLoadDoomFlatInternal(ILimage* image);

ILboolean ilIsValidDpx(ILconst_string FileName);
ILboolean ilIsValidDpxF(ILHANDLE File);
ILboolean ilIsValidDpxL(const void *Lump, ILuint Size);
ILboolean iLoadDpxInternal(ILimage* image);

ILboolean iIsValidExr(SIO* io);
ILboolean iLoadExrInternal(ILimage* image);
ILboolean iSaveExrInternal(ILimage* image);

ILboolean iIsValidFits(void);
ILboolean iLoadFitsInternal(ILimage* image);

ILboolean iLoadFtxInternal(ILimage* image);

ILboolean iIsValidGif(SIO* io);
ILboolean iLoadGifInternal(ILimage* image);

ILboolean iIsValidHdr(SIO* io);
ILboolean iLoadHdrInternal(ILimage* image);
ILboolean iSaveHdrInternal(ILimage* image);

ILboolean iLoadIconInternal(ILimage* image);
ILboolean iIsValidIcon(SIO* io);

ILboolean iIsValidIcns(SIO* io);
ILboolean iLoadIcnsInternal(ILimage* image);

ILboolean iLoadIffInternal(ILimage* image);

ILboolean iIsValidIlbm(SIO* io);
ILboolean iLoadIlbmInternal(ILimage* image);

ILboolean iIsValidIwi(SIO* io);
ILboolean iLoadIwiInternal(ILimage* image);

ILboolean iIsValidJp2(SIO* io);
ILboolean iLoadJp2Internal(ILimage *Image);
ILboolean ilLoadJp2LInternal(ILimage* image, const void *Lump, ILuint Size);
ILboolean iSaveJp2Internal(ILimage* image);

ILboolean iIsValidJpeg(SIO* io);
ILboolean iLoadJpegInternal(ILimage* image);
ILboolean iSaveJpegInternal(ILimage* image);

ILboolean iIsValidLif(SIO* io);
ILboolean iLoadLifInternal(ILimage* image);

ILboolean iIsValidMdl(SIO* io);
ILboolean iLoadMdlInternal(ILimage* image);

ILboolean ilIsValidMng();
ILboolean iLoadMngInternal(ILimage* image);
ILboolean iSaveMngInternal();

ILboolean iIsValidMp3(SIO* io);
ILboolean iLoadMp3Internal(ILimage* image);

ILboolean iLoadPcdInternal(ILimage* image);

ILboolean iIsValidPcx(SIO* io);
ILboolean iLoadPcxInternal(ILimage* image);
ILboolean iSavePcxInternal(ILimage* image);

ILboolean iIsValidPic(SIO* io);
ILboolean iLoadPicInternal(ILimage* image);

ILboolean iIsValidPix(SIO* io);
ILboolean iLoadPixInternal(ILimage* image);

ILboolean iIsValidPng(SIO* io);
ILboolean iLoadPngInternal(ILimage* image);
ILboolean iSavePngInternal(ILimage* image);

ILboolean iIsValidPnm(SIO* io);
ILboolean iLoadPnmInternal(ILimage* image);
ILboolean iSavePnmInternal(ILimage* image);

ILboolean iIsValidPsd(SIO* io);
ILboolean iLoadPsdInternal(ILimage* image);
ILboolean iSavePsdInternal(ILimage* image);

ILboolean iIsValidPsp(SIO* io);
ILboolean iLoadPspInternal(ILimage* image);

ILboolean iLoadPxrInternal(ILimage* image);

ILboolean iLoadRawInternal(ILimage* image);
ILboolean iSaveRawInternal(ILimage* image);

ILboolean iLoadRotInternal(ILimage* image);

ILboolean iIsValidSgi(SIO* io);
ILboolean iLoadSgiInternal(ILimage* image);
ILboolean iSaveSgiInternal(ILimage* image);

ILboolean iIsValidSun(SIO* io);
ILboolean iLoadSunInternal(ILimage* image);

ILboolean iIsValidTarga(SIO* io);
ILboolean iLoadTargaInternal(ILimage* image);
ILboolean iSaveTargaInternal(ILimage* image);

ILboolean ilLoadTexture(ILconst_string FileName);
ILboolean ilLoadTextureF(ILHANDLE File);
ILboolean ilLoadTextureL(const void *Lump, ILuint Size);

ILboolean ilIsValidTiffFunc(SIO* io);
ILboolean iLoadTiffInternal(ILimage* image);
ILboolean iSaveTiffInternal(ILimage* image);

ILboolean iIsValidTpl(SIO* io);
ILboolean iLoadTplInternal(ILimage* image);

ILboolean iLoadUtxInternal(ILimage* image);

ILboolean iIsValidVtf(SIO* io);
ILboolean iLoadVtfInternal(ILimage* image);
ILboolean iSaveVtfInternal(ILimage* image);

ILboolean iLoadWalInternal(ILimage* image);

ILboolean iLoadWbmpInternal(ILimage* image);
ILboolean iSaveWbmpInternal(ILimage* image);

ILboolean ilIsValidWdp(ILconst_string FileName);
ILboolean ilIsValidWdpF(ILHANDLE File);
ILboolean ilIsValidWdpL(const void *Lump, ILuint Size);
ILboolean ilLoadWdp(ILconst_string FileName);
ILboolean ilLoadWdpF(ILHANDLE File);
ILboolean ilLoadWdpL(const void *Lump, ILuint Size);

ILboolean ilIsValidXpm(ILconst_string FileName);
ILboolean ilIsValidXpmF(SIO* io, ILHANDLE File);
ILboolean ilIsValidXpmL(const void *Lump, ILuint Size);
ILboolean iIsValidXpm(SIO* io);
ILboolean iLoadXpmInternal(ILimage* image);


#ifdef _WIN32
#ifndef strnicmp
#define strnicmp _strnicmp
#endif
#endif

ILAPI ILboolean ILAPIENTRY iDxtcDataToSurface(ILimage* image);
ILAPI ILboolean ILAPIENTRY iSurfaceToDxtcData(ILimage* image, ILenum Format);

// The internal structs should be visible to the outside only as void*
typedef void IlImageExtern;
typedef void SIoExtern;


#endif//INTERNAL_H
