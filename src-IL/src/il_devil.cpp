//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified in 2014 by Björn Ganster
//
// Filename: src-IL/src/il_devil.cpp
//
// Description: API 2 implementation
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include <string.h>
#include <limits.h>
#include "il_manip.h"


ILAPI ILboolean ILAPIENTRY il2InitImage(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, 
	ILenum Format, ILenum Type, void *Data)
{
	memset(Image, 0, sizeof(ILimage));

	Image->Origin      = IL_ORIGIN_LOWER_LEFT;
	Image->Pal.use(0, NULL, IL_PAL_NONE);
	Image->Duration    = 0;
	Image->DxtcFormat  = IL_DXT_NO_COMP;
	Image->DxtcData    = NULL;

	return il2TexImage(Image, Width, Height, Depth, Bpp, Format, Type, Data);
}


// Creates a new ILimage based on the specifications given
ILAPI ILimage* ILAPIENTRY il2NewImage(ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILubyte Bpc)
{
	ILimage *Image;

	if (Bpp == 0 || Bpp > 4) {
		return NULL;
	}

	Image = (ILimage*)ialloc(sizeof(ILimage));
	if (Image == NULL) {
		return NULL;
	}

	if (!il2InitImage(Image, Width, Height, Depth, Bpp, ilGetFormatBpp(Bpp), ilGetTypeBpc(Bpc), NULL)) {
		if (Image->Data != NULL) {
			ifree(Image->Data);
		}
		ifree(Image);
		return NULL;
	}
	
	return Image;
}

// Generate a new image
ILAPI ILimage* ILAPIENTRY il2GenImage()
{
	return il2NewImage(1, 1, 1, 1, 1);
}

// Same as above but allows specification of Format and Type
ILAPI ILimage* ILAPIENTRY ilNewImageFull(ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILenum Format, ILenum Type, void *Data)
{
	ILimage *image;

	if (Bpp == 0 || Bpp > 4) {
		return NULL;
	}

	image = (ILimage*)ialloc(sizeof(ILimage));
	if (image == NULL) {
		return NULL;
	}

	if (!il2InitImage(image, Width, Height, Depth, Bpp, Format, Type, Data)) {
		il2DeleteImage(image);
		return NULL;
	}
	
	return image;
}

// Delete an image and all associated data
ILAPI void ILAPIENTRY il2DeleteImage(ILimage * image)
{
	if (image->Data != NULL) 
		ifree(image->Data);

	if (image->Mipmaps != NULL) 
		ifree(image->Mipmaps);

	if (image->Next != NULL) 
		ifree(image->Next);

	if (image->Faces != NULL) 
		ifree(image->Faces);

	if (image->Layers != NULL) 
		ifree(image->Layers);

	if (image->DxtcData != NULL) 
		ifree(image->DxtcData);

	ifree(image);
}

//! Changes an image to use new dimensions (current data is destroyed).
/*! \param Image Specifies the image. The function fails if this is zero. 
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
ILAPI ILboolean ILAPIENTRY il2TexImage(ILimage *Image, ILuint Width, 
	ILuint Height, ILuint Depth, ILubyte Bpp, ILenum Format, ILenum Type, 
	void *Data)
{
	if (Image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	ILubyte BpcType = ilGetBpcType(Type);
	if (BpcType == 0) {
		il2SetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	// Reset palette
	Image->Pal.clear();

	ilCloseImage(Image->Mipmaps);
	Image->Mipmaps = NULL;

	ilCloseImage(Image->Next);
	Image->Next = NULL;

	ilCloseImage(Image->Faces);
	Image->Faces = NULL; 

	ilCloseImage(Image->Layers);
	Image->Layers = NULL;

	if (Image->AnimList) ifree(Image->AnimList);
	if (Image->Profile)  ifree(Image->Profile);
	if (Image->DxtcData) ifree(Image->DxtcData);
	if (Image->Data)	 ifree(Image->Data);

	////

	//@TODO: Also check against format?
	/*if (Width == 0 || Height == 0 || Depth == 0 || Bpp == 0) {
		il2SetError(IL_INVALID_PARAM);
	return IL_FALSE;
	}*/

	////
	if (Width  == 0) Width = 1;
	if (Height == 0) Height = 1;
	if (Depth  == 0) Depth = 1;
	Image->Width	   = Width;
	Image->Height	   = Height;
	Image->Depth	   = Depth;
	Image->Bpp		   = Bpp;
	Image->Bpc		   = BpcType;
	Image->Bps		   = Width * Bpp * Image->Bpc;
	Image->SizeOfPlane = Image->Bps * Height;
	Image->SizeOfData  = Image->SizeOfPlane * Depth;
	Image->Format      = Format;
	Image->Type        = Type;
	Image->Data = (ILubyte*)ialloc(Image->SizeOfData);

	if (Image->Data != NULL) {
		if (Data != NULL) {
			memcpy(Image->Data, Data, Image->SizeOfData);
		}

		return IL_TRUE;
	} else {
		return IL_FALSE;
	}
}


// Internal version of ilTexSubImage.
ILAPI ILboolean ILAPIENTRY il2TexSubImage_(ILimage *Image, void *Data)
{
	if (Image == NULL || Data == NULL) {
		il2SetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}
	if (!Image->Data) {
		Image->Data = (ILubyte*)ialloc(Image->SizeOfData);
		if (Image->Data == NULL)
			return IL_FALSE;
	}
	memcpy(Image->Data, Data, Image->SizeOfData);
	return IL_TRUE;
}


//! Uploads Data of the same size to replace the current image's data.
/*! \param Data New image data to update the currently bound image
	\exception IL_ILLEGAL_OPERATION No currently bound image
	\exception IL_INVALID_PARAM Data was NULL.
	\return Boolean value of failure or success
*/
ILAPI ILboolean ILAPIENTRY il2SetData(ILimage* image, void *Data)
{
	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	
	return il2TexSubImage_(image, Data);
}


//! Returns a pointer to the current image's data.
/*! The pointer to the image data returned by this function is only valid until any
    operations are done on the image.  After any operations, this function should be
	called again.  The pointer can be cast to other types for images that have more
	than one byte per channel for easier access to data.
	\exception IL_ILLEGAL_OPERATION No currently bound image
	\return ILubyte pointer to image data.*/
ILAPI ILubyte* ILAPIENTRY il2GetData(ILimage* image)
{
	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return NULL;
	}
	
	return image->Data;
}


//! Returns a pointer to the current image's palette data.
/*! The pointer to the image palette data returned by this function is only valid until
	any operations are done on the image.  After any operations, this function should be
	called again.
	\exception IL_ILLEGAL_OPERATION No currently bound image
	\return ILubyte pointer to image palette data.*/
ILAPI ILubyte* ILAPIENTRY il2GetPalette(ILimage* image)
{
	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return NULL;
	}
	
	return image->Pal.getPalette();
}


//ILfloat ClearRed = 0.0f, ClearGreen = 0.0f, ClearBlue = 0.0f, ClearAlpha = 0.0f;

// Changed to the colour of the Universe
//	(http://www.newscientist.com/news/news.jsp?id=ns99991775)
//	*(http://www.space.com/scienceastronomy/universe_color_020308.html)*
//ILfloat ClearRed = 0.269f, ClearGreen = 0.388f, ClearBlue = 0.342f, ClearAlpha = 0.0f;
static ILfloat ClearRed   = 1.0f;
static ILfloat ClearGreen = 0.972549f;
static ILfloat ClearBlue  = 0.90588f;
static ILfloat ClearAlpha = 0.0f;
static ILfloat ClearLum   = 1.0f;

ILAPI void ILAPIENTRY il2ClearColour(ILclampf Red, ILclampf Green, ILclampf Blue, ILclampf Alpha)
{
	// Clamp to 0.0f - 1.0f.
	ClearRed	= Red < 0.0f ? 0.0f : (Red > 1.0f ? 1.0f : Red);
	ClearGreen	= Green < 0.0f ? 0.0f : (Green > 1.0f ? 1.0f : Green);
	ClearBlue	= Blue < 0.0f ? 0.0f : (Blue > 1.0f ? 1.0f : Blue);
	ClearAlpha	= Alpha < 0.0f ? 0.0f : (Alpha > 1.0f ? 1.0f : Alpha);
	
	if ((Red == Green) && (Red == Blue) && (Green == Blue)) {
		ClearLum = Red < 0.0f ? 0.0f : (Red > 1.0f ? 1.0f : Red);
	}
	else {
		ClearLum = 0.212671f * ClearRed + 0.715160f * ClearGreen + 0.072169f * ClearBlue;
		ClearLum = ClearLum < 0.0f ? 0.0f : (ClearLum > 1.0f ? 1.0f : ClearLum);
	}
	
	return;
}


ILAPI void ILAPIENTRY il2GetClear(void *Colours, ILenum Format, ILenum Type)
{
	ILubyte 	*BytePtr;
	ILushort	*ShortPtr;
	ILuint		*IntPtr;
	ILfloat 	*FloatPtr;
	ILdouble	*DblPtr;
	
	switch (Type)
	{
		case IL_BYTE:
		case IL_UNSIGNED_BYTE:
			BytePtr = (ILubyte*)Colours;
			switch (Format)
			{
				case IL_RGB:
					BytePtr[0] = (ILubyte)(ClearRed * UCHAR_MAX);
					BytePtr[1] = (ILubyte)(ClearGreen * UCHAR_MAX);
					BytePtr[2] = (ILubyte)(ClearBlue * UCHAR_MAX);
					break;
					
				case IL_RGBA:
					BytePtr[0] = (ILubyte)(ClearRed * UCHAR_MAX);
					BytePtr[1] = (ILubyte)(ClearGreen * UCHAR_MAX);
					BytePtr[2] = (ILubyte)(ClearBlue * UCHAR_MAX);
					BytePtr[3] = (ILubyte)(ClearAlpha * UCHAR_MAX);
					break;
					
				case IL_BGR:
					BytePtr[2] = (ILubyte)(ClearRed * UCHAR_MAX);
					BytePtr[1] = (ILubyte)(ClearGreen * UCHAR_MAX);
					BytePtr[0] = (ILubyte)(ClearBlue * UCHAR_MAX);
					BytePtr[3] = (ILubyte)(ClearAlpha * UCHAR_MAX);
					break;
					
				case IL_BGRA:
					BytePtr[2] = (ILubyte)(ClearRed * UCHAR_MAX);
					BytePtr[1] = (ILubyte)(ClearGreen * UCHAR_MAX);
					BytePtr[0] = (ILubyte)(ClearBlue * UCHAR_MAX);
					BytePtr[3] = (ILubyte)(ClearAlpha * UCHAR_MAX);
					break;
					
				case IL_LUMINANCE:
					BytePtr[0] = (ILubyte)(ClearAlpha * UCHAR_MAX);
					break;
					
				case IL_LUMINANCE_ALPHA:
					BytePtr[0] = (ILubyte)(ClearLum * UCHAR_MAX);
					BytePtr[1] = (ILubyte)(ClearAlpha * UCHAR_MAX);
					
				case IL_COLOUR_INDEX:
					BytePtr[0] = (ILubyte)(ClearAlpha * UCHAR_MAX);
					break;
					
				default:
					il2SetError(IL_INTERNAL_ERROR);
					return;
			}
				break;
			
		case IL_SHORT:
		case IL_UNSIGNED_SHORT:
			ShortPtr = (ILushort*)Colours;
			switch (Format)
			{
				case IL_RGB:
					ShortPtr[0] = (ILushort)(ClearRed * USHRT_MAX);
					ShortPtr[1] = (ILushort)(ClearGreen * USHRT_MAX);
					ShortPtr[2] = (ILushort)(ClearBlue * USHRT_MAX);
					break;
					
				case IL_RGBA:
					ShortPtr[0] = (ILushort)(ClearRed * USHRT_MAX);
					ShortPtr[1] = (ILushort)(ClearGreen * USHRT_MAX);
					ShortPtr[2] = (ILushort)(ClearBlue * USHRT_MAX);
					ShortPtr[3] = (ILushort)(ClearAlpha * USHRT_MAX);
					break;
					
				case IL_BGR:
					ShortPtr[2] = (ILushort)(ClearRed * USHRT_MAX);
					ShortPtr[1] = (ILushort)(ClearGreen * USHRT_MAX);
					ShortPtr[0] = (ILushort)(ClearBlue * USHRT_MAX);
					ShortPtr[3] = (ILushort)(ClearAlpha * USHRT_MAX);
					break;
					
				case IL_BGRA:
					ShortPtr[2] = (ILushort)(ClearRed * USHRT_MAX);
					ShortPtr[1] = (ILushort)(ClearGreen * USHRT_MAX);
					ShortPtr[0] = (ILushort)(ClearBlue * USHRT_MAX);
					ShortPtr[3] = (ILushort)(ClearAlpha * USHRT_MAX);
					break;
					
				case IL_LUMINANCE:
					ShortPtr[0] = (ILushort)(ClearAlpha * USHRT_MAX);
					break;
					
				case IL_LUMINANCE_ALPHA:
					ShortPtr[0] = (ILushort)(ClearLum * USHRT_MAX);
					ShortPtr[1] = (ILushort)(ClearAlpha * USHRT_MAX);
					break;
					
				case IL_COLOUR_INDEX:
					ShortPtr[0] = (ILushort)(ClearAlpha * USHRT_MAX);
					break;
					
				default:
					il2SetError(IL_INTERNAL_ERROR);
					return;
			}
				break;
			
		case IL_INT:
		case IL_UNSIGNED_INT:
			IntPtr = (ILuint*)Colours;
			switch (Format)
			{
				case IL_RGB:
					IntPtr[0] = (ILuint)(ClearRed * UINT_MAX);
					IntPtr[1] = (ILuint)(ClearGreen * UINT_MAX);
					IntPtr[2] = (ILuint)(ClearBlue * UINT_MAX);
					break;
					
				case IL_RGBA:
					IntPtr[0] = (ILuint)(ClearRed * UINT_MAX);
					IntPtr[1] = (ILuint)(ClearGreen * UINT_MAX);
					IntPtr[2] = (ILuint)(ClearBlue * UINT_MAX);
					IntPtr[3] = (ILuint)(ClearAlpha * UINT_MAX);
					break;
					
				case IL_BGR:
					IntPtr[2] = (ILuint)(ClearRed * UINT_MAX);
					IntPtr[1] = (ILuint)(ClearGreen * UINT_MAX);
					IntPtr[0] = (ILuint)(ClearBlue * UINT_MAX);
					IntPtr[3] = (ILuint)(ClearAlpha * UINT_MAX);
					break;
					
				case IL_BGRA:
					IntPtr[2] = (ILuint)(ClearRed * UINT_MAX);
					IntPtr[1] = (ILuint)(ClearGreen * UINT_MAX);
					IntPtr[0] = (ILuint)(ClearBlue * UINT_MAX);
					IntPtr[3] = (ILuint)(ClearAlpha * UINT_MAX);
					break;
					
				case IL_LUMINANCE:
					IntPtr[0] = (ILuint)(ClearAlpha * UINT_MAX);
					break;
					
				case IL_LUMINANCE_ALPHA:
					IntPtr[0] = (ILuint)(ClearLum * UINT_MAX);
					IntPtr[1] = (ILuint)(ClearAlpha * UINT_MAX);
					break;
					
				case IL_COLOUR_INDEX:
					IntPtr[0] = (ILuint)(ClearAlpha * UINT_MAX);
					break;
					
				default:
					il2SetError(IL_INTERNAL_ERROR);
					return;
			}
				break;
			
		case IL_FLOAT:
			FloatPtr = (ILfloat*)Colours;
			switch (Format)
			{
				case IL_RGB:
					FloatPtr[0] = ClearRed;
					FloatPtr[1] = ClearGreen;
					FloatPtr[2] = ClearBlue;
					break;
					
				case IL_RGBA:
					FloatPtr[0] = ClearRed;
					FloatPtr[1] = ClearGreen;
					FloatPtr[2] = ClearBlue;
					FloatPtr[3] = ClearAlpha;
					break;
					
				case IL_BGR:
					FloatPtr[2] = ClearRed;
					FloatPtr[1] = ClearGreen;
					FloatPtr[0] = ClearBlue;
					FloatPtr[3] = ClearAlpha;
					break;
					
				case IL_BGRA:
					FloatPtr[2] = ClearRed;
					FloatPtr[1] = ClearGreen;
					FloatPtr[0] = ClearBlue;
					FloatPtr[3] = ClearAlpha;
					break;
					
				case IL_LUMINANCE:
					FloatPtr[0] = ClearAlpha;
					break;
					
				case IL_LUMINANCE_ALPHA:
					FloatPtr[0] = ClearLum;
					FloatPtr[0] = ClearAlpha;
					break;
					
				case IL_COLOUR_INDEX:
					FloatPtr[0] = ClearAlpha;
					break;
					
				default:
					il2SetError(IL_INTERNAL_ERROR);
					return;
			}
				break;
			
		case IL_DOUBLE:
			DblPtr = (ILdouble*)Colours;
			switch (Format)
			{
				case IL_RGB:
					DblPtr[0] = ClearRed;
					DblPtr[1] = ClearGreen;
					DblPtr[2] = ClearBlue;
					break;
					
				case IL_RGBA:
					DblPtr[0] = ClearRed;
					DblPtr[1] = ClearGreen;
					DblPtr[2] = ClearBlue;
					DblPtr[3] = ClearAlpha;
					break;
					
				case IL_BGR:
					DblPtr[2] = ClearRed;
					DblPtr[1] = ClearGreen;
					DblPtr[0] = ClearBlue;
					DblPtr[3] = ClearAlpha;
					break;
					
				case IL_BGRA:
					DblPtr[2] = ClearRed;
					DblPtr[1] = ClearGreen;
					DblPtr[0] = ClearBlue;
					DblPtr[3] = ClearAlpha;
					break;
					
				case IL_LUMINANCE:
					DblPtr[0] = ClearAlpha;
					break;
					
				case IL_LUMINANCE_ALPHA:
					DblPtr[0] = ClearLum;
					DblPtr[1] = ClearAlpha;
					break;
					
				case IL_COLOUR_INDEX:
					DblPtr[0] = ClearAlpha;
					break;
					
				default:
					il2SetError(IL_INTERNAL_ERROR);
					return;
			}
				break;
			
		default:
			il2SetError(IL_INTERNAL_ERROR);
			return;
	}
	
	return;
}


ILAPI ILboolean ILAPIENTRY il2ClearImage(ILimage *Image)
{
	ILuint		i, c, NumBytes;
	ILubyte 	Colours[32];  // Maximum is sizeof(double) * 4 = 32
	ILubyte 	*BytePtr;
	ILushort	*ShortPtr;
	ILuint		*IntPtr;
	ILfloat 	*FloatPtr;
	ILdouble	*DblPtr;
	
	if (Image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	NumBytes = Image->Bpp * Image->Bpc;
	il2GetClear(Colours, Image->Format, Image->Type);
	
	if (Image->Format != IL_COLOUR_INDEX) {
		switch (Image->Type)
		{
			case IL_BYTE:
			case IL_UNSIGNED_BYTE:
				BytePtr = (ILubyte*)Colours;
				for (c = 0; c < NumBytes; c += Image->Bpc) {
					for (i = c; i < Image->SizeOfData; i += NumBytes) {
						Image->Data[i] = BytePtr[c];
					}
				}
					break;
				
			case IL_SHORT:
			case IL_UNSIGNED_SHORT:
				ShortPtr = (ILushort*)Colours;
				for (c = 0; c < NumBytes; c += Image->Bpc) {
					for (i = c; i < Image->SizeOfData; i += NumBytes) {
						*((ILushort*)(Image->Data + i)) = ShortPtr[c / Image->Bpc];
					}
				}
					break;
				
			case IL_INT:
			case IL_UNSIGNED_INT:
				IntPtr = (ILuint*)Colours;
				for (c = 0; c < NumBytes; c += Image->Bpc) {
					for (i = c; i < Image->SizeOfData; i += NumBytes) {
						*((ILuint*)(Image->Data + i)) = IntPtr[c / Image->Bpc];
					}
				}
					break;
				
			case IL_FLOAT:
				FloatPtr = (ILfloat*)Colours;
				for (c = 0; c < NumBytes; c += Image->Bpc) {
					for (i = c; i < Image->SizeOfData; i += NumBytes) {
						*((ILfloat*)(Image->Data + i)) = FloatPtr[c / Image->Bpc];
					}
				}
					break;
				
			case IL_DOUBLE:
				DblPtr = (ILdouble*)Colours;
				for (c = 0; c < NumBytes; c += Image->Bpc) {
					for (i = c; i < Image->SizeOfData; i += NumBytes) {
						*((ILdouble*)(Image->Data + i)) = DblPtr[c / Image->Bpc];
					}
				}
					break;
		}
	}
	else {
		imemclear(Image->Data, Image->SizeOfData);
		
		Image->Pal.use(1, NULL, IL_PAL_RGBA32);
		Image->Pal.setRGBA(0, Colours[0] * UCHAR_MAX, 
			Colours[1] * UCHAR_MAX, Colours[2] * UCHAR_MAX, Colours[3] * UCHAR_MAX);
	}
	
	return IL_TRUE;
}


//! Overlays the image found in Src on top of the current bound image at the coords specified.
ILboolean ILAPIENTRY il2OverlayImage(ILimage* aSource, ILimage* aTarget, ILint aXCoord, ILint aYCoord, 
	ILint aZCoord)
{
	return il2Blit(aSource, aTarget, aXCoord, aYCoord, aZCoord, 0, 0, 0, aSource->Width, aSource->Height, 
		aSource->Depth);
}

//@NEXT DestX,DestY,DestZ must be set to ILuint
ILboolean ILAPIENTRY il2Blit(ILimage* aSource, ILimage* aTarget, ILint DestX,  ILint DestY,   ILint DestZ, 
	ILuint SrcX,  ILuint SrcY,   ILuint SrcZ, ILuint Width, ILuint Height, ILuint Depth)
{
	ILuint 		x, y, z, ConvBps, ConvSizePlane;
	ILubyte 	*Converted;
	ILuint		DestName = ilGetCurName();
	ILuint		c;
	ILuint		StartX, StartY, StartZ;
	ILboolean	DestFlipped = IL_FALSE;
	ILboolean	DoAlphaBlend = IL_FALSE;
	ILubyte 	*SrcTemp;
	ILfloat		ResultAlpha;

	// Check if source and target images really exist
	if (aSource == NULL || aTarget == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	
	// set the destination image to upper left origin
	if (aTarget->Origin == IL_ORIGIN_LOWER_LEFT) {  // Dest
		DestFlipped = IL_TRUE;
		ilFlipImage(aTarget);
	}

	//determine alpha support
	DoAlphaBlend = il2IsEnabled(IL_BLIT_BLEND);

	//@TODO test if coordinates are inside the images (hard limit for source)
	
	// set the source image to upper left origin
	if (aSource->Origin == IL_ORIGIN_LOWER_LEFT)
	{
		SrcTemp = iGetFlipped(aSource);
		if (SrcTemp == NULL)
		{
			if (DestFlipped)
				ilFlipImage(aTarget);
			return IL_FALSE;
		}
	}
	else
	{
		SrcTemp = aSource->Data;
	}
	
	// convert source image to match the destination image type and format
	Converted = (ILubyte*)ilConvertBuffer(aSource->SizeOfData, aSource->Format, aTarget->Format, 
		aSource->Type, aTarget->Type, NULL, SrcTemp);
	if (Converted == NULL)
		return IL_FALSE;
	
	ConvBps 	  = aTarget->Bpp * aSource->Width;
	ConvSizePlane = ConvBps   * aSource->Height;
	
	//@NEXT in next version this would have to be removed since Dest* will be unsigned
	StartX = DestX >= 0 ? 0 : -DestX;
	StartY = DestY >= 0 ? 0 : -DestY;
	StartZ = DestZ >= 0 ? 0 : -DestZ;
	
	// Limit the copy of data inside of the destination image
	if (Width  + DestX > aTarget->Width)  Width  = aTarget->Width  - DestX;
	if (Height + DestY > aTarget->Height) Height = aTarget->Height - DestY;
	if (Depth  + DestZ > aTarget->Depth)  Depth  = aTarget->Depth  - DestZ;
	
	//@TODO: non funziona con rgba
	if (aSource->Format == IL_RGBA || aSource->Format == IL_BGRA || aSource->Format == IL_LUMINANCE_ALPHA) {
		const ILuint bpp_without_alpha = aTarget->Bpp - 1;
		for (z = 0; z < Depth; z++) {
			for (y = 0; y < Height; y++) {
				for (x = 0; x < Width; x++) {
					const ILuint  SrcIndex  = (z+SrcZ)*ConvSizePlane + (y+SrcY)*ConvBps + (x+SrcX)*aTarget->Bpp;
					const ILuint  DestIndex = (z+DestZ)*aTarget->SizeOfPlane + (y+DestY)*aTarget->Bps + (x+DestX)*aTarget->Bpp;
					const ILuint  AlphaIdx = SrcIndex + bpp_without_alpha;
					ILfloat FrontAlpha = 0; // foreground opacity
					ILfloat BackAlpha = 0;	// background opacity
					
					switch (aTarget->Type)
					{
						case IL_BYTE:
						case IL_UNSIGNED_BYTE:
							FrontAlpha = Converted[AlphaIdx]/((float)IL_MAX_UNSIGNED_BYTE);
							BackAlpha = aTarget->Data[AlphaIdx]/((float)IL_MAX_UNSIGNED_BYTE);
							break;
						case IL_SHORT:
						case IL_UNSIGNED_SHORT:
							FrontAlpha = ((ILshort*)Converted)[AlphaIdx]/((float)IL_MAX_UNSIGNED_SHORT);
							BackAlpha = ((ILshort*)aTarget->Data)[AlphaIdx]/((float)IL_MAX_UNSIGNED_SHORT);
							break;
						case IL_INT:
						case IL_UNSIGNED_INT:
							FrontAlpha = ((ILint*)Converted)[AlphaIdx]/((float)IL_MAX_UNSIGNED_INT);
							BackAlpha = ((ILint*)aTarget->Data)[AlphaIdx]/((float)IL_MAX_UNSIGNED_INT);
							break;
						case IL_FLOAT:
							FrontAlpha = ((ILfloat*)Converted)[AlphaIdx];
							BackAlpha = ((ILfloat*)aTarget->Data)[AlphaIdx];
							break;
						case IL_DOUBLE:
							FrontAlpha = (ILfloat)(((ILdouble*)Converted)[AlphaIdx]);
							BackAlpha = (ILfloat)(((ILdouble*)aTarget->Data)[AlphaIdx]);
							break;
					}
					
					// In case of Alpha channel, the data is blended.
					// Computes composite Alpha
					if (DoAlphaBlend)
					{
						ResultAlpha = FrontAlpha + (1.0f - FrontAlpha) * BackAlpha;
						for (c = 0; c < bpp_without_alpha; c++)
						{
							aTarget->Data[DestIndex + c] = (ILubyte)( 0.5f + 
								(Converted[SrcIndex + c] * FrontAlpha + 
								(1.0f - FrontAlpha) * aTarget->Data[DestIndex + c] * BackAlpha) 
								/ ResultAlpha);
						}
						aTarget->Data[AlphaIdx] = (ILubyte)(0.5f + ResultAlpha * (float)IL_MAX_UNSIGNED_BYTE);
					}
					else {
						for (c = 0; c < aTarget->Bpp; c++)
						{
							aTarget->Data[DestIndex + c] = (ILubyte)(Converted[SrcIndex + c]);
						}
					}
				}
			}
		}
	} else {
		for( z = 0; z < Depth; z++ ) {
			for( y = 0; y < Height; y++ ) {
				for( x = 0; x < Width; x++ ) {
					for( c = 0; c < aTarget->Bpp; c++ ) {
						aTarget->Data[(z+DestZ)*aTarget->SizeOfPlane + (y+DestY)*aTarget->Bps + (x+DestX)*aTarget->Bpp + c] =
						 Converted[(z+SrcZ)*ConvSizePlane + (y+SrcY)*ConvBps + (x+SrcX)*aTarget->Bpp + c];
					}
				}
			}
		}
	}
	
	if (SrcTemp != aSource->Data)
		ifree(SrcTemp);
	
	if (DestFlipped)
		ilFlipImage(aTarget);
	
	ifree(Converted);
	
	return IL_TRUE;
}


ILboolean iCopySubImage(ILimage *Dest, ILimage *Src)
{
	ILimage *DestTemp, *SrcTemp;
	
	DestTemp = Dest;
	SrcTemp = Src;
	
	do {
		il2CopyImageAttr(DestTemp, SrcTemp);
		DestTemp->Data = (ILubyte*)ialloc(SrcTemp->SizeOfData);
		if (DestTemp->Data == NULL) {
			return IL_FALSE;
		}
		memcpy(DestTemp->Data, SrcTemp->Data, SrcTemp->SizeOfData);
		
		if (SrcTemp->Next) {
			DestTemp->Next = (ILimage*)icalloc(1, sizeof(ILimage));
			if (!DestTemp->Next) {
				return IL_FALSE;
			}
		}
		else {
			DestTemp->Next = NULL;
		}
		
		DestTemp = DestTemp->Next;
		SrcTemp = SrcTemp->Next;
	} while (SrcTemp);
	
	return IL_TRUE;
}


ILboolean iCopySubImages(ILimage *Dest, ILimage *Src)
{
	if (Src->Faces) {
		Dest->Faces = (ILimage*)icalloc(1, sizeof(ILimage));
		if (!Dest->Faces) {
			return IL_FALSE;
		}
		if (!iCopySubImage(Dest->Faces, Src->Faces))
			return IL_FALSE;
	}
	
	if (Src->Layers) {
		Dest->Layers = (ILimage*)icalloc(1, sizeof(ILimage));
		if (!Dest->Layers) {
			return IL_FALSE;
		}
		if (!iCopySubImage(Dest->Layers, Src->Layers))
			return IL_FALSE;
	}

	if (Src->Mipmaps) {
		Dest->Mipmaps = (ILimage*)icalloc(1, sizeof(ILimage));
		if (!Dest->Mipmaps) {
			return IL_FALSE;
		}
		if (!iCopySubImage(Dest->Mipmaps, Src->Mipmaps))
			return IL_FALSE;
	}

	if (Src->Next) {
		Dest->Next = (ILimage*)icalloc(1, sizeof(ILimage));
		if (!Dest->Next) {
			return IL_FALSE;
		}
		if (!iCopySubImage(Dest->Next, Src->Next))
			return IL_FALSE;
	}

	return IL_TRUE;
}


// Copies everything but the Data from Src to Dest.
ILAPI ILboolean ILAPIENTRY il2CopyImageAttr(ILimage *Dest, ILimage *Src)
{
	if (Dest == NULL || Src == NULL) {
		il2SetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (Dest->Data != NULL) {
		ifree(Dest->Data);
		Dest->Data = NULL;
	}
	
	Dest->Pal.clear();
	if (Dest->Faces) {
		ilCloseImage(Dest->Faces);
		Dest->Faces = NULL;
	}
	if (Dest->Layers) {
		ilCloseImage(Dest->Layers);
		Dest->Layers = NULL;
	}
	if (Dest->Mipmaps) {
		ilCloseImage(Dest->Mipmaps);
		Dest->Mipmaps = NULL;
	}
	if (Dest->Next) {
		ilCloseImage(Dest->Next);
		Dest->Next = NULL;
	}
	if (Dest->Profile) {
		ifree(Dest->Profile);
		Dest->Profile = NULL;
		Dest->ProfileSize = 0;
	}
	if (Dest->DxtcData) {
		ifree(Dest->DxtcData);
		Dest->DxtcData = NULL;
		Dest->DxtcFormat = IL_DXT_NO_COMP;
		Dest->DxtcSize = 0;
	}
	
	if (Src->AnimList && Src->AnimSize) {
		Dest->AnimList = (ILuint*)ialloc(Src->AnimSize * sizeof(ILuint));
		if (Dest->AnimList == NULL) {
			return IL_FALSE;
		}
		memcpy(Dest->AnimList, Src->AnimList, Src->AnimSize * sizeof(ILuint));
	}
	if (Src->Profile) {
		Dest->Profile = (ILubyte*)ialloc(Src->ProfileSize);
		if (Dest->Profile == NULL) {
			return IL_FALSE;
		}
		memcpy(Dest->Profile, Src->Profile, Src->ProfileSize);
		Dest->ProfileSize = Src->ProfileSize;
	}

	Dest->Pal.use(Src->Pal.getNumCols(), Src->Pal.getPalette(), Src->Pal.getPalType());
	
	Dest->Width = Src->Width;
	Dest->Height = Src->Height;
	Dest->Depth = Src->Depth;
	Dest->Bpp = Src->Bpp;
	Dest->Bpc = Src->Bpc;
	Dest->Bps = Src->Bps;
	Dest->SizeOfPlane = Src->SizeOfPlane;
	Dest->SizeOfData = Src->SizeOfData;
	Dest->Format = Src->Format;
	Dest->Type = Src->Type;
	Dest->Origin = Src->Origin;
	Dest->Duration = Src->Duration;
	Dest->CubeFlags = Src->CubeFlags;
	Dest->AnimSize = Src->AnimSize;
	Dest->OffX = Src->OffX;
	Dest->OffY = Src->OffY;
	
	return IL_TRUE/*iCopySubImages(Dest, Src)*/;
}


// Creates a copy of Src and returns it.
ILAPI ILimage* ILAPIENTRY il2CopyImage_(ILimage *Src)
{
	ILimage *Dest;
	
	if (Src == NULL) {
		il2SetError(IL_INVALID_PARAM);
		return NULL;
	}
	
	Dest = il2NewImage(Src->Width, Src->Height, Src->Depth, Src->Bpp, Src->Bpc);
	if (Dest == NULL) {
		return NULL;
	}
	
	if (il2CopyImageAttr(Dest, Src) == IL_FALSE)
		return NULL;
	
	memcpy(Dest->Data, Src->Data, Src->SizeOfData);
	
	return Dest;
}


// Copy data and attributes of aSource into a new image
ILimage* ILAPIENTRY il2CloneImage(ILimage* aSource)
{
	if (aSource == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return NULL;
	}
	
	ILimage* newImage = il2GenImage();
	
	il2TexImage(newImage, aSource->Width, aSource->Height, aSource->Depth, aSource->Bpp, 
		aSource->Format, aSource->Type, aSource->Data);
	il2CopyImageAttr(newImage, aSource);
	
	return newImage;
}


// Like ilTexImage but doesn't destroy the palette.
ILAPI ILboolean ILAPIENTRY il2ResizeImage(ILimage *Image, ILuint Width, ILuint Height, 
	ILuint Depth, ILubyte Bpp, ILubyte Bpc)
{
	if (Image == NULL) {
		il2SetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}
	
	if (Image->Data != NULL) {
		ifree(Image->Data);
		Image-> Data = NULL;
	}
	
	Image->Depth = Depth;
	Image->Width = Width;
	Image->Height = Height;
	Image->Bpp = Bpp;
	Image->Bpc = Bpc;
	Image->Bps = Bpp * Bpc * Width;
	Image->SizeOfPlane = Image->Bps * Height;
	Image->SizeOfData = Image->SizeOfPlane * Depth;
	
	Image->Data = (ILubyte*)ialloc(Image->SizeOfData);
	if (Image->Data == NULL) {
		return IL_FALSE;
	}
	
	return IL_TRUE;
}

//! Used for querying the current image if it is an animation.
ILimage* ILAPIENTRY il2GetFrame(ILimage* image, ILuint Number)
{
	ILimage *iTempImage = image;
    
	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (Number == 0) {
		return iTempImage;
	}

	// Skip 0 (parent image)
	image = image->Next;
	if (image == NULL) {
		image = iTempImage;
		il2SetError(IL_ILLEGAL_OPERATION);
		return NULL;
	}
	Number--;

	for (ILuint Current = 0; Current < Number; Current++) {
		image = image->Next;
		if (image == NULL) {
			il2SetError(IL_ILLEGAL_OPERATION);
			image = iTempImage;
			return NULL;
		}
	}

	return iTempImage;
}

//! Used for querying facea of a cubemap
ILimage* ILAPIENTRY il2GetFace(ILimage* image, ILuint Number)
{
	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return NULL;
	}

	if (Number == 0) {
		return image;
	}

	ILimage *iTempImage = image;
	image = image->Faces;
	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return NULL;
	}

	//Number--;  // Skip 0 (parent image)
	for (ILuint Current = 1; Current < Number; Current++) {
		image = image->Faces;
		if (image == NULL) {
			il2SetError(IL_ILLEGAL_OPERATION);
			return NULL;
		}
	}

	return image;
}

//! Obtain a mipmap level
ILimage* ILAPIENTRY il2GetMipmap(ILimage* image, ILuint Number)
{
	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return NULL;
	}

	if (Number == 0) {
		return image;
	}

	image->Mipmaps->io = image->io;
	image = image->Mipmaps;
	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return NULL;
	}

	for (ILuint Current = 1; Current < Number; Current++) {
		image->Mipmaps->io = image->io;
		image = image->Mipmaps;
		if (image == NULL) {
			il2SetError(IL_ILLEGAL_OPERATION);
			return NULL;
		}
	}

	return image;
}

//! Get specific image layer, represented by an image
ILimage* ILAPIENTRY il2GetLayer(ILimage* image, ILuint Number)
{
	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return NULL;
	}

	if (Number == 0) {
		return image;
	}

	// Skip 0 (parent image)
	image = image->Layers;
	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	for (ILuint Current = 1; Current < Number; Current++) {
		image = image->Layers;
		if (image == NULL) {
			il2SetError(IL_ILLEGAL_OPERATION);
			return NULL;
		}
	}

	return image;
}

//! Copies everything from aSource to aTarget
ILboolean ILAPIENTRY il2CopyImage(ILimage* aSource, ILimage* aTarget)
{
	if (aSource == NULL || aTarget == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	
	il2TexImage(aTarget, aSource->Width, aSource->Height, aSource->Depth, aSource->Bpp, 
		aSource->Format, aSource->Type, aSource->Data);
	il2CopyImageAttr(aTarget, aSource);
	
	return IL_TRUE;
}

//! Enables a mode
ILboolean ILAPIENTRY il2Enable(ILenum Mode)
{
	return ilAble(Mode, IL_TRUE);
}


//! Disables a mode
ILboolean ILAPIENTRY il2Disable(ILenum Mode)
{
	return ilAble(Mode, IL_FALSE);
}

