//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 2014 by Björn Ganster
//
// Filename: src-IL/src/il_gif.c
//
// Description: Reads from a Graphics Interchange Format (.gif) file.
//
//  The LZW decompression code is based on code released to the public domain
//    by Javier Arevalo and can be found at
//    http://www.programmersheaven.com/zone10/cat452
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_GIF

#include "il_gif.h"
#include <stdio.h>
#include "IL/il2.h"

#define GIF87A 87
#define GIF89A 89

//-----------------------------------------------------------------------------
// ILimage offers all the members needed to reflect a GIF

// Set minimum image size for several images
ILboolean setMinSizes(ILuint w, ILuint h, ILimage* img)
{
   struct ILimage* currImg = img;
   int resizeNeeded = IL_FALSE;

   if (img == NULL)
      return IL_FALSE;

   while (currImg != NULL) {
      if (currImg->Width > w)
         w = currImg->Width;
      else if (w > currImg->Width)
         resizeNeeded = IL_TRUE;
      if (currImg->Height > h)
         h = currImg->Height;
      else if (h > currImg->Height)
         resizeNeeded = IL_TRUE;
      currImg = currImg->Next;
   }

   if (resizeNeeded) {
		ILboolean success = IL_TRUE;
		struct ILimage* currImg = img;
		while (currImg != NULL && success) {
			success &= il2ResizeImage(currImg, w, h, 1, 1, 1);
			currImg = currImg->Next;
		}
		return success;
   } else
		return IL_TRUE;
}

//-----------------------------------------------------------------------------


// Internal function to get the header and check it.
ILboolean iIsValidGif(SIO* io)
{
	char Header[6];
	ILint64 read = io->read(io, Header, 1, 6);
	io->seek(io, -read, IL_SEEK_CUR);

	if (read != 6)
		return IL_FALSE;
	if (!strnicmp(Header, "GIF87A", 6))
		return IL_TRUE;
	if (!strnicmp(Header, "GIF89A", 6))
		return IL_TRUE;

	return IL_FALSE;
}


// If a frame has a local palette, the global palette is not used by this frame
ILboolean iGetPalette(SIO* io, ILubyte Info, ILpal *Pal, ILimage *PrevImage)
{
	// The ld(palettes bpp - 1) is stored in the lower 3 bits of Info
	ILint palBitCount = (Info & 0x7) + 1;
	if (!Pal->use(1 << palBitCount, NULL, IL_PAL_RGB24)) 
		return IL_FALSE;

	// Read the new palette
	return Pal->readFromFile(io);
}


#define MAX_CODES 4096
class GifDecompressor {
public:
   ILint	curr_size, clear, ending, newcodes, top_slot, slot, navail_bytes, nbits_left;
   ILubyte	b1;
	static const ILuint byte_buff_size = 257;
	ILimage* image;
	SIO* io;

   // Default constructor
   GifDecompressor() 
   {
		mStackIndex = 0;
		mBufIndex = 0;
		mValidState = true;
   }

   // Destructor
   ~GifDecompressor()
   {
   }

	inline void setByteBuff(ILuint index, ILubyte value)
	{
		if (index < byte_buff_size)
			mByteBuff[index] = value;
		else
			mValidState = false;
	}

	inline ILubyte getByteBuff(ILuint index)
	{
		if (index < byte_buff_size) {
			return mByteBuff[index];
		} else {
			mValidState = false;
			return 0;
		}
	}

	inline void push(ILubyte value)
	{
		if (mStackIndex <= MAX_CODES) {
			mStack[mStackIndex] = value;
			++mStackIndex;
		} else
			mValidState = false;
	}

	inline ILboolean stackIndexLegal() 
	{
		if (mStackIndex >= MAX_CODES)
			return IL_FALSE;
		else
			return IL_TRUE;
	}

	inline ILubyte getStack()
	{
		if (mStackIndex <= MAX_CODES) {
			return mStack[mStackIndex];
		} else {
			mValidState = false;
			return 0;
		}
	}

	inline void pop() 
	{
		if (mStackIndex > 0)
			mStackIndex--;
		else
			mValidState = false;
	}

	inline ILuint getStackIndex() const
	{
		return mStackIndex;
	}

	inline ILubyte getSuffix(ILint code)
	{
		if (code < MAX_CODES) {
			return mSuffix[code];
		} else {
			mValidState = false;
			return 0;
		}
	}

	inline void setSuffix(ILubyte code)
	{
		if (slot < MAX_CODES)
			mSuffix[slot] = code;
		else
			mValidState = false;
	}

	inline void setPrefix(ILshort code)
	{
		if (slot < MAX_CODES)
				mPrefix[slot++] = code;
		else
			mValidState = false;
	}

	inline ILshort getPrefix(ILint code)
	{
		if (code >= 0 && code <= MAX_CODES) {
			return mPrefix[code];
		} else {
			mValidState = false;
			return 0;
		}
	}

	inline void resetBufIndex()
	{ mBufIndex = 0; }

	inline ILubyte nextInBuffer()
	{
		if (mBufIndex < byte_buff_size) {
			ILubyte val = mByteBuff[mBufIndex];
			++mBufIndex;
			return val;
		} else 
			return 0;
	}

	inline ILboolean isValid() const
	{ return mValidState; }

private:
   ILuint mBufIndex;
   ILubyte	mByteBuff[byte_buff_size];
   ILuint mStackIndex;
   ILubyte	mStack[MAX_CODES + 1];
   ILubyte	mSuffix[MAX_CODES + 1];
   ILshort	mPrefix[MAX_CODES + 1];
	ILboolean mValidState;
};

ILuint code_mask[13] =
{
   0L,
   0x0001L, 0x0003L,
   0x0007L, 0x000FL,
   0x001FL, 0x003FL,
   0x007FL, 0x00FFL,
   0x01FFL, 0x03FFL,
   0x07FFL, 0x0FFFL
};



ILint get_next_code(GifDecompressor* state) {
	ILint	i, t;
	ILuint	ret;

	if (state->nbits_left == 0) {
		if (state->navail_bytes <= 0) {
			state->resetBufIndex();
			state->navail_bytes = state->io->getc(state->io);

			if(state->navail_bytes == IL_EOF) {
				return state->ending;
			}

			if (state->navail_bytes) {
				for (i = 0; i < state->navail_bytes; i++) {
					if((t = state->io->getc(state->io)) == IL_EOF) {
						return state->ending;
					}
					state->setByteBuff(i, t);
				}
			}
		}
		state->b1 = state->nextInBuffer();
		state->nbits_left = 8;
		state->navail_bytes--;
	}

	ret = state->b1 >> (8 - state->nbits_left);
	while (state->curr_size >state-> nbits_left) {
		if (state->navail_bytes <= 0) {
			state->resetBufIndex();
			state->navail_bytes = state->io->getc(state->io);

			if(state->navail_bytes == IL_EOF) {
				return state->ending;
			}

			if (state->navail_bytes) {
				for (i = 0; i < state->navail_bytes; i++) {
					if((t = state->io->getc(state->io)) == IL_EOF) {
						return state->ending;
					}
					state->setByteBuff(i, t);
				}
			}
		}
		state->b1 = state->nextInBuffer();
		ret |= state->b1 << state->nbits_left;
		state->nbits_left += 8;
		state->navail_bytes--;
	}
	state->nbits_left -= state->curr_size;

	return (ret & code_mask[state->curr_size]);
}

class GifLoader {
public:
	ILimage* mBaseImage;
	ILimage* mCurrFrame;
	ILimage* mPrevImage;
	GIFHEAD mHeader;
	SIO* mIo;
	GFXCONTROL gfx;
	ILboolean gfxUsed;
	ILint NumImages;
	ILint		input;
	ILpal	palette, GlobalPal;
	IMAGEDESC	ImageDesc, OldImageDesc;

	GifLoader();
	ILboolean GifGetData(ILuint Stride);
	ILboolean readExtensionBlock();
	ILboolean readImage();
	ILboolean GetImages();
};

GifLoader::GifLoader()
	: mBaseImage (NULL),
	  mCurrFrame (NULL),
	  mPrevImage (NULL),
	  mIo (NULL),
	  gfxUsed (false),
	  NumImages (0),
	  input (0),
	  palette (), 
	  GlobalPal ()
{
}

ILboolean GifLoader::GifGetData(ILuint Stride)
{
	ILimage* image = mCurrFrame;
	SIO * io = &image->io;
	ILubyte *Data = mCurrFrame->Data;

	GifDecompressor state;
	ILint	code, fc, oc;
	ILubyte	DisposalMethod = 0;
	ILint	c, size;
	ILuint	x = ImageDesc.OffX, Read = 0, y = ImageDesc.OffY;
	ILuint dataOffset = y * Stride + x;

	state.navail_bytes = 0;
	state.nbits_left = 0;
	state.io = io;

	if (!gfxUsed)
		DisposalMethod = (gfx.Packed & 0x1C) >> 2;
	if((size = state.io->getc(state.io)) == IL_EOF)
		return IL_FALSE;

	if (size < 2 || 9 < size) {
		return IL_FALSE;
	}

	state.curr_size = size + 1;
	state.top_slot = 1 << state.curr_size;
	state.clear = 1 << size;
	state.ending = state.clear + 1;
	state.slot = state.newcodes = state.ending + 1;
	state.navail_bytes = state.nbits_left = 0;
	oc = fc = 0;

	while ((c = get_next_code(&state)) != state.ending 
	&&     Read < ImageDesc.Height
	&& state.isValid()) 
	{
		if (c == state.clear)
		{
			state.curr_size = size + 1;
			state.slot = state.newcodes;
			state.top_slot = 1 << state.curr_size;
			while ((c = get_next_code(&state)) == state.clear);
			if (c == state.ending)
				break;
			if (c >= state.slot)
				c = 0;
			oc = fc = c;

			if (DisposalMethod == 1 
			&& !gfxUsed 
			&& gfx.Transparent == c 
			&& (gfx.Packed & 0x1) != 0)
			{
				x++;
			} else if (x < ImageDesc.Width) {
				Data[dataOffset+x] = c;
            ++x;
         }

			if (x >= ImageDesc.Width)
			{
				//DataPtr += Stride;
				x = 0;
				Read += 1;
				++y;
            dataOffset = y * Stride +  ImageDesc.OffX;
				if (y >= ImageDesc.Height) {
				   return IL_FALSE;
				}
			}
		}
		else
		{
			code = c;
         //BG-2007-01-10: several fixes for incomplete GIFs
			if (code >= state.slot)
			{
				code = oc;
				if (!state.stackIndexLegal()) {
					return IL_FALSE;
				}
				state.push(fc);
			}

			if (code >= MAX_CODES)
				return IL_FALSE; 
			while (code >= state.newcodes)
			{
				if (!state.stackIndexLegal()) {
					return IL_FALSE;
				}
				state.push(state.getSuffix(code));
				code = state.getPrefix(code);
			}
            
			if (!state.stackIndexLegal()) {
				return IL_FALSE;
			}

			state.push((ILbyte)code);
			if (state.slot < state.top_slot)
			{
				fc = code;
				state.setSuffix(fc);
				state.setPrefix(oc);
				oc = c;
			}
			if (state.slot >= state.top_slot && state.curr_size < 12)
			{
				state.top_slot <<= 1;
				state.curr_size++;
			}
			while (state.getStackIndex() > 0)
			{
				state.pop();
				if (DisposalMethod == 1 
					 && !gfxUsed 
					 && gfx.Transparent == state.getStack()
					 && (gfx.Packed & 0x1) != 0)
            {
					x++;
            } else if (x < ImageDesc.Width) {
					Data[dataOffset+x] = state.getStack();
               x++;
            }

				if (x >= ImageDesc.Width) // end of line
				{
					x = ImageDesc.OffX % ImageDesc.Width;
					Read += 1;
               y = (y+1) % ImageDesc.Height;
					// Needs to start from Data, not Image->Data.
					dataOffset = y * Stride +  ImageDesc.OffX;
				}
			}
		}
	}

	return state.isValid();
}

ILboolean GifLoader::readExtensionBlock()
{
	SIO * io = mIo;
	GFXCONTROL temp;
	auto read = io->read(io, &temp, 1, sizeof(GFXCONTROL));
	if (read == sizeof(GFXCONTROL) 
	&&  temp.ExtensionBlockSignature == 0x21
	&&  temp.GraphicsControlLabel == 0xF9)
	{
		memcpy(&gfx, &temp, sizeof(GFXCONTROL));
		return IL_TRUE;
	} else {
		io->seek(io, -read, IL_SEEK_CUR);
		return IL_FALSE;
	}
}

ILboolean RemoveInterlace(ILimage *image)
{
	ILubyte *NewData;
	ILuint	i, j = 0;

	NewData = (ILubyte*)ialloc(image->SizeOfData);
	if (NewData == NULL)
		return IL_FALSE;

	//changed 20041230: images with offsety != 0 were not
	//deinterlaced correctly before...
	for (i = 0; i < image->OffY; i++, j++) {
		memcpy(&NewData[i * image->Bps], &image->Data[j * image->Bps], image->Bps);
	}

	for (i = 0 + image->OffY; i < image->Height; i += 8, j++) {
		memcpy(&NewData[i * image->Bps], &image->Data[j * image->Bps], image->Bps);
	}

	for (i = 4 + image->OffY; i < image->Height; i += 8, j++) {
		memcpy(&NewData[i * image->Bps], &image->Data[j * image->Bps], image->Bps);
	}

	for (i = 2 + image->OffY; i < image->Height; i += 4, j++) {
		memcpy(&NewData[i * image->Bps], &image->Data[j * image->Bps], image->Bps);
	}

	for (i = 1 + image->OffY; i < image->Height; i += 2, j++) {
		memcpy(&NewData[i * image->Bps], &image->Data[j * image->Bps], image->Bps);
	}

	ifree(image->Data);
	image->Data = NewData;

	return IL_TRUE;
}


// Uses the transparent colour index to make an alpha channel.
ILboolean ConvertTransparent(ILimage *Image, ILubyte TransColour)
{
	if (!Image->Pal.hasPalette())
	{
		return IL_FALSE;
	}

	ILuint colorCount = Image->Pal.getNumCols();
	ILuint newSize = colorCount * 4;
	ILubyte	*tempPal = (ILubyte*)ialloc(newSize);
	if (tempPal == NULL)
		return IL_FALSE;

	// Copy all colors as opaque
	for (ILuint i = 0, j = 0; i < colorCount; ++i, j += 4) {
		ILubyte r, g, b;
		Image->Pal.getRGB(i, r, g, b);
		tempPal[j  ] = r;
		tempPal[j+1] = g;
		tempPal[j+2] = b;
		tempPal[j+3] = 0xFF;
	}

	// Store transparent color if TransColour is a legal index
	ILuint TransColourIndex = 4*TransColour+3;
	if (TransColourIndex < newSize)
		tempPal[TransColourIndex] = 0x00;

	Image->Pal.use(colorCount, tempPal, IL_PAL_RGBA32);

	return IL_TRUE;
}

ILboolean GifLoader::readImage()
{
	ILubyte DisposalMethod = 1;
	SIO * io = mIo;

	if (!gfxUsed)
		DisposalMethod = (gfx.Packed & 0x1C) >> 2;

	//read image descriptor
   auto read = io->read(io, &ImageDesc, 1, sizeof(IMAGEDESC));
   #ifdef __BIG_ENDIAN__
   iSwapUShort(ImageDesc.OffX);
   iSwapUShort(ImageDesc.OffY);
   iSwapUShort(ImageDesc.Width);
   iSwapUShort(ImageDesc.Height);
   #endif
      
	if (read != sizeof(IMAGEDESC))
		return IL_FALSE;

   if (!setMinSizes(ImageDesc.OffX + ImageDesc.Width, 
		ImageDesc.OffY + ImageDesc.Height, mCurrFrame)) 
	{
		mCurrFrame = mCurrFrame->Next;
		return IL_FALSE;
	}

	if (io->eof(io)) {
		il2GetError();  // Gets rid of the IL_FILE_READ_ERROR that inevitably results.
		return IL_FALSE;
	}

	if (mCurrFrame != mBaseImage) {
		NumImages++;
		mCurrFrame->Next = il2NewImage(mCurrFrame->Width, mCurrFrame->Height, 1, 1, 1);
		if (mCurrFrame->Next == NULL) {
			mCurrFrame = mCurrFrame->Next;
			return IL_FALSE;
		}
		//20040612: DisposalMethod controls how the new images data is to be combined
		//with the old image. 0 means that it doesn't matter how they are combined,
		//1 means keep the old image, 2 means set to background color, 3 is
		//load the image that was in place before the current (this is not implemented
		//here! (TODO?))
		if (DisposalMethod == 2 || DisposalMethod == 3)
			//Note that this is actually wrong, too: If the image has a local
			//color table, we should really search for the best fit of the
			//background color table and use that index (?). Furthermore,
			//we should only memset the part of the image that is not read
			//later (if we are sure that no parts of the read image are transparent).
			if (!gfxUsed && gfx.Packed & 0x1)
				memset(mCurrFrame->Next->Data, gfx.Transparent, mCurrFrame->SizeOfData);
			else
				memset(mCurrFrame->Next->Data, mHeader.Background, 
					mCurrFrame->SizeOfData);
		else if (DisposalMethod == 1 || DisposalMethod == 0)
			memcpy(mCurrFrame->Next->Data, mCurrFrame->Data, mCurrFrame->SizeOfData);
		//Interlacing has to be removed after the image was copied (line above)
		if (OldImageDesc.ImageInfo & (1 << 6)) {  // Image is interlaced.
			if (!RemoveInterlace(mCurrFrame)) {
				mCurrFrame = mCurrFrame->Next;
				return IL_FALSE;
			}
		}

		mPrevImage = mCurrFrame;
		mCurrFrame = mCurrFrame->Next;
		mCurrFrame->Format = IL_COLOUR_INDEX;
		mCurrFrame->Origin = IL_ORIGIN_UPPER_LEFT;
	} else {
		if (!gfxUsed && gfx.Packed & 0x1)
			memset(mCurrFrame->Data, gfx.Transparent, mCurrFrame->SizeOfData);
		else
			memset(mCurrFrame->Data, mHeader.Background, mCurrFrame->SizeOfData);
	}

	mCurrFrame->OffX = ImageDesc.OffX;
	mCurrFrame->OffY = ImageDesc.OffY;

	// Check to see if the image has its own palette
	if (ImageDesc.ImageInfo & (1 << 7)) {
		if (!iGetPalette(io, ImageDesc.ImageInfo, &mCurrFrame->Pal, mPrevImage)) {
			mCurrFrame = mCurrFrame->Next;
			return IL_FALSE;
		}
	} else {
		if (!iCopyPalette(&mCurrFrame->Pal, &GlobalPal)) {
			mCurrFrame = mCurrFrame->Next;
			return IL_FALSE;
		}
	}

	if (!GifGetData(mCurrFrame->Bps)) {
		il2SetError(IL_ILLEGAL_FILE_VALUE);
		mCurrFrame = mCurrFrame->Next;
		return IL_FALSE;
	}

	// Add graphics control extension if it is missing
	if (!gfxUsed) {
		gfxUsed = IL_TRUE;
		mCurrFrame->Duration = gfx.Delay * 10;  // We want it in milliseconds.

		// See if a transparent colour is defined.
		if (gfx.Packed & 1) {
			if (!ConvertTransparent(mCurrFrame, gfx.Transparent)) {
				mCurrFrame = mCurrFrame->Next;
				return IL_FALSE;
			}
	   }
	}

	// Terminates each block.
	if((input = io->getc(io)) == IL_EOF) {
		mCurrFrame = mCurrFrame->Next;
		return IL_FALSE;
	}

	if (input != 0x00)
		   io->seek(io, -1, IL_SEEK_CUR);

	OldImageDesc = ImageDesc;
	return IL_TRUE;
}

ILboolean GifLoader::GetImages()
{
	OldImageDesc.ImageInfo = 0; // to initialize the data with an harmless value 
	ILboolean done = IL_FALSE;
	SIO * io = mIo;
	ILboolean success = IL_TRUE;

	while (!io->eof(io) && success && !done) {
		ILubyte controlByte;
		auto read = io->read(io, &controlByte, 1, 1);
		if (read != 1) {
			mCurrFrame = mCurrFrame->Next;
			return IL_FALSE;
		}
		io->seek(io, -1, IL_SEEK_CUR);
		
		switch(controlByte) {
			case 0x21:
				success = readExtensionBlock();
				break;
			case 0x2c:
				success = readImage();
				break;
			case 0x3b:
				done = true;
				break;
		}
	}

	//Deinterlace last image
	if (OldImageDesc.ImageInfo & (1 << 6)) {  // Image is interlaced.
		if (!RemoveInterlace(mCurrFrame)) {
			mCurrFrame = mCurrFrame->Next;
			return IL_FALSE;
		}
	}

	if (mBaseImage == NULL)  // Was not able to load any images in...
		return IL_FALSE;
	else
		return success;
}

// Internal function used to load the Gif.
ILboolean iLoadGifInternal(ILimage* image)
{
	GifLoader loader;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	loader.GlobalPal.clear();

	// Read header
	auto read = image->io.read(&image->io, &loader.mHeader, 1, sizeof(GIFHEAD));
	#ifdef __BIG_ENDIAN__
	iSwapUShort(Header.Width);
	iSwapUShort(Header.Height);
	#endif

	if (  (    strnicmp(loader.mHeader.Sig, "GIF87A", 6) != 0 
	       &&  strnicmp(loader.mHeader.Sig, "GIF89A", 6) != 0) 
	    || read != sizeof(GIFHEAD))
	{
		il2SetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!il2TexImage(image, loader.mHeader.Width, loader.mHeader.Height, 1, 1, 
		IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL))
	{
		return IL_FALSE;
	}
	image->Origin = IL_ORIGIN_UPPER_LEFT;

	// Check for a global colour map.
	if (loader.mHeader.ColourInfo & (1 << 7)) {
		if (!iGetPalette(&image->io, loader.mHeader.ColourInfo, &loader.GlobalPal, NULL)) {
			return IL_FALSE;
		}
	}

	loader.mIo = &image->io;
	loader.mBaseImage = image;
	loader.mCurrFrame = image;
	loader.mPrevImage = NULL;
	loader.gfxUsed = IL_FALSE;
	loader.NumImages = 0;
	loader.input = 0;
	loader.palette = loader.GlobalPal;
	if (!loader.GetImages())
		return IL_FALSE;

	loader.palette.clear();

	return il2FixImage(image);
}


/*From the GIF spec:

  The rows of an Interlaced images are arranged in the following order:

      Group 1 : Every 8th. row, starting with row 0.              (Pass 1)
      Group 2 : Every 8th. row, starting with row 4.              (Pass 2)
      Group 3 : Every 4th. row, starting with row 2.              (Pass 3)
      Group 4 : Every 2nd. row, starting with row 1.              (Pass 4)
*/

#endif //IL_NO_GIF
