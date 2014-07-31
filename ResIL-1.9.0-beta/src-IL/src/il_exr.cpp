//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_exr.cpp
//
// Description: Reads from an OpenEXR (.exr) file using the OpenEXR library.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_EXR

#ifndef HAVE_CONFIG_H // We are probably on a Windows box .
//#define OPENEXR_DLL
#define HALF_EXPORTS
#endif //HAVE_CONFIG_H

#include "il_exr.h"
#include <ImfRgba.h>
#include <ImfArray.h>
#include <ImfRgbaFile.h>
#include "IL/il2.h"


#if (defined(_WIN32) || defined(_WIN64)) && defined(IL_USE_PRAGMA_LIBS)
	#if defined(_MSC_VER) || defined(__BORLANDC__)
		#ifndef _DEBUG
			#pragma comment(lib, "openexr.lib")
		#else
			#pragma comment(lib, "openexr-d.lib")
		#endif
	#endif
#endif


// Internal function used to get the EXR header from the current file.
ILint iGetExrHead(SIO* io, EXRHEAD *Header)
{
	ILint read = (ILint) io->read(io, Header, 1, sizeof(EXRHEAD));

	#ifdef __BIG_ENDIAN__
	iSwapUInt(&Header->MagicNumber);
	iSwapUInt(&Header->Version);
	#endif

	return read;
}


// Internal function used to check if the HEADER is a valid EXR header.
ILboolean iCheckExr(EXRHEAD *Header)
{
	// The file magic number (signature) is 0x76, 0x2f, 0x31, 0x01
	if (Header->MagicNumber != 0x01312F76)
		return IL_FALSE;
	// The only valid version so far is version 2.  The upper value has
	//  to do with tiling.
	if (Header->Version != 0x002 && Header->Version != 0x202)
		return IL_FALSE;

	return IL_TRUE;
}


// Internal function to get the header and check it.
ILboolean iIsValidExr(SIO* io)
{
	EXRHEAD Head;

	auto read = iGetExrHead(io, &Head);
	io->seek(io, -read, IL_SEEK_CUR);
	
	if (read == sizeof(EXRHEAD))
		return iCheckExr(&Head);
	else
		return IL_FALSE;
}


// Nothing to do here in the constructor.
ilIStream::ilIStream(ILimage * aImage) 
	: Imf::IStream("N/A"),
	  mIo(NULL)
{
	if (aImage != NULL) {
		mIo = &aImage->io;
	}

	return;
}


bool ilIStream::read(char c[], int n)
{
	if (mIo->read(mIo, c, 1, n) != n)
		return false;
	return true;
}


//@TODO: Make this work with 64-bit values.
Imf::Int64 ilIStream::tellg()
{
	Imf::Int64 Pos;

	// itell only returns a 32-bit value!
	Pos = mIo->tell(mIo);

	return Pos;
}


// Note that there is no return value here, even though there probably should be.
void ilIStream::seekg(Imf::Int64 Pos)
{
	mIo->seek(mIo, (ILint)Pos, IL_SEEK_SET);  // I am assuming this is seeking from the beginning.
	return;
}


void ilIStream::clear()
{
	return;
}

using namespace Imath;
using namespace Imf;
using namespace std;


ILboolean iLoadExrInternal(ILimage* image)
{
	Array<Rgba> pixels;
	Box2i dataWindow;
	float pixelAspectRatio;
	ILfloat *FloatData;

	ilIStream File(image);
	RgbaInputFile in(File);

	Rgba a;
	dataWindow = in.dataWindow();
	pixelAspectRatio = in.pixelAspectRatio();

	int dw, dh, dx, dy;
 
	dw = dataWindow.max.x - dataWindow.min.x + 1;
	dh = dataWindow.max.y - dataWindow.min.y + 1;
	dx = dataWindow.min.x;
	dy = dataWindow.min.y;

	pixels.resizeErase (dw * dh);
	in.setFrameBuffer (pixels - dx - dy * dw, 1, dw);

	try
    {
		in.readPixels (dataWindow.min.y, dataWindow.max.y);
    }
    catch (const exception &e)
    {
	// If some of the pixels in the file cannot be read,
	// print an error message, and return a partial image
	// to the caller.
		il2SetError(IL_LIB_EXR_ERROR);  // Could I use something a bit more descriptive based on e?
		e;  // Prevent the compiler from yelling at us about this being unused.
		return IL_FALSE;
    }

	//if (ilTexImage(dw, dh, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL) == IL_FALSE)
	//if (ilTexImage(dw, dh, 1, 4, IL_RGBA, IL_UNSIGNED_SHORT, NULL) == IL_FALSE)
	if (il2TexImage(image, dw, dh, 1, 4, IL_RGBA, IL_FLOAT, NULL) == IL_FALSE)
		return IL_FALSE;

	// Determine where the origin is in the original file.
	if (in.lineOrder() == INCREASING_Y)
		image->Origin = IL_ORIGIN_UPPER_LEFT;
	else
		image->Origin = IL_ORIGIN_LOWER_LEFT;

	// Better to access FloatData instead of recasting everytime.
	FloatData = (ILfloat*)image->Data;

	for (int i = 0; i < dw * dh; i++)
	{
		FloatData[i * 4]     = pixels[i].r;
		FloatData[i * 4 + 1] = pixels[i].g;
		FloatData[i * 4 + 2] = pixels[i].b;
		FloatData[i * 4 + 3] = pixels[i].a;
	}

	// Converts the image to predefined type, format and/or origin if needed.
	return il2FixImage(image);
}


// Nothing to do here in the constructor.
ilOStream::ilOStream(ILimage* image) 
	: Imf::OStream("N/A")
{
	if (image != NULL) {
		mIo = &image->io;
	}
	return;
}

void ilOStream::write(const char c[], int n)
{
	mIo->write(c, 1, n, mIo);  //@TODO: Throw an exception here.
	return;
}

//@TODO: Make this work with 64-bit values.
Imf::Int64 ilOStream::tellp()
{
	Imf::Int64 Pos;

	// itellw only returns a 32-bit value!
	Pos = mIo->tell(mIo);

	return Pos;
}

// Note that there is no return value here, even though there probably should be.
//@TODO: Make this work with 64-bit values.
void ilOStream::seekp(Imf::Int64 Pos)
{
	// iseekw only uses a 32-bit value!
	mIo->seek(mIo, (ILint)Pos, IL_SEEK_SET);  // I am assuming this is seeking from the beginning.
	return;
}


ILboolean iSaveExrInternal(ILimage* image)
{
	Imath::Box2i DataWindow(Imath::V2i(0, 0), Imath::V2i(image->Width-1, image->Height-1));
	Imf::LineOrder Order;
	if (image->Origin == IL_ORIGIN_LOWER_LEFT)
		Order = DECREASING_Y;
	else
		Order = INCREASING_Y;
	Imf::Header Head(image->Width, image->Height, DataWindow, 1, Imath::V2f (0, 0), 1, Order);

	ilOStream File(image);
	Imf::RgbaOutputFile Out(File, Head);
	ILimage *TempImage = image;

	//@TODO: Can we always assume that Rgba is packed the same?
	Rgba *HalfData = (Rgba*)ialloc(TempImage->Width * TempImage->Height * sizeof(Rgba));
	if (HalfData == NULL)
		return IL_FALSE;

	if (image->Format != IL_RGBA || image->Type != IL_FLOAT) {
		TempImage = iConvertImage(image, IL_RGBA, IL_FLOAT);
		if (TempImage == NULL) {
			ifree(HalfData);
			return IL_FALSE;
		}
	}

	ILuint Offset = 0;
	ILfloat *FloatPtr = (ILfloat*)TempImage->Data;
	for (unsigned int y = 0; y < TempImage->Height; y++) {
		for (unsigned int x = 0; x < TempImage->Width; x++) {
			HalfData[y * TempImage->Width + x].r = FloatPtr[Offset];
			HalfData[y * TempImage->Width + x].g = FloatPtr[Offset + 1];
			HalfData[y * TempImage->Width + x].b = FloatPtr[Offset + 2];
			HalfData[y * TempImage->Width + x].a = FloatPtr[Offset + 3];
			Offset += 4;  // 4 floats
		}
	}

	Out.setFrameBuffer(HalfData, 1, TempImage->Width);
	Out.writePixels(TempImage->Height);  //@TODO: Do each scanline separately to keep from using so much memory.

	// Free our half data.
	ifree(HalfData);
	// Destroy our temporary image if we used one.
	if (TempImage != image)
		ilCloseImage(TempImage);

	return IL_TRUE;
}


#endif //IL_NO_EXR
