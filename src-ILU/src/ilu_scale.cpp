///////////////////////////////////////////////////////////////////////////////
//
// ImageLib Utility Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 12/27/2008
//
// Filename: src-ILU/src/ilu_scale.c
//
// Description: Scales an image.
//
///////////////////////////////////////////////////////////////////////////////


#include "ilu_internal.h"
#include "ilu_states.h"


ILboolean ILAPIENTRY ilu2EnlargeImage(ILimage* image, ILfloat XDim, ILfloat YDim, ILfloat ZDim)
{
	if (XDim <= 0.0f || YDim <= 0.0f || ZDim <= 0.0f) {
		il2SetError(ILU_INVALID_PARAM);
		return IL_FALSE;
	}

	return ilu2Scale(image, (ILuint)(image->Width * XDim), (ILuint)(image->Height * YDim),
					(ILuint)(image->Depth * ZDim));
}


ILimage *iluScale1D_(ILimage *Image, ILimage *Scaled, ILuint Width);
ILimage *iluScale2D_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height);
ILimage *iluScale3D_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height, ILuint Depth);


ILboolean ILAPIENTRY ilu2Scale(ILimage* image, ILuint Width, ILuint Height, ILuint Depth)
{
	if (image == NULL) {
		il2SetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (image->Width == Width && image->Height == Height && image->Depth == Depth)
		return IL_TRUE;

	// A parameter of 0 is not valid.  Let's just assume that the user wanted a value of 1 instead.
	if (Width == 0)  Width = 1;
	if (Height == 0) Height = 1;
	if (Depth == 0)  Depth = 1;

	ILenum Origin = image->Origin;
	ILimage* Temp = NULL;

	if ((image->Width<Width) || (image->Height<Height)) // only do special scale if there is some zoom?
	{
		switch (iluFilter)
		{
			case ILU_SCALE_BOX:
			case ILU_SCALE_TRIANGLE:
			case ILU_SCALE_BELL:
			case ILU_SCALE_BSPLINE:
			case ILU_SCALE_LANCZOS3:
			case ILU_SCALE_MITCHELL:

				image = ilGetCurImage();
				if (image == NULL) {
					il2SetError(ILU_ILLEGAL_OPERATION);
					return IL_FALSE;
				}

				// Not supported yet.
				if (image->Type != IL_UNSIGNED_BYTE ||
					image->Format == IL_COLOUR_INDEX ||
					image->Depth > 1) {
						il2SetError(ILU_ILLEGAL_OPERATION);
						return IL_FALSE;
				}

				if (image->Width > Width) // shrink width first
				{
					Origin = image->Origin;
					Temp = iluScale_(image, Width, image->Height, image->Depth);
					if (Temp != NULL) {
						if (!il2TexImage(image, Temp->Width, Temp->Height, Temp->Depth, Temp->Bpp, Temp->Format, Temp->Type, Temp->Data)) {
							ilCloseImage(Temp);
							return IL_FALSE;
						}
						image->Origin = Origin;
						ilCloseImage(Temp);
					}
				}
				else if (image->Height > Height) // shrink height first
				{
					Origin = image->Origin;
					Temp = iluScale_(image, image->Width, Height, image->Depth);
					if (Temp != NULL) {
						if (!il2TexImage(image, Temp->Width, Temp->Height, Temp->Depth, Temp->Bpp, Temp->Format, Temp->Type, Temp->Data)) {
							ilCloseImage(Temp);
							return IL_FALSE;
						}
						image->Origin = Origin;
						ilCloseImage(Temp);
					}
				}

				return (ILboolean)ilu2ScaleAdvanced(image, Width, Height, iluFilter);
		}
	}

	ILboolean UsePal = (image->Format == IL_COLOUR_INDEX);
	ILenum PalType = image->Pal.getPalType();
	Temp = iluScale_(image, Width, Height, Depth);
	if (Temp != NULL) {
		if (!il2TexImage(image, Temp->Width, Temp->Height, Temp->Depth, Temp->Bpp, Temp->Format, Temp->Type, Temp->Data)) {
			ilCloseImage(Temp);
			return IL_FALSE;
		}
		image->Origin = Origin;
		ilCloseImage(Temp);
		if (UsePal) {
			if (!il2ConvertImage(image, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE))
				return IL_FALSE;
			il2ConvertPal(image, PalType);
		}
		return IL_TRUE;
	}

	return IL_FALSE;
}


ILAPI ILimage* ILAPIENTRY iluScale_(ILimage *image, ILuint Width, ILuint Height, ILuint Depth)
{
	ILimage	*Scaled = NULL, *ToScale = NULL;
	ILenum	Format, PalType;

	Format = image->Format;
	if (Format == IL_COLOUR_INDEX) {
		PalType = image->Pal.getPalType();
		ToScale = iConvertImage(image, ilGetPalBaseType(image->Pal.getPalType()), image->Type);
	}
	else {
		ToScale = image;
	}

	// So we don't replicate this 3 times (one in each iluScalexD_() function.
	Scaled = (ILimage*)icalloc(1, sizeof(ILimage));
	if (il2CopyImageAttr(Scaled, ToScale) == IL_FALSE) {
		ilCloseImage(Scaled);
		if (ToScale != image)
			ilCloseImage(ToScale);
		ilSetCurImage(image);
		return NULL;
	}
	if (il2ResizeImage(Scaled, Width, Height, Depth, ToScale->Bpp, ToScale->Bpc) == IL_FALSE) {
		ilCloseImage(Scaled);
		if (ToScale != image)
			ilCloseImage(ToScale);
		return NULL;
	}
	
	if (Height <= 1 && image->Height <= 1) {
		iluScale1D_(ToScale, Scaled, Width);
	}
	if (Depth <= 1 && image->Depth <= 1) {
		iluScale2D_(ToScale, Scaled, Width, Height);
	}
	else {
		iluScale3D_(ToScale, Scaled, Width, Height, Depth);
	}

	if (Format == IL_COLOUR_INDEX) {
		//ilSetCurImage(Scaled);
		//ilConvertImage(IL_COLOUR_INDEX);
		ilSetCurImage(image);
		ilCloseImage(ToScale);
	}

	return Scaled;
}


ILimage *iluScale1D_(ILimage *image, ILimage *Scaled, ILuint Width)
{
	ILuint		x1, x2;
	ILuint		NewX1, NewX2, NewX3, x, c;
	ILdouble	ScaleX, t1, t2, f;
	ILushort	*ShortPtr, *SShortPtr;
	ILuint		*IntPtr, *SIntPtr;

	if (image == NULL) {
		il2SetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	ScaleX = (ILdouble)Width / image->Width;

	ShortPtr = (ILushort*)image->Data;
	SShortPtr = (ILushort*)Scaled->Data;
	IntPtr = (ILuint*)image->Data;
	SIntPtr = (ILuint*)Scaled->Data;

	if (iluFilter == ILU_NEAREST) {
		switch (image->Bpc)
		{
			case 1:
				for (x = 0; x < Width; x++) {
					NewX1 = x * Scaled->Bpp;
					NewX2 = (ILuint)(x / ScaleX) * image->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						Scaled->Data[NewX1 + c] = image->Data[NewX2 + c];
					}
				}
				break;
			case 2:
				for (x = 0; x < Width; x++) {
					NewX1 = x * Scaled->Bpp;
					NewX2 = (ILuint)(x / ScaleX) * image->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						SShortPtr[NewX1 + c] = ShortPtr[NewX2 + c];
					}
				}
				break;
			case 4:
				for (x = 0; x < Width; x++) {
					NewX1 = x * Scaled->Bpp;
					NewX2 = (ILuint)(x / ScaleX) * image->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						SIntPtr[NewX1 + c] = IntPtr[NewX2 + c];
					}
				}
				break;
		}
	}
	else {  // IL_LINEAR or IL_BILINEAR
		switch (image->Bpc)
		{
			case 1:
				NewX3 = 0;
				for (x = 0; x < Width; x++) {
					t1 = x / (ILdouble)Width;
					t2 = t1 * Width - (ILuint)(t1 * Width);
					f = (1.0 - cos(t2 * IL_PI)) * .5;
					NewX1 = ((ILuint)(t1 * Width / ScaleX)) * image->Bpp;
					NewX2 = ((ILuint)(t1 * Width / ScaleX) + 1) * image->Bpp;

					for (c = 0; c < Scaled->Bpp; c++) {
						x1 = image->Data[NewX1 + c];
						x2 = image->Data[NewX2 + c];

						Scaled->Data[NewX3 + c] = (ILubyte)(x1 * (1.0 - f) + x2 * f);
					}

					NewX3 += Scaled->Bpp;
				}
				break;
			case 2:
				NewX3 = 0;
				for (x = 0; x < Width; x++) {
					t1 = x / (ILdouble)Width;
					t2 = t1 * Width - (ILuint)(t1 * Width);
					f = (1.0 - cos(t2 * IL_PI)) * .5;
					NewX1 = ((ILuint)(t1 * Width / ScaleX)) * image->Bpp;
					NewX2 = ((ILuint)(t1 * Width / ScaleX) + 1) * image->Bpp;

					for (c = 0; c < Scaled->Bpp; c++) {
						x1 = ShortPtr[NewX1 + c];
						x2 = ShortPtr[NewX2 + c];

						SShortPtr[NewX3 + c] = (ILushort)(x1 * (1.0 - f) + x2 * f);
					}

					NewX3 += Scaled->Bpp;
				}
				break;
			case 4:
				NewX3 = 0;
				for (x = 0; x < Width; x++) {
					t1 = x / (ILdouble)Width;
					t2 = t1 * Width - (ILuint)(t1 * Width);
					f = (1.0 - cos(t2 * IL_PI)) * .5;
					NewX1 = ((ILuint)(t1 * Width / ScaleX)) * image->Bpp;
					NewX2 = ((ILuint)(t1 * Width / ScaleX) + 1) * image->Bpp;

					for (c = 0; c < Scaled->Bpp; c++) {
						x1 = IntPtr[NewX1 + c];
						x2 = IntPtr[NewX2 + c];

						SIntPtr[NewX3 + c] = (ILuint)(x1 * (1.0 - f) + x2 * f);
					}

					NewX3 += Scaled->Bpp;
				}
				break;
		}
	}

	return Scaled;
}
