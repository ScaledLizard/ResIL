//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods, Marco Fabbricatore (fabbrica@ai-lab.fh-furtwangen.de)
// Last modified by Björn Ganster in 2014
//
// Filename: src-IL/src/il_tiff.cpp
//
// Description: Tiff (.tif) functions
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_TIF

#include <time.h>
#include "tiffio.h"
#include "il_manip.h"
//#include "IL/il2.h"

#define MAGIC_HEADER1	0x4949
#define MAGIC_HEADER2	0x4D4D


#if (defined(_WIN32) || defined(_WIN64)) && defined(IL_USE_PRAGMA_LIBS)
	#if defined(_MSC_VER) || defined(__BORLANDC__)
		#ifndef _DEBUG
			#pragma comment(lib, "libtiff.lib")
		#else
			#pragma comment(lib, "libtiff-d.lib")
		#endif
	#endif
#endif


/*----------------------------------------------------------------------------*/

// No need for a separate header
static ILboolean iLoadTiffInternal(void);
static char*     iMakeString(void);

/*----------------------------------------------------------------------------*/

ILboolean ilisValidTiffExtension(ILconst_string FileName)
{
	if (!iCheckExtension((ILstring)FileName, IL_TEXT("tif")) &&
		!iCheckExtension((ILstring)FileName, IL_TEXT("tiff")))
		return IL_FALSE;
	else
		return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

ILboolean ilIsValidTiffFunc(SIO* io)
{
	ILushort Header1 = 0, Header2 = 0;

	Header1 = GetLittleUShort(io);
	ILboolean bRet = IL_TRUE;

	if (Header1 == MAGIC_HEADER1) {
		Header2 = GetLittleUShort(io);
		io->seek(io, -4, SEEK_CUR);
	} else if (Header1 == MAGIC_HEADER2) {
		Header2 = GetBigUShort(io);
		io->seek(io, -4, SEEK_CUR);
	} else {
		bRet = IL_FALSE;
		io->seek(io, -2, SEEK_CUR);
	}

	if (Header2 != 42)
		bRet = IL_FALSE;

	return bRet;
}

/*----------------------------------------------------------------------------*/

static tsize_t 
_tiffFileReadProc(thandle_t fd, tdata_t pData, tsize_t tSize)
{
	ILimage* image = (ILimage*) fd;
	return (tsize_t) image->io.read(&image->io, pData, 1, tSize);
}

/*----------------------------------------------------------------------------*/

// We have trouble sometimes reading when writing a file.  Specifically, the only time
//  I have seen libtiff call this is at the beginning of writing a tiff, before
//  anything is ever even written!  Also, we have to return 0, no matter what tSize
//  is.  If we return tSize like would be expected, then TIFFClientOpen fails.
static tsize_t 
_tiffFileReadProcW(thandle_t fd, tdata_t pData, tsize_t tSize)
{
	ILimage* image = (ILimage*) fd;
	return 0;
}

/*----------------------------------------------------------------------------*/

static tsize_t 
_tiffFileWriteProc(thandle_t fd, tdata_t pData, tsize_t tSize)
{
	ILimage* image = (ILimage*) fd;
	return (tsize_t) image->io.write(pData, 1, tSize, &image->io);
}

/*----------------------------------------------------------------------------*/

static toff_t
_tiffFileSeekProc(thandle_t fd, toff_t tOff, int whence)
{
	ILimage* image = (ILimage*) fd;
	/* we use this as a special code, so avoid accepting it */
	if (tOff == 0xFFFFFFFF)
		return 0xFFFFFFFF;

	image->io.seek(&image->io, tOff, whence);
	return image->io.tell(&image->io);
	//return tOff;
}

/*----------------------------------------------------------------------------*/

static toff_t
_tiffFileSeekProcW(thandle_t fd, toff_t tOff, int whence)
{
	ILimage* image = (ILimage*) fd;

	/* we use this as a special code, so avoid accepting it */
	if (tOff == 0xFFFFFFFF)
		return 0xFFFFFFFF;

	image->io.seek(&image->io, tOff, whence);
	return image->io.tell(&image->io);
	//return tOff;
}

/*----------------------------------------------------------------------------*/

static int
_tiffFileCloseProc(thandle_t fd)
{
	fd;
	return (0);
}

/*----------------------------------------------------------------------------*/

static toff_t
_tiffFileSizeProc(thandle_t fd)
{
	ILimage* image = (ILimage*) fd;
	auto Offset = image->io.tell(&image->io);
	image->io.seek(&image->io, 0, IL_SEEK_END);
	auto Size = image->io.tell(&image->io);
	image->io.seek(&image->io, Offset, IL_SEEK_SET);

	return Size;
}

/*----------------------------------------------------------------------------*/

static toff_t
_tiffFileSizeProcW(thandle_t fd)
{
	ILimage* image = (ILimage*) fd;
	auto Offset = image->io.tell(&image->io);
	image->io.seek(&image->io, 0, IL_SEEK_END);
	auto Size = image->io.tell(&image->io);
	image->io.seek(&image->io, Offset, IL_SEEK_SET);

	return Size;
}

/*----------------------------------------------------------------------------*/

#ifdef __BORLANDC__
#pragma argsused
#endif
static int
_tiffDummyMapProc(thandle_t fd, tdata_t* pbase, toff_t* psize)
{
	fd; pbase; psize;
	return 0;
}

/*----------------------------------------------------------------------------*/

#ifdef __BORLANDC__
#pragma argsused
#endif
static void
_tiffDummyUnmapProc(thandle_t fd, tdata_t base, toff_t size)
{
	fd; base; size;
	return;
}

/*----------------------------------------------------------------------------*/

TIFF *iTIFFOpen(ILimage* image, char *Mode)
{
	TIFF *tif;

	if (Mode[0] == 'w')
		tif = TIFFClientOpen("TIFFMemFile", Mode,
							image,
							_tiffFileReadProcW, _tiffFileWriteProc,
							_tiffFileSeekProcW, _tiffFileCloseProc,
							_tiffFileSizeProcW, _tiffDummyMapProc,
							_tiffDummyUnmapProc);
	else
		tif = TIFFClientOpen("TIFFMemFile", Mode,
							image,
							_tiffFileReadProc, _tiffFileWriteProc,
							_tiffFileSeekProc, _tiffFileCloseProc,
							_tiffFileSizeProc, _tiffDummyMapProc,
							_tiffDummyUnmapProc);
	
	return tif;
}

/*----------------------------------------------------------------------------*/

// Global debug variables
int tiffWarnings = 0;
int tiffErrors = 0;

void warningHandler(const char* mod, const char* fmt, va_list ap)
{
	++tiffWarnings;
	mod; fmt; ap;
	//char buff[1024];
	//vsnprintf(buff, 1024, fmt, ap);
}

void errorHandler(const char* mod, const char* fmt, va_list ap)
{
	++tiffErrors;
	mod; fmt; ap;
	//char buff[1024];
	//vsnprintf(buff, 1024, fmt, ap);
}

////

struct TiffLoadState {
	uint32 w;
	uint32 h;
	uint16 samplesperpixel;
	uint16 bitspersample;
	uint16 * sampleinfo;
	uint16 extrasamples;
	uint16 orientation;
	uint32 linesize;
	uint16 photometric;
	uint16 planarconfig;
	uint32 tilewidth; 
	uint32 tilelength;
	ILimage* frame;
	TIFF *tif;
};

void initTiffLoadState(TIFF *tif, TiffLoadState& state, ILimage* baseImage)
{
	state.w = 0;
	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH,  &state.w);

	state.h = 0;
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &state.h);

	state.samplesperpixel = 0;
	TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &state.samplesperpixel);

	state.bitspersample = 0;
	TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE, &state.bitspersample);

	// todo: use this information
	//uint32 d = 0;
	//TIFFGetFieldDefaulted(tif, TIFFTAG_IMAGEDEPTH,		&d);

	state.sampleinfo = NULL;
	state.extrasamples = 0;
	TIFFGetFieldDefaulted(tif, TIFFTAG_EXTRASAMPLES, &state.extrasamples, &state.sampleinfo);

	state.orientation = 0;
	TIFFGetFieldDefaulted(tif, TIFFTAG_ORIENTATION, &state.orientation);

	state.linesize = TIFFScanlineSize(tif);

	//added 2003-08-31
	//1 bpp tiffs are not neccessarily greyscale, they can
	//have a palette (photometric == 3)...get this information
	state.photometric = 0;
	TIFFGetFieldDefaulted(tif, TIFFTAG_PHOTOMETRIC,  &state.photometric);

	state.planarconfig = 0;
	TIFFGetFieldDefaulted(tif, TIFFTAG_PLANARCONFIG, &state.planarconfig);

	//special-case code for frequent data cases that may be read more
	//efficiently than with the TIFFReadRGBAImage() interface.
		
	//added 2004-05-12
	//Get tile sizes and use TIFFReadRGBAImage() for tiled images for now
	state.tilewidth = state.w; 
	state.tilelength = state.h;
	TIFFGetFieldDefaulted(tif, TIFFTAG_TILEWIDTH,  &state.tilewidth);
	TIFFGetFieldDefaulted(tif, TIFFTAG_TILELENGTH, &state.tilelength);

	state.frame = baseImage;
	state.tif = tif;
}

ILboolean addFrame(TiffLoadState& state, int frameNo,
	ILubyte bpp, ILenum format)
{
	if (frameNo == 0) {
		int type = IL_UNSIGNED_BYTE;
		if (state.bitspersample == 16) type = IL_UNSIGNED_SHORT;
		if (!il2TexImage(state.frame, state.w, state.h, 1, bpp, format, type, NULL)) {
			TIFFClose(state.tif);
			return IL_FALSE;
		}
	}
	else {
		state.frame->Next = il2NewImage(state.w, state.h, 1, 1, 1);
		if (state.frame->Next == NULL) {
			TIFFClose(state.tif);
			return IL_FALSE;
		}
		state.frame = state.frame->Next;
	}

	return IL_TRUE;
}

// Decode luminance and paletted images
ILboolean decodeTiffLuminanceOrPaletted(ILimage* baseImage, TiffLoadState& state,
	int frameNo)
{
	if (!addFrame(state, frameNo, 1, IL_LUMINANCE))
		return IL_FALSE;

	ILubyte* strip;
	tsize_t stripsize;
	ILuint y;
	uint32 rowsperstrip, j, linesread;

	//TODO: 1 bit/pixel images should not be stored as 8 bits...
	//(-> add new format)

	if (state.photometric == PHOTOMETRIC_PALETTE) { //read palette
		uint16 *red, *green, *blue;
		uint32 count = 1 << state.bitspersample, j;
		
		TIFFGetField(state.tif, TIFFTAG_COLORMAP, &red, &green, &blue);

		state.frame->Format = IL_COLOUR_INDEX;
		state.frame->Pal.use(count, NULL, IL_PAL_RGB24);
		for (j = 0; j < count; ++j)
			state.frame->Pal.setRGB(j, red[j] >> 8, green[j] >> 8, blue[j] >> 8);
	}

	TIFFGetField(state.tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
	stripsize = TIFFStripSize(state.tif);

	strip = (ILubyte*)ialloc(stripsize);

	if (state.bitspersample == 8 || state.bitspersample == 16) {
		ILubyte *dat = state.frame->Data;
		for (y = 0; y < state.h; y += rowsperstrip) {
			//the last strip may contain less data if the image
			//height is not evenly divisible by rowsperstrip
			if (y + rowsperstrip > state.h) {
				stripsize = state.linesize*(state.h - y);
				linesread = state.h - y;
			}
			else
				linesread = rowsperstrip;

			if (TIFFReadEncodedStrip(state.tif, TIFFComputeStrip(state.tif, y, 0), strip, stripsize) == -1) {
				il2SetError(IL_LIB_TIFF_ERROR);
				ifree(strip);
				TIFFClose(state.tif);
				return IL_FALSE;
			}

			if (state.photometric == PHOTOMETRIC_MINISWHITE) { //invert channel
				uint32 k, t2;
				for (j = 0; j < linesread; ++j) {
					t2 = j*state.linesize;
					//this works for 16bit images as well: the two bytes
					//making up a pixel can be inverted independently
					for (k = 0; k < state.frame->Bps; ++k)
						dat[k] = ~strip[t2 + k];
					dat += state.w;
				}
			}
			else
				for(j = 0; j < linesread; ++j)
					memcpy(&state.frame->Data[(y + j)*state.frame->Bps], &strip[j*state.linesize], 
						state.frame->Bps);
		}
	}
	else if (state.bitspersample == 1) {
		//TODO: add a native format to devil, so we don't have to
		//unpack the values here
		ILubyte mask, curr, *dat = state.frame->Data;
		uint32 k, sx, t2;
		for (y = 0; y < state.h; y += rowsperstrip) {
			//the last strip may contain less data if the image
			//height is not evenly divisible by rowsperstrip
			if (y + rowsperstrip > state.h) {
				stripsize = state.linesize*(state.h - y);
				linesread = state.h - y;
			}
			else
				linesread = rowsperstrip;

			if (TIFFReadEncodedStrip(state.tif, TIFFComputeStrip(state.tif, y, 0), strip, stripsize) == -1) {
				il2SetError(IL_LIB_TIFF_ERROR);
				ifree(strip);
				TIFFClose(state.tif);
				return IL_FALSE;
			}

			for (j = 0; j < linesread; ++j) {
				k = 0;
				sx = 0;
				t2 = j*state.linesize;
				while (k < state.w) {
					curr = strip[t2 + sx];
					if (state.photometric == PHOTOMETRIC_MINISWHITE)
						curr = ~curr;
					for (mask = 0x80; mask != 0 && k < state.w; mask >>= 1){
						if((curr & mask) != 0)
							dat[k] = 255;
						else
							dat[k] = 0;
						++k;
					}
					++sx;
				}
				dat += state.w;
			}
		}
	}

	ifree(strip);

	if(state.orientation == ORIENTATION_TOPLEFT)
		state.frame->Origin = IL_ORIGIN_UPPER_LEFT;
	else if(state.orientation == ORIENTATION_BOTLEFT)
		state.frame->Origin = IL_ORIGIN_LOWER_LEFT;

	return IL_TRUE;
}

// Decode RGB16 images
ILboolean decodeRGB16 (ILimage* baseImage, TiffLoadState& state, int frameNo)
{
	if (!addFrame(state, frameNo, 3, IL_RGB))
		return IL_FALSE;

	ILubyte *strip, *dat;
	tsize_t stripsize;
	ILuint y;
	uint32 rowsperstrip, j, linesread;

	TIFFGetField(state.tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
	stripsize = TIFFStripSize(state.tif);

	strip = (ILubyte*)ialloc(stripsize);

	dat = state.frame->Data;
	for (y = 0; y < state.h; y += rowsperstrip) {
		//the last strip may contain less data if the image
		//height is not evenly divisible by rowsperstrip
		if (y + rowsperstrip > state.h) {
			stripsize = state.linesize*(state.h - y);
			linesread = state.h - y;
		}
		else
			linesread = rowsperstrip;

		if (TIFFReadEncodedStrip(state.tif, TIFFComputeStrip(state.tif, y, 0), strip, stripsize) == -1) {
			il2SetError(IL_LIB_TIFF_ERROR);
			ifree(strip);
			TIFFClose(state.tif);
			return IL_FALSE;
		}

		for(j = 0; j < linesread; ++j)
				memcpy(&state.frame->Data[(y + j)*state.frame->Bps], &strip[j*state.linesize], 
					state.frame->Bps);
	}

	ifree(strip);
			
	if(state.orientation == ORIENTATION_TOPLEFT)
		state.frame->Origin = IL_ORIGIN_UPPER_LEFT;
	else if(state.orientation == ORIENTATION_BOTLEFT)
		state.frame->Origin = IL_ORIGIN_LOWER_LEFT;

	return IL_TRUE;
}

// decode not directly supported formats
ILboolean decodeOther(ILimage* baseImage, TiffLoadState& state, int frameNo)
{
	if (!addFrame(state, frameNo, 4, IL_RGBA))
		return IL_FALSE;

	if (state.samplesperpixel == 4) {
		TIFFGetFieldDefaulted(state.tif, TIFFTAG_EXTRASAMPLES, &state.extrasamples, &state.sampleinfo);
		if (!state.sampleinfo || state.sampleinfo[0] == EXTRASAMPLE_UNSPECIFIED) {
			ILushort si = EXTRASAMPLE_ASSOCALPHA;
			TIFFSetField(state.tif, TIFFTAG_EXTRASAMPLES, 1, &si);
		}
	}
	state.frame->Format = IL_RGBA;
	state.frame->Type = IL_UNSIGNED_BYTE;
	
	// Siigron: added u_long cast to shut up compiler warning
	//2003-08-31: changed flag from 1 (exit on error) to 0 (keep decoding)
	//this lets me view text.tif, but can give crashes with unsupported
	//tiffs...
	//2003-09-04: keep flag 1 for official version for now
	if (!TIFFReadRGBAImage(state.tif, state.frame->Width, state.frame->Height, 
		(uint32*)state.frame->Data, 0)) 
	{
		TIFFClose(state.tif);
		il2SetError(IL_LIB_TIFF_ERROR);
		return IL_FALSE;
	}
	state.frame->Origin = IL_ORIGIN_LOWER_LEFT;  // eiu...dunno if this is right

	#ifdef __BIG_ENDIAN__ //TIFFReadRGBAImage reads abgr on big endian, convert to rgba
	EndianSwapData(Image);
	#endif

	/*
		The following switch() should not be needed,
		because we take care of the special cases that needed
		these conversions. But since not all special cases
		are handled right now, keep it :)
		*/
	//TODO: put switch into the loop??
	ILimage* TempImage = baseImage;
	baseImage = state.frame; //ilConvertImage() converts image
	switch (state.samplesperpixel)
	{
		case 1:
			//added 2003-08-31 to keep palettized tiffs colored
			if(state.photometric != 3)
				il2ConvertImage(baseImage, IL_LUMINANCE, IL_UNSIGNED_BYTE);
			else //strip alpha as tiff supports no alpha palettes
				il2ConvertImage(baseImage, IL_RGB, IL_UNSIGNED_BYTE);
			break;
					
		case 3:
			il2ConvertImage(baseImage, IL_RGB, IL_UNSIGNED_BYTE);
			break; 
	}
	baseImage = TempImage;

	return IL_TRUE;
}

ILboolean decodeFrame(ILimage* baseImage, TIFF *tif, int frameNo)
{
	ILboolean success = IL_TRUE;
	TiffLoadState state;

	initTiffLoadState(tif, state, baseImage);

	if (state.extrasamples == 0
		&& state.samplesperpixel == 1  //luminance or palette
		&& (state.bitspersample == 8 || state.bitspersample == 1 || state.bitspersample == 16)
		&& (state.photometric == PHOTOMETRIC_MINISWHITE
			|| state.photometric == PHOTOMETRIC_MINISBLACK
			|| state.photometric == PHOTOMETRIC_PALETTE)
		&& (state.orientation == ORIENTATION_TOPLEFT || state.orientation == ORIENTATION_BOTLEFT)
		&& state.tilewidth == state.w && state.tilelength == state.h) 
	{
		success = decodeTiffLuminanceOrPaletted(baseImage, state, frameNo);
	}
	else if (state.extrasamples == 0
		&& state.samplesperpixel == 3
		&& (state.bitspersample == 8 || state.bitspersample == 16)
		&& state.photometric == PHOTOMETRIC_RGB
		&& state.planarconfig == 1
		&& (state.orientation == ORIENTATION_TOPLEFT || state.orientation == ORIENTATION_BOTLEFT)
		&& state.tilewidth == state.w && state.tilelength == state.h
		) 
	{
		success = decodeRGB16(baseImage, state, frameNo);
	} else {
		success = decodeOther(baseImage, state, frameNo);
	} 

	ILuint ProfileLen = 0;
	void	 *Buffer = NULL;
	if (TIFFGetField(tif, TIFFTAG_ICCPROFILE, &ProfileLen, &Buffer)) {
		if (state.frame->Profile && state.frame->ProfileSize)
			ifree(state.frame->Profile);
		state.frame->Profile = (ILubyte*)ialloc(ProfileLen);
		if (state.frame->Profile == NULL) {
			TIFFClose(tif);
			return IL_FALSE;
		}

		memcpy(state.frame->Profile, Buffer, ProfileLen);
		state.frame->ProfileSize = ProfileLen;

		//removed on 2003-08-24 as explained in bug 579574 on sourceforge
		//_TIFFfree(Buffer);
	}
	ILfloat x_position = 0;
	ILfloat x_resolution = 0;
	ILfloat y_position = 0;
	ILfloat y_resolution = 0;
	if (TIFFGetField(tif, TIFFTAG_XPOSITION, &x_position) == 0)
		x_position = 0;
	if (TIFFGetField(tif, TIFFTAG_XRESOLUTION, &x_resolution) == 0)
		x_resolution = 0;
	if (TIFFGetField(tif, TIFFTAG_YPOSITION, &y_position) == 0)
		y_position = 0;
	if (TIFFGetField(tif, TIFFTAG_YRESOLUTION, &y_resolution) == 0)
		y_resolution = 0;

	//offset in pixels of "cropped" image from top left corner of 
	//full image (rounded to nearest integer)
	state.frame->OffX = (uint32) ((x_position * x_resolution) + 0.49);
	state.frame->OffY = (uint32) ((y_position * y_resolution) + 0.49);

	return success;
}

// Internal function used to load the Tiff.
ILboolean iLoadTiffInternal(ILimage* baseImage)
{
	if (baseImage == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	//TIFFSetWarningHandler (NULL);
	//TIFFSetErrorHandler   (NULL);

	//for debugging only
	tiffWarnings = 0;
	tiffErrors = 0;
	TIFFSetWarningHandler(warningHandler);
	TIFFSetErrorHandler(errorHandler);

	TIFF	 *tif = iTIFFOpen(baseImage, "r");
	if (tif == NULL) {
		il2SetError(IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	ILuint DirCount = 0;
	do {
		DirCount++;
	} while (TIFFReadDirectory(tif));

	ILboolean result = IL_TRUE;
	for (ILuint i = 0; i < DirCount; i++) {
		TIFFSetDirectory(tif, (tdir_t)i);
		if (!decodeFrame(baseImage, tif, i))
			result = false;
	}

	if (result)
		TIFFClose(tif);

	result &= il2FixImage(baseImage);
	return result;
}

/*----------------------------------------------------------------------------*/

// @TODO:  Accept palettes!

// Internal function used to save the Tiff.
ILboolean iSaveTiffInternal(ILimage* image)
{
	ILenum	Format;
	ILenum	Compression;
	ILuint	ixLine;
	TIFF	*File;
	char	Description[512];
	ILimage *TempImage;
	const char	*str;
	ILboolean SwapColors;
	ILubyte *OldData;

	if(image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

#if 1
	TIFFSetWarningHandler (NULL);
	TIFFSetErrorHandler   (NULL);
#else
	//for debugging only
	TIFFSetWarningHandler(warningHandler);
	TIFFSetErrorHandler(errorHandler);
#endif
	if (iGetHint(IL_COMPRESSION_HINT) == IL_USE_COMPRESSION)
		Compression = COMPRESSION_LZW;
	else
		Compression = COMPRESSION_NONE;

	if (image->Format == IL_COLOUR_INDEX) {
		if (ilGetBppPal(image->Pal.getPalType()) == 4)  // Preserve the alpha.
			TempImage = iConvertImage(image, IL_RGBA, IL_UNSIGNED_BYTE);
		else
			TempImage = iConvertImage(image, IL_RGB, IL_UNSIGNED_BYTE);
		
		if (TempImage == NULL) {
			return IL_FALSE;
		}
	}
	else {
		TempImage = image;
	}

	/*#ifndef _UNICODE
		File = TIFFOpen(Filename, "w");
	#else
		File = TIFFOpenW(Filename, "w");
	#endif*/

	// Control writing functions ourself.
	File = iTIFFOpen(image, "w");
	if (File == NULL) {
		il2SetError(IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	sprintf(Description, "Tiff generated by %s", il2GetString(IL_VERSION_NUM));

	TIFFSetField(File, TIFFTAG_IMAGEWIDTH, TempImage->Width);
	TIFFSetField(File, TIFFTAG_IMAGELENGTH, TempImage->Height);
	TIFFSetField(File, TIFFTAG_COMPRESSION, Compression);
	if((TempImage->Format == IL_LUMINANCE) || (TempImage->Format == IL_LUMINANCE_ALPHA))
		TIFFSetField(File, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
	else
		TIFFSetField(File, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
	TIFFSetField(File, TIFFTAG_BITSPERSAMPLE, TempImage->Bpc << 3);
	TIFFSetField(File, TIFFTAG_SAMPLESPERPIXEL, TempImage->Bpp);
	if ((TempImage->Bpp == ilGetBppFormat(IL_RGBA)) ||
			(TempImage->Bpp == ilGetBppFormat(IL_LUMINANCE_ALPHA)))
		TIFFSetField(File, TIFFTAG_EXTRASAMPLES, EXTRASAMPLE_ASSOCALPHA);
	TIFFSetField(File, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(File, TIFFTAG_ROWSPERSTRIP, 1);
	TIFFSetField(File, TIFFTAG_SOFTWARE, il2GetString(IL_VERSION_NUM));
	/*TIFFSetField(File, TIFFTAG_DOCUMENTNAME,
		iGetString(IL_TIF_DOCUMENTNAME_STRING) ?
		iGetString(IL_TIF_DOCUMENTNAME_STRING) : FileName);
*/
	str = iGetString(IL_TIF_DOCUMENTNAME_STRING);
	if (str) {
		TIFFSetField(File, TIFFTAG_DOCUMENTNAME, str);
		ifree(str);
	}


	str = iGetString(IL_TIF_AUTHNAME_STRING);
	if (iGetString(IL_TIF_AUTHNAME_STRING)) {
		TIFFSetField(File, TIFFTAG_ARTIST, str);
		ifree(str);
	}

	str = iGetString(IL_TIF_HOSTCOMPUTER_STRING);
	if (str) {
		TIFFSetField(File, TIFFTAG_HOSTCOMPUTER, str);
		ifree(str);
	}

	str = iGetString(IL_TIF_HOSTCOMPUTER_STRING);
	if (str) {
		TIFFSetField(File, TIFFTAG_IMAGEDESCRIPTION, str);
		ifree(str);
	}

	// Set the date and time string.
	TIFFSetField(File, TIFFTAG_DATETIME, iMakeString());

	// 24/4/2003
	// Orientation flag is not always supported (Photoshop, ...), orient the image data 
	// and set it always to normal view
	TIFFSetField(File, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	if (TempImage->Origin != IL_ORIGIN_UPPER_LEFT) {
		ILubyte* Data = iGetFlipped(TempImage);
		OldData = TempImage->Data;
		TempImage->Data = Data;
	}
	else
		OldData = TempImage->Data;

	/*
	 TIFFSetField(File, TIFFTAG_ORIENTATION,
				  TempImage->Origin == IL_ORIGIN_UPPER_LEFT ? ORIENTATION_TOPLEFT : ORIENTATION_BOTLEFT);
	 */

	Format = TempImage->Format;
	SwapColors = (Format == IL_BGR || Format == IL_BGRA);
	if (SwapColors)
 		il2SwapColours(TempImage);

	for (ixLine = 0; ixLine < TempImage->Height; ++ixLine) {
		if (TIFFWriteScanline(File, TempImage->Data + ixLine * TempImage->Bps, ixLine, 0) < 0) {
			TIFFClose(File);
			il2SetError(IL_LIB_TIFF_ERROR);
			if (SwapColors)
				il2SwapColours(TempImage);
			if (TempImage->Data != OldData) {
				ifree( TempImage->Data );
				TempImage->Data = OldData;
			}
			if (TempImage != image)
				il2DeleteImage(TempImage);
			return IL_FALSE;
		}
	}

	if (SwapColors)
 		il2SwapColours(TempImage);

	if (TempImage->Data != OldData) {
		ifree(TempImage->Data);
		TempImage->Data = OldData;
	}

	if (TempImage != image)
		il2DeleteImage(TempImage);

	TIFFClose(File);

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/
// Makes a neat date string for the date field.
// From http://www.awaresystems.be/imaging/tiff/tifftags/datetime.html :
// The format is: "YYYY:MM:DD HH:MM:SS", with hours like those on
// a 24-hour clock, and one space character between the date and the
// time. The length of the string, including the terminating NUL, is
// 20 bytes.)
char *iMakeString()
{
	static char TimeStr[20];
	time_t		Time;
	struct tm	*CurTime;

	imemclear(TimeStr, 20);

	time(&Time);
#ifdef _WIN32
	_tzset();
#endif
	CurTime = localtime(&Time);

	strftime(TimeStr, 20, "%Y:%m:%d %H:%M:%S", CurTime);
	
	return TimeStr;
}

/*----------------------------------------------------------------------------*/

#endif//IL_NO_TIF
