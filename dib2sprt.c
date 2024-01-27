/*
 * dib2sprt.c
 * Copyright (C) 2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Functions to translate dib pictures into sprites
 */

#include <stdio.h>
#include <string.h>
#include "wimpt.h"
#include "antiword.h"

#if 0 /* defined(DEBUG) */
static int iPicCounter = 0;
#endif /* DEBUG */


/*
 * iGetByteWidth - compute the number of bytes needed for a row of pixels
 */
static int
iGetByteWidth(const imagedata_type *pImg)
{
	switch (pImg->iBitsPerComponent) {
	case  1:
		return (pImg->iWidth + 31) / 32 * sizeof(int);
	case  4:
		return (pImg->iWidth + 7) / 8 * sizeof(int);
	case  8:
	case 24:
		return (pImg->iWidth + 3) / 4 * sizeof(int);
	default:
		DBG_DEC(pImg->iBitsPerComponent);
		return 0;
	}
} /* end of iGetByteWidth */

/*
 * bCreateBlankSprite - Create a blank sprite.
 *
 * Create a blank sprite and add a palette if needed
 *
 * returns a pointer to the sprite when successful, otherwise NULL
 */
static unsigned char *
pucCreateBlankSprite(const imagedata_type *pImg, size_t *ptSize)
{
	sprite_area	*pArea;
	unsigned char	*pucTmp;
	size_t	tSize;
	int	iIndex, iMode, iPaletteEntries, iByteWidth;

	DBG_MSG("pCreateBlankSprite");

	fail(pImg == NULL);

	switch (pImg->iBitsPerComponent) {
	case  1:
		iMode = 18;
		iPaletteEntries = 2;
		break;
	case  4:
		iMode = 20;
		iPaletteEntries = 16;
		break;
	case  8:
	case 24:
		iMode = 21;
		iPaletteEntries = 0;
		break;
	default:
		DBG_DEC(pImg->iBitsPerComponent);
		return NULL;
	}
	fail(iPaletteEntries < 0 || iPaletteEntries > 16);

	/* Create the sprite */

	iByteWidth = iGetByteWidth(pImg);

	tSize = sizeof(sprite_area) +
		sizeof(sprite_header) +
		iPaletteEntries * 8 +
		iByteWidth * pImg->iHeight;
	DBG_DEC(tSize);
	pArea = xmalloc(tSize);
	sprite_area_initialise(pArea, tSize);

	wimpt_noerr(sprite_create(pArea, "wordimage",
		iPaletteEntries > 0 ? sprite_haspalette : sprite_nopalette,
		pImg->iWidth, pImg->iHeight, iMode));

	/* Add the palette */

	pucTmp = (unsigned char *)pArea +
		sizeof(sprite_area) + sizeof(sprite_header);
	for (iIndex = 0; iIndex < iPaletteEntries; iIndex++) {
		/* First color */
		*pucTmp++ = 0;
		*pucTmp++ = pImg->aucPalette[iIndex][0];
		*pucTmp++ = pImg->aucPalette[iIndex][1];
		*pucTmp++ = pImg->aucPalette[iIndex][2];
		/* Second color */
		*pucTmp++ = 0;
		*pucTmp++ = pImg->aucPalette[iIndex][0];
		*pucTmp++ = pImg->aucPalette[iIndex][1];
		*pucTmp++ = pImg->aucPalette[iIndex][2];
	}

	if (ptSize != NULL) {
		*ptSize = tSize;
	}
	return (unsigned char *)pArea;
} /* end of pucCreateBlankSprite */

/*
 * iReduceColor - reduce from 24 bit to 8 bit color
 *
 * Reduce 24 bit true colors to RISC OS default 256 color palette
 *
 * returns the resulting color
 */
static int
iReduceColor(int iRed, int iGreen, int iBlue)
{
	int	iResult;

	iResult = (iBlue & 0x80) ? 0x80 : 0;
	iResult |= (iGreen & 0x80) ? 0x40 : 0;
	iResult |= (iGreen & 0x40) ? 0x20 : 0;
	iResult |= (iRed & 0x80) ? 0x10 : 0;
	iResult |= (iBlue & 0x40) ? 0x08 : 0;
	iResult |= (iRed & 0x40) ? 0x04 : 0;
	iResult |= ((iRed | iGreen | iBlue) & 0x20) ? 0x02 : 0;
	iResult |= ((iRed | iGreen | iBlue) & 0x10) ? 0x01 : 0;
	return iResult;
} /* end of iReduceColor */

/*
 * vDecode1bpp - decode an uncompressed 1 bit per pixel image
 */
static void
vDecode1bpp(FILE *pFile, unsigned char *pucData, const imagedata_type *pImg)
{
	int	iX, iY, iByteWidth, iOffset, iTmp, iEighthWidth, iPadding;
	unsigned char	ucTmp;

	iByteWidth = iGetByteWidth(pImg);

	iEighthWidth = (pImg->iWidth + 7) / 8;
	iPadding = MakeFour(iEighthWidth) - iEighthWidth;

	for (iY = pImg->iHeight - 1; iY >= 0; iY--) {
		for (iX = 0; iX < iEighthWidth; iX++) {
			iTmp = iNextByte(pFile);
			/* Reverse the bit order */
			ucTmp  = (iTmp & BIT(0)) ? BIT(7) : 0;
			ucTmp |= (iTmp & BIT(1)) ? BIT(6) : 0;
			ucTmp |= (iTmp & BIT(2)) ? BIT(5) : 0;
			ucTmp |= (iTmp & BIT(3)) ? BIT(4) : 0;
			ucTmp |= (iTmp & BIT(4)) ? BIT(3) : 0;
			ucTmp |= (iTmp & BIT(5)) ? BIT(2) : 0;
			ucTmp |= (iTmp & BIT(6)) ? BIT(1) : 0;
			ucTmp |= (iTmp & BIT(7)) ? BIT(0) : 0;
			iOffset = iY * iByteWidth + iX;
			*(pucData + iOffset) = ucTmp;
		}
		(void)iSkipBytes(pFile, iPadding);
	}
} /* end of vDecode1bpp */

/*
 * vDecode4bpp - decode an uncompressed 4 bits per pixel image
 */
static void
vDecode4bpp(FILE *pFile, unsigned char *pucData, const imagedata_type *pImg)
{
	int	iX, iY, iByteWidth, iOffset, iTmp, iHalfWidth, iPadding;
	unsigned char	ucTmp;

	iByteWidth = iGetByteWidth(pImg);

	iHalfWidth = (pImg->iWidth + 1) / 2;
	iPadding = MakeFour(iHalfWidth) - iHalfWidth;

	for (iY = pImg->iHeight - 1; iY >= 0; iY--) {
		for (iX = 0; iX < iHalfWidth; iX++) {
			iTmp = iNextByte(pFile);
			/* Reverse the nibble order */
			ucTmp = (iTmp & 0xf0) >> 4;
			ucTmp |= (iTmp & 0x0f) << 4;
			iOffset = iY * iByteWidth + iX;
			*(pucData + iOffset) = ucTmp;
		}
		(void)iSkipBytes(pFile, iPadding);
	}
} /* end of vDecode4bpp */

/*
 * vDecode8bpp - decode an uncompressed 8 bits per pixel image
 */
static void
vDecode8bpp(FILE *pFile, unsigned char *pucData, const imagedata_type *pImg)
{
	int	iX, iY, iByteWidth, iOffset, iIndex, iPadding;

	fail(pFile == NULL);
	fail(pucData == NULL);
	fail(pImg == NULL);

	iByteWidth = iGetByteWidth(pImg);

	iPadding = MakeFour(pImg->iWidth) - pImg->iWidth;

	for (iY = pImg->iHeight - 1; iY >= 0; iY--) {
		for (iX = 0; iX < pImg->iWidth; iX++) {
			iIndex = iNextByte(pFile);
			iOffset = iY * iByteWidth + iX;
			*(pucData + iOffset) = iReduceColor(
				pImg->aucPalette[iIndex][0],
				pImg->aucPalette[iIndex][1],
				pImg->aucPalette[iIndex][2]);
		}
		(void)iSkipBytes(pFile, iPadding);
	}
} /* end of vDecode8bpp */

/*
 * vDecode24bpp - decode an uncompressed 24 bits per pixel image
 */
static void
vDecode24bpp(FILE *pFile, unsigned char *pucData, const imagedata_type *pImg)
{
	int	iX, iY, iTripleWidth, iByteWidth, iOffset, iPadding;
	int	iRed, iGreen, iBlue;

	iByteWidth = iGetByteWidth(pImg);

	iTripleWidth = pImg->iWidth * 3;
	iPadding = MakeFour(iTripleWidth) - iTripleWidth;

	for (iY = pImg->iHeight - 1; iY >= 0; iY--) {
		for (iX = 0; iX < pImg->iWidth; iX++) {
			iBlue = iNextByte(pFile);
			iGreen = iNextByte(pFile);
			iRed = iNextByte(pFile);
			iOffset = iY * iByteWidth + iX;
			*(pucData + iOffset) =
					iReduceColor(iRed, iGreen, iBlue);
		}
		(void)iSkipBytes(pFile, iPadding);
	}
} /* end of vDecode24bpp */

/*
 * vDecodeRle4 - decode a RLE compressed 4 bits per pixel image
 */
static void
vDecodeRle4(FILE *pFile, unsigned char *pucData, const imagedata_type *pImg)
{
} /* end of vDecodeRle4 */

/*
 * vDecodeRle8 - decode a RLE compressed 8 bits per pixel image
 */
static void
vDecodeRle8(FILE *pFile, unsigned char *pucData, const imagedata_type *pImg)
{
} /* end of vDecodeRle8 */

#if 0 /* defined(DEBUG) */
static void
vCopy2File(unsigned char *pucSprite, size_t tSpriteSize)
{
	FILE	*pOutFile;
	int	iIndex;
	char	szFilename[30];

	sprintf(szFilename, "<Wimp$ScrapDir>.sprt%04d", ++iPicCounter);
	pOutFile = fopen(szFilename, "wb");
	if (pOutFile == NULL) {
		return;
	}
	DBG_MSG(szFilename);
	for (iIndex = 4; iIndex < (int)tSpriteSize; iIndex++) {
		if (putc(pucSprite[iIndex], pOutFile) == EOF) {
			break;
		}
	}
	(void)fclose(pOutFile);
	vSetFiletype(szFilename, FILETYPE_SPRITE);
} /* end of vCopy2File */
#endif /* DEBUG */

/*
 * vDecodeDIB - decode a dib picture
 */
static void
vDecodeDIB(diagram_type *pDiag, FILE *pFile, const imagedata_type *pImg)
{
	unsigned char	*pucSprite, *pucPalette, *pucData;
	size_t	tSpriteSize;
	int	iHeaderSize;

	/* Skip the bitmap info header */
	iHeaderSize = (int)ulNextLong(pFile);
	(void)iSkipBytes(pFile, iHeaderSize - 4);
	/* Skip the colortable */
	if (pImg->iBitsPerComponent <= 8) {
		(void)iSkipBytes(pFile,
			pImg->iColorsUsed * ((iHeaderSize > 12) ? 4 : 3));
	}

	/* Create an empty sprite */
	pucSprite = pucCreateBlankSprite(pImg, &tSpriteSize);
	pucPalette = pucSprite + sizeof(sprite_area) + sizeof(sprite_header);

	/* Add the pixel information */
	switch (pImg->iBitsPerComponent) {
	case  1:
		fail(pImg->eCompression != compression_none);
		pucData = pucPalette + 2 * 8;
		vDecode1bpp(pFile, pucData, pImg);
		break;
	case  4:
		fail(pImg->eCompression != compression_none &&
				pImg->eCompression != compression_rle4);
		pucData = pucPalette + 16 * 8;
		if (pImg->eCompression == compression_rle4) {
			vDecodeRle4(pFile, pucData, pImg);
		} else {
			vDecode4bpp(pFile, pucData, pImg);
		}
		break;
	case  8:
		fail(pImg->eCompression != compression_none &&
				pImg->eCompression != compression_rle8);
		pucData = pucPalette;
		if (pImg->eCompression == compression_rle8) {
			vDecodeRle8(pFile, pucData, pImg);
		} else {
			vDecode8bpp(pFile, pucData, pImg);
		}
		break;
	case 24:
		fail(pImg->eCompression != compression_none);
		pucData = pucPalette;
		vDecode24bpp(pFile, pucData, pImg);
		break;
	default:
		DBG_DEC(pImg->iBitsPerComponent);
		break;
	}

#if 0 /* defined(DEBUG) */
	vCopy2File(pucSprite, tSpriteSize);
#endif /* DEBUG */

	/* Add the sprite to the Draw file */
	vImage2Diagram(pDiag,
		lPoints2DrawUnits(pImg->iHorizontalSize),
		lPoints2DrawUnits(pImg->iVerticalSize),
		pucSprite + sizeof(sprite_area),
		tSpriteSize - sizeof(sprite_area));

	/* Clean up before you leave */
	pucSprite = xfree(pucSprite);
} /* end of vDecodeDIB */

/*
 * bTranslateDIB - translate a DIB picture
 *
 * This function translates a picture from dib to sprite
 *
 * return TRUE when sucessful, otherwise FALSE
 */
BOOL
bTranslateDIB(diagram_type *pDiag, FILE *pFile,
		long lFileOffset, const imagedata_type *pImg)
{
	/* seek to start position of DIB data */
	if (!bSetDataOffset(pFile, lFileOffset)) {
		return FALSE;
	}

	vDecodeDIB(pDiag, pFile, pImg);

	return TRUE;
} /* end of bTranslateDIB */
