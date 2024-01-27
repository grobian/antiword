/*
 * jpeg2sprt.c
 * Copyright (C) 2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Functions to translate jpeg pictures into sprites
 */

#include <stdio.h>
#include "antiword.h"


/*
 * bTranslateJPEG - translate a JPEG picture
 *
 * This function translates a picture from jpeg to sprite
 *
 * return TRUE when sucessful, otherwise FALSE
 */
BOOL
bTranslateJPEG(diagram_type *pDiag, FILE *pFile,
	long lFileOffset, int iPictureLen, const imagedata_type *pImg)
{
  	/* JPEG is not supported yet */
	return bAddDummyImage(pDiag, pImg);
} /* end of bTranslateJPEG */
