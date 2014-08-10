//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_stack.h
//
// Description: The main image stack
//
//-----------------------------------------------------------------------------

#ifndef IMAGESTACK_H
#define IMAGESTACK_H

#include "il_internal.h"


typedef struct iFree
{
	ILuint	Name;
	void	*Next;
} iFree;


// Internal functions
ILboolean	iEnlargeStack(void);
void		iFreeMem(void);
ILimage *iGetBaseImage();

#endif//IMAGESTACK_H
