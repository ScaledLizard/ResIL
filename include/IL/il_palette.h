//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Written by Björn Ganster in 2014
//
// Description: class for palette access
//
//-----------------------------------------------------------------------------

#ifndef IL_PALETTE
#define IL_PALETTE

#include <string.h>

class ILpal
{
private:
	ILubyte* mPalette; //!< the image palette (if any)
	ILuint   mPalSize; //!< size of the palette (in bytes)
	ILenum   mPalType; //!< the palette types in il.h (0x0500 range)
	ILuint   mNumCols; //!< number of colors in the palette, 
	ILbyte  mNumComponents, mRedOffset, mGreenOffset, mBlueOffset, mAlphaOffset; // negative values indicate unused components
public:
	// Construct empty palette
	ILpal ()
		: mPalette (NULL),
		  mPalSize (0),
		  mPalType (IL_PAL_NONE),
		  mNumCols (0),
		  mNumComponents (0),
		  mRedOffset (0), 
		  mGreenOffset (0), 
		  mBlueOffset (0), 
		  mAlphaOffset (0)
	{
	}

	// Copy palette
	ILpal (const ILpal& other)
		: mPalette (NULL),
		  mPalSize (0),
		  mPalType (IL_PAL_NONE),
		  mNumCols (0),
		  mNumComponents (0),
		  mRedOffset (0), 
		  mGreenOffset (0), 
		  mBlueOffset (0), 
		  mAlphaOffset (0)
	{
		operator=(other);
	}

	// Destructor
	virtual ~ILpal()
	{
		delete mPalette;
		mPalette = NULL; // Prevent use after free
		mPalSize = 0;
	}

	// Use palette
	ILboolean use (ILint aNumCols, ILubyte* aPal, ILenum aPalType)
	{
		if (mPalette != NULL) {
			ifree(mPalette);
			mPalette = NULL;
		}

		switch(aPalType) {
		case IL_PAL_RGB24:
			mNumComponents = 3;
			mRedOffset = 0;
			mGreenOffset = 1;
			mBlueOffset = 2;
			mAlphaOffset = -1;
			break;
		case IL_PAL_RGB32:
			mNumComponents = 4;
			mRedOffset = 0;
			mGreenOffset = 1;
			mBlueOffset = 2;
			mAlphaOffset = -1;
			break;
		case IL_PAL_RGBA32:
			mNumComponents = 4;
			mRedOffset = 0;
			mGreenOffset = 1;
			mBlueOffset = 2;
			mAlphaOffset = 3;
			break;
		case IL_PAL_BGR24:
			mNumComponents = 3;
			mRedOffset = 2;
			mGreenOffset = 1;
			mBlueOffset = 0;
			mAlphaOffset = -1;
			break;
		case IL_PAL_BGR32:
			mNumComponents = 4;
			mRedOffset = 2;
			mGreenOffset = 1;
			mBlueOffset = 0;
			mAlphaOffset = -1;
			break;
		case IL_PAL_BGRA32:
			mNumComponents = 4;
			mRedOffset = 3;
			mGreenOffset = 2;
			mBlueOffset = 1;
			mAlphaOffset = 0;
			break;
		case IL_PAL_NONE:
		default:
			mNumComponents = 0;
			mRedOffset = 0;
			mGreenOffset = 0;
			mBlueOffset = 0;
			mAlphaOffset = 0;
			break;
		}

		if (mNumComponents > 0 && aNumCols > 0) {
			mNumCols = aNumCols;
			mPalSize = aNumCols * mNumComponents;
			mPalette = (ILubyte*)ialloc(mPalSize);
			if (mPalette != NULL) {
				if (aPal != NULL)
					memcpy(mPalette, aPal, mPalSize);
				else
					memset(mPalette, 0, mPalSize);
				mPalType = aPalType;
				return IL_TRUE;
			} else
				return IL_FALSE;
		} else
			return IL_TRUE;
	}

	inline ILboolean setRGBA(ILuint index, ILubyte red, ILubyte green, ILubyte blue, ILubyte alpha)
	{
		index *= mNumComponents;
		if (index + mNumComponents < mPalSize 
		&&  mPalette != NULL) 
		{
			mPalette[index + mRedOffset] = red;
			mPalette[index + mGreenOffset] = green;
			mPalette[index + mBlueOffset] = blue;
			if (mAlphaOffset >= 0)
				mPalette[index + mAlphaOffset] = alpha;
			return IL_TRUE;
		} else
			return IL_FALSE;
	}

	inline ILboolean setRGB(ILuint index, ILubyte red, ILubyte green, ILubyte blue)
	{
		index *= mNumComponents;
		if (index + mNumComponents < mPalSize && mPalette != NULL) {
			mPalette[index + mRedOffset] = red;
			mPalette[index + mGreenOffset] = green;
			mPalette[index + mBlueOffset] = blue;
			return IL_TRUE;
		} else
			return IL_FALSE;
	}

	inline ILboolean getRGBA(ILuint index, ILubyte& red, ILubyte& green, ILubyte& blue, ILubyte& alpha)
	{
		index *= mNumComponents;
		if (index + mNumComponents < mPalSize 
		&&  mPalette != NULL) 
		{
			red   = mPalette[index + mRedOffset];
			green = mPalette[index + mGreenOffset];
			blue  = mPalette[index + mBlueOffset];
			if (mAlphaOffset >= 0) 
				alpha = mPalette[index + mAlphaOffset];
			return IL_TRUE;
		} else
			return IL_FALSE;
	}

	inline ILboolean getRGB(ILuint index, ILubyte& red, ILubyte& green, ILubyte& blue)
	{
		index *= mNumComponents;
		if (index + mNumComponents < mPalSize && mPalette != NULL) {
			red   = mPalette[index + mRedOffset];
			green = mPalette[index + mGreenOffset];
			blue  = mPalette[index + mBlueOffset];
			return IL_TRUE;
		} else
			return IL_FALSE;
	}

	inline void clear()
	{
		use(0, NULL, IL_PAL_NONE);
	}

	inline ILenum getPalType() const
	{
		return mPalType;
	}

	inline ILuint getPalSize() const
	{
		return mPalSize;
	}

	inline ILboolean readFromFile(SIO* io)
	{
		auto read = io->read(io, mPalette, 1, mPalSize);
		if (read == mPalSize)
			return IL_TRUE;
		else
			return IL_FALSE;
	}

	inline ILboolean writeToFile(SIO* io)
	{
		auto written = io->write(mPalette, sizeof(ILubyte), mPalSize, io);
		if (written == mPalSize)
			return IL_TRUE;
		else
			return IL_FALSE;
	}

	inline ILboolean hasPalette() const
	{
		if (mPalette != NULL && mPalSize > 0 && mPalType != IL_PAL_NONE)
			return IL_TRUE;
		else
			return IL_FALSE;
	}

	void operator=(const ILpal& other)
	{
		if (other.hasPalette()) {
			use(other.mNumCols, other.mPalette, other.mPalType);
		} else
			clear();
	}

	ILuint getNumCols() const
	{
		return mNumCols;
	}

	// Return a pointer to the internal palette - USE CAUTIOUSLY!
	// The returned pointer should not be used to manipulate the palette
	ILubyte* getPalette()
	{
		return mPalette;
	}
};

#endif