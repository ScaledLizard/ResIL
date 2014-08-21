//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: IL/ilu.h
//
// Description: The main include file for ILU
//
//-----------------------------------------------------------------------------

// Doxygen comment
/*! \file ilu.h
    The main include file for ILU
*/

#ifndef __ILU2_H__

#define __ILU2_H__

#include "il2.h"


#ifdef _WIN32
	#if (defined(IL_USE_PRAGMA_LIBS)) && (!defined(_IL_BUILD_LIBRARY))
		#if defined(_MSC_VER) || defined(__BORLANDC__)
			#pragma comment(lib, "ILU.lib")
		#endif
	#endif
#endif


#define ILU_VERSION_1_9_0 1
#define ILU_VERSION       190


#define ILU_FILTER         0x2600
#define ILU_NEAREST        0x2601
#define ILU_LINEAR         0x2602
#define ILU_BILINEAR       0x2603
#define ILU_SCALE_BOX      0x2604
#define ILU_SCALE_TRIANGLE 0x2605
#define ILU_SCALE_BELL     0x2606
#define ILU_SCALE_BSPLINE  0x2607
#define ILU_SCALE_LANCZOS3 0x2608
#define ILU_SCALE_MITCHELL 0x2609


// Error types
#define ILU_INVALID_ENUM      0x0501
#define ILU_OUT_OF_MEMORY     0x0502
#define ILU_INTERNAL_ERROR    0x0504
#define ILU_INVALID_VALUE     0x0505
#define ILU_ILLEGAL_OPERATION 0x0506
#define ILU_INVALID_PARAM     0x0509


// Values
#define ILU_PLACEMENT          0x0700
#define ILU_LOWER_LEFT         0x0701
#define ILU_LOWER_RIGHT        0x0702
#define ILU_UPPER_LEFT         0x0703
#define ILU_UPPER_RIGHT        0x0704
#define ILU_CENTER             0x0705
#define ILU_CONVOLUTION_MATRIX 0x0710
  
#define ILU_VERSION_NUM IL_VERSION_NUM
#define ILU_VENDOR      IL_VENDOR


// Languages
#define ILU_ENGLISH            0x0800
#define ILU_ARABIC             0x0801
#define ILU_DUTCH              0x0802
#define ILU_JAPANESE           0x0803
#define ILU_SPANISH            0x0804
#define ILU_GERMAN             0x0805
#define ILU_FRENCH             0x0806

typedef struct ILpointf {
	ILfloat x;
	ILfloat y;
} ILpointf;

typedef struct ILpointi {
	ILint x;
	ILint y;
} ILpointi;

ILAPI ILboolean      ILAPIENTRY ilu2Alienify(ILimage* image);
ILAPI ILboolean      ILAPIENTRY ilu2BlurAvg(ILimage* image, ILuint Iter);
ILAPI ILboolean      ILAPIENTRY ilu2BlurGaussian(ILimage* image, ILuint Iter);
ILAPI ILboolean      ILAPIENTRY ilu2BuildMipmaps(ILimage* image);
ILAPI ILuint         ILAPIENTRY ilu2ColoursUsed(ILimage* image);
ILAPI ILboolean      ILAPIENTRY ilu2CompareImage(ILimage* imageA, ILimage* imageB);
ILAPI ILboolean      ILAPIENTRY ilu2Contrast(ILimage* image, ILfloat Contrast);
ILAPI ILboolean ILAPIENTRY ilu2Convolution(ILimage* image, ILint *matrix, ILint scale, ILint bias);
ILAPI ILboolean      ILAPIENTRY ilu2Crop(ILimage* image, ILuint XOff, ILuint YOff, ILuint ZOff, ILuint Width, ILuint Height, ILuint Depth);
ILAPI ILboolean      ILAPIENTRY ilu2EdgeDetectE(ILimage* image);
ILAPI ILboolean      ILAPIENTRY ilu2EdgeDetectP(ILimage* image);
ILAPI ILboolean      ILAPIENTRY ilu2EdgeDetectS(ILimage* image);
ILAPI ILboolean      ILAPIENTRY ilu2Emboss(ILimage* image);
ILAPI ILboolean      ILAPIENTRY ilu2EnlargeCanvas(ILimage* image, ILuint Width, ILuint Height, ILuint Depth);
ILAPI ILboolean      ILAPIENTRY ilu2EnlargeImage(ILimage* image, ILfloat XDim, ILfloat YDim, ILfloat ZDim);
ILAPI ILboolean      ILAPIENTRY ilu2Equalize(ILimage* image);
ILAPI ILconst_string 		 ILAPIENTRY ilu2ErrorString(ILenum Error);
ILAPI ILboolean      ILAPIENTRY ilu2Convolution(ILimage* image, ILint *matrix, ILint scale, ILint bias);
ILAPI ILboolean      ILAPIENTRY ilu2FlipImage(ILimage* image);
ILAPI ILboolean      ILAPIENTRY ilu2GammaCorrect(ILimage* image, ILfloat Gamma);
ILAPI ILint          ILAPIENTRY ilu2GetInteger(ILenum Mode);
ILAPI void           ILAPIENTRY ilu2GetIntegerv(ILenum Mode, ILint *Param);
ILAPI ILstring 		 ILAPIENTRY ilu2GetString(ILenum StringName);
ILAPI void           ILAPIENTRY ilu2ImageParameter(ILenum PName, ILenum Param);
ILAPI void           ILAPIENTRY ilu2Init();
ILAPI ILboolean      ILAPIENTRY ilu2InvertAlpha(ILimage* image);
ILAPI ILimage* ILAPIENTRY ilu2LoadImage(ILconst_string FileName);
ILAPI ILboolean      ILAPIENTRY ilu2Mirror(ILimage* image);
ILAPI ILboolean      ILAPIENTRY ilu2Negative(ILimage* image);
ILAPI ILboolean      ILAPIENTRY ilu2Noisify(ILimage* image, ILclampf Tolerance);
ILAPI ILboolean      ILAPIENTRY ilu2Pixelize(ILimage* image, ILuint PixSize);
ILAPI ILboolean      ILAPIENTRY ilu2ReplaceColour(ILimage* image, ILubyte Red, ILubyte Green, ILubyte Blue, ILfloat Tolerance);
ILAPI ILboolean      ILAPIENTRY ilu2Rotate(ILimage* image, ILfloat Angle);
ILAPI ILboolean      ILAPIENTRY ilu2Rotate3D(ILimage* image, ILfloat x, ILfloat y, ILfloat z, ILfloat Angle);
ILAPI ILboolean      ILAPIENTRY ilu2Saturate1f(ILimage* image, ILfloat Saturation);
ILAPI ILboolean      ILAPIENTRY ilu2Saturate4f(ILimage* image, ILfloat r, ILfloat g, ILfloat b, ILfloat Saturation);
ILAPI ILboolean      ILAPIENTRY ilu2Scale(ILimage* image, ILuint Width, ILuint Height, ILuint Depth);
ILAPI ILboolean      ILAPIENTRY ilu2ScaleAlpha(ILimage* image, ILfloat scale);
ILAPI ILboolean      ILAPIENTRY ilu2ScaleColours(ILimage* image, ILfloat r, ILfloat g, ILfloat b);
ILAPI ILboolean      ILAPIENTRY ilu2SetLanguage(ILenum Language);
ILAPI ILboolean      ILAPIENTRY ilu2Sharpen(ILimage* image, ILfloat Factor, ILuint Iter);
ILAPI ILboolean      ILAPIENTRY ilu2SwapColours(ILimage* image);
ILAPI ILboolean      ILAPIENTRY ilu2Wave(ILimage* image, ILfloat Angle);

#define ilu2ColorsUsed   ilu2ColoursUsed
#define ilu2SwapColors   ilu2SwapColours
#define ilu2ReplaceColor ilu2ReplaceColour
#define ilu2ScaleColor   ilu2ScaleColour

#endif // __ILU2_H__
