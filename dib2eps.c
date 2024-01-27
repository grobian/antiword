/*
 * dib2eps.c
 * Copyright (C) 2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Functions to translate dib pictures into eps
 *
 *================================================================
 * This part of the software is based on:
 * The Windows Bitmap Decoder Class part of paintlib
 * Paintlib is copyright (c) 1996-2000 Ulrich von Zadow
 *================================================================
 * The credit should go to him, but all the bugs are mine.
 */

#include <stdio.h>
#include "antiword.h"


/*
 * vDecode1bpp - decode an uncompressed 1 bit per pixel image
 */
static void
vDecode1bpp(FILE *pFile, FILE *pOutFile, const imagedata_type *pImg)
{
	size_t	tPadding;
	int	iX, iY, iN, iByte, iTmp, iEighthWidth, iUse;

	DBG_MSG("vDecode1bpp");

	fail(pOutFile == NULL);
	fail(pImg == NULL);
	fail(pImg->iColorsUsed < 1 || pImg->iColorsUsed > 2);

	DBG_DEC(pImg->iWidth);
	DBG_DEC(pImg->iHeight);

	iEighthWidth = (pImg->iWidth + 7) / 8;
	tPadding = (size_t)(MakeFour(iEighthWidth) - iEighthWidth);

	for (iY = 0; iY < pImg->iHeight; iY++) {
		for (iX = 0; iX < iEighthWidth; iX++) {
			iByte = iNextByte(pFile);
			if (iByte == EOF) {
				DBG_DEC(iY);
				DBG_DEC(iX);
				vASCII85EncodeByte(pOutFile, EOF);
				return;
			}
			if (iX == iEighthWidth - 1 && pImg->iWidth % 8 != 0) {
				iUse = pImg->iWidth % 8;
			} else {
				iUse = 8;
			}
			for (iN = 0; iN < iUse; iN++) {
				switch (iN) {
				case 0: iTmp = (iByte & 0x80) / 128; break;
				case 1: iTmp = (iByte & 0x40) / 64; break;
				case 2: iTmp = (iByte & 0x20) / 32; break;
				case 3: iTmp = (iByte & 0x10) / 16; break;
				case 4: iTmp = (iByte & 0x08) / 8; break;
				case 5: iTmp = (iByte & 0x04) / 4; break;
				case 6: iTmp = (iByte & 0x02) / 2; break;
				case 7: iTmp = (iByte & 0x01); break;
				default: iTmp = 0; break;
				}
				vASCII85EncodeByte(pOutFile, iTmp);
			}
		}
		(void)iSkipBytes(pFile, tPadding);
	}
	vASCII85EncodeByte(pOutFile, EOF);
} /* end of vDecode1bpp */

/*
 * vDecode4bpp - decode an uncompressed 4 bits per pixel image
 */
static void
vDecode4bpp(FILE *pFile, FILE *pOutFile, const imagedata_type *pImg)
{
	size_t	tPadding;
	int	iX, iY, iN, iByte, iTmp, iHalfWidth, iUse;

	DBG_MSG("vDecode4bpp");

	fail(pFile == NULL);
	fail(pOutFile == NULL);
	fail(pImg == NULL);
	fail(pImg->iColorsUsed < 1 || pImg->iColorsUsed > 16);

	DBG_DEC(pImg->iWidth);
	DBG_DEC(pImg->iHeight);

	iHalfWidth = (pImg->iWidth + 1) / 2;
	tPadding = (size_t)(MakeFour(iHalfWidth) - iHalfWidth);

	for (iY = 0; iY < pImg->iHeight; iY++) {
		for (iX = 0; iX < iHalfWidth; iX++) {
			iByte = iNextByte(pFile);
			if (iByte == EOF) {
				DBG_DEC(iY);
				DBG_DEC(iX);
				vASCII85EncodeByte(pOutFile, EOF);
				return;
			}
			if (iX == iHalfWidth - 1 && odd(pImg->iWidth)) {
				iUse = 1;
			} else {
				iUse = 2;
			}
			for (iN = 0; iN < iUse; iN++) {
				if (odd(iN)) {
					iTmp = iByte & 0x0f;
				} else {
					iTmp = (iByte & 0xf0) / 16;
				}
				vASCII85EncodeByte(pOutFile, iTmp);
			}
		}
		(void)iSkipBytes(pFile, tPadding);
	}
	vASCII85EncodeByte(pOutFile, EOF);
} /* end of vDecode4bpp */

/*
 * vDecode8bpp - decode an uncompressed 8 bits per pixel image
 */
static void
vDecode8bpp(FILE *pFile, FILE *pOutFile, const imagedata_type *pImg)
{
	size_t	tPadding;
	int	iX, iY, iByte;

	DBG_MSG("vDecode8bpp");

	fail(pFile == NULL);
	fail(pOutFile == NULL);
	fail(pImg == NULL);
	fail(pImg->iColorsUsed < 1 || pImg->iColorsUsed > 256);

	DBG_DEC(pImg->iWidth);
	DBG_DEC(pImg->iHeight);

	tPadding = (size_t)(MakeFour(pImg->iWidth) - pImg->iWidth);

	for (iY = 0; iY < pImg->iHeight; iY++) {
		for (iX = 0; iX < pImg->iWidth; iX++) {
			iByte = iNextByte(pFile);
			if (iByte == EOF) {
				DBG_DEC(iY);
				DBG_DEC(iX);
				vASCII85EncodeByte(pOutFile, EOF);
				return;
			}
			vASCII85EncodeByte(pOutFile, iByte);
		}
		(void)iSkipBytes(pFile, tPadding);
	}
	vASCII85EncodeByte(pOutFile, EOF);
} /* end of vDecode8bpp */

/*
 * vDecode24bpp - decode an uncompressed 24 bits per pixel image
 */
static void
vDecode24bpp(FILE *pFile, FILE *pOutFile, const imagedata_type *pImg)
{
	size_t	tPadding;
	int	iX, iY, iBlue, iGreen, iRed, iTripleWidth;

	DBG_MSG("vDecode24bpp");

	fail(pFile == NULL);
	fail(pOutFile == NULL);
	fail(pImg == NULL);
	fail(!pImg->bColorImage);

	DBG_DEC(pImg->iWidth);
	DBG_DEC(pImg->iHeight);

	iTripleWidth = pImg->iWidth * 3;
	tPadding = (size_t)(MakeFour(iTripleWidth) - iTripleWidth);

	for (iY = 0; iY < pImg->iHeight; iY++) {
		for (iX = 0; iX < iTripleWidth; iX += 3) {
			/* Change from BGR order to RGB order */
			iBlue = iNextByte(pFile);
			if (iBlue == EOF) {
				iGreen = EOF;
				iRed = EOF;
			} else {
				iGreen = iNextByte(pFile);
				if (iGreen == EOF) {
					iRed = EOF;
				} else {
					iRed = iNextByte(pFile);
				}
			}
			if (iBlue == EOF || iGreen == EOF || iRed == EOF) {
				DBG_DEC(iY);
				DBG_DEC(iX);
				vASCII85EncodeByte(pOutFile, EOF);
				return;
			}
			vASCII85EncodeByte(pOutFile, iRed);
			vASCII85EncodeByte(pOutFile, iGreen);
			vASCII85EncodeByte(pOutFile, iBlue);
		}
		(void)iSkipBytes(pFile, tPadding);
	}
	vASCII85EncodeByte(pOutFile, EOF);
} /* end of vDecode24bpp */

/*
 * vDecodeRle4 - decode a RLE compressed 4 bits per pixel image
 */
static void
vDecodeRle4(FILE *pFile, FILE *pOutFile, const imagedata_type *pImg)
{
	int	iX, iY, iN, iByte, iTmp, iRunLength, iHalfRunLength, iUse;
	BOOL	bEOF, bEOL;

	DBG_MSG("vDecodeRle4");

	fail(pFile == NULL);
	fail(pOutFile == NULL);
	fail(pImg == NULL);
	fail(pImg->iColorsUsed < 1 || pImg->iColorsUsed > 16);

	DBG_DEC(pImg->iWidth);
	DBG_DEC(pImg->iHeight);

	bEOF = FALSE;

	for (iY =  0; iY < pImg->iHeight && !bEOF; iY++) {
		bEOL = FALSE;
		while (!bEOL) {
			iRunLength = iNextByte(pFile);
			if (iRunLength == 0) {
				/* Literal or escape */
				iRunLength = iNextByte(pFile);
				switch (iRunLength) {
				case 0:	/* End of line escape */
					bEOL = TRUE;
					break;
				case 1:	/* End of file escape */
					bEOF = TRUE;
					bEOL = TRUE;
					break;
				case 2:	/* Delta escape */
					DBG_MSG("Encountered delta escape");
					bEOF = TRUE;
					bEOL = TRUE;
					break;
				default:/* Literal packet*/
					iHalfRunLength = (iRunLength + 1) / 2;
					for (iX = 0;
					     iX < iHalfRunLength;
					     iX++) {
						if (iX == iHalfRunLength - 1 &&
						    odd(iRunLength)) {
							iUse = 1;
						} else {
							iUse = 2;
						}
						iByte = iNextByte(pFile);
						for (iN = 0; iN < iUse; iN++) {
							if (odd(iN)) {
								iTmp =
								iByte & 0x0f;
							} else {
								iTmp =
							(iByte & 0xf0) / 16;
							}
							vASCII85EncodeByte(
								pOutFile, iTmp);
						}
					}
					if (odd(iHalfRunLength)) {
						(void)iNextByte(pFile);
					}
					break;
				}
			} else {
				/*
				 * Encoded packet:
				 * RunLength pixels, all the "same" value
				 */
				iByte = iNextByte(pFile);
				for (iX = 0; iX < iRunLength; iX++) {
					if (odd(iX)) {
						iTmp = iByte & 0x0f;
					} else {
						iTmp = (iByte & 0xf0) / 16;
					}
					vASCII85EncodeByte(pOutFile, iTmp);
				}
			}
		}
	}
	vASCII85EncodeByte(pOutFile, EOF);
} /* end of vDecodeRle4 */

/*
 * vDecodeRle8 - decode a RLE compressed 8 bits per pixel image
 */
static void
vDecodeRle8(FILE *pFile, FILE *pOutFile, const imagedata_type *pImg)
{
	int	iX, iY, iByte, iRunLength;
	BOOL	bEOF, bEOL;

	DBG_MSG("vDecodeRle8");

	fail(pFile == NULL);
	fail(pOutFile == NULL);
	fail(pImg == NULL);
	fail(pImg->iColorsUsed < 1 || pImg->iColorsUsed > 256);

	DBG_DEC(pImg->iWidth);
	DBG_DEC(pImg->iHeight);

	bEOF = FALSE;

	for (iY = 0; iY < pImg->iHeight && !bEOF; iY++) {
		bEOL = FALSE;
		while (!bEOL) {
			iRunLength = iNextByte(pFile);
			if (iRunLength == 0) {
				/* Literal or escape */
				iRunLength = iNextByte(pFile);
				switch (iRunLength) {
				case 0:	/* End of line escape */
					bEOL = TRUE;
					break;
				case 1:	/* End of file escape */
					bEOF = TRUE;
					bEOL = TRUE;
					break;
				case 2:	/* Delta escape */
					DBG_MSG("Encountered delta escape");
					bEOF = TRUE;
					bEOL = TRUE;
					break;
				default:/* Literal packet*/
					for (iX = 0; iX < iRunLength; iX++) {
						iByte = iNextByte(pFile);
						vASCII85EncodeByte(pOutFile,
									iByte);
					}
					if (odd(iRunLength)) {
						(void)iNextByte(pFile);
					}
					break;
				}
			} else {
				/*
				 * Encoded packet:
				 * RunLength pixels, all the same value
				 */
				iByte = iNextByte(pFile);
				for (iX = 0; iX < iRunLength; iX++) {
					vASCII85EncodeByte(pOutFile, iByte);
				}
			}
		}
	}
	vASCII85EncodeByte(pOutFile, EOF);
} /* end of vDecodeRle8 */

/*
 * vDecodeDIB - decode a dib picture
 */
static void
vDecodeDIB(FILE *pFile, FILE *pOutFile, const imagedata_type *pImg)
{
	size_t	tHeaderSize;

	fail(pFile == NULL);
	fail(pOutFile == NULL);
	fail(pImg == NULL);

	/* Skip the bitmap info header */
	tHeaderSize = (size_t)ulNextLong(pFile);
	(void)iSkipBytes(pFile, tHeaderSize - 4);
	/* Skip the colortable */
	if (pImg->iBitsPerComponent <= 8) {
		(void)iSkipBytes(pFile,
			(size_t)(pImg->iColorsUsed *
			 ((tHeaderSize > 12) ? 4 : 3)));
	}

	switch (pImg->iBitsPerComponent) {
	case 1:
		fail(pImg->eCompression != compression_none);
		vDecode1bpp(pFile, pOutFile, pImg);
		break;
	case 4:
		fail(pImg->eCompression != compression_none &&
				pImg->eCompression != compression_rle4);
		if (pImg->eCompression == compression_rle4) {
			vDecodeRle4(pFile, pOutFile, pImg);
		} else {
			vDecode4bpp(pFile, pOutFile, pImg);
		}
		break;
	case 8:
		fail(pImg->eCompression != compression_none &&
				pImg->eCompression != compression_rle8);
		if (pImg->eCompression == compression_rle8) {
			vDecodeRle8(pFile, pOutFile, pImg);
		} else {
			vDecode8bpp(pFile, pOutFile, pImg);
		}
		break;
	case 24:
		fail(pImg->eCompression != compression_none);
		vDecode24bpp(pFile, pOutFile, pImg);
		break;
	default:
		DBG_DEC(pImg->iBitsPerComponent);
		break;
	}
} /* end of vDecodeDIB */

#if defined(DEBUG)
/*
 * vCopy2File
 */
static void
vCopy2File(FILE *pFile, long lFileOffset, int iPictureLen)
{
	static int	iPicCounter = 0;
	FILE	*pOutFile;
	int	iIndex, iTmp;
	char	szFilename[30];

	if (!bSetDataOffset(pFile, lFileOffset)) {
		return;
	}

	sprintf(szFilename, "/tmp/pic/pic%04d.bmp", ++iPicCounter);
	pOutFile = fopen(szFilename, "wb");
	if (pOutFile == NULL) {
		return;
	}
	/* Turn a dib into a bmp by adding a fake 14 byte header */
	(void)putc('B', pOutFile);
	(void)putc('M', pOutFile);
	for (iTmp = 0; iTmp < 12; iTmp++) {
		if (putc(0, pOutFile) == EOF) {
			break;
		}
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
 * bTranslateDIB - translate a DIB picture
 *
 * This function translates a picture from dib to eps
 *
 * return TRUE when sucessful, otherwise FALSE
 */
BOOL
bTranslateDIB(diagram_type *pDiag, FILE *pFile,
		long lFileOffset, const imagedata_type *pImg)
{
#if defined(DEBUG)
	vCopy2File(pFile, lFileOffset, pImg->iLength - pImg->iPosition);
#endif /* DEBUG */

	/* Seek to start position of DIB data */
	if (!bSetDataOffset(pFile, lFileOffset)) {
		return FALSE;
	}

	vImagePrologue(pDiag, pImg);
	vDecodeDIB(pFile, pDiag->pOutFile, pImg);
	vImageEpilogue(pDiag);

	return TRUE;
} /* end of bTranslateDIB */
