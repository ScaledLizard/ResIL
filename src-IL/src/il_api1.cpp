///////////////////////////////////////////////////////////////////////////////
// il_api1.cpp
// Function implementations for ResIL 1.0 API
// Written by Björn Ganster in 2014
///////////////////////////////////////////////////////////////////////////////

/*
Many functions are just wrappers that use iCurImage as the first parameter,
but the functions that do not have a direct API 2.x counterpart have been moved
here
*/

// il.h functions should be implemented here without including that header
// to prevent warnings about mixing API 1 and 2
#include "il_internal.h"
#include "IL/il2.h"
#include "il_stack.h"

// Just a guess...seems large enough
#define I_STACK_INCREMENT 1024

// Global variables
ILimage *iCurImage = NULL;
ILuint		StackSize = 0;
ILuint		LastUsed = 0;
ILuint		CurName = 0;
ILimage		**ImageStack = NULL;
iFree		*FreeNames = NULL;

//! Creates Num images and puts their index in Images - similar to glGenTextures().
ILAPI void ILAPIENTRY ilGenImages(ILsizei Num, ILuint *Images)
{
	ILsizei	Index = 0;
	iFree	*TempFree = FreeNames;

	if (Num < 1 || Images == NULL) {
		il2SetError(IL_INVALID_VALUE);
		return;
	}

	// No images have been generated yet, so create the image stack.
	if (ImageStack == NULL)
		if (!iEnlargeStack())
			return;

	do {
		if (FreeNames != NULL) {  // If any have been deleted, then reuse their image names.
			TempFree = (iFree*)FreeNames->Next;
			Images[Index] = FreeNames->Name;
			ImageStack[FreeNames->Name] = il2NewImage(1, 1, 1, 1, 1);
			ifree(FreeNames);
			FreeNames = TempFree;
		} else {
			if (LastUsed >= StackSize)
				if (!iEnlargeStack())
					return;
			Images[Index] = LastUsed;
			// Must be all 1's instead of 0's, because some functions would divide by 0.
			ImageStack[LastUsed] = il2NewImage(1, 1, 1, 1, 1);
			LastUsed++;
		}
	} while (++Index < Num);

	return;
}

ILAPI ILuint ILAPIENTRY ilGenImage()
{
    ILuint i;
    ilGenImages(1,&i);
    return i;
}

//! Makes Image the current active image - similar to glBindTexture().
ILAPI void ILAPIENTRY ilBindImage(ILuint Image)
{
	if (ImageStack == NULL || StackSize == 0) {
		if (!iEnlargeStack()) {
			return;
		}
	}

	// If the user requests a high image name.
	while (Image >= StackSize) {
		if (!iEnlargeStack()) {
			return;
		}
	}

	if (ImageStack[Image] == NULL) {
		ImageStack[Image] = il2NewImage(1, 1, 1, 1, 1);
		if (Image >= LastUsed) // >= ?
			LastUsed = Image + 1;
	}

	iCurImage = ImageStack[Image];
	CurName = Image;

	return;
}


//! Deletes Num images from the image stack - similar to glDeleteTextures().
ILAPI void ILAPIENTRY ilDeleteImages(ILsizei Num, const ILuint *Images)
{
	iFree	*Temp = FreeNames;
	ILuint	Index = 0;

	if (Num < 1) {
		//il2SetError(IL_INVALID_VALUE);
		return;
	}
	if (StackSize == 0)
		return;

	do {
		if (Images[Index] > 0 && Images[Index] < LastUsed) {  // <= ?
			/*if (FreeNames != NULL) {  // Terribly inefficient
				Temp = FreeNames;
				do {
					if (Temp->Name == Images[Index]) {
						continue;  // Sufficient?
					}
				} while ((Temp = Temp->Next));
			}*/

			// Already has been deleted or was never used.
			if (ImageStack[Images[Index]] == NULL)
				continue;

			// Find out if current image - if so, set to default image zero.
			if (Images[Index] == CurName || Images[Index] == 0) {
				iCurImage = ImageStack[0];
				CurName = 0;
			}
			
			// Should *NOT* be NULL here!
			ilCloseImage(ImageStack[Images[Index]]);
			ImageStack[Images[Index]] = NULL;

			// Add to head of list - works for empty and non-empty lists
			Temp = (iFree*)ialloc(sizeof(iFree));
			if (!Temp) {
				return;
			}
			Temp->Name = Images[Index];
			Temp->Next = FreeNames;
			FreeNames = Temp;
		}
		/*else {  // Shouldn't set an error...just continue onward.
			il2SetError(IL_ILLEGAL_OPERATION);
		}*/
	} while (++Index < (ILuint)Num);
}


ILAPI void ILAPIENTRY ilDeleteImage(const ILuint Num) 
{
    //ilDeleteImages(1,&Num);
	ilCloseImage(ImageStack[Num]);
	ImageStack[Num] = NULL;
}

//! Checks if Image is a valid ilGenImages-generated image (like glIsTexture()).
ILAPI ILboolean ILAPIENTRY ilIsImage(ILuint Image)
{
	//iFree *Temp = FreeNames;

	if (ImageStack == NULL)
		return IL_FALSE;
	if (Image >= LastUsed || Image == 0)
		return IL_FALSE;

	/*do {
		if (Temp->Name == Image)
			return IL_FALSE;
	} while ((Temp = Temp->Next));*/

	if (ImageStack[Image] == NULL)  // Easier check.
		return IL_FALSE;

	return IL_TRUE;
}


//! Closes Image and frees all memory associated with it.
ILAPI void ILAPIENTRY ilCloseImage(void * image)
{
	ILimage* Image = (ILimage*) image;
	if (Image == NULL)
		return;

	if (Image->Data != NULL) {
		ifree(Image->Data);
		Image->Data = NULL;
	}

	Image->Pal.clear();

	if (Image->Next != NULL) {
		ilCloseImage(Image->Next);
		Image->Next = NULL;
	}

	if (Image->Faces != NULL) {
		ilCloseImage(Image->Faces);
		Image->Mipmaps = NULL;
	}

	if (Image->Mipmaps != NULL) {
		ilCloseImage(Image->Mipmaps);
		Image->Mipmaps = NULL;
	}

	if (Image->Layers != NULL) {
		ilCloseImage(Image->Layers);
		Image->Layers = NULL;
	}

	if (Image->AnimList != NULL && Image->AnimSize != 0) {
		ifree(Image->AnimList);
		Image->AnimList = NULL;
	}

	if (Image->Profile != NULL && Image->ProfileSize != 0) {
		ifree(Image->Profile);
		Image->Profile = NULL;
		Image->ProfileSize = 0;
	}

	if (Image->DxtcData != NULL && Image->DxtcFormat != IL_DXT_NO_COMP) {
		ifree(Image->DxtcData);
		Image->DxtcData = NULL;
		Image->DxtcFormat = IL_DXT_NO_COMP;
		Image->DxtcSize = 0;
	}

	ifree(Image);
	Image = NULL;

	return;
}


// Restore file-based i/o functions
ILAPI void ILAPIENTRY ilResetWrite()
{
	il2SetWrite(iCurImage, iDefaultOpenW, iDefaultCloseW, iDefaultPutc,
				iDefaultSeek, iDefaultTell, iDefaultWrite);
	return;
}

//! Allows you to override the default file-reading functions.
ILAPI void ILAPIENTRY ilSetRead(fOpenProc aOpen, fCloseProc aClose, fEofProc aEof, fGetcProc aGetc, 
	fReadProc aRead, fSeekProc aSeek, fTellProc aTell)
{
	il2SetRead(iCurImage, aOpen, aClose, aEof, aGetc, aRead, aSeek, aTell);
}

// Reset read functions to use file system for io
ILAPI void ILAPIENTRY ilResetRead()
{
	il2ResetRead(iCurImage);
}

// Allows you to override the default file-writing functions
ILAPI void ILAPIENTRY ilSetWrite(fOpenProc aOpen, fCloseProc aClose, fPutcProc aPutc, fSeekProc aSeek, 
	fTellProc aTell, fWriteProc aWrite)
{
	il2SetWrite(iCurImage, aOpen, aClose, aPutc, aSeek, aTell, aWrite);
}

// Get current lump read/write pos
ILAPI ILuint64 ILAPIENTRY ilGetLumpPos()
{
	return il2GetLumpPos(iCurImage);
}

// Return a type ID for the image file that can be accessed using the current
// set of io functions
ILAPI ILenum ILAPIENTRY ilDetermineTypeFuncs()
{
	return iDetermineTypeFuncs(&iCurImage->io);
}

ILAPI ILenum ILAPIENTRY ilDetermineType(ILconst_string FileName)
{
	return il2DetermineType(FileName);
}

//! Attempts to load an image from a file stream.  The file format is specified by the user.
/*! \param Type Format of this file.  Acceptable values are IL_BLP, IL_BMP, IL_CUT, IL_DCX, IL_DDS,
	IL_DICOM, IL_DOOM, IL_DOOM_FLAT, IL_DPX, IL_EXR, IL_FITS, IL_FTX, IL_GIF, IL_HDR, IL_ICO, IL_ICNS,
	IL_IFF, IL_IWI, IL_JP2, IL_JPG, IL_LIF, IL_MDL,	IL_MNG, IL_MP3, IL_PCD, IL_PCX, IL_PIX, IL_PNG,
	IL_PNM, IL_PSD, IL_PSP, IL_PXR, IL_ROT, IL_SGI, IL_SUN, IL_TEXTURE, IL_TGA, IL_TIF, IL_TPL,
	IL_UTX, IL_VTF, IL_WAL, IL_WBMP, IL_XPM, IL_RAW, IL_JASC_PAL and IL_TYPE_UNKNOWN.
	If IL_TYPE_UNKNOWN is specified, ilLoadF will try to determine the type of the file and load it.
	\param File File stream to load from. The caller is responsible for closing the handle.
	\return Boolean value of failure or success.  Returns IL_FALSE if loading fails.*/
ILAPI ILboolean ILAPIENTRY ilLoadF(ILenum Type, ILHANDLE File)
{
	return il2LoadF(iCurImage, Type, File);
}


//! Attempts to load an image from a memory buffer.  The file format is specified by the user.
/*! \param Type Format of this file.  Acceptable values are IL_BLP, IL_BMP, IL_CUT, IL_DCX, IL_DDS,
	IL_DICOM, IL_DOOM, IL_DOOM_FLAT, IL_DPX, IL_EXR, IL_FITS, IL_FTX, IL_GIF, IL_HDR, IL_ICO, IL_ICNS,
	IL_IFF, IL_IWI, IL_JP2, IL_JPG, IL_LIF, IL_MDL,	IL_MNG, IL_MP3, IL_PCD, IL_PCX, IL_PIX, IL_PNG,
	IL_PNM, IL_PSD, IL_PSP, IL_PXR, IL_ROT, IL_SGI, IL_SUN, IL_TEXTURE, IL_TGA, IL_TIF, IL_TPL,
	IL_UTX, IL_VTF, IL_WAL, IL_WBMP, IL_XPM, IL_RAW, IL_JASC_PAL and IL_TYPE_UNKNOWN.
	If IL_TYPE_UNKNOWN is specified, ilLoadL will try to determine the type of the file and load it.
	\param Lump The buffer where the file data is located
	\param Size Size of the buffer
	\return Boolean value of failure or success.  Returns IL_FALSE if loading fails.*/
ILAPI ILboolean ILAPIENTRY ilLoadL(ILenum Type, const void *Lump, ILuint Size)
{
	return il2LoadL(iCurImage, Type, Lump, Size);
}


ILAPI ILboolean ILAPIENTRY ilLoad(ILenum Type, ILconst_string FileName)
{
	return il2Load(iCurImage, Type, FileName);
}

//! Attempts to load an image using the currently set IO functions. The file format is specified by the user.
/*! \param Type Format of this file.  Acceptable values are IL_BLP, IL_BMP, IL_CUT, IL_DCX, IL_DDS,
	IL_DICOM, IL_DOOM, IL_DOOM_FLAT, IL_DPX, IL_EXR, IL_FITS, IL_FTX, IL_GIF, IL_HDR, IL_ICO, IL_ICNS,
	IL_IFF, IL_IWI, IL_JP2, IL_JPG, IL_LIF, IL_MDL,	IL_MNG, IL_MP3, IL_PCD, IL_PCX, IL_PIX, IL_PNG,
	IL_PNM, IL_PSD, IL_PSP, IL_PXR, IL_ROT, IL_SGI, IL_SUN, IL_TEXTURE, IL_TGA, IL_TIF, IL_TPL,
	IL_UTX, IL_VTF, IL_WAL, IL_WBMP, IL_XPM, IL_RAW, IL_JASC_PAL and IL_TYPE_UNKNOWN.
	If IL_TYPE_UNKNOWN is specified, ilLoadFuncs fails.
	\param File File stream to load from.
	\return Boolean value of failure or success.  Returns IL_FALSE if loading fails.*/
ILAPI ILboolean ILAPIENTRY ilLoadFuncs(ILenum type)
{
	return il2LoadFuncs(iCurImage, type);
}

ILAPI ILboolean ILAPIENTRY ilLoadImage(ILconst_string FileName)
{
	return il2LoadImage(iCurImage, FileName);
}

ILAPI ILboolean ILAPIENTRY ilSaveFuncs(ILenum type)
{
	return il2SaveFuncs(iCurImage, type);
}

// Returns the size of the memory buffer needed to save the current image into this Type.
//  A return value of 0 is an error.
ILAPI ILint64	ILAPIENTRY ilDetermineSize(ILenum type)
{
	return il2DetermineSize(iCurImage, type);
}

//! Attempts to save an image to a file.  The file format is specified by the user.
/*! \param Type Format of this file.  Acceptable values are IL_BMP, IL_CHEAD, IL_DDS, IL_EXR,
	IL_HDR, IL_JP2, IL_JPG, IL_PCX, IL_PNG, IL_PNM, IL_PSD, IL_RAW, IL_SGI, IL_TGA, IL_TIF,
	IL_VTF, IL_WBMP and IL_JASC_PAL.
	\param FileName Ansi or Unicode string, depending on the compiled version of DevIL, that gives
	       the filename to save to.
	\return Boolean value of failure or success.  Returns IL_FALSE if saving failed.*/
ILAPI ILboolean ILAPIENTRY ilSave(ILenum type, ILconst_string fileName)
{
	return il2Save(iCurImage, type, fileName);
}

// Save  image, determines file type from extension
ILAPI ILboolean ILAPIENTRY ilSaveImage(ILconst_string FileName)
{
	return il2SaveImage(iCurImage, FileName);
}

// Save image, using caller's FILE*
ILAPI ILuint ILAPIENTRY ilSaveF(ILenum type, ILHANDLE File)
{
	return il2SaveF(iCurImage, type, File);
}

// Save image to lump
ILAPI ILint64 ILAPIENTRY ilSaveL(ILenum Type, void *Lump, ILuint Size)
{
	return il2SaveL(iCurImage, Type, Lump, Size);
}

//! Changes the current bound image to use these new dimensions (current data is destroyed).
/*! \param Width Specifies the new image width.  This cannot be 0.
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
ILAPI ILboolean ILAPIENTRY ilTexImage(ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILenum Format, ILenum Type, void *Data)
{
	return il2TexImage(iCurImage, Width, Height, Depth, Bpp, Format, Type, Data);
}

ILAPI ILboolean ILAPIENTRY il2ApplyProfile(ILstring InProfile, ILstring OutProfile)
{
	return il2ApplyProfile(iCurImage, InProfile, OutProfile);
}

ILAPI ILboolean ILAPIENTRY ilConvertImage(ILenum DestFormat, ILenum DestType)
{
	return il2ConvertImage(iCurImage, DestFormat, DestType);
}

ILAPI ILboolean ilSwapColours()
{
	return il2SwapColours(iCurImage);
}

ILAPI ILboolean ilFixImage()
{
	return il2FixImage(iCurImage);
}

ILAPI ILboolean ilRemoveAlpha()
{
	return il2RemoveAlpha(iCurImage);
}

ILAPI ILboolean ilFixCur()
{
	return il2FixImage(iCurImage);
}

//! Uploads Data of the same size to replace the current image's data.
/*! \param Data New image data to update the currently bound image
	\exception IL_ILLEGAL_OPERATION No currently bound image
	\exception IL_INVALID_PARAM Data was NULL.
	\return Boolean value of failure or success
*/
ILAPI ILboolean ILAPIENTRY ilSetData(void *Data)
{
	return il2SetData(iCurImage, Data);
}


//! Returns a pointer to the current image's data.
/*! The pointer to the image data returned by this function is only valid until any
    operations are done on the image.  After any operations, this function should be
	called again.  The pointer can be cast to other types for images that have more
	than one byte per channel for easier access to data.
	\exception IL_ILLEGAL_OPERATION No currently bound image
	\return ILubyte pointer to image data.*/
ILAPI ILubyte* ILAPIENTRY ilGetData(void)
{

	return il2GetData(iCurImage);
}

//! Returns a pointer to the current image's palette data.
/*! The pointer to the image palette data returned by this function is only valid until
	any operations are done on the image.  After any operations, this function should be
	called again.
	\exception IL_ILLEGAL_OPERATION No currently bound image
	\return ILubyte pointer to image palette data.*/
ILAPI ILubyte* ILAPIENTRY ilGetPalette(void)
{
	return il2GetPalette(iCurImage);
}

//! Clears the current bound image to the values specified in ilClearColour
ILAPI ILboolean ILAPIENTRY ilClearImage()
{
	return il2ClearImage(iCurImage);
}

ILAPI ILboolean ILAPIENTRY ilSurfaceToDxtcData(ILenum Format)
{
	return iSurfaceToDxtcData(iCurImage, Format);
}

ILAPI ILboolean ILAPIENTRY ilDefaultImage()
{
	return il2DefaultImage(iCurImage);
}

ILAPI ILubyte*  ILAPIENTRY ilGetAlpha(ILenum Type)
{
	return il2GetAlpha(iCurImage, Type);
}

ILAPI ILboolean ILAPIENTRY ilBlit(ILuint Source, ILint DestX,  ILint DestY,   ILint DestZ, 
	ILuint SrcX,  ILuint SrcY,   ILuint SrcZ,
	ILuint Width, ILuint Height, ILuint Depth)
{
	ILimage * targetImage = iCurImage;
	ILuint targetName = ilGetCurName();

	ilBindImage(Source);
	ILimage * sourceImage = iCurImage;

	ILboolean result = il2Blit(sourceImage, targetImage, DestX, DestY, DestZ, 
		SrcX, SrcY, SrcZ, Width, Height, Depth);

	ilBindImage(targetName);
	return result;
}

//! Overlays the image found in Src on top of the current bound image at the coords specified.
ILAPI ILboolean ILAPIENTRY ilOverlayImage(ILuint Source, ILint XCoord, ILint YCoord, ILint ZCoord)
{
	ILuint	Width, Height, Depth;
	ILuint	Dest;
	
	Dest = ilGetCurName();
	ilBindImage(Source);
	Width = iCurImage->Width;  
	Height = iCurImage->Height;  
	Depth = iCurImage->Depth;
	ilBindImage(Dest);

	return ilBlit(Source, XCoord, YCoord, ZCoord, 0, 0, 0, Width, Height, Depth);
}

//! Copies everything from Src to the current bound image.
ILAPI ILboolean ILAPIENTRY ilCopyImage(ILuint Src)
{
	ILuint DestName = ilGetCurName();
	ILimage *DestImage = iCurImage, *SrcImage;
	
	if (iCurImage == NULL || DestName == 0) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	
	ilBindImage(Src);
	SrcImage = iCurImage;
	ilBindImage(DestName);
	return il2CopyImage(SrcImage, DestImage);
	
	return IL_TRUE;
}

// Copy data and attributes of the currently bound image into a new image
ILAPI ILuint ILAPIENTRY ilCloneCurImage()
{
	ILuint Id;
	ILimage *CurImage;
	
	if (iCurImage == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return 0;
	}
	
	ilGenImages(1, &Id);
	if (Id == 0)
		return 0;
	
	CurImage = iCurImage;
	
	ilBindImage(Id);
	ilTexImage(CurImage->Width, CurImage->Height, CurImage->Depth, CurImage->Bpp, CurImage->Format, CurImage->Type, CurImage->Data);
	il2CopyImageAttr(iCurImage, CurImage);
	
	iCurImage = CurImage;
	
	return Id;
}


ILimage *iGetBaseImage()
{
	return ImageStack[ilGetCurName()];
}


// Returns the current index.
ILAPI ILuint ILAPIENTRY ilGetCurName()
{
	if (iCurImage == NULL || ImageStack == NULL || StackSize == 0)
		return 0;
	return CurName;
}


// Returns the current image.
ILAPI ILimage* ILAPIENTRY ilGetCurImage()
{
	return iCurImage;
}


//! Used for setting the current image if it is an animation.
ILAPI ILboolean ILAPIENTRY ilActiveImage(ILuint Number)
{
	ILimage* candidate = il2GetFrame(iCurImage, Number);

	if (candidate != NULL) {
		iCurImage = candidate;
		return IL_TRUE;
	} else 
		return IL_FALSE;
}


// Completely replaces the current image and the version in the image stack.
ILAPI void ILAPIENTRY ilReplaceCurImage(ILimage *Image)
{
	if (iCurImage) {
		ilActiveImage(0);
		ilCloseImage(iCurImage);
	}
	ImageStack[ilGetCurName()] = Image;
	iCurImage = Image;
	return;
}


// Like realloc but sets new memory to 0.
ILAPI void* ILAPIENTRY ilRecalloc(void *Ptr, ILuint OldSize, ILuint NewSize)
{
	void *Temp = ialloc(NewSize);
	ILuint CopySize = (OldSize < NewSize) ? OldSize : NewSize;

	if (Temp != NULL) {
		if (Ptr != NULL) {
			memcpy(Temp, Ptr, CopySize);
			ifree(Ptr);
		}

		Ptr = Temp;

		if (OldSize < NewSize)
			imemclear((ILubyte*)Temp + OldSize, NewSize - OldSize);
	}

	return Temp;
}


// Internal function to enlarge the image stack by I_STACK_INCREMENT members.
ILboolean iEnlargeStack()
{
	if (!(ImageStack = (ILimage**)ilRecalloc(ImageStack, StackSize * sizeof(ILimage*), (StackSize + I_STACK_INCREMENT) * sizeof(ILimage*)))) {
		return IL_FALSE;
	}
	StackSize += I_STACK_INCREMENT;
	return IL_TRUE;
}


// Frees any extra memory in the stack.
//	- Should be called on exit or when unloading the library
ILAPI void ILAPIENTRY ilShutDown()
{
	// if it is not initialized do not shutdown
	iFree* TempFree = (iFree*)FreeNames;
	ILuint i;
	
	while (TempFree != NULL) {
		FreeNames = (iFree*)TempFree->Next;
		ifree(TempFree);
		TempFree = FreeNames;
	}

	//for (i = 0; i < LastUsed; i++) {
	for (i = 0; i < StackSize; i++) {
		if (ImageStack[i] != NULL)
			ilCloseImage(ImageStack[i]);
	}

	if (ImageStack)
		ifree(ImageStack);
	ImageStack = NULL;
	LastUsed = 0;
	StackSize = 0;
	return;
}


// Initializes the image stack's first entry (default image) -- ONLY CALL ONCE!
void iSetImage0()
{
	if (ImageStack == NULL)
		if (!iEnlargeStack())
			return;

	LastUsed = 1;
	CurName = 0;
	if (!ImageStack[0])
		ImageStack[0] = il2NewImage(1, 1, 1, 1, 1);
	iCurImage = ImageStack[0];
	il2DefaultImage(iCurImage);

	return;
}


ILAPI void ILAPIENTRY iBindImageTemp()
{
	if (ImageStack == NULL || StackSize <= 1)
		if (!iEnlargeStack())
			return;

	if (LastUsed < 2)
		LastUsed = 2;
	CurName = 1;
	if (!ImageStack[1])
		ImageStack[1] = il2NewImage(1, 1, 1, 1, 1);
	iCurImage = ImageStack[1];

	return;
}


//! Sets the current mipmap level
ILAPI ILboolean ILAPIENTRY ilActiveMipmap(ILuint Number)
{
	ILimage* candidate = il2GetMipmap(iCurImage, Number);

	if (candidate != NULL) {
		iCurImage = candidate;
		return IL_TRUE;
	} else
		return IL_FALSE;
}


//! Used for setting the current face if it is a cubemap.
ILAPI ILboolean ILAPIENTRY ilActiveFace(ILuint Number)
{
	ILimage* candidate = il2GetFace(iCurImage, Number);

	if (candidate != NULL) {
		iCurImage = candidate;
		return IL_TRUE;
	} else
		return IL_FALSE;
}


//! Used for setting the current layer if layers exist.
ILAPI ILboolean ILAPIENTRY ilActiveLayer(ILuint Number)
{
	ILimage* candidate = il2GetLayer(iCurImage, Number);

	if (candidate != NULL) {
		iCurImage = candidate;
		return IL_TRUE;
	} else
		return IL_FALSE;
}


// To be only used when the original image is going to be set back almost immediately.
ILAPI void ILAPIENTRY ilSetCurImage(ILimage *Image)
{
	iCurImage = Image;
	return;
}

ILAPI ILuint ILAPIENTRY ilCreateSubImage(ILenum Type, ILuint Num)
{
	return il2CreateSubImage(iCurImage, Type, Num);
}

//! Internal function to figure out where we are in an image chain.
//@TODO: This may get much more complex with mipmaps under faces, etc.
ILAPI ILuint iGetActiveNum(ILenum Type)
{
	ILimage *BaseImage;
	ILuint Num = 0;

	if (iCurImage == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return 0;
	}

	BaseImage = iGetBaseImage();
	if (BaseImage == iCurImage)
		return 0;

	switch (Type)
	{
		case IL_ACTIVE_IMAGE:
			BaseImage = BaseImage->Next;
			do {
				if (BaseImage == NULL)
					return 0;
				Num++;
				if (BaseImage == iCurImage)
					return Num;
			} while ((BaseImage = BaseImage->Next));
			break;
		case IL_ACTIVE_MIPMAP:
			BaseImage = BaseImage->Mipmaps;
			do {
				if (BaseImage == NULL)
					return 0;
				Num++;
				if (BaseImage == iCurImage)
					return Num;
			} while ((BaseImage = BaseImage->Mipmaps));
			break;
		/*case IL_ACTIVE_LAYER:
			BaseImage = BaseImage->Layers;
			do {
				if (BaseImage == NULL)
					return 0;
				Num++;
				if (BaseImage == iCurImage)
					return Num;
			} while ((BaseImage = BaseImage->Layers));
			break;*/
		case IL_ACTIVE_FACE:
			BaseImage = BaseImage->Faces;
			do {
				if (BaseImage == NULL)
					return 0;
				Num++;
				if (BaseImage == iCurImage)
					return Num;
			} while ((BaseImage = BaseImage->Faces));
			break;
	}

	//@TODO: Any error needed here?

	return 0;
}


ILAPI void ILAPIENTRY ilSetImageInteger(ILenum Mode, ILint Param)
{
	il2SetImageInteger(iCurImage, Mode, Param);
}

ILAPI void ILAPIENTRY ilGetImageInteger(ILenum Mode, ILint *Param)
{
	il2GetImageInteger(iCurImage, Mode, Param);
}

// ONLY call at startup.
ILAPI void ILAPIENTRY ilInit()
{
	il2Init();
}

ILAPI void ILAPIENTRY ilRegisterPal(void *Pal, ILuint Size, ILenum Type)
{
	il2RegisterPal(iCurImage, Pal, Size, Type);
}

//! Sets Param equal to the current value of the Mode
ILAPI void ILAPIENTRY ilGetIntegerv(ILenum Mode, ILint *Param)
{
	return il2GetIntegerv(Mode, Param);
}

//! Returns the current value of the Mode
ILAPI ILint ILAPIENTRY ilGetInteger(ILenum Mode)
{
	ILint Temp;
	Temp = 0;
	ilGetIntegerv(Mode, &Temp);
	return Temp;
}

ILAPI ILuint ILAPIENTRY ilGetDXTCData(void *Buffer, ILuint BufferSize, ILenum DXTCFormat)
{
	return il2GetDXTCData(iCurImage, Buffer, BufferSize, DXTCFormat);
}

ILAPI void ILAPIENTRY ilClearColour(ILclampf Red, ILclampf Green, ILclampf Blue, ILclampf Alpha)
{
	il2ClearColour(Red, Green, Blue, Alpha);
}

//! Enables a mode
ILAPI ILboolean ILAPIENTRY ilEnable(ILenum Mode)
{
	return ilAble(Mode, IL_TRUE);
}


//! Disables a mode
ILAPI ILboolean ILAPIENTRY ilDisable(ILenum Mode)
{
	return ilAble(Mode, IL_FALSE);
}

//! Sets Param equal to the current value of the Mode
ILAPI void ILAPIENTRY ilGetBooleanv(ILenum mode, ILboolean *param)
{
	il2GetBooleanv(mode, param);
}


//! Returns the current value of the Mode
ILAPI ILboolean ILAPIENTRY ilGetBoolean(ILenum mode)
{
	return il2GetBoolean(mode);
}

// Sets the current error
ILAPI void ILAPIENTRY ilSetError(ILenum Error)
{
	il2SetError(Error);
}

//! Gets the last error
ILAPI ILenum ILAPIENTRY ilGetError(void)
{
	return il2GetError();
}

// Set global state
ILAPI void ILAPIENTRY ilSetInteger(ILenum Mode, ILint Param)
{
	il2SetInteger(Mode, Param);
}

ILAPI void ILAPIENTRY ilSetPixels(ILint XOff, ILint YOff, ILint ZOff, 
	ILuint Width, ILuint Height, ILuint Depth, ILenum Format, ILenum Type, void *Data)
{
	il2SetPixels(iCurImage, XOff, YOff, ZOff, Width, Height, Depth, Format, Type, Data);
}

//! Converts the current image to the DestFormat format.
ILAPI ILboolean ILAPIENTRY ilConvertPal(ILenum DestFormat)
{
	return il2ConvertPal(iCurImage, DestFormat);
}

//! Checks whether the mode is enabled.
ILAPI ILboolean ILAPIENTRY ilIsEnabled(ILenum Mode)
{
	return il2IsEnabled(Mode);
}

//! Returns a constant string detailing aspects about this library.
ILAPI ILconst_string ILAPIENTRY ilGetString(ILenum StringName)
{
	return il2GetString(StringName);
}

// Sets the current palette.
ILAPI void ILAPIENTRY ilSetPal(ILpal *pal)
{
	il2SetPal(iCurImage, pal);
}

//! Loads a palette from FileName into the current image's palette.
ILAPI ILboolean ILAPIENTRY ilLoadPal(ILconst_string FileName)
{
	return il2LoadPal(iCurImage, FileName);
}