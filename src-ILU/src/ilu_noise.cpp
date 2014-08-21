///////////////////////////////////////////////////////////////////////////////
//
// ImageLib Utility Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-ILU/src/ilu_noise.c
//
// Description: Noise generation functions
//
///////////////////////////////////////////////////////////////////////////////


#include "ilu_internal.h"
#include <math.h>
#include <limits.h>


// Very simple right now.
//	This will probably use Perlin noise and parameters in the future.
ILboolean ILAPIENTRY ilu2Noisify(ILimage* image, ILclampf Tolerance)
{
	ILuint		i, j, c, Factor, Factor2, NumPix;
	ILint		Val;
	ILushort	*ShortPtr;
	ILuint		*IntPtr;
	ILubyte		*RegionMask;

	if (image == NULL) {
		il2SetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	RegionMask = iScanFill(image);

	// @TODO:  Change this to work correctly without time()!
	//srand(time(NULL));
	NumPix = image->SizeOfData / image->Bpc;

	switch (image->Bpc)
	{
		case 1:
			Factor = (ILubyte)(Tolerance * (UCHAR_MAX / 2));
			if (Factor == 0)
				return IL_TRUE;
			Factor2 = Factor + Factor;
			for (i = 0, j = 0; i < NumPix; i += image->Bpp, j++) {
				if (RegionMask) {
					if (!RegionMask[j])
						continue;
				}
				Val = (ILint)((ILint)(rand() % Factor2) - Factor);
				for (c = 0; c < image->Bpp; c++) {
					if ((ILint)image->Data[i + c] + Val > UCHAR_MAX)
						image->Data[i + c] = UCHAR_MAX;
					else if ((ILint)image->Data[i + c] + Val < 0)
						image->Data[i + c] = 0;
					else
						image->Data[i + c] += Val;
				}
			}
			break;
		case 2:
			Factor = (ILushort)(Tolerance * (USHRT_MAX / 2));
			if (Factor == 0)
				return IL_TRUE;
			Factor2 = Factor + Factor;
			ShortPtr = (ILushort*)image->Data;
			for (i = 0, j = 0; i < NumPix; i += image->Bpp, j++) {
				if (RegionMask) {
					if (!RegionMask[j])
						continue;
				}
				Val = (ILint)((ILint)(rand() % Factor2) - Factor);
				for (c = 0; c < image->Bpp; c++) {
					if ((ILint)ShortPtr[i + c] + Val > USHRT_MAX)
						ShortPtr[i + c] = USHRT_MAX;
					else if ((ILint)ShortPtr[i + c] + Val < 0)
						ShortPtr[i + c] = 0;
					else
						ShortPtr[i + c] += Val;
				}
			}
			break;
		case 4:
			Factor = (ILuint)(Tolerance * (UINT_MAX / 2));
			if (Factor == 0)
				return IL_TRUE;
			Factor2 = Factor + Factor;
			IntPtr = (ILuint*)image->Data;
			for (i = 0, j = 0; i < NumPix; i += image->Bpp, j++) {
				if (RegionMask) {
					if (!RegionMask[j])
						continue;
				}
				Val = (ILint)((ILint)(rand() % Factor2) - Factor);
				for (c = 0; c < image->Bpp; c++) {
					if (IntPtr[i + c] + Val > UINT_MAX)
						IntPtr[i + c] = UINT_MAX;
					else if ((ILint)IntPtr[i + c] + Val < 0)
						IntPtr[i + c] = 0;
					else
						IntPtr[i + c] += Val;
				}
			}
			break;
	}

	ifree(RegionMask);

	return IL_TRUE;
}


// Information on Perlin Noise taken from
//	http://freespace.virgin.net/hugo.elias/models/m_perlin.htm

