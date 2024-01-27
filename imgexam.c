/*
 * imgexam.c
 * Copyright (C) 2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Functions to examine image headers
 *
 *================================================================
 * Part of this software is based on:
 * jpeg2ps - convert JPEG compressed images to PostScript Level 2
 * Copyright (C) 1994-99 Thomas Merz (tm@muc.de)
 *================================================================
 * The credit should go to him, but all the bugs are mine.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "antiword.h"

/* BMP compression types */
#define BI_RGB		0
#define BI_RLE8		1
#define BI_RLE4		2

/* PNG colortype bits */
#define PNG_CB_PALETTE		0x01
#define PNG_CB_COLOR		0x02
#define PNG_CB_ALPHA		0x04

/* The following enum is stolen from the IJG JPEG library */
typedef enum {		/* JPEG marker codes			*/
	M_SOF0	= 0xc0,	/* baseline DCT				*/
	M_SOF1	= 0xc1,	/* extended sequential DCT		*/
	M_SOF2	= 0xc2,	/* progressive DCT			*/
	M_SOF3	= 0xc3,	/* lossless (sequential)		*/

	M_SOF5	= 0xc5,	/* differential sequential DCT		*/
	M_SOF6	= 0xc6,	/* differential progressive DCT		*/
	M_SOF7	= 0xc7,	/* differential lossless		*/

	M_JPG	= 0xc8,	/* JPEG extensions			*/
	M_SOF9	= 0xc9,	/* extended sequential DCT		*/
	M_SOF10	= 0xca,	/* progressive DCT			*/
	M_SOF11	= 0xcb,	/* lossless (sequential)		*/

	M_SOF13	= 0xcd,	/* differential sequential DCT		*/
	M_SOF14	= 0xce,	/* differential progressive DCT		*/
	M_SOF15	= 0xcf,	/* differential lossless		*/

	M_DHT	= 0xc4,	/* define Huffman tables		*/

	M_DAC	= 0xcc,	/* define arithmetic conditioning table	*/

	M_RST0	= 0xd0,	/* restart				*/
	M_RST1	= 0xd1,	/* restart				*/
	M_RST2	= 0xd2,	/* restart				*/
	M_RST3	= 0xd3,	/* restart				*/
	M_RST4	= 0xd4,	/* restart				*/
	M_RST5	= 0xd5,	/* restart				*/
	M_RST6	= 0xd6,	/* restart				*/
	M_RST7	= 0xd7,	/* restart				*/

	M_SOI	= 0xd8,	/* start of image			*/
	M_EOI	= 0xd9,	/* end of image				*/
	M_SOS	= 0xda,	/* start of scan			*/
	M_DQT	= 0xdb,	/* define quantization tables		*/
	M_DNL	= 0xdc,	/* define number of lines		*/
	M_DRI	= 0xdd,	/* define restart interval		*/
	M_DHP	= 0xde,	/* define hierarchical progression	*/
	M_EXP	= 0xdf,	/* expand reference image(s)		*/

	M_APP0	= 0xe0,	/* application marker, used for JFIF	*/
	M_APP1	= 0xe1,	/* application marker			*/
	M_APP2	= 0xe2,	/* application marker			*/
	M_APP3	= 0xe3,	/* application marker			*/
	M_APP4	= 0xe4,	/* application marker			*/
	M_APP5	= 0xe5,	/* application marker			*/
	M_APP6	= 0xe6,	/* application marker			*/
	M_APP7	= 0xe7,	/* application marker			*/
	M_APP8	= 0xe8,	/* application marker			*/
	M_APP9	= 0xe9,	/* application marker			*/
	M_APP10	= 0xea,	/* application marker			*/
	M_APP11	= 0xeb,	/* application marker			*/
	M_APP12	= 0xec,	/* application marker			*/
	M_APP13	= 0xed,	/* application marker			*/
	M_APP14	= 0xee,	/* application marker, used by Adobe	*/
	M_APP15	= 0xef,	/* application marker			*/

	M_JPG0	= 0xf0,	/* reserved for JPEG extensions		*/
	M_JPG13	= 0xfd,	/* reserved for JPEG extensions		*/
	M_COM	= 0xfe,	/* comment				*/

	M_TEM	= 0x01	/* temporary use			*/
} JPEG_MARKER;


/*
 * bFillPaletteDIB - fill the palette part of the imagesdata
 *
 * returns TRUE if the images must be a color image, otherwise FALSE;
 */
static BOOL
bFillPaletteDIB(FILE *pFile, imagedata_type *pImg, BOOL bNewFormat)
{
	int	iIndex;
	BOOL	bIsColorPalette;

	fail(pFile == NULL);
	fail(pImg == NULL);

	if (pImg->iBitsPerComponent > 8) {
		/* No palette, images uses more than 256 colors */
		return TRUE;
	}

	if (pImg->iColorsUsed <= 0) {
		pImg->iColorsUsed = 1 << pImg->iBitsPerComponent;
	}

	fail(pImg->iColorsUsed > 256);
	if (pImg->iColorsUsed > 256) {
		pImg->iColorsUsed = 256;
	}

	bIsColorPalette = FALSE;
	for (iIndex = 0; iIndex < pImg->iColorsUsed; iIndex++) {
		/* From BGR order to RGB order */
		pImg->aucPalette[iIndex][2] = (unsigned char)iNextByte(pFile);
		pImg->aucPalette[iIndex][1] = (unsigned char)iNextByte(pFile);
		pImg->aucPalette[iIndex][0] = (unsigned char)iNextByte(pFile);
		if (bNewFormat) {
			(void)iNextByte(pFile);
		}
		NO_DBG_PRINT_BLOCK(pImg->aucPalette[iIndex], 3);
		if (pImg->aucPalette[iIndex][0] !=
		     pImg->aucPalette[iIndex][1] ||
		    pImg->aucPalette[iIndex][1] !=
		     pImg->aucPalette[iIndex][2]) {
			bIsColorPalette = TRUE;
		}
	}

	return bIsColorPalette;
} /* end of bFillPaletteDIB */

/*
 * bExamineDIB - Examine a DIB header
 *
 * return TRUE if successful, otherwise FALSE
 */
static BOOL
bExamineDIB(FILE *pFile, imagedata_type *pImg)
{
	size_t	tHeaderSize;
	int	iPlanes, iCompression;

	tHeaderSize = (size_t)ulNextLong(pFile);
	switch (tHeaderSize) {
	case 12:
		pImg->iWidth = (int)usNextWord(pFile);
		pImg->iHeight = (int)usNextWord(pFile);
		iPlanes = (int)usNextWord(pFile);
		pImg->iBitsPerComponent = (int)usNextWord(pFile);
		iCompression = BI_RGB;
		pImg->iColorsUsed = 0;
		break;
	case 40:
	case 64:
		pImg->iWidth = (int)ulNextLong(pFile);
		pImg->iHeight = (int)ulNextLong(pFile);
		iPlanes = (int)usNextWord(pFile);
		pImg->iBitsPerComponent = (int)usNextWord(pFile);
		iCompression = (int)ulNextLong(pFile);
		(void)iSkipBytes(pFile, 12);
		pImg->iColorsUsed = (int)ulNextLong(pFile);
		(void)iSkipBytes(pFile, tHeaderSize - 36);
		break;
	default:
		DBG_DEC(tHeaderSize);
		return FALSE;
	}
	DBG_DEC(pImg->iWidth);
	DBG_DEC(pImg->iHeight);
	DBG_DEC(pImg->iBitsPerComponent);
	DBG_DEC(iCompression);
	DBG_DEC(pImg->iColorsUsed);

	/* Do some sanity checks with the parameters */
	if (iPlanes != 1) {
		DBG_DEC(iPlanes);
		return FALSE;
	}
	if (pImg->iWidth <= 0 || pImg->iHeight <= 0) {
		DBG_DEC(pImg->iWidth);
		DBG_DEC(pImg->iHeight);
		return FALSE;
	}
	if (pImg->iBitsPerComponent != 1 && pImg->iBitsPerComponent != 4 &&
	    pImg->iBitsPerComponent != 8 && pImg->iBitsPerComponent != 24) {
		DBG_DEC(pImg->iBitsPerComponent);
		return FALSE;
	}
	if (iCompression != BI_RGB &&
	    (pImg->iBitsPerComponent == 1 || pImg->iBitsPerComponent == 24)) {
		return FALSE;
	}
	if (iCompression == BI_RLE8 && pImg->iBitsPerComponent == 4) {
		return FALSE;
	}
	if (iCompression == BI_RLE4 && pImg->iBitsPerComponent == 8) {
		return FALSE;
	}

	switch (iCompression) {
	case BI_RGB:
		pImg->eCompression = compression_none;
		break;
	case BI_RLE4:
		pImg->eCompression = compression_rle4;
		break;
	case BI_RLE8:
		pImg->eCompression = compression_rle8;
		break;
	default:
		DBG_DEC(iCompression);
		return FALSE;
	}

	pImg->bColorImage = bFillPaletteDIB(pFile, pImg, tHeaderSize > 12);

	if (pImg->iBitsPerComponent <= 8) {
		pImg->iComponents = 1;
	} else {
		pImg->iComponents = pImg->iBitsPerComponent / 8;
	}

	return TRUE;
} /* end of bExamineDIB */

/*
 * iNextMarker - read the next JPEG marker
 */
static int
iNextMarker(FILE *pFile)
{
	int	iMarker;

	do {
		do {
			iMarker = iNextByte(pFile);
		} while (iMarker != 0xff && iMarker != EOF);
		if (iMarker == EOF) {
			return EOF;
		}
		do {
			iMarker = iNextByte(pFile);
		} while (iMarker == 0xff);
	} while (iMarker == 0x00);			/* repeat if ff/00 */

	return iMarker;
} /* end of iNextMarker */

/*
 * bExamineJPEG - Examine a JPEG header
 *
 * return TRUE if successful, otherwise FALSE
 */
static BOOL
bExamineJPEG(FILE *pFile, imagedata_type *pImg)
{
	size_t	tLength;
	int	iMarker, iIndex;
	char	appstring[10];
	BOOL	bSOFDone;

	tLength = 0;
	bSOFDone = FALSE;

	/* process JPEG markers */
	while (!bSOFDone && (iMarker = iNextMarker(pFile)) != (int)M_EOI) {
		switch (iMarker) {
		case EOF:
			DBG_MSG("Error: unexpected end of JPEG file");
			return FALSE;
	/* The following are not officially supported in PostScript level 2 */
		case M_SOF2:
		case M_SOF3:
		case M_SOF5:
		case M_SOF6:
		case M_SOF7:
		case M_SOF9:
		case M_SOF10:
		case M_SOF11:
		case M_SOF13:
		case M_SOF14:
		case M_SOF15:
			DBG_HEX(iMarker);
			return FALSE;
		case M_SOF0:
		case M_SOF1:
			tLength = (size_t)usNextWordBE(pFile);
			pImg->iBitsPerComponent = iNextByte(pFile);
			pImg->iHeight = (int)usNextWordBE(pFile);
			pImg->iWidth = (int)usNextWordBE(pFile);
			pImg->iComponents = iNextByte(pFile);
			bSOFDone = TRUE;
			break;
		case M_APP14:
		/*
		 * Check for Adobe application marker. It is known (per Adobe's
		 * TN5116) to contain the string "Adobe" at the start of the
		 * APP14 marker.
		 */
			tLength = (size_t)usNextWordBE(pFile);
			if (tLength < 12) {
				(void)iSkipBytes(pFile, tLength - 2);
			} else {
				for (iIndex = 0; iIndex < 5; iIndex++) {
					appstring[iIndex] =
							(char)iNextByte(pFile);
				}
				appstring[5] = '\0';
				if (STREQ(appstring, "Adobe")) {
					pImg->bAdobe = TRUE;
				}
				(void)iSkipBytes(pFile, tLength - 7);
			}
			break;
		case M_SOI:		/* ignore markers without parameters */
		case M_EOI:
		case M_TEM:
		case M_RST0:
		case M_RST1:
		case M_RST2:
		case M_RST3:
		case M_RST4:
		case M_RST5:
		case M_RST6:
		case M_RST7:
			break;
		default:		/* skip variable length markers */
			tLength = (size_t)usNextWordBE(pFile);
			(void)iSkipBytes(pFile, tLength - 2);
			break;
		}
	}

	DBG_DEC(pImg->iWidth);
	DBG_DEC(pImg->iHeight);
	DBG_DEC(pImg->iBitsPerComponent);
	DBG_DEC(pImg->iComponents);

	/* Do some sanity checks with the parameters */
	if (pImg->iHeight <= 0 ||
	    pImg->iWidth <= 0 ||
	    pImg->iComponents <= 0) {
		DBG_DEC(pImg->iHeight);
		DBG_DEC(pImg->iWidth);
		DBG_DEC(pImg->iComponents);
		return FALSE;
	}

	/* Some broken JPEG files have this but they print anyway... */
	if (pImg->iComponents * 3 + 8 != (int)tLength) {
		DBG_MSG("Warning: SOF marker has incorrect length - ignored");
	}

	if (pImg->iBitsPerComponent != 8) {
		DBG_DEC(pImg->iBitsPerComponent);
		DBG_MSG("Not supported in PostScript level 2");
		return FALSE;
	}

	if (pImg->iComponents != 1 &&
	    pImg->iComponents != 3 &&
	    pImg->iComponents != 4) {
		DBG_DEC(pImg->iComponents);
		return FALSE;
	}

	pImg->bColorImage = pImg->iComponents >= 3;
	pImg->iColorsUsed = 0;
	pImg->eCompression = compression_jpeg;

	return TRUE;
} /* end of bExamineJPEG */

/*
 * bFillPalettePNG - fill the palette part of the imagesdata
 *
 * returns TRUE if sucessful, otherwise FALSE;
 */
static BOOL
bFillPalettePNG(FILE *pFile, imagedata_type *pImg, size_t tLength)
{
	int	iIndex, iEntries;

	fail(pFile == NULL);
	fail(pImg == NULL);

	if (pImg->iBitsPerComponent > 8) {
		/* No palette, image uses more than 256 colors */
		return TRUE;
	}

	if (!pImg->bColorImage) {
		/* Only color images can have a palette */
		return FALSE;
	}

	if (tLength % 3 != 0) {
		/* Each palette entry takes three bytes */
		DBG_DEC(tLength);
		return FALSE;
	}

	iEntries = tLength / 3;
	DBG_DEC(iEntries);
	pImg->iColorsUsed = 1 << pImg->iBitsPerComponent;
	DBG_DEC(pImg->iColorsUsed);

	if (iEntries > 256) {
		DBG_DEC(iEntries);
		return FALSE;
	}

	for (iIndex = 0; iIndex < iEntries; iIndex++) {
		pImg->aucPalette[iIndex][0] = (unsigned char)iNextByte(pFile);
		pImg->aucPalette[iIndex][1] = (unsigned char)iNextByte(pFile);
		pImg->aucPalette[iIndex][2] = (unsigned char)iNextByte(pFile);
		NO_DBG_PRINT_BLOCK(pImg->aucPalette[iIndex], 3);
	}
	for (;iIndex < pImg->iColorsUsed; iIndex++) {
		pImg->aucPalette[iIndex][0] = 0;
		pImg->aucPalette[iIndex][1] = 0;
		pImg->aucPalette[iIndex][2] = 0;
	}

	return TRUE;
} /* end of bFillPalettePNG */

/*
 * bExaminePNG - Examine a PNG header
 *
 * return TRUE if successful, otherwise FALSE
 */
static BOOL
bExaminePNG(FILE *pFile, imagedata_type *pImg)
{
	size_t		tLength;
	unsigned long	ulLong1, ulLong2, ulName;
	int		iIndex, iTmp;
	int		iCompressionMethod, iFilterMethod, iInterlaceMethod;
	int		iColor, iIncrement;
	BOOL		bHasPalette, bHasAlpha;
	unsigned char	aucBuf[4];

	/* Check signature */
	ulLong1 = ulNextLongBE(pFile);
	ulLong2 = ulNextLongBE(pFile);
	if (ulLong1 != 0x89504e47 || ulLong2 != 0x0d0a1a0a) {
		DBG_HEX(ulLong1);
		DBG_HEX(ulLong2);
		return FALSE;
	}

	ulName = 0x00;
	bHasPalette = FALSE;

	/* Examine chunks */
	while (ulName != PNG_CN_IEND) {
		tLength = (size_t)ulNextLongBE(pFile);
		ulName = 0x00;
		for (iIndex = 0; iIndex < (int)elementsof(aucBuf); iIndex++) {
			aucBuf[iIndex] = (unsigned char)iNextByte(pFile);
			if (!isalpha(aucBuf[iIndex])) {
				DBG_HEX(aucBuf[iIndex]);
				return FALSE;
			}
			ulName <<= 8;
			ulName |= aucBuf[iIndex];
		}

		switch (ulName) {
		case PNG_CN_IHDR:
			/* Header chunck */
			if (tLength < 13) {
				DBG_DEC(tLength);
				return FALSE;
			}
			pImg->iWidth = (int)ulNextLongBE(pFile);
			pImg->iHeight = (int)ulNextLongBE(pFile);
			pImg->iBitsPerComponent = iNextByte(pFile);
			iTmp = iNextByte(pFile);
			NO_DBG_HEX(iTmp);
			pImg->bColorImage = (iTmp & PNG_CB_COLOR) != 0;
			bHasPalette = (iTmp & PNG_CB_PALETTE) != 0;
			bHasAlpha = (iTmp & PNG_CB_ALPHA) != 0;
			if (bHasPalette && pImg->iBitsPerComponent > 8) {
				/* This should not happen */
				return FALSE;
			}
			pImg->iComponents =
				(bHasPalette || !pImg->bColorImage) ? 1 : 3;
			if (bHasAlpha) {
				pImg->iComponents++;
			}
			iCompressionMethod = iNextByte(pFile);
			if (iCompressionMethod != 0) {
				DBG_DEC(iCompressionMethod);
				return FALSE;
			}
			iFilterMethod = iNextByte(pFile);
			if (iFilterMethod != 0) {
				DBG_DEC(iFilterMethod);
				return FALSE;
			}
			iInterlaceMethod = iNextByte(pFile);
			if (iInterlaceMethod != 0) {
				DBG_DEC(iInterlaceMethod);
				return FALSE;
			}
			pImg->iColorsUsed = 0;
			(void)iSkipBytes(pFile, tLength - 13 + 4);
			break;
		case PNG_CN_PLTE:
			if (!bHasPalette) {
				return FALSE;
			}
			if (!bFillPalettePNG(pFile, pImg, tLength)) {
				return FALSE;
			}
			(void)iSkipBytes(pFile, 4);
			break;
		default:
			(void)iSkipBytes(pFile, tLength + 4);
			break;
		}
	}

	DBG_DEC(pImg->iWidth);
	DBG_DEC(pImg->iHeight);
	DBG_DEC(pImg->iBitsPerComponent);
	DBG_DEC(pImg->iColorsUsed);
	DBG_DEC(pImg->iComponents);

	/* Do some sanity checks with the parameters */
	if (pImg->iWidth <= 0 || pImg->iHeight <= 0) {
		return FALSE;
	}

	if (pImg->iBitsPerComponent != 1 && pImg->iBitsPerComponent != 2 &&
	    pImg->iBitsPerComponent != 4 && pImg->iBitsPerComponent != 8 &&
	    pImg->iBitsPerComponent != 16) {
		DBG_DEC(pImg->iBitsPerComponent);
		return  FALSE;
	}

	if (pImg->iComponents != 1 && pImg->iComponents != 3) {
		/* Not supported */
		DBG_DEC(pImg->iComponents);
		return FALSE;
	}

	if (pImg->iBitsPerComponent > 8) {
		/* Not supported */
		DBG_DEC(pImg->iBitsPerComponent);
		return FALSE;
	}

	if (pImg->iColorsUsed == 0 &&
	    pImg->iComponents == 1 &&
	    pImg->iBitsPerComponent <= 4) {
		/*
		 * No palette is supplied, but Postscript needs one in these
		 * cases, so we add a default one here
		 */
		pImg->iColorsUsed = 1 << pImg->iBitsPerComponent;
		iIncrement = 0xff / (pImg->iColorsUsed - 1);
		for (iIndex = 0, iColor = 0x00;
		     iIndex < pImg->iColorsUsed;
		     iIndex++, iColor += iIncrement) {
			pImg->aucPalette[iIndex][0] = (unsigned char)iColor;
			pImg->aucPalette[iIndex][1] = (unsigned char)iColor;
			pImg->aucPalette[iIndex][2] = (unsigned char)iColor;
		}
		/* Just to be sure */
		pImg->bColorImage = FALSE;
	}

	pImg->eCompression = compression_zlib;

	return TRUE;
} /* end of bExaminePNG */

/*
 * iFind6Image - skip until the image is found
 *
 * Find the image in Word 6/7 files
 *
 * returns the new position when a image is found, otherwise -1
 */
static int
iFind6Image(FILE *pFile, int iPos, int iLen, imagetype_enum *peImageType)
{
	unsigned long	ulMarker;
	int		iElementLen, iToSkip;
	unsigned short	usMarker;

	fail(pFile == NULL);
	fail(iPos < 0);
	fail(iLen < 0);
	fail(peImageType == NULL);

	*peImageType = imagetype_is_unknown;
	if (iPos + 18 >= iLen) {
		return -1;
	}

	ulMarker = ulNextLong(pFile);
	if (ulMarker != 0x00090001) {
		DBG_HEX(ulMarker);
		return -1;
	}
	usMarker = usNextWord(pFile);
	if (usMarker != 0x0300) {
		DBG_HEX(usMarker);
		return -1;
	}
	(void)iSkipBytes(pFile, 10);
	usMarker = usNextWord(pFile);
	if (usMarker != 0x0000) {
		DBG_HEX(usMarker);
		return -1;
	}
	iPos += 18;

	while (iPos + 6 < iLen) {
		iElementLen = (int)ulNextLong(pFile);
		usMarker = usNextWord(pFile);
		iPos += 6;
		DBG_DEC(iElementLen);
		DBG_HEX(usMarker);
		if (iElementLen == 3) {
#if 0
			DBG_MSG("DIB");
			*peImageType = imagetype_is_dib;
			return iPos;
#else
			DBG_DEC(iPos);
			return -1;
#endif
		}

		switch (usMarker) {
		case 0x0b41:
			DBG_MSG("DIB");
			*peImageType = imagetype_is_dib;
			iPos += iSkipBytes(pFile, 20);
			return iPos;
		case 0x0f43:
			DBG_MSG("DIB");
			*peImageType = imagetype_is_dib;
			iPos += iSkipBytes(pFile, 22);
			return iPos;
		default:
			if (iElementLen > INT_MAX / 2 + 3) {
				/*
				 * This value is so big the number of bytes
				 * to skip can not be computed
				 */
				DBG_DEC(iElementLen);
				return -1;
			}
			iToSkip = (iElementLen - 3) * 2;
			if (iToSkip <= 0 || iToSkip > iLen - iPos) {
				/* You can't skip this number of bytes */
				DBG_DEC(iToSkip);
				DBG_DEC(iLen - iPos);
				return -1;
			}
			iPos += iSkipBytes(pFile, (size_t)iToSkip);
			break;
		}
	}

	return -1;
} /* end of iFind6Image */

/*
 * iFind8Image - skip until the image is found
 *
 * Find the image in Word 8/97 files
 *
 * returns the new position when a image is found, otherwise -1
 */
static int
iFind8Image(FILE *pFile, int iPos, int iLen, imagetype_enum *peImageType)
{
	int	iElementTag, iElementLen;
	unsigned short usID;

	fail(pFile == NULL);
	fail(iPos < 0);
	fail(iLen < 0);
	fail(peImageType == NULL);

	*peImageType = imagetype_is_unknown;
	while (iPos + 8 < iLen) {
		usID = usNextWord(pFile) >> 4;
		iElementTag = (int)usNextWord(pFile);
		iElementLen = (int)ulNextLong(pFile);
		iPos += 8;
		NO_DBG_HEX(usID);
		NO_DBG_HEX(iElementTag);
		NO_DBG_DEC(iElementLen);
		switch (iElementTag) {
		case 0xf001:
		case 0xf002:
		case 0xf003:
		case 0xf004:
		case 0xf005:
			break;
		case 0xf007:
			iPos += iSkipBytes(pFile, 36);
			break;
		case 0xf008:
			iPos += iSkipBytes(pFile, 8);
			break;
		case 0xf009:
			iPos += iSkipBytes(pFile, 16);
			break;
		case 0xf000:
		case 0xf006:
		case 0xf00a:
		case 0xf00b:
		case 0xf00d:
		case 0xf00e:
		case 0xf00f:
		case 0xf010:
		case 0xf011:
			if (iElementLen < 0) {
				DBG_DEC(iElementLen);
				return -1;
			}
			iPos += iSkipBytes(pFile, (size_t)iElementLen);
			break;
		case 0xf01a:
			DBG_MSG("EMF");
			*peImageType = imagetype_is_emf;
			iPos += iSkipBytes(pFile, usID == 0x3d4 ? 50 : 66);
			return iPos;
		case 0xf01b:
			DBG_MSG("WMF");
			*peImageType = imagetype_is_wmf;
			iPos += iSkipBytes(pFile, usID == 0x216 ? 50 : 66);
			return iPos;
		case 0xf01c:
			DBG_MSG("PICT");
			*peImageType = imagetype_is_pict;
			iPos += iSkipBytes(pFile, usID == 0x542 ? 17 : 33) ;
			return iPos;
		case 0xf01d:
			DBG_MSG("JPEG");
			*peImageType = imagetype_is_jpeg;
			iPos += iSkipBytes(pFile, usID == 0x46a ? 17 : 33);
			return iPos;
		case 0xf01e:
			DBG_MSG("PNG");
			*peImageType = imagetype_is_png;
			iPos += iSkipBytes(pFile, usID == 0x6e0 ? 17 : 33);
			return iPos;
		case 0xf01f:
			DBG_MSG("DIB");
			/* DIB is a BMP minus its 14 byte header */
			*peImageType = imagetype_is_dib;
			iPos += iSkipBytes(pFile, usID == 0x7a8 ? 17 : 33);
			return iPos;
		case 0xf00c:
		default:
			DBG_HEX(iElementTag);
			return -1;
		}
	}

	return -1;
} /* end of iFind8Image */

/*
 * eExamineImage - Examine the image
 *
 * Returns an indication of the amount of information found
 */
image_info_enum
eExamineImage(FILE *pFile, long lFileOffsetImage, imagedata_type *pImg)
{
	long	lTmp;
	size_t	tWordHeaderLen;
	int	iLen, iPos;
	int	iType, iHorizontalSize, iVerticalSize;
	unsigned short	usHorizontalScalingFactor, usVerticalScalingFactor;

	if (lFileOffsetImage < 0) {
		return image_no_information;
	}
	DBG_HEX(lFileOffsetImage);

	if (!bSetDataOffset(pFile, lFileOffsetImage)) {
		return image_no_information;
	}

	iLen = (int)ulNextLong(pFile);
	DBG_DEC(iLen);
	if (iLen < 58) {
		/* Smaller than the smallest known header */
		return image_no_information;
	}
	tWordHeaderLen = (size_t)usNextWord(pFile);
	DBG_DEC(tWordHeaderLen);
	fail(tWordHeaderLen != 58 && tWordHeaderLen != 68);

	if (iLen < (int)tWordHeaderLen) {
		/* Smaller than the current header */
		return image_no_information;
	}
	iType = (int)usNextWord(pFile);
	DBG_DEC(iType);
	(void)iSkipBytes(pFile, 28 - 8);

	lTmp = lTwips2MilliPoints(usNextWord(pFile));
	iHorizontalSize = (int)(lTmp / 1000);
	if (lTmp % 1000 != 0) {
		iHorizontalSize++;
	}
	DBG_DEC(iHorizontalSize);
	lTmp = lTwips2MilliPoints(usNextWord(pFile));
	iVerticalSize = (int)(lTmp / 1000);
	if (lTmp % 1000 != 0) {
		iVerticalSize++;
	}
	DBG_DEC(iVerticalSize);

	usHorizontalScalingFactor = usNextWord(pFile);
	DBG_DEC(usHorizontalScalingFactor);
	usVerticalScalingFactor = usNextWord(pFile);
	DBG_DEC(usVerticalScalingFactor);
	
	/* Sanity checks */
	lTmp = (long)iHorizontalSize * (long)usHorizontalScalingFactor;
	if (lTmp < 2835) {
		/* This image would be less than 1 millimeter wide */
		DBG_DEC(lTmp);
		return image_no_information;
	}
	lTmp = (long)iVerticalSize * (long)usVerticalScalingFactor;
	if (lTmp < 2835) {
		/* This image would be less than 1 millimeter high */
		DBG_DEC(lTmp);
		return image_no_information;
	}

	/* Skip the rest of the header */
	(void)iSkipBytes(pFile, tWordHeaderLen - 36);
	iPos = (int)tWordHeaderLen;

	(void)memset(pImg, 0, sizeof(*pImg));

	switch (iType) {
	case   7:
	case   8:
		iPos = iFind6Image(pFile, iPos, iLen, &pImg->eImageType);
		if (iPos < 0) {
			/* No image found */
			return image_no_information;
		}
		DBG_HEX(iPos);
		break;
	case  94:
		DBG_HEX(lFileOffsetImage + iPos);
		return image_no_information;
	case 100:
		iPos = iFind8Image(pFile, iPos, iLen, &pImg->eImageType);
		if (iPos < 0) {
			/* No image found */
			return image_no_information;
		}
		DBG_HEX(iPos);
		break;
	case 102:
		DBG_HEX(lFileOffsetImage + iPos);
		return image_no_information;
	default:
		DBG_DEC(iType);
		DBG_HEX(lFileOffsetImage + iPos);
		return image_no_information;
	}

	/* Minimal information is now available */
	pImg->iLength = iLen;
	pImg->iPosition = iPos;
	pImg->iHorizontalSize = iHorizontalSize;
	pImg->iVerticalSize = iVerticalSize;
	pImg->dHorizontalScalingFactor =
				(double)usHorizontalScalingFactor / 1000.0;
	pImg->dVerticalScalingFactor = (double)usVerticalScalingFactor / 1000.0;

	/* Image type specific examinations */
	switch (pImg->eImageType) {
	case imagetype_is_dib:
		if (bExamineDIB(pFile, pImg)) {
			return image_full_information;
		}
		return image_minimal_information;
	case imagetype_is_jpeg:
		if (bExamineJPEG(pFile, pImg)) {
			return image_full_information;
		}
		return image_minimal_information;
	case imagetype_is_png:
		if (bExaminePNG(pFile, pImg)) {
			return image_full_information;
		}
		return image_minimal_information;
	case imagetype_is_emf:
	case imagetype_is_wmf:
	case imagetype_is_pict:
		return image_minimal_information;
	case imagetype_is_unknown:
	default:
		return image_no_information;
	}
} /* end of eExamineImage */
