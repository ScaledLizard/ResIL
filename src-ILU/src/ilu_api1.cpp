//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Created in 2014 by Björn Ganster
//
// Filename: src-ILU/src/ilu_error.c
//
// Description: simple wrappers that provide API 1.x compatibility
//
//-----------------------------------------------------------------------------

//#include "IL/ilu.h"

#include "IL/il.h"
#include "ilu_internal.h"
#include "IL/ilu2.h"

ILconst_string ILAPIENTRY iluErrorString(ILenum Error)
{
	return ilu2ErrorString(Error);
}

ILboolean ILAPIENTRY iluSetLanguage(ILenum Language)
{
	return ilu2SetLanguage(Language);
}

ILboolean ILAPIENTRY iluPixelize(ILuint PixSize)
{
	ILimage* image = ilGetCurImage();
	return ilu2Pixelize(image, PixSize);
}

ILboolean ILAPIENTRY iluEdgeDetectP()
{
	ILimage* image = ilGetCurImage();
	return ilu2EdgeDetectP(image);
}

ILboolean ILAPIENTRY iluEdgeDetectS()
{
	ILimage* image = ilGetCurImage();
	return ilu2EdgeDetectS(image);
}

ILboolean ILAPIENTRY iluBlurAvg(ILuint Iter)
{
	ILimage* image = ilGetCurImage();
	return ilu2BlurAvg(image, Iter);
}

ILboolean ILAPIENTRY iluBlurGaussian(ILuint Iter)
{
	ILimage* image = ilGetCurImage();
	return ilu2BlurGaussian(image, Iter);
}

ILboolean ILAPIENTRY iluEmboss()
{
	ILimage* image = ilGetCurImage();
	return ilu2Emboss(image);
}

ILboolean ILAPIENTRY iluEdgeDetectE()
{
	ILimage* image = ilGetCurImage();
	return ilu2EdgeDetectE(image);
}

ILboolean ILAPIENTRY iluScaleAlpha(ILfloat scale)
{
	ILimage* image = ilGetCurImage();
	return ilu2ScaleAlpha(image, scale);
}

ILboolean ILAPIENTRY iluScaleColours(ILfloat r, ILfloat g, ILfloat b) 
{
	ILimage* image = ilGetCurImage();
	return ilu2ScaleColours(image, r, g, b);
}

ILboolean ILAPIENTRY iluGammaCorrect(ILfloat Gamma)
{
	ILimage* image = ilGetCurImage();
	return ilu2GammaCorrect(image, Gamma);
}

ILboolean ILAPIENTRY iluSaturate1f(ILfloat Saturation)
{
	ILimage* image = ilGetCurImage();
	return ilu2Saturate4f(image, 0.3086f, 0.6094f, 0.0820f, Saturation);
}

ILboolean ILAPIENTRY iluSaturate4f(ILfloat r, ILfloat g, ILfloat b, ILfloat Saturation)
{
	ILimage* image = ilGetCurImage();
	return ilu2Saturate4f(image, r, g, b, Saturation);
}

ILboolean ILAPIENTRY iluAlienify(void)
{
	ILimage* image = ilGetCurImage();
	return ilu2Alienify(image);
}

ILboolean ILAPIENTRY iluContrast(ILfloat Contrast)
{
	ILimage* image = ilGetCurImage();
	return ilu2Contrast(image, Contrast);
}

ILboolean ILAPIENTRY iluSharpen(ILfloat Factor, ILuint Iter)
{
	ILimage* image = ilGetCurImage();
	return ilu2Sharpen(image, Factor, Iter);
}

ILAPI ILboolean ILAPIENTRY iluConvolution(ILint *matrix, ILint scale, ILint bias) 
{
	ILimage* image = ilGetCurImage();
	return ilu2Convolution(image, matrix, scale, bias);
}

ILboolean ILAPIENTRY iluEnlargeImage(ILfloat XDim, ILfloat YDim, ILfloat ZDim)
{
	ILimage* image = ilGetCurImage();
	return ilu2EnlargeImage(image, XDim, YDim, ZDim);
}

ILboolean ILAPIENTRY iluScale(ILuint Width, ILuint Height, ILuint Depth)
{
	ILimage* image = ilGetCurImage();
	return ilu2Scale(image, Width, Height, Depth);
}

ILboolean ILAPIENTRY iluCompareImage(ILuint Comp)
{
	ILuint OrigName = ilGetCurName();
	ILimage* imageA = ilGetCurImage();
	ilBindImage(Comp);
	ILimage* imageB = ilGetCurImage();
	ILboolean result = ilu2CompareImage(imageA, imageB);
	ilBindImage(OrigName);
	return result;
}

ILuint ILAPIENTRY iluLoadImage(ILconst_string FileName)
{
	ILuint Id;
	ilGenImages(1, &Id);
	if (Id == 0)
		return 0;
	if (!ilLoadImage(FileName)) {
		ilDeleteImages(1, &Id);
		return 0;
	}
	return Id;
}

ILAPI ILboolean ILAPIENTRY iluCrop(ILuint XOff, ILuint YOff, ILuint ZOff, ILuint Width, ILuint Height, ILuint Depth)
//ILboolean iluCrop(ILuint XOff, ILuint YOff, ILuint ZOff, ILuint Width, ILuint Height, ILuint Depth) 
{
	ILimage* image = ilGetCurImage();
	return ilu2Crop(image, XOff, YOff, ZOff, Width, Height, Depth);
}

ILboolean ILAPIENTRY iluEnlargeCanvas(ILuint Width, ILuint Height, ILuint Depth)
{
	ILimage* image = ilGetCurImage();
	return ilu2EnlargeCanvas(image, Width, Height, Depth);
}

ILboolean ILAPIENTRY iluFlipImage() 
{
	ILimage* image = ilGetCurImage();
	return ilu2FlipImage(image);
}

ILboolean ILAPIENTRY iluMirror() 
{
	ILimage* image = ilGetCurImage();
	return ilu2Mirror(image);
}

ILboolean ILAPIENTRY iluInvertAlpha() 
{
	ILimage* image = ilGetCurImage();
	return ilu2InvertAlpha(image);
}

ILboolean ILAPIENTRY iluNegative()
{
	ILimage* image = ilGetCurImage();
	return ilu2Negative(image);
}

ILboolean ILAPIENTRY iluWave(ILfloat Angle)
{
	ILimage* image = ilGetCurImage();
	return ilu2Wave(image, Angle);
}

ILboolean ILAPIENTRY iluSwapColours() 
{
	ILimage* image = ilGetCurImage();
	return ilu2SwapColours(image);
}

ILuint ILAPIENTRY iluColoursUsed()
{
	ILimage* image = ilGetCurImage();
	return ilu2ColoursUsed(image);
}

ILboolean ILAPIENTRY iluReplaceColour(ILubyte Red, ILubyte Green, ILubyte Blue, ILfloat Tolerance)
{
	ILimage* image = ilGetCurImage();
	return ilu2ReplaceColour(image, Red, Green, Blue, Tolerance);
}

ILboolean ILAPIENTRY iluEqualize() 
{
	ILimage* image = ilGetCurImage();
	return ilu2Equalize(image);
}

ILboolean ILAPIENTRY iluBuildMipmaps()
{
	ILimage* image = ilGetCurImage();
	return ilu2BuildMipmaps(image);
}

ILboolean ILAPIENTRY iluNoisify(ILclampf Tolerance)
{
	ILimage* image = ilGetCurImage();
	return ilu2Noisify(image, Tolerance);
}

void ILAPIENTRY iluGetIntegerv(ILenum Mode, ILint *Param)
{
	return ilu2GetIntegerv(Mode, Param);
}

ILstring ILAPIENTRY iluGetString(ILenum StringName)
{
	return ilu2GetString(StringName);
}

ILint ILAPIENTRY iluGetInteger(ILenum Mode)
{
	return ilu2GetInteger(Mode);
}

void ILAPIENTRY iluImageParameter(ILenum PName, ILenum Param)
{
	ilu2ImageParameter(PName, Param);
}

void ILAPIENTRY iluInit()
{
	ilu2Init();
}

ILboolean ILAPIENTRY iluRotate3D(ILfloat x, ILfloat y, ILfloat z, ILfloat Angle)
{
	ILimage* image = ilGetCurImage();
	return ilu2Rotate3D(image, x, y, z, Angle);
}

ILboolean ILAPIENTRY iluRotate(ILfloat Angle)
{
	ILimage* image = ilGetCurImage();
	return ilu2Rotate(image, Angle);
}