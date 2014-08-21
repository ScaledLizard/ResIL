//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 2014 by Björn Ganster
//
// Filename: src-IL/src/il_io.cpp
//
// Description: Determines image types and loads/saves images
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#include "il_register.h"
#include "il_pal.h"
//#include "IL/il.h"
#include <string.h>


// Returns a widened version of a string.
// Make sure to free this after it is used.
#if defined(_UNICODE)
wchar_t *WideFromMultiByte(const char *Multi)
{
	ILint	Length;
	wchar_t	*Temp;

	Length = (ILint)mbstowcs(NULL, (const char*)Multi, 0) + 1; // note error return of -1 is possible
	if (Length == 0) {
		il2SetError(IL_INVALID_PARAM);
		return NULL;
	}
	if (Length > ULONG_MAX/sizeof(wchar_t)) {
		il2SetError(IL_INTERNAL_ERROR);
		return NULL;
	}
	Temp = (wchar_t*)ialloc(Length * sizeof(wchar_t));
	mbstowcs(Temp, (const char*)Multi, Length); 

	return Temp;
}
#endif


ILenum ILAPIENTRY ilTypeFromExt(ILconst_string FileName)
{
	ILenum		Type;
	ILstring	Ext;

	if (FileName == NULL || ilStrLen(FileName) < 1) {
		il2SetError(IL_INVALID_PARAM);
		return IL_TYPE_UNKNOWN;
	}

	Ext = iGetExtension(FileName);
	//added 2003-08-31: fix sf bug 789535
	if (Ext == NULL) {
		return IL_TYPE_UNKNOWN;
	}

	if (!iStrCmp(Ext, IL_TEXT("tga")) || !iStrCmp(Ext, IL_TEXT("vda")) ||
		!iStrCmp(Ext, IL_TEXT("icb")) || !iStrCmp(Ext, IL_TEXT("vst")))
		Type = IL_TGA;
	else if (!iStrCmp(Ext, IL_TEXT("jpg")) || !iStrCmp(Ext, IL_TEXT("jpe")) ||
		!iStrCmp(Ext, IL_TEXT("jpeg")) || !iStrCmp(Ext, IL_TEXT("jif")) || !iStrCmp(Ext, IL_TEXT("jfif")))
		Type = IL_JPG;
	else if (!iStrCmp(Ext, IL_TEXT("jp2")) || !iStrCmp(Ext, IL_TEXT("jpx")) ||
		!iStrCmp(Ext, IL_TEXT("j2k")) || !iStrCmp(Ext, IL_TEXT("j2c")))
		Type = IL_JP2;
	else if (!iStrCmp(Ext, IL_TEXT("dds")))
		Type = IL_DDS;
	else if (!iStrCmp(Ext, IL_TEXT("png")))
		Type = IL_PNG;
	else if (!iStrCmp(Ext, IL_TEXT("bmp")) || !iStrCmp(Ext, IL_TEXT("dib")))
		Type = IL_BMP;
	else if (!iStrCmp(Ext, IL_TEXT("gif")))
		Type = IL_GIF;
	else if (!iStrCmp(Ext, IL_TEXT("blp")))
		Type = IL_BLP;
	else if (!iStrCmp(Ext, IL_TEXT("cut")))
		Type = IL_CUT;
	else if (!iStrCmp(Ext, IL_TEXT("dcm")) || !iStrCmp(Ext, IL_TEXT("dicom")))
		Type = IL_DICOM;
	else if (!iStrCmp(Ext, IL_TEXT("dpx")))
		Type = IL_DPX;
	else if (!iStrCmp(Ext, IL_TEXT("exr")))
		Type = IL_EXR;
	else if (!iStrCmp(Ext, IL_TEXT("fit")) || !iStrCmp(Ext, IL_TEXT("fits")))
		Type = IL_FITS;
	else if (!iStrCmp(Ext, IL_TEXT("ftx")))
		Type = IL_FTX;
	else if (!iStrCmp(Ext, IL_TEXT("hdr")))
		Type = IL_HDR;
	else if (!iStrCmp(Ext, IL_TEXT("iff")))
		Type = IL_IFF;
	else if (!iStrCmp(Ext, IL_TEXT("ilbm")) || !iStrCmp(Ext, IL_TEXT("lbm")) ||
        !iStrCmp(Ext, IL_TEXT("ham")))
		Type = IL_ILBM;
	else if (!iStrCmp(Ext, IL_TEXT("ico")) || !iStrCmp(Ext, IL_TEXT("cur")))
		Type = IL_ICO;
	else if (!iStrCmp(Ext, IL_TEXT("icns")))
		Type = IL_ICNS;
	else if (!iStrCmp(Ext, IL_TEXT("iwi")))
		Type = IL_IWI;
	else if (!iStrCmp(Ext, IL_TEXT("iwi")))
		Type = IL_IWI;
	else if (!iStrCmp(Ext, IL_TEXT("jng")))
		Type = IL_JNG;
	else if (!iStrCmp(Ext, IL_TEXT("lif")))
		Type = IL_LIF;
	else if (!iStrCmp(Ext, IL_TEXT("mdl")))
		Type = IL_MDL;
	else if (!iStrCmp(Ext, IL_TEXT("mng")) || !iStrCmp(Ext, IL_TEXT("jng")))
		Type = IL_MNG;
	else if (!iStrCmp(Ext, IL_TEXT("mp3")))
		Type = IL_MP3;
	else if (!iStrCmp(Ext, IL_TEXT("pcd")))
		Type = IL_PCD;
	else if (!iStrCmp(Ext, IL_TEXT("pcx")))
		Type = IL_PCX;
	else if (!iStrCmp(Ext, IL_TEXT("pic")))
		Type = IL_PIC;
	else if (!iStrCmp(Ext, IL_TEXT("pix")))
		Type = IL_PIX;
	else if (!iStrCmp(Ext, IL_TEXT("pbm")) || !iStrCmp(Ext, IL_TEXT("pgm")) ||
		!iStrCmp(Ext, IL_TEXT("pnm")) || !iStrCmp(Ext, IL_TEXT("ppm")))
		Type = IL_PNM;
	else if (!iStrCmp(Ext, IL_TEXT("psd")) || !iStrCmp(Ext, IL_TEXT("pdd")))
		Type = IL_PSD;
	else if (!iStrCmp(Ext, IL_TEXT("psp")))
		Type = IL_PSP;
	else if (!iStrCmp(Ext, IL_TEXT("pxr")))
		Type = IL_PXR;
	else if (!iStrCmp(Ext, IL_TEXT("rot")))
		Type = IL_ROT;
	else if (!iStrCmp(Ext, IL_TEXT("sgi")) || !iStrCmp(Ext, IL_TEXT("bw")) ||
		!iStrCmp(Ext, IL_TEXT("rgb")) || !iStrCmp(Ext, IL_TEXT("rgba")))
		Type = IL_SGI;
	else if (!iStrCmp(Ext, IL_TEXT("sun")) || !iStrCmp(Ext, IL_TEXT("ras")) ||
			 !iStrCmp(Ext, IL_TEXT("rs")) || !iStrCmp(Ext, IL_TEXT("im1")) ||
			 !iStrCmp(Ext, IL_TEXT("im8")) || !iStrCmp(Ext, IL_TEXT("im24")) ||
			 !iStrCmp(Ext, IL_TEXT("im32")))
		Type = IL_SUN;
	else if (!iStrCmp(Ext, IL_TEXT("texture")))
		Type = IL_TEXTURE;
	else if (!iStrCmp(Ext, IL_TEXT("tif")) || !iStrCmp(Ext, IL_TEXT("tiff")))
		Type = IL_TIF;
	else if (!iStrCmp(Ext, IL_TEXT("tpl")))
		Type = IL_TPL;
	else if (!iStrCmp(Ext, IL_TEXT("utx")))
		Type = IL_UTX;
	else if (!iStrCmp(Ext, IL_TEXT("vtf")))
		Type = IL_VTF;
	else if (!iStrCmp(Ext, IL_TEXT("wal")))
		Type = IL_WAL;
	else if (!iStrCmp(Ext, IL_TEXT("wbmp")))
		Type = IL_WBMP;
	else if (!iStrCmp(Ext, IL_TEXT("wdp")) || !iStrCmp(Ext, IL_TEXT("hdp")))
		Type = IL_WDP;
	else if (!iStrCmp(Ext, IL_TEXT("xpm")))
		Type = IL_XPM;
	else
		Type = IL_TYPE_UNKNOWN;

	return Type;
}

ILAPI ILenum ILAPIENTRY iDetermineTypeFuncs(SIO*  io)
{
	// Read data only once, then check the contents once: this is quicker than 
	// loading and checking the data for every check that is run sequentially
	// The following code assumes that it is okay to identify formats even when they're not supported
	const int bufSize = 512;
	ILubyte buf[bufSize];

	ILint64 read = io->read(io, buf, 1, bufSize);
	io->seek(io, -read, SEEK_CUR);
	if (read < 16)
		return IL_TYPE_UNKNOWN;

	switch(buf[0]) {
		case 0x01:
			if (buf[1] == 0xda)
				#ifndef IL_NO_SGI
				if (iIsValidSgi(io))
				#endif
					return IL_SGI;
			break;
		#ifndef IL_NO_PCX
		case 0x0a:
			if (iIsValidPcx(io))
				return IL_PCX;
			break;
		#endif
		case '#':
			if (buf[1] == '?' && buf[2] == 'R')
				return IL_HDR;
			break;
		case '8':
			if (buf[1] == 'B' && buf[2] == 'P' && buf[3] == 'S')
				#ifndef IL_NO_PSD
				if (iIsValidPsd(io))
					return IL_PSD;
				#else
				return IL_PSD;
				#endif
			break;
		case 'A':
			if (buf[1] == 'H')
				return IL_HALO_PAL;
			break;
		case 'B':
			if (buf[1] == 'M') {
				#ifndef IL_NO_BMP
				if (iIsValidBmp(io))
					return IL_BMP;
				#else
				return IL_BMP;
				#endif
			} else if (buf[1] == 'L' && buf[2] == 'P' && (buf[3] == '1' || buf[3] == '2')) {
				#ifndef IL_NO_BLP
				if (iIsValidBlp(io))
				#endif
					return IL_BLP;
			}
			break;
		case 'G':
			if (!strnicmp((const char*) buf, "GIF87A", 6))
				return IL_GIF;
			if (!strnicmp((const char*) buf, "GIF89A", 6))
				return IL_GIF;		
			break;
		case 'I':
			switch(buf[1]) {
			case 'I':
				if (buf[2] == 42) {
					#ifndef IL_NO_TIF
					if (ilIsValidTiffFunc(io))
						return IL_TIF;
					#else
					return IL_TIF;
					#endif
				}
				break;
			case 0xBC:
					return IL_WDP;
			case 'W':
				if (buf[2] == 'i')
					#ifndef IL_NO_IWI
					if (iIsValidIwi(io))
					#endif
						return IL_IWI;
			}
		case 'M':
			if (buf[1] == 'M')
				#ifndef IL_NO_TIF
				if (ilIsValidTiffFunc(io))
					return IL_TIF;
				#else
				return IL_TIF;
				#endif
		case 'P':
			if (strnicmp((const char*) buf, "Paint Shop Pro Image File", 25) == 0) {
				#ifndef IL_NO_PSP
				if (iIsValidPsp(io))
				#endif
					return IL_PSP;
			} else if (buf[1] >= '1' && buf[1] <= '6') {
					return IL_PNM; // il_pnm's test doesn't add anything here
			}
			break;
		case 'S':
			if (!strnicmp((const char*) buf, "SDPX", 4))
				return IL_DPX;
			if (!strnicmp((const char*) buf, "SIMPLE", 6))
				return IL_FITS;
			break;
		case 'V':
			if (buf[1] == 'T' && buf[2] == 'F')
				#ifndef IL_NO_VTF
				if (iIsValidVtf(io))
					return IL_VTF;
				#else
				return IL_VTF;
				#endif
			break;
		case 'X':
			if (!strnicmp((const char*) buf, "XDPX", 4))
				return IL_DPX;
			break;
		case 0x59:
			if (buf[1] == 0xA6 && buf[2] == 0x6A && buf[3] == 0x95)
				#ifndef IL_NO_SUN
				if (iIsValidSun(io))
					return IL_SUN;
				#endif
			break;
		case 'v':
			if (buf[1] == '/' && buf[2] == '1' && buf[3] == 1)
				#ifndef IL_NO_EXR
				if (iIsValidExr(io))
				#endif
					return IL_EXR;
			break;
		case 0x89:
			if (buf[1] == 'P' && buf[2] == 'N' && buf[3] == 'G')
				#ifndef IL_NO_PNG
				if (iIsValidPng(io))
					return IL_PNG;
				#else
				return IL_PNG;
				#endif
			break;
		case 0x8a:
			if (buf[1] == 0x4D
			&&  buf[2] == 0x4E
			&&  buf[3] == 0x47
			&&  buf[4] == 0x0D
			&&  buf[5] == 0x0A
			&&  buf[6] == 0x1A
			&&  buf[7] == 0x0A)
			{
				return IL_MNG;
			}
			break;
		case 0xC1:
			if (buf[1] == 0x83 
			&&  buf[2] == 0x2a
			&&  buf[3] == 0x9e)
			{
				return IL_UTX;
			}
		case 0xff:
			if (buf[1] == 0xd8)
				return IL_JPG;
			break;
	}

	if (read >= 131) {
		if (buf[128] == 'D' 
		&&  buf[129] == 'I' 
		&&  buf[130] == 'C' 
		&&  buf[131] == 'M')
		{
			return IL_DICOM;
		}
	}

	#ifndef IL_NO_DDS
	if (iIsValidDds(io))
		return IL_DDS;
	#endif

	#ifndef IL_NO_ICNS
	if (iIsValidIcns(io))
		return IL_ICNS;
	#endif

	#ifndef IL_NO_ILBM
	if (iIsValidIlbm(io))
		return IL_ILBM;
	#endif

	#ifndef IL_NO_IWI
	if (iIsValidIwi(io))
		return IL_IWI;
	#endif

	#ifndef IL_NO_JP2
	if (iIsValidJp2(io))
		return IL_JP2;
	#endif

	#ifndef IL_NO_LIF
	if (iIsValidLif(io))
		return IL_LIF;
	#endif

	#ifndef IL_NO_MDL
	if (iIsValidMdl(io))
		return IL_MDL;
	#endif

	#ifndef IL_NO_MP3
	if (iIsValidMp3(io))
		return IL_MP3;
	#endif

	#ifndef IL_NO_PIC
	if (iIsValidPic(io))
		return IL_PIC;
	#endif

	#ifndef IL_NO_PIX
	if (iIsValidPix(io))
		return IL_PIX;
	#endif

	#ifndef IL_NO_TPL
	if (iIsValidTpl(io))
		return IL_TPL;
	#endif

	#ifndef IL_NO_XPM
	if (iIsValidXpm(io))
		return IL_XPM;
	#endif

	// Some file types have a weak signature, so we test for these formats 
	// after checking for most other formats
	#ifndef IL_NO_ICO
	if (iIsValidIcon(io))
		return IL_ICO;
	#endif

	//moved tga to end of list because it has no magic number
	//in header to assure that this is really a tga... (20040218)
	#ifndef IL_NO_TGA
	if (iIsValidTarga(io))
		return IL_TGA;
	#endif
	
	return IL_TYPE_UNKNOWN;
}

ILAPI ILenum ILAPIENTRY il2DetermineTypeFuncs(ILimage* imageExt)
{
	ILimage* image = (ILimage*) imageExt;
	SIO* io = &image->io;
	return iDetermineTypeFuncs (io);
}

ILenum ILAPIENTRY il2DetermineTypeF(ILHANDLE File)
{
	SIO io;
	iSetInputFile(&io, File);
	return iDetermineTypeFuncs(&io);
}

ILenum ILAPIENTRY il2DetermineTypeL(const void *Lump, ILuint Size)
{
	SIO io;
	iSetInputLump(&io, Lump, Size);
	return iDetermineTypeFuncs(&io);
}


ILenum ILAPIENTRY il2DetermineType(ILconst_string FileName)
{
	ILenum type = IL_TYPE_UNKNOWN;

	if (FileName != NULL) {
		// If we can open the file, determine file type from contents
		// This is more reliable than the information given by the file extension 
		SIO io;
		iSetRead(&io, NULL, iDefaultCloseR, iDefaultEof, iDefaultGetc, iDefaultRead, iDefaultSeek, iDefaultTell);
		io.handle = iDefaultOpenR(FileName);
		if (io.handle != NULL) {
			type = iDetermineTypeFuncs(&io);
			io.close(&io);
		}

		// Not in all cases can the file type be determined based on the contents
		// In that case, determine the type from the file extension
		if (type == IL_TYPE_UNKNOWN)
			type = ilTypeFromExt(FileName);
	}

	return type;
}


ILboolean ILAPIENTRY iIsValid(ILenum Type, SIO* io)
{
	if (io == NULL) {
		il2SetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	switch (Type)
	{
		#ifndef IL_NO_TGA
		case IL_TGA:
			return iIsValidTarga(io);
		#endif

		#ifndef IL_NO_JPG
		case IL_JPG:
			return iIsValidJpeg(io);
		#endif

		#ifndef IL_NO_DDS
		case IL_DDS:
			return iIsValidDds(io);
		#endif

		#ifndef IL_NO_PNG
		case IL_PNG:
			return iIsValidPng(io);
		#endif

		#ifndef IL_NO_BMP
		case IL_BMP:
			return iIsValidBmp(io);
		#endif

		#ifndef IL_NO_DICOM
		case IL_DICOM:
			return iIsValidDicom(io);
		#endif

		#ifndef IL_NO_EXR
		case IL_EXR:
			return iIsValidExr(io);
		#endif

		#ifndef IL_NO_GIF
		case IL_GIF:
			return iIsValidGif(io);
		#endif

		#ifndef IL_NO_HDR
		case IL_HDR:
			return iIsValidHdr(io);
		#endif

		#ifndef IL_NO_ICNS
		case IL_ICNS:
			return iIsValidIcns(io);
		#endif

		#ifndef IL_NO_IWI
		case IL_IWI:
			return iIsValidIwi(io);
		#endif

    	#ifndef IL_NO_ILBM
        case IL_ILBM:
            return iIsValidIlbm(io);
	    #endif

		#ifndef IL_NO_JP2
		case IL_JP2:
			return iIsValidJp2(io);
		#endif

		#ifndef IL_NO_LIF
		case IL_LIF:
			return iIsValidLif(io);
		#endif

		#ifndef IL_NO_MDL
		case IL_MDL:
			return iIsValidMdl(io);
		#endif

		#ifndef IL_NO_MP3
		case IL_MP3:
			return iIsValidMp3(io);
		#endif

		#ifndef IL_NO_PCX
		case IL_PCX:
			return iIsValidPcx(io);
		#endif

		#ifndef IL_NO_PIC
		case IL_PIC:
			return iIsValidPic(io);
		#endif

		#ifndef IL_NO_PNM
		case IL_PNM:
			return iIsValidPnm(io);
		#endif

		#ifndef IL_NO_PSD
		case IL_PSD:
			return iIsValidPsd(io);
		#endif

		#ifndef IL_NO_PSP
		case IL_PSP:
			return iIsValidPsp(io);
		#endif

		#ifndef IL_NO_SGI
		case IL_SGI:
			return iIsValidSgi(io);
		#endif

		#ifndef IL_NO_SUN
		case IL_SUN:
			return iIsValidSun(io);
		#endif

		#ifndef IL_NO_TIF
		case IL_TIF:
			return ilIsValidTiffFunc(io);
		#endif

		#ifndef IL_NO_TPL
		case IL_TPL:
			return iIsValidTpl(io);
		#endif

		#ifndef IL_NO_VTF
		case IL_VTF:
			return iIsValidVtf(io);
		#endif

		#ifndef IL_NO_XPM
		case IL_XPM:
			return iIsValidXpm(io);
		#endif
	}

	il2SetError(IL_INVALID_ENUM);
	return IL_FALSE;
}


ILboolean ILAPIENTRY ilIsValidF(ILenum Type, ILHANDLE File)
{
	SIO io;
	iSetInputFile(&io, File);
	return iIsValid(Type, &io);
}


ILboolean ILAPIENTRY ilIsValidL(ILenum Type, void *Lump, ILuint Size)
{
	SIO io;
	iSetInputLump(&io, Lump, Size);
	return iIsValid(Type, &io);
}


ILboolean ILAPIENTRY ilIsValid(ILenum Type, ILconst_string FileName)
{
	ILboolean result = false;
	if (FileName != NULL) {
		ILHANDLE f = iDefaultOpenR(FileName);
		result = ilIsValidF(Type, f);
		fclose((FILE*) f);
	}

	return result;
}


ILAPI ILboolean ILAPIENTRY il2LoadFuncs(ILimage* imageExt, ILenum type)
{
	ILimage* image = (ILimage*) imageExt;
	if (type == IL_TYPE_UNKNOWN)
		type = iDetermineTypeFuncs(&image->io);

	switch (type)
	{
		case IL_TYPE_UNKNOWN:
			return IL_FALSE;

		#ifndef IL_NO_BMP
		case IL_BMP:
			return iLoadBitmapInternal(image);
		#endif

		#ifndef IL_NO_CUT
		case IL_CUT:
			return iLoadCutInternal(image);
		#endif

		#ifndef IL_NO_ICO
		case IL_ICO:
			return iLoadIconInternal(image);
		#endif

		#ifndef IL_NO_JPG
			#ifndef IL_USE_IJL
			case IL_JPG:
				return iLoadJpegInternal(image);
			#endif
		#endif

		#ifndef IL_NO_ILBM
		case IL_ILBM:
			return iLoadIlbmInternal(image);
		#endif

		#ifndef IL_NO_PCD
		case IL_PCD:
			return iLoadPcdInternal(image);
		#endif

		#ifndef IL_NO_PCX
		case IL_PCX:
			return iLoadPcxInternal(image);
		#endif

		#ifndef IL_NO_PIC
		case IL_PIC:
			return iLoadPicInternal(image);
		#endif

		#ifndef IL_NO_PNG
		case IL_PNG:
			return iLoadPngInternal(image);
		#endif

		#ifndef IL_NO_PNM
		case IL_PNM:
			return iLoadPnmInternal(image);
		#endif

		#ifndef IL_NO_SGI
		case IL_SGI:
			return iLoadSgiInternal(image);
		#endif

		#ifndef IL_NO_TGA
		case IL_TGA:
			return iLoadTargaInternal(image);
		#endif

		#ifndef IL_NO_TIF
		case IL_TIF:
			return iLoadTiffInternal(image);
		#endif

		#ifndef IL_NO_RAW
		case IL_RAW:
			return iLoadRawInternal(image);
		#endif

		// Currently broken - need wrappers for streams?
		/*#ifndef IL_NO_JP2
		case IL_JP2:
			return iLoadJp2Internal(image);
		#endif*/

		#ifndef IL_NO_MNG
		case IL_MNG:
			return iLoadMngInternal(image);
		#endif

		#ifndef IL_NO_GIF
		case IL_GIF:
			return iLoadGifInternal(image);
		#endif

		#ifndef IL_NO_DDS
		case IL_DDS:
			return iLoadDdsInternal(image);
		#endif

		#ifndef IL_NO_PSD
		case IL_PSD:
			return iLoadPsdInternal(image);
		#endif

		#ifndef IL_NO_BLP
		case IL_BLP:
			return iLoadBlpInternal(image);
		#endif

		#ifndef IL_NO_PSP
		case IL_PSP:
			return iLoadPspInternal(image);
		#endif

		#ifndef IL_NO_PIX
		case IL_PIX:
			return iLoadPixInternal(image);
		#endif

		#ifndef IL_NO_PXR
		case IL_PXR:
			return iLoadPxrInternal(image);
		#endif

		#ifndef IL_NO_XPM
		case IL_XPM:
			return iLoadXpmInternal(image);
		#endif

		#ifndef IL_NO_HDR
		case IL_HDR:
			return iLoadHdrInternal(image);
		#endif

		#ifndef IL_NO_ICNS
		case IL_ICNS:
			return iLoadIcnsInternal(image);
		#endif

		#ifndef IL_NO_VTF
		case IL_VTF:
			return iLoadVtfInternal(image);
		#endif

		#ifndef IL_NO_WBMP
		case IL_WBMP:
			return iLoadWbmpInternal(image);
		#endif

		#ifndef IL_NO_SUN
		case IL_SUN:
			return iLoadSunInternal(image);
		#endif

		#ifndef IL_NO_IFF
		case IL_IFF:
			return iLoadIffInternal(image);
		#endif

		#ifndef IL_NO_FITS
		case IL_FITS:
			return iLoadFitsInternal(image);
		#endif

		#ifndef IL_NO_DICOM
		case IL_DICOM:
			return iLoadDicomInternal(image);
		#endif

		#ifndef IL_NO_DOOM
		case IL_DOOM:
			return iLoadDoomInternal(image);
		case IL_DOOM_FLAT:
			return iLoadDoomFlatInternal(image);
		#endif

		#ifndef IL_NO_TEXTURE
		case IL_TEXTURE:
			//return ilLoadTextureF(File);
			// From http://forums.totalwar.org/vb/showthread.php?t=70886, all that needs to be done
			//  is to strip out the first 48 bytes, and then it is DDS data.
			image->io.seek(&image->io, 48, IL_SEEK_CUR);
			return iLoadDdsInternal(image);
		#endif

		#ifndef IL_NO_DPX
		case IL_DPX:
			return iLoadDpxInternal(image);
		#endif

		#ifndef IL_NO_EXR
		case IL_EXR:
			return iLoadExrInternal(image);
		#endif

		#ifndef IL_NO_FTX
		case IL_FTX:
			return iLoadFtxInternal(image);
		#endif

		#ifndef IL_NO_IWI
		case IL_IWI:
			return iLoadIwiInternal(image);
		#endif

		#ifndef IL_NO_LIF
		case IL_LIF:
			return iLoadLifInternal(image);
		#endif

		#ifndef IL_NO_MDL
		case IL_MDL:
			return iLoadMdlInternal(image);
		#endif

		#ifndef IL_NO_MP3
		case IL_MP3:
			return iLoadMp3Internal(image);
		#endif

		#ifndef IL_NO_ROT
		case IL_ROT:
			return iLoadRotInternal(image);
		#endif

		#ifndef IL_NO_TPL
		case IL_TPL:
			return iLoadTplInternal(image);
		#endif

		#ifndef IL_NO_UTX
		case IL_UTX:
			return iLoadUtxInternal(image);
		#endif

		#ifndef IL_NO_WAL
		case IL_WAL:
			return iLoadWalInternal(image);
		#endif
	}

	il2SetError(IL_INVALID_ENUM);
	return IL_FALSE;
}


//! Attempts to load an image from a file.  The file format is specified by the user.
/*! \param Type Format of this file.  Acceptable values are IL_BLP, IL_BMP, IL_CUT, IL_DCX, IL_DDS,
	IL_DICOM, IL_DOOM, IL_DOOM_FLAT, IL_DPX, IL_EXR, IL_FITS, IL_FTX, IL_GIF, IL_HDR, IL_ICO, IL_ICNS,
	IL_IFF, IL_IWI, IL_JP2, IL_JPG, IL_LIF, IL_MDL,	IL_MNG, IL_MP3, IL_PCD, IL_PCX, IL_PIX, IL_PNG,
	IL_PNM, IL_PSD, IL_PSP, IL_PXR, IL_ROT, IL_SGI, IL_SUN, IL_TEXTURE, IL_TGA, IL_TIF, IL_TPL,
	IL_UTX, IL_VTF, IL_WAL, IL_WBMP, IL_XPM, IL_RAW, IL_JASC_PAL and IL_TYPE_UNKNOWN.
	If IL_TYPE_UNKNOWN is specified, ilLoad will try to determine the type of the file and load it.
	\param FileName Ansi or Unicode string, depending on the compiled version of DevIL, that gives
	       the filename of the file to load.
	\return Boolean value of failure or success.  Returns IL_FALSE if all three loading methods
	       have been tried and failed.*/
ILboolean ILAPIENTRY il2Load(ILimage* imageExt, ILenum Type, ILconst_string FileName)
{
	if (FileName == NULL || ilStrLen(FileName) < 1) {
		il2SetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	ILimage* image = (ILimage*) imageExt;
	il2ResetRead(image);
	image->io.handle = image->io.openReadOnly(FileName);
	if (image->io.handle != NULL) {
		ILboolean bLoaded = il2LoadFuncs(image, Type);
		image->io.close(&image->io);

		if (bLoaded && Type == IL_CUT) {
			// Attempt to load the palette for DR Halo pictures
			#ifdef  UNICODE
			auto fnLen = wcslen(FileName);
			#else
			auto fnLen = strlen(FileName);
			#endif
			if (fnLen > 4) {
				if (FileName[fnLen-4] == '.'
				&&  FileName[fnLen-3] == 'c'
				&&  FileName[fnLen-2] == 'u'
				&&  FileName[fnLen-1] == 't') 
				{
					#ifdef  UNICODE
					TCHAR* palFN = (TCHAR*) ialloc(2*fnLen+2);
					wcscpy(palFN, FileName);
					wcscpy(&palFN[fnLen-3], L"pal");
					#else
					char* palFN = (char*) ialloc(fnLen+2);
					strcpy(palFN, FileName);
					strcpy(&palFN[fnLen-3], "pal");
					#endif
					ilLoadHaloPal(image, palFN);
					ifree(palFN);
				}
			}
		}

		return bLoaded;
	} else
		return IL_FALSE;
}


//! Attempts to load an image from a file stream.  The file format is specified by the user.
/*! \param Type Format of this file.  Acceptable values are IL_BLP, IL_BMP, IL_CUT, IL_DCX, IL_DDS,
	IL_DICOM, IL_DOOM, IL_DOOM_FLAT, IL_DPX, IL_EXR, IL_FITS, IL_FTX, IL_GIF, IL_HDR, IL_ICO, IL_ICNS,
	IL_IFF, IL_IWI, IL_JP2, IL_JPG, IL_LIF, IL_MDL,	IL_MNG, IL_MP3, IL_PCD, IL_PCX, IL_PIX, IL_PNG,
	IL_PNM, IL_PSD, IL_PSP, IL_PXR, IL_ROT, IL_SGI, IL_SUN, IL_TEXTURE, IL_TGA, IL_TIF, IL_TPL,
	IL_UTX, IL_VTF, IL_WAL, IL_WBMP, IL_XPM, IL_RAW, IL_JASC_PAL and IL_TYPE_UNKNOWN.
	If IL_TYPE_UNKNOWN is specified, ilLoadF will try to determine the type of the file and load it.
	\param File File stream to load from. The caller is responsible for closing the handle.
	\return Boolean value of failure or success.  Returns IL_FALSE if loading fails.*/
ILAPI ILboolean ILAPIENTRY il2LoadF(ILimage* imageExt, ILenum Type, ILHANDLE File)
{
	if (File == NULL) {
		il2SetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	ILimage* image = (ILimage*) imageExt;
	il2ResetRead(image);
	image->io.handle = File;
	image->io.seek(&image->io, 0, IL_SEEK_SET);
	if (Type == IL_TYPE_UNKNOWN)
		Type = iDetermineTypeFuncs(&image->io);

	ILboolean bRet = il2LoadFuncs(image, Type);

	return bRet;
}

//! Attempts to load an image from a memory buffer.  The file format is specified by the user.
/*! \param Type Format of this file.  Acceptable values are IL_BLP, IL_BMP, IL_CUT, IL_DCX, IL_DDS,
	IL_DICOM, IL_DOOM, IL_DOOM_FLAT, IL_DPX, IL_EXR, IL_FITS, IL_FTX, IL_GIF, IL_HDR, IL_ICO, IL_ICNS,
	IL_IFF, IL_IWI, IL_JP2, IL_JPG, IL_LIF, IL_MDL,	IL_MNG, IL_MP3, IL_PCD, IL_PCX, IL_PIX, IL_PNG,
	IL_PNM, IL_PSD, IL_PSP, IL_PXR, IL_ROT, IL_SGI, IL_SUN, IL_TEXTURE, IL_TGA, IL_TIF, IL_TPL,
	IL_UTX, IL_VTF, IL_WAL, IL_WBMP, IL_XPM, IL_RAW, IL_JASC_PAL and IL_TYPE_UNKNOWN.
	If IL_TYPE_UNKNOWN is specified, ilLoadL will try to determine the type of the file and load it.
	\param Lump The buffer where the file data is located
	\param Size Size of the buffer
	\return Boolean value of failure or success.  Returns IL_FALSE if loading fails.*/
ILAPI ILboolean ILAPIENTRY il2LoadL(ILimage* imageExt, ILenum Type, const void *Lump, ILuint Size)
{
	ILimage* image = (ILimage*) imageExt;
	if (Lump == NULL || Size == 0 || image == NULL) {
		il2SetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	iSetInputLump(&image->io, Lump, Size);
	return il2LoadFuncs(image, Type);
}

//! Attempts to load an image from a file with various different methods before failing - very generic.
/*! The ilLoadImage function allows a general interface to the specific internal file-loading
	routines.  First, it finds the extension and checks to see if any user-registered functions
	(registered through ilRegisterLoad) match the extension. If nothing matches, it takes the
	extension and determines which function to call based on it. Lastly, it attempts to identify
	the image based on various image header verification functions, such as ilIsValidPngF.
	If all this checking fails, IL_FALSE is returned with no modification to the current bound image.
	\param FileName Ansi or Unicode string, depending on the compiled version of DevIL, that gives
	       the filename of the file to load.
	\return Boolean value of failure or success.  Returns IL_FALSE if all three loading methods
	       have been tried and failed.*/
ILAPI ILboolean ILAPIENTRY il2LoadImage(ILimage* imageExt, ILconst_string FileName)
{
	ILimage* image = (ILimage*) imageExt;
	ILenum type = il2DetermineType(FileName);

	if (type != IL_TYPE_UNKNOWN) {
		return il2Load(image, type, FileName);
	} else {
		il2SetError(IL_INVALID_EXTENSION);
		return IL_FALSE;
	}
}


ILAPI ILboolean ILAPIENTRY il2SaveFuncs(ILimage* imageExt, ILenum type)
{
	ILboolean bRet = false;
	ILimage* image = (ILimage*) imageExt;

	switch(type) {
	#ifndef IL_NO_BMP
	case IL_BMP:
		bRet = iSaveBitmapInternal(image);
		break;
	#endif

	#ifndef IL_NO_JPG
	case IL_JPG:
		bRet = iSaveJpegInternal(image);
		break;
	#endif

	#ifndef IL_NO_PCX
	case IL_PCX:
		bRet = iSavePcxInternal(image);
		break;
	#endif

	#ifndef IL_NO_PNG
	case IL_PNG:
		bRet = iSavePngInternal(image);
		break;
	#endif

	#ifndef IL_NO_PNM  // Not sure if binary or ascii should be defaulted...maybe an option?
	case IL_PNM:
		bRet = iSavePnmInternal(image);
		break;
	#endif

	#ifndef IL_NO_SGI
	case IL_SGI:
		bRet = iSaveSgiInternal(image);
		break;
	#endif

	#ifndef IL_NO_TGA
	case IL_TGA:
		bRet = iSaveTargaInternal(image);
		break;
	#endif

	#ifndef IL_NO_TIF
	case IL_TIF:
		bRet = iSaveTiffInternal(image);
		break;
	#endif

	#ifndef IL_NO_CHEAD
	case IL_CHEAD:
		bRet = ilSaveCHeader(image, "IL_IMAGE");
		break;
	#endif

	#ifndef IL_NO_RAW
	case IL_RAW:
		bRet = iSaveRawInternal(image);
		break;
	#endif

	#ifndef IL_NO_MNG
	case IL_MNG:
		bRet = iSaveMngInternal();
		break;
	#endif

	#ifndef IL_NO_DDS
	case IL_DDS:
		bRet = iSaveDdsInternal(image);
		break;
	#endif

	#ifndef IL_NO_PSD
	case IL_PSD:
		bRet = iSavePsdInternal(image);
		break;
	#endif

	#ifndef IL_NO_HDR
	case IL_HDR:
		bRet = iSaveHdrInternal(image);
		break;
	#endif

	#ifndef IL_NO_JP2
	case IL_JP2:
		bRet = iSaveJp2Internal(image);
		break;
	#endif

	#ifndef IL_NO_EXR
	case IL_EXR:
		bRet = iSaveExrInternal(image);
		break;
	#endif

	#ifndef IL_NO_VTF
	case IL_VTF:
		bRet = iSaveVtfInternal(image);
		break;
	#endif

	#ifndef IL_NO_WBMP
	case IL_WBMP:
		bRet = iSaveWbmpInternal(image);
		break;
	#endif

	// Check if we just want to save the palette.
	// @todo: must be ported to use iCurImage->io
/*	case IL_JASC_PAL:
		bRet = ilSavePal(FileName);
		break;*/
	default:
		il2SetError(IL_INVALID_EXTENSION);
	}

	// Try registered procedures
	// @todo: must be ported to use iCurImage->io
	/*if (iRegisterSave(FileName))
		return IL_TRUE;*/

	return bRet;
}

//! Attempts to save an image to a file.  The file format is specified by the user.
/*! \param Type Format of this file.  Acceptable values are IL_BMP, IL_CHEAD, IL_DDS, IL_EXR,
	IL_HDR, IL_JP2, IL_JPG, IL_PCX, IL_PNG, IL_PNM, IL_PSD, IL_RAW, IL_SGI, IL_TGA, IL_TIF,
	IL_VTF, IL_WBMP and IL_JASC_PAL.
	\param FileName Ansi or Unicode string, depending on the compiled version of DevIL, that gives
	       the filename to save to.
	\return Boolean value of failure or success.  Returns IL_FALSE if saving failed.*/
ILAPI ILboolean ILAPIENTRY il2Save(ILimage* imageExt, ILenum type, ILconst_string FileName)
{
	ILimage* image = (ILimage*) imageExt;
	SIO * io = &image->io;

	if (FileName == NULL || ilStrLen(FileName) < 1) {
		il2SetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	ILboolean	bRet = IL_FALSE;
	il2ResetWrite(image);
	io->handle = io->openWrite(FileName);
	if (image->io.handle != NULL) {
		bRet = il2SaveFuncs(image, type);
		io->close(io);
		io->handle = NULL;
	}
	return bRet;
}


//! Saves the current image based on the extension given in FileName.
/*! \param FileName Ansi or Unicode string, depending on the compiled version of DevIL, that gives
	       the filename to save to.
	\return Boolean value of failure or success.  Returns IL_FALSE if saving failed.*/
ILAPI ILboolean ILAPIENTRY il2SaveImage(ILimage* imageExt, ILconst_string FileName)
{
	ILimage* image = (ILimage*) imageExt;

	if (FileName == NULL || ilStrLen(FileName) < 1) {
		il2SetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	ILenum type = ilTypeFromExt(FileName);
	ILboolean	bRet = IL_FALSE;
	il2ResetWrite(image);
	image->io.handle = image->io.openWrite(FileName);
	if (image->io.handle != NULL) {
		bRet = il2SaveFuncs(image, type);
		image->io.close(&image->io);
		image->io.handle = NULL;
	}
	return bRet;
}

//! Attempts to save an image to a file stream.  The file format is specified by the user.
/*! \param Type Format of this file.  Acceptable values are IL_BMP, IL_CHEAD, IL_DDS, IL_EXR,
	IL_HDR, IL_JP2, IL_JPG, IL_PCX, IL_PNG, IL_PNM, IL_PSD, IL_RAW, IL_SGI, IL_TGA, IL_TIF,
	IL_VTF, IL_WBMP and IL_JASC_PAL.
	\param File File stream to save to.
	\return Boolean value of failure or success.  Returns IL_FALSE if saving failed.*/
ILAPI ILuint ILAPIENTRY il2SaveF(ILimage* imageExt, ILenum type, ILHANDLE File)
{
	ILimage* image = (ILimage*) imageExt;
	iSetOutputFile(&image->io, File);
	return il2SaveFuncs(image, type);
}


//! Attempts to save an image to a memory buffer.  The file format is specified by the user.
/*! \param Type Format of this image file.  Acceptable values are IL_BMP, IL_CHEAD, IL_DDS, IL_EXR,
	IL_HDR, IL_JP2, IL_JPG, IL_PCX, IL_PNG, IL_PNM, IL_PSD, IL_RAW, IL_SGI, IL_TGA, IL_TIF,
	IL_VTF, IL_WBMP and IL_JASC_PAL.
	\param Lump Memory buffer to save to
	\param Size Size of the memory buffer
	\return The number of bytes written to the lump, or 0 in case of failure*/
ILAPI ILint64 ILAPIENTRY il2SaveL(ILimage* imageExt, ILenum Type, void *Lump, ILuint Size)
{
	ILimage* image = (ILimage*) imageExt;
	iSetOutputLump(&image->io, Lump, Size);
	ILint64 pos1 = image->io.tell(&image->io);
	ILboolean bRet = il2SaveFuncs(image, Type);
	image->io.seek(&image->io, 0, IL_SEEK_END);
	ILint64 pos2 = image->io.tell(&image->io);

	if (bRet)
		return pos2-pos1;  // Return the number of bytes written.
	else
		return 0;  // Error occurred
}
