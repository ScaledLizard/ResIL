
#include "ilu_internal.h"
#include "ilu_states.h"
#include <float.h>
#include <limits.h>
#include "IL/il2.h"

ILboolean ilu2Crop2D(ILimage* image, ILuint XOff, ILuint YOff, ILuint Width, ILuint Height) 
{
	ILuint	x, y, c, OldBps;
	ILubyte	*Data;
	ILenum	Origin;

	if (image == NULL) {
		il2SetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	// Uh-oh, what about 0 dimensions?!
	if (Width > image->Width || Height > image->Height) {
		il2SetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Data = (ILubyte*)ialloc(image->SizeOfData);
	if (Data == NULL) {
		return IL_FALSE;
	}

	OldBps = image->Bps;
	Origin = image->Origin;
	il2CopyPixels(image, 0, 0, 0, image->Width, image->Height, 1, image->Format, image->Type, Data);
	if (!il2TexImage(image, Width, Height, image->Depth, image->Bpp, image->Format, image->Type, NULL)) {
		free(Data);
		return IL_FALSE;
	}
	image->Origin = Origin;

	// @TODO:  Optimize!  (Especially XOff * image->Bpp...get rid of it!)
	for (y = 0; y < image->Height; y++) {
		for (x = 0; x < image->Bps; x += image->Bpp) {
			for (c = 0; c < image->Bpp; c++) {
				image->Data[y * image->Bps + x + c] = 
					Data[(y + YOff) * OldBps + x + XOff * image->Bpp + c];
			}
		}
	}

	ifree(Data);

	return IL_TRUE;
}


ILboolean ilu2Crop3D(ILimage* image, ILuint XOff, ILuint YOff, ILuint ZOff, ILuint Width, ILuint Height, ILuint Depth)
{
	ILuint	x, y, z, c, OldBps, OldPlane;
	ILubyte	*Data;
	ILenum	Origin;

	if (image == NULL) {
		il2SetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	// Uh-oh, what about 0 dimensions?!
	if (Width > image->Width || Height > image->Height || Depth > image->Depth) {
		il2SetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Data = (ILubyte*)ialloc(image->SizeOfData);
	if (Data == NULL) {
		return IL_FALSE;
	}

	OldBps = image->Bps;
	OldPlane = image->SizeOfPlane;
	Origin = image->Origin;
	il2CopyPixels(image, 0, 0, 0, image->Width, image->Height, image->Depth, image->Format, image->Type, Data);
	if (!il2TexImage(image, Width - XOff, Height - YOff, Depth - ZOff, image->Bpp, image->Format, image->Type, NULL)) {
		ifree(Data);
	}
	image->Origin = Origin;

	for (z = 0; z < image->Depth; z++) {
		for (y = 0; y < image->Height; y++) {
			for (x = 0; x < image->Bps; x += image->Bpp) {
				for (c = 0; c < image->Bpp; c++) {
					image->Data[z * image->SizeOfPlane + y * image->Bps + x + c] = 
						Data[(z + ZOff) * OldPlane + (y + YOff) * OldBps + (x + XOff) + c];
				}
			}
		}
	}

	ifree(Data);

	return IL_TRUE;
}


ILboolean ILAPIENTRY ilu2Crop(ILimage* image, ILuint XOff, ILuint YOff, ILuint ZOff, ILuint Width, ILuint Height, ILuint Depth)
{
	if (ZOff <= 1)
		return ilu2Crop2D(image, XOff, YOff, Width, Height);
	return ilu2Crop3D(image, XOff, YOff, ZOff, Width, Height, Depth);
}


//! Enlarges the canvas
ILboolean ILAPIENTRY ilu2EnlargeCanvas(ILimage* image, ILuint Width, ILuint Height, ILuint Depth)
{
	ILubyte	*Data/*, Clear[4]*/;
	ILuint	x, y, z, OldBps, OldH, OldD, OldPlane, AddX, AddY;
	ILenum	Origin;

	if (image == NULL) {
		il2SetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	// Uh-oh, what about 0 dimensions?!
	if (Width < image->Width || Height < image->Height || Depth < image->Depth) {
		il2SetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (image->Origin == IL_ORIGIN_LOWER_LEFT) {
		switch (iluPlacement)
		{
			case ILU_LOWER_LEFT:
				AddX = 0;
				AddY = 0;
				break;
			case ILU_LOWER_RIGHT:
				AddX = Width - image->Width;
				AddY = 0;
				break;
			case ILU_UPPER_LEFT:
				AddX = 0;
				AddY = Height - image->Height;
				break;
			case ILU_UPPER_RIGHT:
				AddX = Width - image->Width;
				AddY = Height - image->Height;
				break;
			case ILU_CENTER:
				AddX = (Width - image->Width) >> 1;
				AddY = (Height - image->Height) >> 1;
				break;
			default:
				il2SetError(ILU_INVALID_PARAM);
				return IL_FALSE;
		}
	}
	else {  // IL_ORIGIN_UPPER_LEFT
		switch (iluPlacement)
		{
			case ILU_LOWER_LEFT:
				AddX = 0;
				AddY = Height - image->Height;
				break;
			case ILU_LOWER_RIGHT:
				AddX = Width - image->Width;
				AddY = Height - image->Height;
				break;
			case ILU_UPPER_LEFT:
				AddX = 0;
				AddY = 0;
				break;
			case ILU_UPPER_RIGHT:
				AddX = Width - image->Width;
				AddY = 0;
				break;
			case ILU_CENTER:
				AddX = (Width - image->Width) >> 1;
				AddY = (Height - image->Height) >> 1;
				break;
			default:
				il2SetError(ILU_INVALID_PARAM);
				return IL_FALSE;
		}
	}

	AddX *= image->Bpp;

	Data = (ILubyte*)ialloc(image->SizeOfData);
	if (Data == NULL) {
		return IL_FALSE;
	}

	// Preserve old data.
	OldPlane = image->SizeOfPlane;
	OldBps   = image->Bps;
	OldH     = image->Height;
	OldD     = image->Depth;
	Origin   = image->Origin;
	il2CopyPixels(image, 0, 0, 0, image->Width, image->Height, OldD, image->Format, image->Type, Data);

	il2TexImage(image, Width, Height, Depth, image->Bpp, image->Format, image->Type, NULL);
	image->Origin = Origin;

	il2ClearImage(image);

	for (z = 0; z < OldD; z++) {
		for (y = 0; y < OldH; y++) {
			for (x = 0; x < OldBps; x++) {
				image->Data[z * image->SizeOfPlane + (y + AddY) * image->Bps + x + AddX] =
					Data[z * OldPlane + y * OldBps + x];
			}
		}
	}

	ifree(Data);

	return IL_TRUE;
}

//! Flips an image over its x axis
ILboolean ILAPIENTRY ilu2FlipImage(ILimage* image) 
{
	if( image == NULL ) {
		il2SetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	iFlipBuffer(image->Data,image->Depth,image->Bps,image->Height);
	return IL_TRUE;
}


//! Mirrors an image over its y axis
ILboolean ILAPIENTRY ilu2Mirror(ILimage* image) {
	return iMirror(image);
}


//! Inverts the alpha in the image
ILboolean ILAPIENTRY ilu2InvertAlpha(ILimage* image) 
{
	ILuint		i, *IntPtr, NumPix;
	ILubyte		*Data;
	ILushort	*ShortPtr;
	ILfloat		*FltPtr;
	ILdouble	*DblPtr;
	ILubyte		Bpp;

	if (image == NULL) {
		il2SetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (image->Format != IL_RGBA &&
		image->Format != IL_BGRA &&
		image->Format != IL_LUMINANCE_ALPHA) {
			il2SetError(ILU_ILLEGAL_OPERATION);
			return IL_FALSE;
	}

	Data = image->Data;
	Bpp = image->Bpp;
	NumPix = image->Width * image->Height * image->Depth;

	switch (image->Type)
	{
		case IL_BYTE:
		case IL_UNSIGNED_BYTE:
			Data += (Bpp - 1);
			for( i = Bpp - 1; i < NumPix; i++, Data += Bpp )
				*(Data) = ~*(Data);
			break;

		case IL_SHORT:
		case IL_UNSIGNED_SHORT:
			ShortPtr = ((ILushort*)Data) + Bpp-1;	
			for (i = Bpp - 1; i < NumPix; i++, ShortPtr += Bpp)
				*(ShortPtr) = ~*(ShortPtr);
			break;

		case IL_INT:
		case IL_UNSIGNED_INT:
			IntPtr = ((ILuint*)Data) + Bpp-1;
			for (i = Bpp - 1; i < NumPix; i++, IntPtr += Bpp)
				*(IntPtr) = ~*(IntPtr);
			break;

		case IL_FLOAT:
			FltPtr = ((ILfloat*)Data) + Bpp - 1;
			for (i = Bpp - 1; i < NumPix; i++, FltPtr += Bpp)
				*(FltPtr) = 1.0f - *(FltPtr);
			break;

		case IL_DOUBLE:
			DblPtr = ((ILdouble*)Data) + Bpp - 1;
			for (i = Bpp - 1; i < NumPix; i++, DblPtr += Bpp)
				*(DblPtr) = 1.0f - *(DblPtr);
			break;
	}

	return IL_TRUE;
}


//! Inverts the colours in the image
ILboolean ILAPIENTRY ilu2Negative(ILimage* image)
{
	ILuint		i, j, c, *IntPtr, NumPix, Bpp;
	ILubyte		*Data;
	ILushort	*ShortPtr;
	ILubyte		*RegionMask;

	if (image == NULL) {
		il2SetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (image->Format == IL_COLOUR_INDEX) {
		if (!image->Pal.hasPalette()) {
			il2SetError(ILU_ILLEGAL_OPERATION);
			return IL_FALSE;
		}
		Data = image->Pal.getPalette();
		i = image->Pal.getPalSize();
	}
	else {
		Data = image->Data;
		i = image->SizeOfData;
	}

	RegionMask = iScanFill(image);
	
	// @TODO:  Optimize this some.

	NumPix = i / image->Bpc;
	Bpp = image->Bpp;

	if (RegionMask) {
		switch (image->Bpc)
		{
			case 1:
				for (j = 0, i = 0; j < NumPix; j += Bpp, i++, Data += Bpp) {
					for (c = 0; c < Bpp; c++) {
						if (RegionMask[i])
							*(Data+c) = ~*(Data+c);
					}
				}
				break;

			case 2:
				ShortPtr = (ILushort*)Data;
				for (j = 0, i = 0; j < NumPix; j += Bpp, i++, ShortPtr += Bpp) {
					for (c = 0; c < Bpp; c++) {
						if (RegionMask[i])
							*(ShortPtr+c) = ~*(ShortPtr+c);
					}
				}
				break;

			case 4:
				IntPtr = (ILuint*)Data;
				for (j = 0, i = 0; j < NumPix; j += Bpp, i++, IntPtr += Bpp) {
					for (c = 0; c < Bpp; c++) {
						if (RegionMask[i])
							*(IntPtr+c) = ~*(IntPtr+c);
					}
				}
				break;
		}
	}
	else {
		switch (image->Bpc)
		{
			case 1:
				for (j = 0; j < NumPix; j++, Data++) {
					*(Data) = ~*(Data);
				}
				break;

			case 2:
				ShortPtr = (ILushort*)Data;
				for (j = 0; j < NumPix; j++, ShortPtr++) {
					*(ShortPtr) = ~*(ShortPtr);
				}
				break;

			case 4:
				IntPtr = (ILuint*)Data;
				for (j = 0; j < NumPix; j++, IntPtr++) {
					*(IntPtr) = ~*(IntPtr);
				}
				break;
		}
	}

	ifree(RegionMask);

	return IL_TRUE;
}


// Taken from
//	http://www-classic.be.com/aboutbe/benewsletter/volume_III/Issue2.html#Insight
//	Hope they don't mind too much. =]
ILboolean ILAPIENTRY ilu2Wave(ILimage* image, ILfloat Angle)
{
	ILint	Delta;
	ILuint	y;
	ILubyte	*DataPtr, *TempBuff;

	if (image == NULL) {
		il2SetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	TempBuff = (ILubyte*)ialloc(image->SizeOfData);
	if (TempBuff == NULL) {
		return IL_FALSE;
	}

	for (y = 0; y < image->Height; y++) {
		Delta = (ILint)
			(30 * sin((10 * Angle + y) * IL_DEGCONV) +
			 15 * sin(( 7 * Angle + 3 * y) * IL_DEGCONV));

		DataPtr = image->Data + y * image->Bps;

		if (Delta < 0) {
			Delta = -Delta;
			memcpy(TempBuff, DataPtr, image->Bpp * Delta);
			memcpy(DataPtr, DataPtr + image->Bpp * Delta, image->Bpp * (image->Width - Delta));
			memcpy(DataPtr + image->Bpp * (image->Width - Delta), TempBuff, image->Bpp * Delta);
		}
		else if (Delta > 0) {
			memcpy(TempBuff, DataPtr, image->Bpp * (image->Width - Delta));
			memcpy(DataPtr, DataPtr + image->Bpp * (image->Width - Delta), image->Bpp * Delta);
			memcpy(DataPtr + image->Bpp * Delta, TempBuff, image->Bpp * (image->Width - Delta));
		}
	}

	ifree(TempBuff);

	return IL_TRUE;
}


// Swaps the colour order of the current image (rgb(a)->bgr(a) or vice-versa).
//	Must be either an 8, 24 or 32-bit (coloured) image (or palette).
ILboolean ILAPIENTRY ilu2SwapColours(ILimage* image) 
{
	// Use ilConvert or other like that to convert the data?
	// and extend that function to work even on paletted data
	
	ILimage *img = ilGetCurImage();
	if( img == NULL ) {
		il2SetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (image->Bpp == 1) {
		if (ilGetBppPal(image->Pal.getPalType()) == 0 || image->Format != IL_COLOUR_INDEX) {
			il2SetError(ILU_ILLEGAL_OPERATION);  // Can be luminance.
			return IL_FALSE;
		}
		
		switch (img->Pal.getPalType()) {
			case IL_PAL_RGB24:
				return il2ConvertPal(image, IL_PAL_BGR24);
			case IL_PAL_RGB32:
				return il2ConvertPal(image, IL_PAL_BGR32);
			case IL_PAL_RGBA32:
				return il2ConvertPal(image, IL_PAL_BGRA32);
			case IL_PAL_BGR24:
				return il2ConvertPal(image, IL_PAL_RGB24);
			case IL_PAL_BGR32:
				return il2ConvertPal(image, IL_PAL_RGB32);
			case IL_PAL_BGRA32:
				return il2ConvertPal(image, IL_PAL_RGBA32);
			default:
				il2SetError(ILU_INTERNAL_ERROR);
				return IL_FALSE;
		}
	}

	switch( img->Format) {
		case IL_RGB:
			return il2ConvertImage(image, IL_BGR, img->Type);
		case IL_RGBA:
			return il2ConvertImage(image, IL_BGRA, img->Type);
		case IL_BGR:
			return il2ConvertImage(image, IL_RGB, img->Type);
		case IL_BGRA:
			return il2ConvertImage(image, IL_RGBA, img->Type);
	}

	il2SetError(ILU_INTERNAL_ERROR);
	return IL_FALSE;
}


typedef struct BUCKET { ILubyte Colours[4];  struct BUCKET *Next; } BUCKET;

ILuint ILAPIENTRY ilu2ColoursUsed(ILimage* image)
{
	ILuint i, c, Bpp, ColVal, SizeData, BucketPos = 0, NumCols = 0;
	BUCKET Buckets[8192], *Temp;
	ILubyte ColTemp[4];
	ILboolean Matched;
	BUCKET *Heap[9];
	ILuint HeapPos = 0, HeapPtr = 0, HeapSize;

	imemclear(Buckets, sizeof(BUCKET) * 8192);
	for (c = 0; c < 9; c++) {
		Heap[c] = 0;
	}

	if (image == NULL) {
		il2SetError(ILU_ILLEGAL_OPERATION);
		return 0;
	}

	Bpp = image->Bpp;
	SizeData = image->SizeOfData;

	// Create our miniature memory heap.
	// I have determined that the average number of colours versus
	//	the number of pixels is about a 1:8 ratio, so divide by 8.
	HeapSize = IL_MAX(1, image->SizeOfData / image->Bpp / 8);
	Heap[0] = (BUCKET*)ialloc(HeapSize * sizeof(BUCKET));
	if (Heap[0] == NULL)
		return IL_FALSE;

	for (i = 0; i < SizeData; i += Bpp) {
		*(ILuint*)ColTemp = 0;
		ColTemp[0] = image->Data[i];
		if (Bpp > 1) {
			ColTemp[1] = image->Data[i + 1];
			ColTemp[2] = image->Data[i + 2];
		}
		if (Bpp > 3)
			ColTemp[3] = image->Data[i + 3];

		BucketPos = *(ILuint*)ColTemp % 8192;

		// Add to hash table
		if (Buckets[BucketPos].Next == NULL) {
			NumCols++;
			//Buckets[BucketPos].Next = (BUCKET*)ialloc(sizeof(BUCKET));
			Buckets[BucketPos].Next = Heap[HeapPos] + HeapPtr++;
			if (HeapPtr >= HeapSize) {
				Heap[++HeapPos] = (BUCKET*)ialloc(HeapSize * sizeof(BUCKET));
				if (Heap[HeapPos] == NULL)
					goto alloc_error;
				HeapPtr = 0;
			}
			*(ILuint*)Buckets[BucketPos].Next->Colours = *(ILuint*)ColTemp;
			Buckets[BucketPos].Next->Next = NULL;
		}
		else {
			Matched = IL_FALSE;
			Temp = Buckets[BucketPos].Next;

			ColVal = *(ILuint*)ColTemp;
			while (Temp->Next != NULL) {
				if (ColVal == *(ILuint*)Temp->Colours) {
					Matched = IL_TRUE;
					break;
				}
				Temp = Temp->Next;
			}
			if (!Matched) {
				if (ColVal != *(ILuint*)Temp->Colours) {  // Check against last entry
					NumCols++;
					Temp = Buckets[BucketPos].Next;
					//Buckets[BucketPos].Next = (BUCKET*)ialloc(sizeof(BUCKET));
					Buckets[BucketPos].Next = Heap[HeapPos] + HeapPtr++;
					if (HeapPtr >= HeapSize) {
						Heap[++HeapPos] = (BUCKET*)ialloc(HeapSize * sizeof(BUCKET));
						if (Heap[HeapPos] == NULL)
							goto alloc_error;
						HeapPtr = 0;
					}
					Buckets[BucketPos].Next->Next = Temp;
					*(ILuint*)Buckets[BucketPos].Next->Colours = *(ILuint*)ColTemp;
				}
			}
		}
	}

	// Delete our mini heap.
	for (i = 0; i < 9; i++) {
		if (Heap[i] == NULL)
			break;
		ifree(Heap[i]);
	}

	return NumCols;

alloc_error:
	for (i = 0; i < 9; i++) {
		ifree(Heap[i]);
	}

	return 0;
}


ILboolean ILAPIENTRY ilu2CompareImage(ILimage* imageA, ILimage* imageB)
{
	ILboolean	Same = IL_TRUE;

	// Same image, so return true.
	if (imageA == imageB)
		return IL_TRUE;

	if (imageA == NULL || imageB == NULL) {
		il2SetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	// @TODO:  Should we check palettes, too?
	if (imageA->Bpp != imageB->Bpp       ||
		imageA->Depth != imageB->Depth   ||
		imageA->Format != imageB->Format ||
		imageA->Height != imageB->Height ||
		imageA->Origin != imageB->Origin ||
		imageA->Type != imageB->Type ||
		imageA->Width != imageB->Width) 
	{
			return IL_FALSE;
	}

	for (ILuint i = 0; i < imageB->SizeOfData; i++) {
		if (imageA->Data[i] != imageB->Data[i]) {
			Same = IL_FALSE;
			break;
		}
	}

	return Same;
}


// @TODO:  FIX ILGETCLEARCALL!
ILboolean ILAPIENTRY ilu2ReplaceColour(ILimage* image, ILubyte Red, ILubyte Green, ILubyte Blue, ILfloat Tolerance)
{
	ILubyte	ClearCol[4];
	ILint	TolVal, Distance, Dist1, Dist2, Dist3;
	ILuint	i, NumPix;

	if (image == NULL) {
		il2SetError(ILU_ILLEGAL_OPERATION);
		return 0;
	}

	il2GetClear(ClearCol, IL_RGBA, IL_UNSIGNED_BYTE);
	if (Tolerance > 1.0f || Tolerance < -1.0f)
		Tolerance = 1.0f;  // Clamp it.
	TolVal = (ILuint)(fabs(Tolerance) * UCHAR_MAX);  // To be changed.
	NumPix = image->Width * image->Height * image->Depth;

	if (Tolerance <= FLT_EPSILON && Tolerance >= 0) {
 			
            //@TODO what is this?
	}
	else {
		switch (image->Format)
		{
			case IL_RGB:
			case IL_RGBA:
				for (i = 0; i < image->SizeOfData; i += image->Bpp) {
					Dist1 = (ILint)image->Data[i] - (ILint)ClearCol[0];
					Dist2 = (ILint)image->Data[i+1] - (ILint)ClearCol[1];
					Dist3 = (ILint)image->Data[i+2] - (ILint)ClearCol[2];
					Distance = (ILint)sqrt((float)(Dist1 * Dist1 + Dist2 * Dist2 + Dist3 * Dist3));
					if (Distance >= -TolVal && Distance <= TolVal) {
						image->Data[i] = Red;
						image->Data[i+1] = Green;
						image->Data[i+2] = Blue;
					}
				}
				break;
			case IL_BGR:
			case IL_BGRA:
				for (i = 0; i < image->SizeOfData; i += image->Bpp) {
					Dist1 = (ILint)image->Data[i] - (ILint)ClearCol[0];
					Dist2 = (ILint)image->Data[i+1] - (ILint)ClearCol[1];
					Dist3 = (ILint)image->Data[i+2] - (ILint)ClearCol[2];
					Distance = (ILint)sqrt((float)(Dist1 * Dist1 + Dist2 * Dist2 + Dist3 * Dist3));
					if (Distance >= -TolVal && Distance <= TolVal) {
						image->Data[i+2] = Red;
						image->Data[i+1] = Green;
						image->Data[i] = Blue;
					}
				}
				break;
			case IL_LUMINANCE:
			case IL_LUMINANCE_ALPHA:
				for (i = 0; i < image->SizeOfData; i += image->Bpp) {
					Dist1 = (ILint)image->Data[i] - (ILint)ClearCol[0];
					if (Dist1 >= -TolVal && Dist1 <= TolVal) {
						image->Data[i] = Blue;
					}
				}
				break;
			//case IL_COLOUR_INDEX:  // @TODO
		}
	}

	return IL_TRUE;
}


// Credit goes to Lionel Brits for this (refer to credits.txt)
ILboolean ILAPIENTRY ilu2Equalize(ILimage* image) 
{
	ILuint	Histogram[256]; // image Histogram
	ILuint	SumHistm[256]; // normalized Histogram and LUT
	ILuint	i = 0; // index variable
	ILuint	j = 0; // index variable
	ILuint	Sum=0;
	ILuint	NumPixels, Bpp;
	ILint	Intensity;
	ILfloat	Scale;
	ILint	IntensityNew;
	ILimage	*LumImage;
	ILuint	NewColour[4];
	ILubyte		*BytePtr;
	ILushort	*ShortPtr;
	ILuint		*IntPtr;

	NewColour[0] = NewColour[1] = NewColour[2] = NewColour[3] = 0;

	if (image == NULL) {
		il2SetError(ILU_ILLEGAL_OPERATION);
		return 0;
	}

	// @TODO:  Change to work with other types!
	if (image->Bpc > 1) {
		il2SetError(ILU_INTERNAL_ERROR);
		return IL_FALSE;
	}

	if (image->Format == IL_COLOUR_INDEX) {
		Bpp = ilGetBppPal(image->Pal.getPalType());
		NumPixels = image->Pal.getPalSize() / Bpp;
	} else {
		NumPixels = image->Width * image->Height * image->Depth;
		Bpp = image->Bpp;
	}

	// Clear the tables.
	imemclear(Histogram, 256 * sizeof(ILuint));
	imemclear(SumHistm,  256 * sizeof(ILuint));

	LumImage = iConvertImage(image, IL_LUMINANCE, IL_UNSIGNED_BYTE); // the type must be left as it is!
	if (LumImage == NULL)
		return IL_FALSE;
	for (i = 0; i < NumPixels; i++) {
		Histogram[LumImage->Data[i]]++;
	}

	// Calculate normalized Sum of Histogram.
	for (i = 0; i < 256; i++) {
		for (j = 0; j < i; j++)
			Sum += Histogram[j];

		SumHistm[i] = (Sum << 8) / NumPixels;
		Sum = 0;
	}


	BytePtr = (image->Format == IL_COLOUR_INDEX) ? image->Pal.getPalette() : image->Data;
	ShortPtr = (ILushort*)image->Data;
	IntPtr = (ILuint*)image->Data;

	// Transform image using new SumHistm as a LUT
	for (i = 0; i < NumPixels; i++) {
		Intensity = LumImage->Data[i];

		// Look up the normalized intensity
		IntensityNew = (ILint)SumHistm[Intensity];

		// Find out by how much the intensity has been Scaled
		Scale = (ILfloat)IntensityNew / (ILfloat)Intensity;

		switch (image->Bpc)
		{
			case 1:
				// Calculate new pixel(s)
				NewColour[0] = (ILuint)(BytePtr[i * image->Bpp] * Scale);
				if (Bpp >= 3) {
					NewColour[1] = (ILuint)(BytePtr[i * image->Bpp + 1] * Scale);
					NewColour[2] = (ILuint)(BytePtr[i * image->Bpp + 2] * Scale);
				}

				// Clamp values
				if (NewColour[0] > UCHAR_MAX)
					NewColour[0] = UCHAR_MAX;
				if (Bpp >= 3) {
					if (NewColour[1] > UCHAR_MAX)
						NewColour[1] = UCHAR_MAX;
					if (NewColour[2] > UCHAR_MAX)
						NewColour[2] = UCHAR_MAX;
				}

				// Store pixel(s)
				BytePtr[i * image->Bpp] = (ILubyte)NewColour[0];
				if (Bpp >= 3) {
					BytePtr[i * image->Bpp + 1]	= (ILubyte)NewColour[1];
					BytePtr[i * image->Bpp + 2]	= (ILubyte)NewColour[2];
				}
				break;
		}
	}

	ilCloseImage(LumImage);

	return IL_TRUE;
}
