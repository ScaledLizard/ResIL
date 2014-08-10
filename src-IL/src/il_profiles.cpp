//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 01/23/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_profiles.c
//
// Description: Colour profile handler
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#ifndef IL_NO_LCMS

#ifdef PACKAGE_NAME
#define IL_PACKAGE_NAME PACKAGE_NAME;
#undef  PACKAGE_NAME
#endif

#if defined(IL_USE_PRAGMA_LIBS)
	#if defined(_MSC_VER) || defined(__BORLANDC__)
		#ifndef _DEBUG
			#pragma comment(lib, "lcms.lib")
		#else
			#pragma comment(lib, "lcms-d.lib")
 		#endif
 	#endif
#endif

#include <lcms2.h>

#ifdef PACKAGE_NAME
#undef PACKAGE_NAME
#endif

#ifdef IL_PACKAGE_NAME
#define PACKAGE_NAME IL_PACKAGE_NAME
#undef  IL_PACKAGE_NAME
#endif

#endif//IL_NO_LCMS

ILboolean ILAPIENTRY il2ApplyProfile(ILimage* image, ILstring InProfile, ILstring OutProfile)
{
#ifndef IL_NO_LCMS
	cmsHPROFILE		hInProfile, hOutProfile;
	cmsHTRANSFORM	hTransform;
	ILubyte			*Temp;
	ILint			Format=0;
#ifdef _UNICODE
	char AnsiName[512];
#endif//_UNICODE

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	switch (image->Type)
	{
		case IL_BYTE:
		case IL_UNSIGNED_BYTE:
			switch (image->Format)
			{
				case IL_LUMINANCE:
					Format = TYPE_GRAY_8;
					break;
				case IL_RGB:
					Format = TYPE_RGB_8;
					break;
				case IL_BGR:
					Format = TYPE_BGR_8;
					break;
				case IL_RGBA:
					Format = TYPE_RGBA_8;
					break;
				case IL_BGRA:
					Format = TYPE_BGRA_8;
					break;
				default:
					il2SetError(IL_INTERNAL_ERROR);
					return IL_FALSE;
			}
			break;

		case IL_SHORT:
		case IL_UNSIGNED_SHORT:
			switch (image->Format)
			{
				case IL_LUMINANCE:
					Format = TYPE_GRAY_16;
					break;
				case IL_RGB:
					Format = TYPE_RGB_16;
					break;
				case IL_BGR:
					Format = TYPE_BGR_16;
					break;
				case IL_RGBA:
					Format = TYPE_RGBA_16;
					break;
				case IL_BGRA:
					Format = TYPE_BGRA_16;
					break;
				default:
					il2SetError(IL_INTERNAL_ERROR);
					return IL_FALSE;
			}
			break;

		// These aren't supported right now.
		case IL_INT:
		case IL_UNSIGNED_INT:
		case IL_FLOAT:
		case IL_DOUBLE:
			il2SetError(IL_ILLEGAL_OPERATION);
			return IL_FALSE;
	}


	if (InProfile == NULL) {
		if (!image->Profile || !image->ProfileSize) {
			il2SetError(IL_INVALID_PARAM);
			return IL_FALSE;
		}
		hInProfile = image->Profile;
	}
	else {
#ifndef _UNICODE
 		hInProfile = cmsOpenProfileFromFile(InProfile, "r");
#else
		wcstombs(AnsiName, InProfile, 512);
		hInProfile = cmsOpenProfileFromFile(AnsiName, "r");
#endif//_UNICODE
	}
#ifndef _UNICODE
 	hOutProfile = cmsOpenProfileFromFile(OutProfile, "r");
#else
	wcstombs(AnsiName, OutProfile, 512);
	hOutProfile = cmsOpenProfileFromFile(AnsiName, "r");
#endif//_UNICODE

	hTransform = cmsCreateTransform(hInProfile, Format, hOutProfile, Format, INTENT_PERCEPTUAL, 0);

	Temp = (ILubyte*)ialloc(image->SizeOfData);
	if (Temp == NULL) {
		return IL_FALSE;
	}

	cmsDoTransform(hTransform, image->Data, Temp, image->SizeOfData / 3);

	ifree(image->Data);
	image->Data = Temp;

	cmsDeleteTransform(hTransform);
	if (InProfile != NULL)
		cmsCloseProfile(hInProfile);
	cmsCloseProfile(hOutProfile);

#endif//IL_NO_LCMS

	return IL_TRUE;
}
