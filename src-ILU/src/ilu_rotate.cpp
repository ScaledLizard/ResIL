///////////////////////////////////////////////////////////////////////////////
//
// ImageLib Utility Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2002 <--Y2K Compliant! =]
//
// Filename: src-ILU/src/ilu_rotate.c
//
// Description: Rotates an image.
//
///////////////////////////////////////////////////////////////////////////////


#include "ilu_internal.h"
#include "ilu_states.h"
#include "IL/il2.h"


ILboolean ILAPIENTRY ilu2Rotate3D(ILimage* image, ILfloat x, ILfloat y, ILfloat z, ILfloat Angle)
{
	ILimage *Temp = iluRotate3D_(image, x, y, z, Angle);
	if (Temp != NULL) {
		il2TexImage(image, Temp->Width, Temp->Height, Temp->Depth, Temp->Bpp, Temp->Format, Temp->Type, Temp->Data);
		image->Origin = Temp->Origin;
		il2SetPal(image, &Temp->Pal);
		ilCloseImage(Temp);
		return IL_TRUE;
	}
	return IL_FALSE;
}


//! Rotates a bitmap any angle.
//  Code help comes from http://www.leunen.com/cbuilder/rotbmp.html.
ILAPI ILimage* ILAPIENTRY ilu2Rotate_(ILimage *image, ILfloat Angle)
{
	ILimage		*Rotated = NULL;
	ILint		x, y, c;
	ILdouble	Cos, Sin;
	ILuint		RotOffset, ImgOffset;
	ILint		MinX, MinY, MaxX, MaxY;
	ILushort	*ShortPtr;
	ILuint		*IntPtr;
	ILdouble	*DblPtr;
	ILdouble	Point1x, Point1y, Point2x, Point2y, Point3x, Point3y;
	ILint		SrcX, SrcY;

	// Multiples of 90 are special.
	Angle = (ILfloat)fmod((ILdouble)Angle, 360.0);
	if (Angle < 0)
		Angle = 360.0f + Angle;

	Cos = (ILdouble)cos((IL_PI * Angle) / 180.0);
	Sin = (ILdouble)sin((IL_PI * Angle) / 180.0);

	Point1x = (-(ILint)image->Height * Sin);
	Point1y = (image->Height * Cos);
	Point2x = (image->Width * Cos - image->Height * Sin);
	Point2y = (image->Height * Cos + image->Width * Sin);
	Point3x = (image->Width * Cos);
	Point3y = (image->Width * Sin);

	MinX = (ILint)IL_MIN(0, IL_MIN(Point1x, IL_MIN(Point2x, Point3x)));
	MinY = (ILint)IL_MIN(0, IL_MIN(Point1y, IL_MIN(Point2y, Point3y)));
	MaxX = (ILint)IL_MAX(Point1x, IL_MAX(Point2x, Point3x));
	MaxY = (ILint)IL_MAX(Point1y, IL_MAX(Point2y, Point3y));

	Rotated = (ILimage*)icalloc(1, sizeof(ILimage));
	if (Rotated == NULL)
		return NULL;
	if (il2CopyImageAttr(Rotated, image) == IL_FALSE) {
		ilCloseImage(Rotated);
		return NULL;
	}

	if (il2ResizeImage(Rotated, abs(MaxX) - MinX, abs(MaxY) - MinY, 1, image->Bpp, image->Bpc) == IL_FALSE) {
		ilCloseImage(Rotated);
		return IL_FALSE;
	}

	il2ClearImage(Rotated);

	ShortPtr = (ILushort*)image->Data;
	IntPtr = (ILuint*)image->Data;
	DblPtr = (ILdouble*)image->Data;

	//if (iluFilter == ILU_NEAREST) {
	switch (image->Bpc)
	{
		case 1:  // Byte-based (most images)
			if (Angle == 90.0) {
				for (x = 0; x < (ILint)image->Width; x++) {
					for (y = 0; y < (ILint)image->Height; y++) {
						RotOffset = x * Rotated->Bps + (image->Width - 1 - y) * Rotated->Bpp;
						ImgOffset = y * image->Bps + x * image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							Rotated->Data[RotOffset + c] = image->Data[ImgOffset + c];
						}
					} 
				} 
			}
			else if (Angle == 180.0) {
				for (x = 0; x < (ILint)image->Width; x++) {
					for (y = 0; y < (ILint)image->Height; y++) {
						RotOffset = (image->Height - 1 - y) * Rotated->Bps + x * Rotated->Bpp;
						ImgOffset = y * image->Bps + x * image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							Rotated->Data[RotOffset + c] = image->Data[ImgOffset + c];
						}
					} 
				} 
			}
			else if (Angle == 270.0) {
				for (x = 0; x < (ILint)image->Width; x++) {
					for (y = 0; y < (ILint)image->Height; y++) {
						RotOffset = (image->Height - 1 - x) * Rotated->Bps + y * Rotated->Bpp;
						ImgOffset = y * image->Bps + x * image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							Rotated->Data[RotOffset + c] = image->Data[ImgOffset + c];
						}
					} 
				} 
			}
			else {
				for (x = 0; x < (ILint)Rotated->Width; x++) {
					for (y = 0; y < (ILint)Rotated->Height; y++) {
						SrcX = (ILint)((x + MinX) * Cos + (y + MinY) * Sin);
						SrcY = (ILint)((y + MinY) * Cos - (x + MinX) * Sin);
						if (SrcX >= 0 && SrcX < (ILint)image->Width && SrcY >= 0 && SrcY < (ILint)image->Height) {
							RotOffset = y * Rotated->Bps + x * Rotated->Bpp;
							ImgOffset = (ILuint)SrcY * image->Bps + (ILuint)SrcX * image->Bpp;
							for (c = 0; c < Rotated->Bpp; c++) {
								Rotated->Data[RotOffset + c] = image->Data[ImgOffset + c];
							}
						}
					}
				}
			}
			break;

		case 2:  // Short-based
			image->Bps /= 2;   // Makes it easier to just
			Rotated->Bps /= 2; //   cast to short.

			if (Angle == 90.0) {
				for (x = 0; x < (ILint)image->Width; x++) {
					for (y = 0; y < (ILint)image->Height; y++) {
						RotOffset = x * Rotated->Bps + (image->Width - 1 - y) * Rotated->Bpp;
						ImgOffset = y * image->Bps + x * image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							((ILushort*)(Rotated->Data))[RotOffset + c] = ShortPtr[ImgOffset + c];
						}
					} 
				} 
			}
			else if (Angle == 180.0) {
				for (x = 0; x < (ILint)image->Width; x++) {
					for (y = 0; y < (ILint)image->Height; y++) {
						RotOffset = (image->Height - 1 - y) * Rotated->Bps + x * Rotated->Bpp;
						ImgOffset = y * image->Bps + x * image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							((ILushort*)(Rotated->Data))[RotOffset + c] = ShortPtr[ImgOffset + c];
						}
					} 
				} 
			}
			else if (Angle == 270.0) {
				for (x = 0; x < (ILint)image->Width; x++) {
					for (y = 0; y < (ILint)image->Height; y++) {
						RotOffset = (image->Height - 1 - x) * Rotated->Bps + y * Rotated->Bpp;
						ImgOffset = y * image->Bps + x * image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							((ILushort*)(Rotated->Data))[RotOffset + c] = ShortPtr[ImgOffset + c];
						}
					} 
				} 
			}
			else {
				for (x = 0; x < (ILint)Rotated->Width; x++) {
					for (y = 0; y < (ILint)Rotated->Height; y++) {
						SrcX = (ILint)((x + MinX) * Cos + (y + MinY) * Sin);
						SrcY = (ILint)((y + MinY) * Cos - (x + MinX) * Sin);
						if (SrcX >= 0 && SrcX < (ILint)image->Width && SrcY >= 0 && SrcY < (ILint)image->Height) {
							RotOffset = y * Rotated->Bps + x * Rotated->Bpp;
							ImgOffset = (ILuint)SrcY * image->Bps + (ILuint)SrcX * image->Bpp;
							for (c = 0; c < Rotated->Bpp; c++) {
								((ILushort*)(Rotated->Data))[RotOffset + c] = ShortPtr[ImgOffset + c];
							}
						}
					}
				}
			}
			image->Bps *= 2;
			Rotated->Bps *= 2;
			break;

		case 4:  // Floats or 32-bit integers
			image->Bps /= 4;
			Rotated->Bps /= 4;

			if (Angle == 90.0) {
				for (x = 0; x < (ILint)image->Width; x++) {
					for (y = 0; y < (ILint)image->Height; y++) {
						RotOffset = x * Rotated->Bps + (image->Width - 1 - y) * Rotated->Bpp;
						ImgOffset = y * image->Bps + x * image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							((ILuint*)(Rotated->Data))[RotOffset + c] = IntPtr[ImgOffset + c];
						}
					} 
				} 
			}
			else if (Angle == 180.0) {
				for (x = 0; x < (ILint)image->Width; x++) {
					for (y = 0; y < (ILint)image->Height; y++) {
						RotOffset = (image->Height - 1 - y) * Rotated->Bps + x * Rotated->Bpp;
						ImgOffset = y * image->Bps + x * image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							((ILuint*)(Rotated->Data))[RotOffset + c] = IntPtr[ImgOffset + c];
						}
					} 
				} 
			}
			else if (Angle == 270.0) {
				for (x = 0; x < (ILint)image->Width; x++) {
					for (y = 0; y < (ILint)image->Height; y++) {
						RotOffset = (image->Height - 1 - x) * Rotated->Bps + y * Rotated->Bpp;
						ImgOffset = y * image->Bps + x * image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							((ILuint*)(Rotated->Data))[RotOffset + c] = IntPtr[ImgOffset + c];
						}
					} 
				} 
			}
			else {
				for (x = 0; x < (ILint)Rotated->Width; x++) {
					for (y = 0; y < (ILint)Rotated->Height; y++) {
						SrcX = (ILint)((x + MinX) * Cos + (y + MinY) * Sin);
						SrcY = (ILint)((y + MinY) * Cos - (x + MinX) * Sin);
						if (SrcX >= 0 && SrcX < (ILint)image->Width && SrcY >= 0 && SrcY < (ILint)image->Height) {
							RotOffset = y * Rotated->Bps + x * Rotated->Bpp;
							ImgOffset = (ILuint)SrcY * image->Bps + (ILuint)SrcX * image->Bpp;
							for (c = 0; c < Rotated->Bpp; c++) {
								((ILuint*)(Rotated->Data))[RotOffset + c] = IntPtr[ImgOffset + c];
							}
						}
					}
				}
			}
			image->Bps *= 4;
			Rotated->Bps *= 4;
			break;

		case 8:  // Double or 64-bit integers
			image->Bps /= 8;
			Rotated->Bps /= 8;

			if (Angle == 90.0) {
				for (x = 0; x < (ILint)image->Width; x++) {
					for (y = 0; y < (ILint)image->Height; y++) {
						RotOffset = x * Rotated->Bps + (image->Width - 1 - y) * Rotated->Bpp;
						ImgOffset = y * image->Bps + x * image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							((ILdouble*)(Rotated->Data))[RotOffset + c] = DblPtr[ImgOffset + c];
						}
					} 
				} 
			}
			else if (Angle == 180.0) {
				for (x = 0; x < (ILint)image->Width; x++) { 
					for (y = 0; y < (ILint)image->Height; y++) { 
						RotOffset = (image->Height - 1 - y) * Rotated->Bps + x * Rotated->Bpp;
						ImgOffset = y * image->Bps + x * image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							((ILdouble*)(Rotated->Data))[RotOffset + c] = DblPtr[ImgOffset + c];
						}
					} 
				} 
			}
			else if (Angle == 270.0) {
				for (x = 0; x < (ILint)image->Width; x++) {
					for (y = 0; y < (ILint)image->Height; y++) {
						RotOffset = (image->Height - 1 - x) * Rotated->Bps + y * Rotated->Bpp;
						ImgOffset = y * image->Bps + x * image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							((ILdouble*)(Rotated->Data))[RotOffset + c] = DblPtr[ImgOffset + c];
						}
					} 
				} 
			}
			else {
				for (x = 0; x < (ILint)Rotated->Width; x++) {
					for (y = 0; y < (ILint)Rotated->Height; y++) {
						SrcX = (ILint)((x + MinX) * Cos + (y + MinY) * Sin);
						SrcY = (ILint)((y + MinY) * Cos - (x + MinX) * Sin);
						if (SrcX >= 0 && SrcX < (ILint)image->Width && SrcY >= 0 && SrcY < (ILint)image->Height) {
							RotOffset = y * Rotated->Bps + x * Rotated->Bpp;
							ImgOffset = (ILuint)SrcY * image->Bps + (ILuint)SrcX * image->Bpp;
							for (c = 0; c < Rotated->Bpp; c++) {
								((ILdouble*)(Rotated->Data))[RotOffset + c] = DblPtr[ImgOffset + c];
							}
						}
					}
				}
			}
			image->Bps *= 8;
			Rotated->Bps *= 8;
			break;
	}

	return Rotated;
}


ILAPI ILimage* ILAPIENTRY iluRotate3D_(ILimage *Image, ILfloat x, ILfloat y, ILfloat z, ILfloat Angle)
{
	Image; x; y; z; Angle;
	return NULL;
}

ILboolean ILAPIENTRY ilu2Rotate(ILimage* image, ILfloat Angle)
{
	ILimage	*Temp, *Temp1;
	ILenum	PalType = 0;

	if (image == NULL) {
		il2SetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (image->Format == IL_COLOUR_INDEX) {
		PalType = image->Pal.getPalType();
		image = iConvertImage(image, ilGetPalBaseType(PalType), IL_UNSIGNED_BYTE);
	}

	Temp = ilu2Rotate_(image, Angle);
	if (Temp != NULL) {
		if (PalType != 0) {
			ilCloseImage(image);
			Temp1 = iConvertImage(Temp, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE);
			ilCloseImage(Temp);
			Temp = Temp1;
		}
		il2TexImage(image, Temp->Width, Temp->Height, Temp->Depth, Temp->Bpp, Temp->Format, Temp->Type, Temp->Data);
		if (PalType != 0) {
			image->Pal = Temp->Pal;
		}

		image->Origin = Temp->Origin;
		ilCloseImage(Temp);
		return IL_TRUE;
	}
	return IL_FALSE;
}


