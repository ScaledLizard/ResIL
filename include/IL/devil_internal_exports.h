//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 01/06/2009
//
// Filename: IL/devil_internal_exports.h
//
// Description: Internal stuff for DevIL (IL, ILU and ILUT)
//
//-----------------------------------------------------------------------------

#ifndef IL_EXPORTS_H
#define IL_EXPORTS_H

#include "il_internal.h"
#include "IL/il2.h"

#ifdef DEBUG
	#include <assert.h>
#else
	#define assert(x)
#endif

//#ifndef NOINLINE
#ifndef INLINE
#if defined(__GNUC__)
	#define INLINE extern inline
#elif defined(_MSC_VER)	//@TODO: Get this working in MSVC++.
						//  http://www.greenend.org.uk/rjk/2003/03/inline.html
	//#define NOINLINE
	//#define INLINE
	/*#ifndef _WIN64  // Cannot use inline assembly in x64 target platform.
		#define USE_WIN32_ASM
	#endif//_WIN64*/
	#define INLINE __inline
#else
	#define INLINE inline
#endif
#endif
//#else
//#define INLINE
//#endif //NOINLINE

#define IL_MAX(a,b) (((a) > (b)) ? (a) : (b))
#define IL_MIN(a,b) (((a) < (b)) ? (a) : (b))

// Memory functions
ILAPI void* ILAPIENTRY ialloc(const ILsizei Size);
ILAPI void  ILAPIENTRY ifree(const void *Ptr);
ILAPI void* ILAPIENTRY icalloc(const ILsizei Size, const ILsizei Num);
#ifdef ALTIVEC_GCC
ILAPI void* ILAPIENTRY ivec_align_buffer(void *buffer, const ILuint size);
#endif

struct SIO {
	// Function pointers set by ilSetRead, ilSetWrite
	fOpenProc openReadOnly;
	fOpenProc openWrite;
	fCloseProc close;
	fReadProc read;
	fSeekProc seek;
	fEofProc eof;
	fGetcProc getc;
	fTellProc tell;
	fPutcProc putc;
	fWriteProc write;

	// Write position for lumps and size computation
	// Size computation should be possible even if the file does not fit into
	// 4GB (32 bit address space)
	ILint64 rwPos;

	// Lump members
	const void *lump;
	ILuint lumpSize;

	// File members
	ILHANDLE handle;
	ILint64 ReadFileStart, WriteFileStart;

	// Size computation
	ILint64 MaxPos;
};

#include <IL/il_palette.h>

// glibc's attempt to redefine putc to _IO_putc doesn't go well with the
// putc member in struct SIO
#ifndef _WIN32
#undef putc
#undef getc
#endif

//! The Fundamental Image structure
/*! Every bit of information about an image is stored in this internal structure.*/
typedef struct ILimage
{
	ILuint          Width;       //!< the image's width
	ILuint          Height;      //!< the image's height
	ILuint          Depth;       //!< the image's depth
	ILubyte         Bpp;         //!< bytes per pixel (now number of channels)
	ILubyte         Bpc;         //!< bytes per channel
	ILuint          Bps;         //!< bytes per scanline (components for IL)
	ILubyte*        Data;        //!< the image data
	ILuint          SizeOfData;  //!< the total size of the data (in bytes)
	ILuint          SizeOfPlane; //!< SizeOfData in a 2d image, size of each plane slice in a 3d image (in bytes)
	ILenum          Format;      //!< image format (in IL enum style)
	ILenum          Type;        //!< image type (in IL enum style)
	ILenum          Origin;      //!< origin of the image
	ILpal           Pal;         //!< palette details
	ILuint          Duration;    //!< length of the time to display this "frame"
	ILenum          CubeFlags;   //!< cube map flags for sides present in chain
	struct ILimage* Mipmaps;     //!< mipmapped versions of this image terminated by a NULL - usu. NULL
	struct ILimage* Next;        //!< next image in the chain - usu. NULL
	struct ILimage* Faces;       //!< next cubemap face in the chain - usu. NULL
	struct ILimage* Layers;      //!< subsequent layers in the chain - usu. NULL
	ILuint*         AnimList;    //!< animation list
	ILuint          AnimSize;    //!< animation list size
	void*           Profile;     //!< colour profile
	ILuint          ProfileSize; //!< colour profile size
	ILuint          OffX;        //!< x-offset of the image
	ILuint			OffY;        //!< y-offset of the image
	ILubyte*        DxtcData;    //!< compressed data
	ILenum          DxtcFormat;  //!< compressed data format
	ILuint          DxtcSize;    //!< compressed data size
	struct SIO io;
} ILimage;


// Returns the current image.
ILAPI ILimage* ILAPIENTRY ilGetCurImage();

// To be only used when the original image is going to be set back almost immediately.
ILAPI void ILAPIENTRY ilSetCurImage(ILimage *Image);


//
// Utility functions
//
ILAPI ILubyte ILAPIENTRY ilGetBppFormat(ILenum Format);
ILAPI ILenum  ILAPIENTRY ilGetFormatBpp(ILubyte Bpp);
ILAPI ILubyte ILAPIENTRY ilGetBpcType(ILenum Type);
ILAPI ILenum  ILAPIENTRY ilGetTypeBpc(ILubyte Bpc);
ILAPI ILubyte ILAPIENTRY ilGetBppPal(ILenum PalType);
ILAPI ILenum  ILAPIENTRY ilGetPalBaseType(ILenum PalType);
ILAPI ILuint  ILAPIENTRY ilNextPower2(ILuint Num);
ILAPI ILenum  ILAPIENTRY ilTypeFromExt(ILconst_string FileName);
ILAPI void    ILAPIENTRY ilReplaceCurImage(ILimage *Image);
ILAPI void    ILAPIENTRY iMemSwap(ILubyte *, ILubyte *, const ILuint);

//
// Image functions
//
ILAPI void	    ILAPIENTRY iBindImageTemp  (void);
ILAPI ILboolean ILAPIENTRY ilClearImage_   (ILimage *Image);
ILAPI void      ILAPIENTRY ilCloseImage    (void *Image);
ILAPI void      ILAPIENTRY ilClosePal      (ILpal *Palette);
ILAPI ILpal*    ILAPIENTRY iCopyPal        (void);
ILAPI ILboolean ILAPIENTRY il2CopyImageAttr (ILimage *Dest, ILimage *Src);
ILAPI ILimage*  ILAPIENTRY ilCopyImage_    (ILimage *Src);
ILAPI void      ILAPIENTRY il2GetClear      (void *Colours, ILenum Format, ILenum Type);
ILAPI ILuint    ILAPIENTRY ilGetCurName    (void);
ILAPI ILboolean ILAPIENTRY ilIsValidPal    (ILpal *Palette);
ILAPI ILimage*  ILAPIENTRY il2NewImage      (ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILubyte Bpc);
ILAPI ILimage*  ILAPIENTRY ilNewImageFull  (ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILenum Format, ILenum Type, void *Data);
ILAPI ILboolean ILAPIENTRY il2InitImage     (ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILenum Format, ILenum Type, void *Data);
ILAPI ILboolean ILAPIENTRY il2ResizeImage   (ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILubyte Bpc);
ILAPI ILboolean ILAPIENTRY ilTexImage_     (ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILenum Format, ILenum Type, void *Data);
ILAPI ILboolean ILAPIENTRY ilTexSubImage_  (ILimage *Image, void *Data);
ILAPI void*     ILAPIENTRY ilConvertBuffer (ILuint SizeOfData, ILenum SrcFormat, ILenum DestFormat, ILenum SrcType, ILenum DestType, ILpal *SrcPal, void *Buffer);
ILAPI ILimage*  ILAPIENTRY iConvertImage   (ILimage *Image, ILenum DestFormat, ILenum DestType);
ILAPI ILpal    ILAPIENTRY iConvertPal     (ILpal *Pal, ILenum DestFormat);
ILAPI ILubyte*  ILAPIENTRY iGetFlipped     (ILimage *Image);
ILAPI ILboolean	ILAPIENTRY iMirror(ILimage *Image);
ILAPI void      ILAPIENTRY iFlipBuffer(ILubyte *buff, ILuint depth, ILuint line_size, ILuint line_num);
ILubyte*				   iFlipNewBuffer(ILubyte *buff, ILuint depth, ILuint line_size, ILuint line_num);
ILboolean ilAble(ILenum Mode, ILboolean Flag);

// Sets the current palette for an image
ILAPI void ILAPIENTRY il2SetPal(ILimage* image, ILpal *Pal);

// Internal library functions in ILU
ILAPI ILimage* ILAPIENTRY iluRotate_(ILimage *Image, ILfloat Angle);
ILAPI ILimage* ILAPIENTRY iluRotate3D_(ILimage *Image, ILfloat x, ILfloat y, ILfloat z, ILfloat Angle);
ILAPI ILimage* ILAPIENTRY iluScale_(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth);

#define imemclear(x,y) memset(x,0,y);

#endif//IL_EXPORTS_H
