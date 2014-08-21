///////////////////////////////////////////////////////////////////////////////
//
// ImageLib Utility Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-ILU/src/ilu_states.c
//
// Description: The state machine
//
///////////////////////////////////////////////////////////////////////////////

#include "ilu_internal.h"
#include "ilu_states.h"

// Constants 
ILconst_string _iluVendor	= IL_TEXT("Abysmal Software");
ILconst_string _iluVersion	= IL_TEXT("Developer's Image Library Utilities (ILU) 1.9.0");// IL_TEXT(__DATE__));

// Global variables
ILenum iluFilter = ILU_NEAREST;
ILenum iluPlacement = ILU_CENTER;


ILstring ILAPIENTRY ilu2GetString(ILenum StringName)
{
	switch (StringName)
	{
		case ILU_VENDOR:
			return (ILstring)_iluVendor;
		//changed 2003-09-04
		case ILU_VERSION_NUM:
			return (ILstring)_iluVersion;
		default:
			il2SetError(ILU_INVALID_PARAM);
			break;
	}
	return NULL;
}


void ILAPIENTRY ilu2GetIntegerv(ILenum Mode, ILint *Param)
{
	switch (Mode)
	{
		case ILU_VERSION_NUM:
			*Param = ILU_VERSION;
			break;

		case ILU_FILTER:
			*Param = iluFilter;
			break;

		default:
			il2SetError(ILU_INVALID_ENUM);
	}
	return;
}


ILint ILAPIENTRY ilu2GetInteger(ILenum Mode)
{
	ILint Temp;
	Temp = 0;
	ilu2GetIntegerv(Mode, &Temp);
	return Temp;
}


void ILAPIENTRY ilu2ImageParameter(ILenum PName, ILenum Param)
{
	switch (PName)
	{
		case ILU_FILTER:
			switch (Param)
			{
				case ILU_NEAREST:
				case ILU_LINEAR:
				case ILU_BILINEAR:
				case ILU_SCALE_BOX:
				case ILU_SCALE_TRIANGLE:
				case ILU_SCALE_BELL:
				case ILU_SCALE_BSPLINE:
				case ILU_SCALE_LANCZOS3:
				case ILU_SCALE_MITCHELL:
					iluFilter = Param;
					break;
				default:
					il2SetError(ILU_INVALID_ENUM);
					return;
			}
			break;

		case ILU_PLACEMENT:
			switch (Param)
			{
				case ILU_LOWER_LEFT:
				case ILU_LOWER_RIGHT:
				case ILU_UPPER_LEFT:
				case ILU_UPPER_RIGHT:
				case ILU_CENTER:
					iluPlacement = Param;
					break;
				default:
					il2SetError(ILU_INVALID_ENUM);
					return;
			}
			break;

		default:
			il2SetError(ILU_INVALID_ENUM);
			return;
	}
	return;
}
