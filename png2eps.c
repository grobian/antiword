/*
 * png2eps.c
 * Copyright (C) 2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Functions to translate png images into eps
 *
 */

#include <stdio.h>
#include <ctype.h>
#include "antiword.h"

#if defined(DEBUG)
static int	iPicCounter = 0;
#endif /* DEBUG */


/*
 * iSkipToData - skip until a IDAT chunk is found
 *
 * returns the length of the pixeldata or -1 in case of error
 */
static int
iSkipToData(FILE *pFile, int iMaxBytes, int *piSkipped)
{
	unsigned long	ulName, ulTmp;
	int	iDataLength, iCounter, iToSkip;

	fail(pFile == NULL);
	fail(iMaxBytes < 0);
	fail(piSkipped == NULL);
	fail(*piSkipped < 0);

	/* Examine chunks */
	while (*piSkipped + 8 < iMaxBytes) {
		iDataLength = (int)ulNextLongBE(pFile);
		if (iDataLength < 0) {
			DBG_DEC(iDataLength);
			return -1;
		}
		DBG_DEC(iDataLength);
		*piSkipped += 4;

		ulName = 0x00;
		for (iCounter = 0; iCounter < 4; iCounter++) {
			ulTmp = (unsigned long)iNextByte(pFile);
			if (!isalpha((int)ulTmp)) {
				DBG_HEX(ulTmp);
				return -1;
			}
			ulName <<= 8;
			ulName |= ulTmp;
		}
		DBG_HEX(ulName);
		*piSkipped += 4;

		if (ulName == PNG_CN_IEND) {
			break;
		}
		if (ulName == PNG_CN_IDAT) {
			return iDataLength;
		}

		iToSkip = iDataLength + 4;
		if (iToSkip >= iMaxBytes - *piSkipped) {
			DBG_DEC(iToSkip);
			DBG_DEC(iMaxBytes - *piSkipped);
			return -1;
		}
		(void)iSkipBytes(pFile, (size_t)iToSkip);
		*piSkipped += iToSkip;
	}

	return -1;
} /* end of iSkipToData */

/*
 * iFindFirstPixelData - find the first pixeldata if a PNG image
 *
 * returns the length of the pixeldata or -1 in case of error
 */
static int
iFindFirstPixelData(FILE *pFile, int iMaxBytes, int *piSkipped)
{
	fail(pFile == NULL);
	fail(iMaxBytes <= 0);
	fail(piSkipped == NULL);

	if (iMaxBytes < 8) {
		DBG_DEC(iMaxBytes);
		return -1;
	}

	/* Skip over the PNG signature */
	(void)iSkipBytes(pFile, 8);
	*piSkipped = 8;

	return iSkipToData(pFile, iMaxBytes, piSkipped);
} /* end of iFindFirstPixelData */

/*
 * iFindNextPixelData - find the next pixeldata if a PNG image
 *
 * returns the length of the pixeldata or -1 in case of error
 */
static int
iFindNextPixelData(FILE *pFile, int iMaxBytes, int *piSkipped)
{
	fail(pFile == NULL);
	fail(iMaxBytes <= 0);
	fail(piSkipped == NULL);

	if (iMaxBytes < 4) {
		DBG_DEC(iMaxBytes);
		return -1;
	}

	/* Skip over the crc */
	(void)iSkipBytes(pFile, 4);
	*piSkipped = 4;

	return iSkipToData(pFile, iMaxBytes, piSkipped);
} /* end of iFindNextPixelData */

#if defined(DEBUG)
/*
 * vCopy2File
 */
static void
vCopy2File(FILE *pFile, long lFileOffset, int iPictureLen)
{
	FILE	*pOutFile;
	int	iIndex, iTmp;
	char	szFilename[30];

	if (!bSetDataOffset(pFile, lFileOffset)) {
		return;
	}

	sprintf(szFilename, "/tmp/pic/pic%04d.png", ++iPicCounter);
	pOutFile = fopen(szFilename, "wb");
	if (pOutFile == NULL) {
		return;
	}
	for (iIndex = 0; iIndex < iPictureLen; iIndex++) {
		iTmp = iNextByte(pFile);
		if (putc(iTmp, pOutFile) == EOF) {
			break;
		}
	}
	(void)fclose(pOutFile);
} /* end of vCopy2File */
#endif /* DEBUG */

/*
 * bTranslatePNG - translate a PNG image
 *
 * This function translates an image from png to eps
 *
 * return TRUE when sucessful, otherwise FALSE
 */
BOOL
bTranslatePNG(diagram_type *pDiag, FILE *pFile,
	long lFileOffset, int iPictureLen, const imagedata_type *pImg)
{
	int	iMaxBytes, iDataLength, iSkipped;

#if defined(DEBUG)
	vCopy2File(pFile, lFileOffset, iPictureLen);
#endif /* DEBUG */

	/* Seek to start position of PNG data */
	if (!bSetDataOffset(pFile, lFileOffset)) {
		return FALSE;
	}

	iMaxBytes = iPictureLen;
	iDataLength = iFindFirstPixelData(pFile, iMaxBytes, &iSkipped);
	if (iDataLength < 0) {
		return FALSE;
	}

	vImagePrologue(pDiag, pImg);
	do {
		iMaxBytes -= iSkipped;
		vASCII85EncodeArray(pFile, pDiag->pOutFile, iDataLength);
		iMaxBytes -= iDataLength;
		iDataLength = iFindNextPixelData(pFile, iMaxBytes, &iSkipped);
	} while (iDataLength >= 0);
	vASCII85EncodeByte(pDiag->pOutFile, EOF);
	vImageEpilogue(pDiag);

	return TRUE;
} /* end of bTranslatePNG */
