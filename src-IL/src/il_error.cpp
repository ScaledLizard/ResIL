//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 06/02/2007
//
// Filename: src-IL/src/il_error.c
//
// Description: The error functions
//
//-----------------------------------------------------------------------------


#include "il_internal.h"


// Every thread has its own copy of the variable holding the last error, so be 
// sure to copy it elsewhere if you still need it after closing the thread!
IL_THREAD_VAR ILenum	lastError;


// Sets the current error
ILAPI void ILAPIENTRY il2SetError(ILenum Error)
{
	lastError = Error;
}


//! Gets the last error
ILAPI ILenum ILAPIENTRY il2GetError(void)
{
	auto retVal = lastError;
	lastError = IL_NO_ERROR;
	return retVal;
}
