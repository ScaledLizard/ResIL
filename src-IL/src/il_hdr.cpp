//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2008 by Denton Woods (this file by thakis / Denton)
// Last modified: 02/09/2009
//
// Filename: src-IL/src/il_hdr.c
//
// Description: Reads/writes a RADIANCE High Dynamic Range Image
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_HDR
#include "il_hdr.h"
#include "il_endian.h"
#include <string.h>

#ifndef _WIN32
#undef putc
#undef getc
#endif

// Find substr in bigstr, checking no more than max characters
// Zero-termination of bigstr is ignored
const char* strnstr(const char* bigstr, const char* substr, size_t max)
{
	size_t i = 0;

	while (i < max) {
		int j = 0;
		while (bigstr[i+j] == substr[j])
			++j;
		if (substr[j] == 0) 
			return bigstr+i;
		else
			++i;
	}

	return NULL;
}

ILboolean iIsValidHdr(SIO* io)
{
	char header[512];
	auto	read = io->read(io, header, 1, sizeof(header));
	if (strcmp(header, "#?RADIANCE") == 0)
		return true;
	else
		return false;
}

void ReadScanline(ILimage* image, ILubyte *scanline, ILuint w) {
	ILubyte *runner;
	ILuint r, g, b, e, read, shift;

	r = image->io.getc(&image->io);
	g = image->io.getc(&image->io);
	b = image->io.getc(&image->io);
	e = image->io.getc(&image->io);

	//check if the scanline is in the new format
	//if so, e, r, g, b are stored separated and are
	//rle-compressed independently.
	if (r == 2 && g == 2) {
		ILuint length = (b << 8) | e;
		ILuint j, t, k;
		if (length > w)
			length = w; //fix broken files
		for (k = 0; k < 4; ++k) {
			runner = scanline + k;
			j = 0;
			while (j < length) {
				t = image->io.getc(&image->io);
				if (t > 128) { //Run?
					ILubyte val = image->io.getc(&image->io);
					t &= 127;
					//copy current byte
					while (t > 0 && j < length) {
						*runner = val;
						runner += 4;
						--t;
						++j;
					}
				}
				else { //No Run.
					//read new bytes
					while (t > 0 && j < length) {
						*runner = image->io.getc(&image->io);
						runner += 4;
						--t;
						++j;
					}
				}
			}
		}
		return; //done decoding a scanline in separated format
	}

	//if we come here, we are dealing with old-style scanlines
	shift = 0;
	read = 0;
	runner = scanline;
	while (read < w) {
		if (read != 0) {
			r = image->io.getc(&image->io);
			g = image->io.getc(&image->io);
			b = image->io.getc(&image->io);
			e = image->io.getc(&image->io);
		}

		//if all three mantissas are 1, then this is a rle
		//count dword
		if (r == 1 && g == 1 && b == 1) {
			ILuint length = e;
			ILuint j;
			for (j = length << shift; j > 0 && read < w; --j) {
				memcpy(runner, runner - 4, 4);
				runner += 4;
				++read;
			}
			//if more than one rle count dword is read
			//consecutively, they are higher order bytes
			//of the first read value. shift keeps track of
			//that.
			shift += 8;
		}
		else {
			runner[0] = r;
			runner[1] = g;
			runner[2] = b;
			runner[3] = e;

			shift = 0;
			runner += 4;
			++read;
		}
	}
}


// Internal function used to load the .hdr.
ILboolean iLoadHdrInternal(ILimage* image)
{
	ILfloat *data;
	ILubyte *scanline;
	ILuint i, j, e, r, g, b;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	// Header consists of text which must be parsed
	char header[1024]; // should be sufficient...
	size_t read = (size_t) image->io.read(&image->io, header, 1, sizeof(header));
	const char* wstr = strnstr(header, "+X ", read) + 3;
	const char* hstr = strnstr(header, "-Y ", read) + 3;
	ILuint width = 0;
	ILuint height = 0;
	if (wstr != NULL) 
		width = atoi(wstr);
	if (hstr != NULL)
		height = atoi(hstr);

	// Seek to start of data
	auto headerEnd = hstr-header;
	while (headerEnd < read && header[headerEnd] != 0x0a)
		++headerEnd;
	image->io.seek(&image->io, headerEnd+1, IL_SEEK_SET);

	if (width == 0 || height == 0) {
		il2SetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// Update the current image with the new dimensions
	if (!il2TexImage(image, width, height, 1, 3, IL_RGB, IL_FLOAT, NULL)) {
		return IL_FALSE;
	}
	image->Origin = IL_ORIGIN_UPPER_LEFT;

	data = (ILfloat*)image->Data;
	scanline = (ILubyte*)ialloc(width*4);
	for (i = 0; i < height; ++i) {
		ReadScanline(image, scanline, width);

		//convert hdrs internal format to floats
		for (j = 0; j < 4*width; j += 4) {
			ILuint *ee;
			ILfloat t, *ff;
			e = scanline[j + 3];
			r = scanline[j + 0];
			g = scanline[j + 1];
			b = scanline[j + 2];

			//t = (float)pow(2.f, ((ILint)e) - 128);
			if (e != 0)
				e = (e - 1) << 23;
			
			// All this just to avoid stric-aliasing warnings...
			// was: t = *(ILfloat*)&e
			ee = &e;
			ff = (ILfloat*)ee;
			t = *ff;
			
			data[0] = (r/255.0f)*t;
			data[1] = (g/255.0f)*t;
			data[2] = (b/255.0f)*t;
			data += 3;
		}
	}
	ifree(scanline);

	return il2FixImage(image);
}

//
// Much of the saving code is based on the code by Bruce Walter,
//  available at http://www.graphics.cornell.edu/online/formats/rgbe/.
//
// The actual source code file is
//  http://www.graphics.cornell.edu/online/formats/rgbe/rgbe.c
//


/* standard conversion from float pixels to rgbe pixels */
/* note: you can remove the "inline"s if your compiler complains about it */
//static INLINE void 
static void 
float2rgbe(unsigned char rgbe[4], float red, float green, float blue)
{
  float v;
  int e;

  v = red;
  if (green > v) v = green;
  if (blue > v) v = blue;
  if (v < 1e-32) {
    rgbe[0] = rgbe[1] = rgbe[2] = rgbe[3] = 0;
  }
  else {
    v = (float)(frexp(v,&e) * 256.0/v);
    rgbe[0] = (unsigned char) (red * v);
    rgbe[1] = (unsigned char) (green * v);
    rgbe[2] = (unsigned char) (blue * v);
    rgbe[3] = (unsigned char) (e + 128);
  }
}


typedef struct {
  ILuint	valid;            /* indicate which fields are valid */
  ILbyte	programtype[16]; /* listed at beginning of file to identify it 
                         * after "#?".  defaults to "RGBE" */ 
  ILfloat	gamma;          /* image has already been gamma corrected with 
                         * given gamma.  defaults to 1.0 (no correction) */
  ILfloat	exposure;       /* a value of 1.0 in an image corresponds to
			 * <exposure> watts/steradian/m^2. 
			 * defaults to 1.0 */
} rgbe_header_info;

/* flags indicating which fields in an rgbe_header_info are valid */
#define RGBE_VALID_PROGRAMTYPE 0x01
#define RGBE_VALID_GAMMA       0x02
#define RGBE_VALID_EXPOSURE    0x04

/* offsets to red, green, and blue components in a data (float) pixel */
#define RGBE_DATA_RED    0
#define RGBE_DATA_GREEN  1
#define RGBE_DATA_BLUE   2
/* number of floats per pixel */
#define RGBE_DATA_SIZE   3


/* default minimal header. modify if you want more information in header */
ILboolean RGBE_WriteHeader(ILuint width, ILuint height, rgbe_header_info *info)
{
	char *programtype = "RGBE";

	if (info && (info->valid & RGBE_VALID_PROGRAMTYPE))
		programtype = (char*) info->programtype;
	if (ilprintf(&iCurImage->io, "#?%s\n",programtype) < 0)
		return IL_FALSE;
	/* The #? is to identify file type, the programtype is optional. */
	if (info && (info->valid & RGBE_VALID_GAMMA)) {
		if (ilprintf(&iCurImage->io, "GAMMA=%g\n",info->gamma) < 0)
		  return IL_FALSE;
	}
	if (info && (info->valid & RGBE_VALID_EXPOSURE)) {
		if (ilprintf(&iCurImage->io, "EXPOSURE=%g\n",info->exposure) < 0)
		  return IL_FALSE;
	}
	if (ilprintf(&iCurImage->io, "FORMAT=32-bit_rle_rgbe\n\n") < 0)
		return IL_FALSE;
	if (ilprintf(&iCurImage->io, "-Y %d +X %d\n", height, width) < 0)
		return IL_FALSE;
	return IL_TRUE;
}


/* simple write routine that does not use run length encoding */
/* These routines can be made faster by allocating a larger buffer and
   fread-ing and iwrite-ing the data in larger chunks */
int RGBE_WritePixels(ILimage* image, float *data, int numpixels)
{
	unsigned char rgbe[4];

	while (numpixels-- > 0) {
		float2rgbe(rgbe,data[RGBE_DATA_RED],data[RGBE_DATA_GREEN],data[RGBE_DATA_BLUE]);
		data += RGBE_DATA_SIZE;
		if (image->io.write(rgbe, sizeof(rgbe), 1, &image->io) < 1)
			return IL_FALSE;
	}
	return IL_TRUE;
}


/* The code below is only needed for the run-length encoded files. */
/* Run length encoding adds considerable complexity but does */
/* save some space.  For each scanline, each channel (r,g,b,e) is */
/* encoded separately for better compression. */

ILboolean RGBE_WriteBytes_RLE(ILimage* image, ILubyte *data, ILuint numbytes)
{
#define MINRUNLENGTH 4
	ILuint	cur, beg_run, run_count, old_run_count, nonrun_count;
	ILubyte	buf[2];

	cur = 0;
	while (cur < numbytes) {
		beg_run = cur;
		/* find next run of length at least 4 if one exists */
		run_count = old_run_count = 0;
		while((run_count < MINRUNLENGTH) && (beg_run < numbytes)) {
			beg_run += run_count;
			old_run_count = run_count;
			run_count = 1;
			// 01-25-2009: Moved test for beg_run + run_count first so that it is
			//  tested first.  This keeps it from going out of bounds by 1.
			while((beg_run + run_count < numbytes) && (run_count < 127) && 
				(data[beg_run] == data[beg_run + run_count]))
			run_count++;
		}
		/* if data before next big run is a short run then write it as such */
		if ((old_run_count > 1)&&(old_run_count == beg_run - cur)) {
			buf[0] = 128 + old_run_count;   /*write short run*/
			buf[1] = data[cur];
			if (image->io.write(buf,sizeof(buf[0])*2,1, &image->io) < 1)
				return IL_FALSE;
			cur = beg_run;
		}
		/* write out bytes until we reach the start of the next run */
		while(cur < beg_run) {
			nonrun_count = beg_run - cur;
			if (nonrun_count > 128) 
				nonrun_count = 128;
			buf[0] = nonrun_count;
			if (image->io.write(buf,sizeof(buf[0]),1, &image->io) < 1)
				return IL_FALSE;
			if (image->io.write(&data[cur],sizeof(data[0])*nonrun_count,1, &image->io) < 1)
				return IL_FALSE;
			cur += nonrun_count;
		}
		/* write out next run if one was found */
		if (run_count >= MINRUNLENGTH) {
			buf[0] = 128 + run_count;
			buf[1] = data[beg_run];
			if (image->io.write(buf,sizeof(buf[0])*2,1, &image->io) < 1)
				return IL_FALSE;
			cur += run_count;
		}
	}
	return IL_TRUE;
#undef MINRUNLENGTH
}


// Internal function used to save the Hdr.
ILboolean iSaveHdrInternal(ILimage* image)
{
	ILimage *TempImage;
	rgbe_header_info stHeader;
	unsigned char rgbe[4];
	ILubyte		*buffer;
	ILfloat		*data;
	ILuint		i;
	ILboolean	bRet;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	stHeader.exposure = 0;
	stHeader.gamma = 0;
	stHeader.programtype[0] = 0;
	stHeader.valid = 0;

	if (image->Format != IL_UNSIGNED_BYTE) {
		TempImage = iConvertImage(image, IL_RGB, IL_FLOAT);
		if (TempImage == NULL)
			return IL_FALSE;
	}
	else
		TempImage = image;

	if (!RGBE_WriteHeader(TempImage->Width, TempImage->Height, &stHeader))
		return IL_FALSE;

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT)
		iFlipBuffer(TempImage->Data, TempImage->Depth, TempImage->Bps, TempImage->Height);
	data = (ILfloat*)TempImage->Data;

	if ((TempImage->Width < 8)||(TempImage->Width > 0x7fff)) {
		/* run length encoding is not allowed so write flat*/
		bRet = RGBE_WritePixels(image, data,TempImage->Width*TempImage->Height);
		if (image != TempImage)
			ilCloseImage(TempImage);
		return bRet;
	}
	buffer = (ILubyte*)ialloc(sizeof(ILubyte)*4*TempImage->Width);
	if (buffer == NULL) {
		/* no buffer space so write flat */
		bRet = RGBE_WritePixels(image, data,TempImage->Width*TempImage->Height);
		if (image != TempImage)
			ilCloseImage(TempImage);
		return bRet;
	}

	while(TempImage->Height-- > 0) {
		rgbe[0] = 2;
		rgbe[1] = 2;
		rgbe[2] = TempImage->Width >> 8;
		rgbe[3] = TempImage->Width & 0xFF;
		if (image->io.write(rgbe, sizeof(rgbe), 1, &image->io) < 1) {
			free(buffer);
			if (image != TempImage)
				ilCloseImage(TempImage);
			return IL_FALSE;
		}

		for(i=0;i<TempImage->Width;i++) {
			float2rgbe(rgbe,data[RGBE_DATA_RED],data[RGBE_DATA_GREEN],data[RGBE_DATA_BLUE]);
			buffer[i] = rgbe[0];
			buffer[i+TempImage->Width] = rgbe[1];
			buffer[i+2*TempImage->Width] = rgbe[2];
			buffer[i+3*TempImage->Width] = rgbe[3];
			data += RGBE_DATA_SIZE;
		}
		/* write out each of the four channels separately run length encoded */
		/* first red, then green, then blue, then exponent */
		for(i=0;i<4;i++) {
			if (RGBE_WriteBytes_RLE(image, &buffer[i*TempImage->Width],TempImage->Width) != IL_TRUE) {
				ifree(buffer);
				if (image != TempImage)
					ilCloseImage(TempImage);
				return IL_FALSE;
			}
		}
	}
	ifree(buffer);

	if (image != TempImage)
		ilCloseImage(TempImage);
	return IL_TRUE;
}


#endif//IL_NO_HDR
