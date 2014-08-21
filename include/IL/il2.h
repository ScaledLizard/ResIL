///////////////////////////////////////////////////////////////////////////////
// il2.h
// Declarations for ResIL API 2.0, designed to be thread safe
// Written by Björn Ganster in 2014
///////////////////////////////////////////////////////////////////////////////

#ifndef IL__2__h
#define IL__2__h

#ifdef __IL_H__
#pragma message("Mixed usage of ResIL API 1 and 2 found")
#endif

#include <stdio.h>
#include <stdint.h>
#include "il_constants.h"

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

//
// Section shamelessly modified from the glut header.
//

// This is from Win32's <windef.h>
#if (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED) || defined(__BORLANDC__) || defined(__LCC__)
	#define ILAPIENTRY __stdcall 
	#define IL_PACKSTRUCT
//#elif defined(linux) || defined(MACOSX) || defined(__CYGWIN__) //fix bug 840364
#elif defined( __GNUC__ )
  // this should work for any of the above commented platforms 
  // plus any platform using GCC
	#ifdef __MINGW32__
		#define ILAPIENTRY __stdcall
	#else
		#define ILAPIENTRY
	#endif
	#define IL_PACKSTRUCT __attribute__ ((packed))
#else
	#define ILAPIENTRY
	#define IL_PACKSTRUCT
#endif

// This is from Win32's <wingdi.h> and <winnt.h>
#if defined(__LCC__)
	#define ILAPI __stdcall
#elif defined(_WIN32) //changed 20031221 to fix bug 840421
	#ifdef IL_STATIC_LIB
		#define ILAPI
	#else
		#ifdef _IL_BUILD_LIBRARY
			#define ILAPI __declspec(dllexport)
		#else
			#define ILAPI 
		#endif
	#endif
#elif __APPLE__
	#define ILAPI extern
#else
	#define ILAPI
#endif

#ifdef _MSC_VER
typedef __int64          ILint64;
typedef unsigned __int64 ILuint64;
#else
typedef long long int          ILint64;
typedef long long unsigned int ILuint64;
#endif

struct SIO;
struct ILimage;

// Callback functions for file reading and writing
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

// The internal structs should be visible to the outside only as void*
typedef void IlSIOExtern;

// Initialize ResIL API
void ILAPIENTRY il2Init(void);

// Generate a new image 
// Return value: handle used by API 2.x functions
// Use ilCloseImage to close the image later
ILAPI ILimage* ILAPIENTRY il2GenImage();

// Creates a new ILimage based on the specifications given
ILAPI ILimage* ILAPIENTRY il2NewImage(ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILubyte Bpc);

// Delete an image and all associated data
ILAPI void ILAPIENTRY il2DeleteImage(ILimage * imageExt);

// Reset read functions to use file system for io
ILAPI void ILAPIENTRY il2ResetRead(ILimage* image);

// Allows you to override the default file-writing functions
ILAPI void ILAPIENTRY il2SetWrite(ILimage* image, fOpenProc Open, fCloseProc Close, 
	fPutcProc Putc, fSeekProc Seek, fTellProc Tell, fWriteProc Write);

// Restore file-based i/o functions
ILAPI void ILAPIENTRY il2ResetWrite(ILimage* aImageExt);

// Allows you to override the default file-reading functions.
ILAPI void ILAPIENTRY il2SetRead(ILimage* image, fOpenProc aOpen, fCloseProc aClose, 
	fEofProc aEof, fGetcProc aGetc, fReadProc aRead, fSeekProc aSeek, 
	fTellProc aTell);

// Get current read/write position for a lump
ILAPI ILuint64 ILAPIENTRY il2GetLumpPos(ILimage* aImageExt);

// Determine file type for given file handle or lump
ILAPI ILenum ILAPIENTRY il2DetermineTypeF(ILHANDLE File);
ILAPI ILenum ILAPIENTRY il2DetermineTypeL(const void *Lump, ILuint Size);
ILAPI ILenum ILAPIENTRY il2DetermineType(ILconst_string FileName);
ILAPI ILenum ILAPIENTRY il2DetermineTypeFuncs(ILimage* imageExt);

// Load an image using a file pointer
// (implemented in il_io.cpp)
ILAPI ILboolean ILAPIENTRY il2LoadF(ILimage* image, ILenum Type, ILHANDLE File);

// Load an image from a memory buffer
ILAPI ILboolean ILAPIENTRY il2LoadL(ILimage* image, ILenum Type, const void *Lump, ILuint Size);

// Load an image, given the file name. File type is deduced automatically.
ILAPI ILboolean ILAPIENTRY il2LoadImage(ILimage* imageExt, ILconst_string FileName);

// Load an image using the current set of io functions 
// (implemented in il_io.cpp)
ILAPI ILboolean ILAPIENTRY il2LoadFuncs(ILimage* image, ILenum type);

// Load image using the file system
ILAPI ILboolean ILAPIENTRY il2Load(ILimage* image, ILenum Type, ILconst_string FileName);

ILAPI ILint64	ILAPIENTRY il2DetermineSize(ILimage* image, ILenum Type);

// Apply color profile
ILAPI ILboolean ILAPIENTRY il2ApplyProfile(ILimage* image, ILstring InProfile, ILstring OutProfile);

ILAPI ILboolean ILAPIENTRY il2Blit(ILimage* aSource, ILimage* aTarget, ILint DestX,  ILint DestY,   ILint DestZ, 
	ILuint SrcX,  ILuint SrcY,   ILuint SrcZ, ILuint Width, ILuint Height, ILuint Depth);

ILAPI void ILAPIENTRY il2ClearColour(ILclampf Red, ILclampf Green, ILclampf Blue, ILclampf Alpha);

ILAPI ILboolean ILAPIENTRY il2ClearImage(ILimage *Image);

// Copy data and attributes of aSource into a new image
ILAPI ILimage* ILAPIENTRY il2CloneImage(ILimage* aSource);

ILAPI ILboolean  ILAPIENTRY il2ConvertImage (ILimage *Image, ILenum DestFormat, ILenum DestType);

//! Converts the current image to the DestFormat format.
ILAPI ILboolean ILAPIENTRY il2ConvertPal(ILimage* image, ILenum DestFormat);

//! Copies everything from source to dest
ILAPI ILboolean ILAPIENTRY il2CopyImage(ILimage* source, ILimage* dest);

ILAPI ILuint ILAPIENTRY il2CopyPixels(ILimage* image, ILuint XOff, ILuint YOff, ILuint ZOff, 
	ILuint Width, ILuint Height, ILuint Depth, ILenum Format, ILenum Type, void *Data);

ILAPI ILuint ILAPIENTRY il2CreateSubImage(ILimage* image, ILenum Type, ILuint Num);

//! Creates an ugly 64x64 black and yellow checkerboard image.
ILboolean ILAPIENTRY il2DefaultImage(ILimage* image);

//! Disables a mode
ILAPI ILboolean ILAPIENTRY il2Disable(ILenum Mode);

//! Enables a mode
ILAPI ILboolean ILAPIENTRY il2Enable(ILenum Mode);

ILAPI ILboolean il2FixImage(ILimage* image);

ILAPI ILubyte*  ILAPIENTRY il2GetAlpha(ILimage *Image, ILenum Type);

//! Returns the current value of the Mode
ILAPI ILboolean ILAPIENTRY il2GetBoolean(ILenum Mode);

//! Sets Param equal to the current value of the Mode
ILAPI void ILAPIENTRY il2GetBooleanv(ILenum Mode, ILboolean *Param);

//! Returns a pointer to the image's raster data.
/*! The pointer to the image data returned by this function is only valid until any
    operations are done on the image.  After any operations, this function should be
	called again.  The pointer can be cast to other types for images that have more
	than one byte per channel for easier access to data. 
	\param image The image to use the data
	\exception IL_ILLEGAL_OPERATION if image==NULL
	\return ILubyte pointer to image data.*/
ILAPI ILubyte* ILAPIENTRY il2GetData(ILimage* image);

ILAPI ILuint ILAPIENTRY il2GetDXTCData(ILimage* curImage, void *Buffer, ILuint BufferSize, ILenum DXTCFormat);

//! Gets the last error and sets the last error to IL_NO_ERROR
ILAPI ILenum ILAPIENTRY il2GetError(void);

//! Used for querying facea of a cubemap
ILAPI ILimage* ILAPIENTRY il2GetFace(ILimage* image, ILuint Number);

// Used for querying the current image if it is an animation.
// (Replacement for ilActiveImage)
ILAPI ILimage* ILAPIENTRY il2GetFrame(ILimage* image, ILuint Number);

//! Returns the current value of the Mode
ILAPI ILint ILAPIENTRY il2GetInteger(ILenum Mode);

// Get image-related integer values
ILAPI void ILAPIENTRY il2GetImageInteger(ILimage *Image, ILenum Mode, ILint *Param);

//! Sets Param equal to the current value of the Mode
ILAPI void ILAPIENTRY il2GetIntegerv(ILenum Mode, ILint *Param);

//! Get specific image layer, represented by an image
ILAPI ILimage* ILAPIENTRY il2GetLayer(ILimage* image, ILuint Number);

//! Obtain a mipmap level
ILAPI ILimage* ILAPIENTRY il2GetMipmap(ILimage* image, ILuint Number);

//! Returns a pointer to the current image's palette data.
/*! The pointer to the image palette data returned by this function is only valid until
	any operations are done on the image.  After any operations, this function should be
	called again.
	\param image The image to use the data
	\exception IL_ILLEGAL_OPERATION No currently bound image
	\return ILubyte pointer to image palette data.*/
ILAPI ILubyte* ILAPIENTRY il2GetPalette(ILimage* image);

//! Returns a constant string detailing aspects about this library.
ILAPI ILconst_string ILAPIENTRY il2GetString(ILenum StringName);

//! Checks whether the mode is enabled.
ILAPI ILboolean ILAPIENTRY il2IsEnabled(ILenum Mode);

//! Loads a palette from FileName into the current image's palette.
ILAPI ILboolean ILAPIENTRY il2LoadPal(ILimage* image, ILconst_string FileName);

//! Overlays the image found in Src on top of the current bound image at the coords specified.
ILAPI ILboolean ILAPIENTRY il2OverlayImage(ILimage* aSource, ILimage* aTarget, ILint aXCoord, ILint aYCoord, 
	ILint aZCoord);

ILAPI void ILAPIENTRY il2RegisterPal(ILimage* image, void *Pal, ILuint Size, ILenum Type);

ILAPI ILboolean il2RemoveAlpha(ILimage* image);

//! Save image, caller specifies file type
ILAPI ILboolean ILAPIENTRY il2Save(ILimage* image, ILenum type, ILconst_string FileName);

// Save image using the current set of io functions
ILAPI ILboolean ILAPIENTRY il2SaveFuncs(ILimage* image, ILenum type);

// Save  image, determines file type from extension
ILAPI ILboolean ILAPIENTRY il2SaveImage(ILimage* image, ILconst_string FileName);

// Save image, using caller's FILE*
ILAPI ILuint ILAPIENTRY il2SaveF(ILimage* image, ILenum type, ILHANDLE File);

// Save image to lump
ILAPI ILint64 ILAPIENTRY il2SaveL(ILimage* image, ILenum Type, void *Lump, ILuint Size);

//! Uploads Data of the same size to replace the image's data.
/*! 
	\param image The image to use the data
	\param Data New image data to update the image
	\exception IL_ILLEGAL_OPERATION No currently bound image
	\exception IL_INVALID_PARAM Data was NULL.
	\return Boolean value of failure or success
*/
ILAPI ILboolean ILAPIENTRY il2SetData(ILimage* image, void *Data);

// Sets the current error
ILAPI void ILAPIENTRY il2SetError(ILenum Error);

// Set global state
ILAPI void ILAPIENTRY il2SetInteger(ILenum Mode, ILint Param);

// Set image-related integer values
ILAPI void ILAPIENTRY il2SetImageInteger(ILimage* image, ILenum Mode, ILint Param);

ILAPI void ILAPIENTRY il2SetPixels(ILimage* image, ILint XOff, ILint YOff, ILint ZOff, 
	ILuint Width, ILuint Height, ILuint Depth, ILenum Format, ILenum Type, void *Data);

ILAPI ILboolean il2SwapColours(ILimage* image);

/*! 
   \param Image - the image affected by this operation
   \param Width Specifies the new image width.  This cannot be 0.
	\param Height Specifies the new image height.  This cannot be 0.
	\param Depth Specifies the new image depth.  This cannot be 0.
	\param Bpp Number of channels (ex. 3 for RGB)
	\param Format Enum of the desired format.  Any format values are accepted.
	\param Type Enum of the desired type.  Any type values are accepted.
	\param Data Specifies data that should be copied to the new image. If this parameter is NULL, no data is copied, and the new image data consists of undefined values.
	\exception IL_ILLEGAL_OPERATION No currently bound image.
	\exception IL_INVALID_PARAM One of the parameters is incorrect, such as one of the dimensions being 0.
	\exception IL_OUT_OF_MEMORY Could not allocate enough memory.
	\return Boolean value of failure or success*/
ILAPI ILboolean ILAPIENTRY il2TexImage(ILimage * image, ILuint Width, ILuint Height, 
	ILuint Depth, ILubyte Bpp, ILenum Format, ILenum Type, void *Data);

#endif
