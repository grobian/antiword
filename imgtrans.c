/*
 * imgtrans.c
 * Copyright (C) 2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Generic functions to translate Word images
 */

#include <stdio.h>
#include "antiword.h"


/*
 * bTranslateImage - translate the image
 *
 * This function reads the type of the given image and and gets it translated.
 *
 * return TRUE when sucessful, otherwise FALSE
 */
BOOL
bTranslateImage(diagram_type *pDiag, FILE *pFile, BOOL bMinimalInformation,
	long lFileOffsetImage, const imagedata_type *pImg)
{
	options_type	tOptions;

	DBG_MSG("bTranslateImage");

	fail(pDiag == NULL);
	fail(pFile == NULL);
	fail(lFileOffsetImage < 0);
	fail(pImg == NULL);
	fail(pImg->iHorizontalSize <= 0);
	fail(pImg->iVerticalSize <= 0);

	vGetOptions(&tOptions);
	fail(tOptions.eImageLevel == level_no_images);

	if (bMinimalInformation) {
		return bAddDummyImage(pDiag, pImg);
	}

	switch (pImg->eImageType) {
	case imagetype_is_dib:
		return bTranslateDIB(pDiag, pFile,
				lFileOffsetImage + pImg->iPosition,
				pImg);
	case imagetype_is_jpeg:
		return bTranslateJPEG(pDiag, pFile,
				lFileOffsetImage + pImg->iPosition,
				pImg->iLength - pImg->iPosition,
				pImg);
	case imagetype_is_png:
		if (tOptions.eImageLevel == level_ps_2) {
			return bAddDummyImage(pDiag, pImg);
		}
		return bTranslatePNG(pDiag, pFile,
				lFileOffsetImage + pImg->iPosition,
				pImg->iLength - pImg->iPosition,
				pImg);
	case imagetype_is_emf:
	case imagetype_is_wmf:
	case imagetype_is_pict:
		/* FIXME */
		return bAddDummyImage(pDiag, pImg);
	case imagetype_is_unknown:
	default:
		DBG_DEC(pImg->eImageType);
		return bAddDummyImage(pDiag, pImg);
	}
} /* end of bTranslateImage */
