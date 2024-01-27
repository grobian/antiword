/*
 * paragraph.c
 * Copyright (C) 1998-2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Read the paragraph information from a MS Word file
 */

#include <stdlib.h>
#include <string.h>
#include "antiword.h"

typedef enum row_info_tag {
	found_nothing,
	found_a_cell,
	found_end_of_row
} row_info_enum;


/*
 * iGet6InfoLength - the length of the information for Word 6/7 files
 */
static int
iGet6InfoLength(int iByteNbr, const unsigned char *aucFpage)
{
	int	iTmp, iDel, iAdd;

	switch (ucGetByte(iByteNbr, aucFpage)) {
	case 0x02: case 0x10: case 0x11: case 0x13:
	case 0x15: case 0x16: case 0x1b: case 0x1c:
	case 0x26: case 0x27: case 0x28: case 0x29:
	case 0x2a: case 0x2b: case 0x2d: case 0x2e:
	case 0x2f: case 0x30: case 0x31: case 0x50:
	case 0x5d: case 0x60: case 0x61: case 0x63:
	case 0x65: case 0x69: case 0x6a: case 0x6b:
	case 0x90: case 0xa4: case 0xa5: case 0xb6:
	case 0xb8: case 0xbd: case 0xc3: case 0xc5:
	case 0xc6:
		return 3;
	case 0x03: case 0x0c: case 0x0f: case 0x51:
	case 0x67: case 0x6c: case 0xbc: case 0xbe:
	case 0xbf:
		return 2 + (int)ucGetByte(iByteNbr + 1, aucFpage);
	case 0x14: case 0x49: case 0x4a: case 0xc0:
	case 0xc2: case 0xc4: case 0xc8:
		return 5;
	case 0x17:
		iTmp = (int)ucGetByte(iByteNbr + 1, aucFpage);
		if (iTmp == 255) {
			iDel = (int)ucGetByte(iByteNbr + 2, aucFpage);
			iAdd = (int)ucGetByte(
					iByteNbr + 3 + iDel * 4, aucFpage);
			iTmp = 2 + iDel * 4 + iAdd * 3;
		}
		return 2 + iTmp;
	case 0x44: case 0xc1: case 0xc7:
		return 6;
	case 0x5f: case 0x88: case 0x89:
		return 4;
	case 0x78:
		return 14;
	case 0xbb:
		return 13;
	default:
		return 2;
	}
} /* end of iGet6InfoLength */

/*
 * Returns 2 and fills the row information block if end of a row was found
 *	   1 if a cell was found
 *	   0 nothing useful was found
 */
static row_info_enum
eGet6RowInfo(int iFodo, const unsigned char *aucFpage, row_block_type *pRow)
{
	int	iBytes, iFodoOff, iInfoLen;
	int	iIndex, iSize, iCol;
	int	iPosCurr, iPosPrev;
	BOOL	bFound18, bFound19, bFoundBE;

	fail(iFodo <= 0 || aucFpage == NULL || pRow == NULL);

	iBytes = 2 * (int)ucGetByte(iFodo, aucFpage);
	iFodoOff = 3;
	bFound18 = FALSE;
	bFound19 = FALSE;
	bFoundBE = FALSE;
	while (iBytes >= iFodoOff + 1) {
		iInfoLen = 0;
		switch (ucGetByte(iFodo + iFodoOff, aucFpage)) {
		case 0x18:
			if (ucGetByte(iFodo + iFodoOff + 1,
					aucFpage) == 0x01) {
				bFound18 = TRUE;
			}
			break;
		case 0x19:
			if (ucGetByte(iFodo + iFodoOff + 1,
					aucFpage) == 0x01) {
				bFound19 = TRUE;
			}
			break;
		case 0xbe:
			iSize = (int)ucGetByte(iFodo + iFodoOff + 1, aucFpage);
			if (iSize < 6 || iBytes < iFodoOff + 7) {
				DBG_DEC(iSize);
				DBG_DEC(iFodoOff);
				iInfoLen = 1;
				break;
			}
			iCol = (int)ucGetByte(iFodo + iFodoOff + 3, aucFpage);
			if (iCol < 1 ||
			    iBytes < iFodoOff + 3 + (iCol + 1) * 2) {
				DBG_DEC(iCol);
				DBG_DEC(iFodoOff);
				iInfoLen = 1;
				break;
			}
			if (iCol >= (int)elementsof(pRow->asColumnWidth)) {
				werr(1, "The number of columns is corrupt");
			}
			pRow->ucNumberOfColumns = (unsigned char)iCol;
			pRow->iColumnWidthSum = 0;
			iPosPrev = (int)(short)usGetWord(
					iFodo + iFodoOff + 4,
					aucFpage);
			for (iIndex = 0; iIndex < iCol; iIndex++) {
				iPosCurr = (int)(short)usGetWord(
					iFodo + iFodoOff + 6 + iIndex * 2,
					aucFpage);
				pRow->asColumnWidth[iIndex] =
						(short)(iPosCurr - iPosPrev);
				pRow->iColumnWidthSum +=
					pRow->asColumnWidth[iIndex];
				iPosPrev = iPosCurr;
			}
			bFoundBE = TRUE;
			break;
		default:
			break;
		}
		if (iInfoLen <= 0) {
			iInfoLen =
				iGet6InfoLength(iFodo + iFodoOff, aucFpage);
			fail(iInfoLen <= 0);
		}
		iFodoOff += iInfoLen;
	}
	if (bFound18 && bFound19 && bFoundBE) {
		return found_end_of_row;
	}
	if (bFound18) {
		return found_a_cell;
	}
	return found_nothing;
} /* end of eGet6RowInfo */

/*
 * Fill the style information block with information
 * from a Word 6/7 file.
 * Returns TRUE when successful, otherwise FALSE
 */
static BOOL
bGet6StyleInfo(int iFodo, const unsigned char *aucFpage,
		style_block_type *pStyle)
{
	int	iBytes, iFodoOff, iInfoLen;
	int	iTmp, iDel, iAdd;
	short	sTmp;

	fail(iFodo <= 0 || aucFpage == NULL || pStyle == NULL);

	iBytes = 2 * (int)ucGetByte(iFodo, aucFpage);
	iFodoOff = 3;
	if (iBytes < 1) {
		return FALSE;
	}
	(void)memset(pStyle, 0, sizeof(*pStyle));
	pStyle->ucStyle = ucGetByte(iFodo + 1, aucFpage);
	while (iBytes >= iFodoOff + 1) {
		iInfoLen = 0;
		switch (ucGetByte(iFodo + iFodoOff, aucFpage)) {
		case 0x05:
			pStyle->ucAlignment = ucGetByte(
					iFodo + iFodoOff + 1, aucFpage);
			break;
		case 0x0c:
			iTmp = (int)ucGetByte(
					iFodo + iFodoOff + 1, aucFpage);
			if (iTmp >= 1) {
				pStyle->ucListType = ucGetByte(
					iFodo + iFodoOff + 2, aucFpage);
				pStyle->bInList = TRUE;
			}
			if (iTmp >= 21) {
				pStyle->ucListCharacter = ucGetByte(
					iFodo + iFodoOff + 22, aucFpage);
			}
			break;
		case 0x0d:
			iTmp = (int)ucGetByte(
					iFodo + iFodoOff + 1, aucFpage);
			pStyle->bInList = TRUE;
			if (iTmp == 0x0c) {
				pStyle->bUnmarked = TRUE;
			}
			break;
		case 0x0f:
			iTmp = (int)ucGetByte(iFodo + iFodoOff + 1, aucFpage);
			if (iTmp < 2) {
				iInfoLen = 1;
				break;
			}
			NO_DBG_DEC(iTmp);
			iDel = (int)ucGetByte(iFodo + iFodoOff + 2, aucFpage);
			if (iTmp < 2 + 2 * iDel) {
				iInfoLen = 1;
				break;
			}
			NO_DBG_DEC(iDel);
			iAdd = (int)ucGetByte(
				iFodo + iFodoOff + 3 + 2 * iDel, aucFpage);
			if (iTmp < 2 + 2 * iDel + 2 * iAdd) {
				iInfoLen = 1;
				break;
			}
			NO_DBG_DEC(iAdd);
			break;
		case 0x10:
			pStyle->sRightIndent = (short)usGetWord(
					iFodo + iFodoOff + 1, aucFpage);
			NO_DBG_DEC(pStyle->sRightIndent);
			break;
		case 0x11:
			pStyle->sLeftIndent = (short)usGetWord(
					iFodo + iFodoOff + 1, aucFpage);
			DBG_DEC_C(pStyle->sLeftIndent < 0, pStyle->sLeftIndent);
			break;
		case 0x12:
			sTmp = (short)usGetWord(iFodo + iFodoOff + 1, aucFpage);
			pStyle->sLeftIndent += sTmp;
			if (pStyle->sLeftIndent < 0) {
				pStyle->sLeftIndent = 0;
			}
			NO_DBG_DEC(sTmp);
			NO_DBG_DEC(pStyle->sLeftIndent);
			break;
		default:
			break;
		}
		if (iInfoLen <= 0) {
			iInfoLen =
				iGet6InfoLength(iFodo + iFodoOff, aucFpage);
			fail(iInfoLen <= 0);
		}
		iFodoOff += iInfoLen;
	}
	return TRUE;
} /* end of bGet6StyleInfo */

/*
 * Build the lists with Paragraph Information for Word 6/7 files
 */
static void
vGet6ParInfo(FILE *pFile, int iStartBlock,
	const int *aiBBD, size_t tBBDLen,
	const unsigned char *aucHeader)
{
	row_block_type		tRow;
	style_block_type	tStyle;
	unsigned short	*ausParfPage;
	unsigned char	*aucBuffer;
	long	lParmOffset, lParmOffsetFirst, lParmOffsetLast;
	size_t	tBeginParfInfo, tParfInfoLen;
	size_t	tParfPageNum, tOffset, tSize, tLenOld, tLen;
	int	iIndex, iIndex2, iNbr, iFodo;
	unsigned short	usParfFirstPage, usCount;
	unsigned char	aucFpage[BIG_BLOCK_SIZE];

	fail(pFile == NULL || aucHeader == NULL);
	fail(iStartBlock < 0);
	fail(aiBBD == NULL);

	tBeginParfInfo = (size_t)ulGetLong(0xc0, aucHeader);
	NO_DBG_HEX(tBeginParfInfo);
	tParfInfoLen = (size_t)ulGetLong(0xc4, aucHeader);
	NO_DBG_DEC(tParfInfoLen);
	if (tParfInfoLen < 4) {
		DBG_DEC(tParfInfoLen);
		return;
	}

	aucBuffer = xmalloc(tParfInfoLen);
	if (!bReadBuffer(pFile, iStartBlock,
			aiBBD, tBBDLen, BIG_BLOCK_SIZE,
			aucBuffer, tBeginParfInfo, tParfInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return;
	}
	NO_DBG_PRINT_BLOCK(aucBuffer, tParfInfoLen);

	tLen = (tParfInfoLen - 4) / 6;
	tSize = tLen * sizeof(unsigned short);
	ausParfPage = xmalloc(tSize);
	for (iIndex = 0, tOffset = (tLen + 1) * 4;
	     iIndex < (int)tLen;
	     iIndex++, tOffset += 2) {
		 ausParfPage[iIndex] = usGetWord(tOffset, aucBuffer);
		 NO_DBG_DEC(ausParfPage[iIndex]);
	}
	DBG_HEX(ulGetLong(0, aucBuffer));
	aucBuffer = xfree(aucBuffer);
	aucBuffer = NULL;
	tParfPageNum = (size_t)usGetWord(0x190, aucHeader);
	DBG_DEC(tParfPageNum);
	if (tLen < tParfPageNum) {
		/* Replace ParfPage by a longer version */
		tLenOld = tLen;
		usParfFirstPage = usGetWord(0x18c, aucHeader);
		DBG_DEC(usParfFirstPage);
		tLen += tParfPageNum - 1;
		tSize = tLen * sizeof(unsigned short);
		ausParfPage = xrealloc(ausParfPage, tSize);
		/* Add new values */
		usCount = usParfFirstPage + 1;
		for (iIndex = (int)tLenOld; iIndex < (int)tLen; iIndex++) {
			ausParfPage[iIndex] = usCount;
			NO_DBG_DEC(ausParfPage[iIndex]);
			usCount++;
		}
	}

	(void)memset(&tStyle, 0, sizeof(tStyle));
	(void)memset(&tRow, 0, sizeof(tRow));
	lParmOffsetFirst = -1;
	for (iIndex = 0; iIndex < (int)tLen; iIndex++) {
		if (!bReadBuffer(pFile, iStartBlock,
				aiBBD, tBBDLen, BIG_BLOCK_SIZE,
				aucFpage,
				(size_t)ausParfPage[iIndex] * BIG_BLOCK_SIZE,
				BIG_BLOCK_SIZE)) {
			break;
		}
		iNbr = (int)ucGetByte(0x1ff, aucFpage);
		NO_DBG_DEC(iNbr);
		for (iIndex2 = 0; iIndex2 < iNbr; iIndex2++) {
			NO_DBG_HEX(ulGetLong(iIndex2 * 4, aucFpage));
			iFodo = 2 * (int)ucGetByte(
				(iNbr + 1) * 4 + iIndex2 * 7, aucFpage);
			if (iFodo <= 0) {
				continue;
			}
			if (bGet6StyleInfo(iFodo, aucFpage, &tStyle)) {
				lParmOffset = (long)ulGetLong(
						iIndex2 * 4, aucFpage);
				NO_DBG_HEX(lParmOffset);
				tStyle.lFileOffset = lTextOffset2FileOffset(
							lParmOffset);
				vAdd2StyleInfoList(&tStyle);
				(void)memset(&tStyle, 0, sizeof(tStyle));
			}
			switch (eGet6RowInfo(iFodo, aucFpage, &tRow)) {
			case found_a_cell:
				if (lParmOffsetFirst >= 0) {
					break;
				}
				lParmOffsetFirst = (long)ulGetLong(
						iIndex2 * 4, aucFpage);
				NO_DBG_HEX(lParmOffsetFirst);
				tRow.lOffsetStart = lTextOffset2FileOffset(
							lParmOffsetFirst);
				DBG_HEX_C(tRow.lOffsetStart < 0,
							lParmOffsetFirst);
				break;
			case found_end_of_row:
				lParmOffsetLast = (long)ulGetLong(
						iIndex2 * 4, aucFpage);
				NO_DBG_HEX(lParmOffsetLast);
				tRow.lOffsetEnd = lTextOffset2FileOffset(
							lParmOffsetLast);
				DBG_HEX_C(tRow.lOffsetEnd < 0, lParmOffsetLast);
				vAdd2RowInfoList(&tRow);
				(void)memset(&tRow, 0, sizeof(tRow));
				lParmOffsetFirst = -1;
				break;
			default:
				break;
			}
		}
	}
	ausParfPage = xfree(ausParfPage);
} /* end of vGet6ParInfo */

/*
 * Fill the font information block with information
 * from a Word 6/7 file.
 * Returns TRUE when successful, otherwise FALSE
 */
static BOOL
bGet6FontInfo(int iFodo, const unsigned char *aucFpage,
		font_block_type *pFont)
{
	int	iBytes, iFodoOff, iInfoLen;
	unsigned short	usTmp;
	unsigned char	ucTmp;

	fail(iFodo <= 0 || aucFpage == NULL || pFont == NULL);

	iBytes = (int)ucGetByte(iFodo, aucFpage);
	iFodoOff = 1;
	if (iBytes < 1) {
		return FALSE;
	}
	(void)memset(pFont, 0, sizeof(*pFont));
	pFont->sFontsize = DEFAULT_FONT_SIZE;
	while (iBytes >= iFodoOff + 1) {
		switch (ucGetByte(iFodo + iFodoOff, aucFpage)) {
		case 0x52:
			pFont->ucFontstyle &= FONT_HIDDEN;
			pFont->ucFontcolor = FONT_COLOR_DEFAULT;
			break;
		case 0x55:
			ucTmp = ucGetByte(iFodo + iFodoOff + 1, aucFpage);
			if (ucTmp == 0x01 || ucTmp == 0x81) {
				pFont->ucFontstyle |= FONT_BOLD;
			}
			DBG_HEX_C(ucTmp != 0x01 && ucTmp != 0x81, ucTmp);
			break;
		case 0x56:
			ucTmp = ucGetByte(iFodo + iFodoOff + 1, aucFpage);
			if (ucTmp == 0x01 || ucTmp == 0x81) {
				pFont->ucFontstyle |= FONT_ITALIC;
			}
			DBG_HEX_C(ucTmp != 0x01 && ucTmp != 0x81, ucTmp);
			break;
		case 0x57:
			ucTmp = ucGetByte(iFodo + iFodoOff + 1, aucFpage);
			if (ucTmp == 0x01 || ucTmp == 0x81) {
				NO_DBG_MSG("Strike text");
				pFont->ucFontstyle |= FONT_STRIKE;
			}
			DBG_HEX_C(ucTmp != 0x01 && ucTmp != 0x81, ucTmp);
			break;
		case 0x5a:
			ucTmp = ucGetByte(iFodo + iFodoOff + 1, aucFpage);
			if (ucTmp == 0x01 || ucTmp == 0x81) {
				pFont->ucFontstyle |= FONT_SMALL_CAPITALS;
			}
			DBG_HEX_C(ucTmp != 0x01 && ucTmp != 0x81, ucTmp);
			break;
		case 0x5b:
			ucTmp = ucGetByte(iFodo + iFodoOff + 1, aucFpage);
			if (ucTmp == 0x01 || ucTmp == 0x81) {
				pFont->ucFontstyle |= FONT_CAPITALS;
			}
			DBG_HEX_C(ucTmp != 0x01 && ucTmp != 0x81, ucTmp);
			break;
		case 0x5c:
			ucTmp = ucGetByte(iFodo + iFodoOff + 1, aucFpage);
			if (ucTmp == 0x01 || ucTmp == 0x81) {
				NO_DBG_MSG("Hidden text");
				pFont->ucFontstyle |= FONT_HIDDEN;
			}
			DBG_HEX_C(ucTmp != 0x01 && ucTmp != 0x81, ucTmp);
			break;
		case 0x5d:
			usTmp = usGetWord(iFodo + iFodoOff + 1, aucFpage);
			if (usTmp <= UCHAR_MAX) {
				pFont->ucFontnumber = (unsigned char)usTmp;
			}
			break;
		case 0x5e:
			ucTmp = ucGetByte(iFodo + iFodoOff + 1, aucFpage);
			if (ucTmp != 0) {
				DBG_HEX_C(ucTmp != 0x01, ucTmp);
				NO_DBG_MSG("Underline text");
				pFont->ucFontstyle |= FONT_UNDERLINE;
			}
			break;
		case 0x62:
			pFont->ucFontcolor =
				ucGetByte(iFodo + iFodoOff + 1, aucFpage);
			break;
		case 0x63:
			usTmp = usGetWord(iFodo + iFodoOff + 1, aucFpage);
			if (usTmp > SHRT_MAX) {
				pFont->sFontsize = SHRT_MAX;
			} else {
				pFont->sFontsize = (short)usTmp;
			}
			break;
		default:
			break;
		}
		iInfoLen = iGet6InfoLength(iFodo + iFodoOff, aucFpage);
		fail(iInfoLen <= 0);
		iFodoOff += iInfoLen;
	}
	return TRUE;
} /* end of bGet6FontInfo */

/*
 * Fill the picture information block with information
 * from a Word 6/7 file.
 * Returns TRUE when successful, otherwise FALSE
 */
static BOOL
bGet6PicInfo(int iFodo, const unsigned char *aucFpage,
		picture_block_type *pPicture)
{
	int	iBytes, iFodoOff, iInfoLen;
	BOOL	bFound;
	unsigned char	ucTmp;

	fail(iFodo <= 0 || aucFpage == NULL || pPicture == NULL);

	iBytes = (int)ucGetByte(iFodo, aucFpage);
	iFodoOff = 1;
	if (iBytes < 1) {
		return FALSE;
	}
	bFound = FALSE;
	(void)memset(pPicture, 0, sizeof(*pPicture));
	while (iBytes >= iFodoOff + 1) {
		switch (ucGetByte(iFodo + iFodoOff, aucFpage)) {
		case 0x44:
			pPicture->lPictureOffset = (long)ulGetLong(
					iFodo + iFodoOff + 2, aucFpage);
			bFound = TRUE;
			break;
		case 0x47:
			ucTmp = ucGetByte(iFodo + iFodoOff + 1, aucFpage);
			if (ucTmp == 0x01) {
				/* Not a picture, but a form field */
				return FALSE;
			}
			DBG_DEC_C(ucTmp != 0, ucTmp);
			break;
		case 0x4b:
			ucTmp = ucGetByte(iFodo + iFodoOff + 1, aucFpage);
			if (ucTmp == 0x01) {
				/* Not a picture, but an OLE object */
				return FALSE;
			}
			DBG_DEC_C(ucTmp != 0, ucTmp);
			break;
		default:
			break;
		}
		iInfoLen = iGet6InfoLength(iFodo + iFodoOff, aucFpage);
		fail(iInfoLen <= 0);
		iFodoOff += iInfoLen;
	}
	return bFound;
} /* end of bGet6PicInfo */

/*
 * Build the lists with Character Information for Word 6/7 files
 */
static void
vGet6ChrInfo(FILE *pFile, const pps_info_type *pPPS,
	const int *aiBBD, size_t tBBDLen,
	const unsigned char *aucHeader)
{
	font_block_type		tFont;
	picture_block_type	tPicture;
	unsigned short	*ausCharPage;
	unsigned char	*aucBuffer;
	long	lFileOffset, lParmOffset;
	size_t	tBeginCharInfo, tCharInfoLen, tOffset, tSize, tLenOld, tLen;
	size_t	tCharPageNum;
	int	iIndex, iIndex2, iNbr, iFodo;
	unsigned short	usCharFirstPage, usCount;
	unsigned char	aucFpage[BIG_BLOCK_SIZE];

	fail(pFile == NULL || pPPS == NULL || aucHeader == NULL);
	fail(aiBBD == NULL);

	tBeginCharInfo = (size_t)ulGetLong(0xb8, aucHeader);
	NO_DBG_HEX(tBeginCharInfo);
	tCharInfoLen = (size_t)ulGetLong(0xbc, aucHeader);
	NO_DBG_DEC(tCharInfoLen);
	if (tCharInfoLen < 4) {
		DBG_DEC(tCharInfoLen);
		return;
	}

	aucBuffer = xmalloc(tCharInfoLen);
	if (!bReadBuffer(pFile, pPPS->tWordDocument.iSb,
			aiBBD, tBBDLen, BIG_BLOCK_SIZE,
			aucBuffer, tBeginCharInfo, tCharInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return;
	}

	tLen = (tCharInfoLen - 4) / 6;
	tSize = tLen * sizeof(unsigned short);
	ausCharPage = xmalloc(tSize);
	for (iIndex = 0, tOffset = (tLen + 1) * 4;
	     iIndex < (int)tLen;
	     iIndex++, tOffset += 2) {
		 ausCharPage[iIndex] = usGetWord(tOffset, aucBuffer);
		 NO_DBG_DEC(ausCharPage[iIndex]);
	}
	DBG_HEX(ulGetLong(0, aucBuffer));
	aucBuffer = xfree(aucBuffer);
	aucBuffer = NULL;
#if defined(__hpux)
	/* Strange: the HPUX cpp can not deal with the number 0x18e */
	tCharPageNum = (size_t)aucHeader[0x18f] << 8 | (int)aucHeader[0x18e];
#else
	tCharPageNum = (size_t)usGetWord(0x18e, aucHeader);
#endif /* __hpux */
	DBG_DEC(tCharPageNum);
	if (tLen < tCharPageNum) {
		/* Replace CharPage by a longer version */
		tLenOld = tLen;
		usCharFirstPage = usGetWord(0x18a, aucHeader);
		DBG_DEC(usCharFirstPage);
		tLen += tCharPageNum - 1;
		tSize = tLen * sizeof(unsigned short);
		ausCharPage = xrealloc(ausCharPage, tSize);
		/* Add new values */
		usCount = usCharFirstPage + 1;
		for (iIndex = (int)tLenOld; iIndex < (int)tLen; iIndex++) {
			ausCharPage[iIndex] = usCount;
			NO_DBG_DEC(ausCharPage[iIndex]);
			usCount++;
		}
	}

	(void)memset(&tFont, 0, sizeof(tFont));
	(void)memset(&tPicture, 0, sizeof(tPicture));
	for (iIndex = 0; iIndex < (int)tLen; iIndex++) {
		if (!bReadBuffer(pFile, pPPS->tWordDocument.iSb,
				aiBBD, tBBDLen, BIG_BLOCK_SIZE,
				aucFpage,
				(size_t)ausCharPage[iIndex] * BIG_BLOCK_SIZE,
				BIG_BLOCK_SIZE)) {
			break;
		}
		iNbr = (int)ucGetByte(0x1ff, aucFpage);
		NO_DBG_DEC(iNbr);
		for (iIndex2 = 0; iIndex2 < iNbr; iIndex2++) {
		  	lParmOffset = (long)ulGetLong(
						iIndex2 * 4, aucFpage);
			lFileOffset = lTextOffset2FileOffset(lParmOffset);
			iFodo = 2 * (int)ucGetByte(
				(iNbr + 1) * 4 + iIndex2, aucFpage);
			if (iFodo == 0) {
				NO_DBG_HEX(lParmOffset);
				vReset2FontInfoList(lFileOffset);
			} else {
				if (bGet6FontInfo(iFodo, aucFpage, &tFont)) {
					tFont.lFileOffset = lFileOffset;
					vAdd2FontInfoList(&tFont);
				}
			}
			(void)memset(&tFont, 0, sizeof(tFont));
			if (iFodo <= 0) {
				continue;
			}
			if (bGet6PicInfo(iFodo, aucFpage, &tPicture)) {
				tPicture.lFileOffset = lFileOffset;
				tPicture.lFileOffsetPicture =
					lDataOffset2FileOffset(tPicture.lPictureOffset);
				vAdd2PicInfoList(&tPicture);
			}
			(void)memset(&tPicture, 0, sizeof(tPicture));
		}
	}
	ausCharPage = xfree(ausCharPage);
} /* end of vGet6ChrInfo */

/*
 * iGet8InfoLength - the length of the information for Word 8/97 files
 */
static int
iGet8InfoLength(int iByteNbr, const unsigned char *aucFpage)
{
	int	iTmp, iDel, iAdd;
	unsigned short	usOpCode;

	usOpCode = usGetWord(iByteNbr, aucFpage);

	switch (usOpCode & 0xe000) {
	case 0x0000: case 0x2000:
		return 3;
	case 0x4000: case 0x8000: case 0xa000:
		return 4;
	case 0xe000:
		return 5;
	case 0x6000:
		return 6;
	case 0xc000:
		iTmp = (int)ucGetByte(iByteNbr + 2, aucFpage);
		if (usOpCode == 0xc615 && iTmp == 255) {
			iDel = (int)ucGetByte(iByteNbr + 3, aucFpage);
			iAdd = (int)ucGetByte(
					iByteNbr + 4 + iDel * 4, aucFpage);
			iTmp = 2 + iDel * 4 + iAdd * 3;
		}
		return 3 + iTmp;
	default:
		DBG_HEX(usOpCode);
		return 1;
	}
} /* end of iGet8InfoLength */

/*
 * Returns 2 and fills the row information block if end of a row was found
 *	   1 if a cell was found
 *	   0 nothing useful was found
 */
static row_info_enum
eGet8RowInfo(int iFodo, const unsigned char *aucFpage, row_block_type *pRow)
{
	int	iBytes, iFodoOff, iInfoLen;
	int	iIndex, iSize, iCol;
	int	iPosCurr, iPosPrev;
	BOOL	bFound2416, bFound2417, bFoundd608;

	fail(iFodo <= 0 || aucFpage == NULL || pRow == NULL);

	iBytes = 2 * (int)ucGetByte(iFodo, aucFpage);
	iFodoOff = 3;
	if (iBytes == 0) {
		iFodo++;
		iBytes = 2 * (int)ucGetByte(iFodo, aucFpage);
	}
	NO_DBG_DEC(iBytes);
	bFound2416 = FALSE;
	bFound2417 = FALSE;
	bFoundd608 = FALSE;
	while (iBytes >= iFodoOff + 2) {
		iInfoLen = 0;
		switch (usGetWord(iFodo + iFodoOff, aucFpage)) {
		case 0x2416:
			if (ucGetByte(iFodo + iFodoOff + 2,
					aucFpage) == 0x01) {
				bFound2416 = TRUE;
			}
			break;
		case 0x2417:
			if (ucGetByte(iFodo + iFodoOff + 2,
					aucFpage) == 0x01) {
				bFound2417 = TRUE;
			}
			break;
		case 0xd608:
			iSize = (int)ucGetByte(iFodo + iFodoOff + 2, aucFpage);
			if (iSize < 6 || iBytes < iFodoOff + 8) {
				DBG_DEC(iSize);
				DBG_DEC(iFodoOff);
				iInfoLen = 2;
				break;
			}
			iCol = (int)ucGetByte(iFodo + iFodoOff + 4, aucFpage);
			if (iCol < 1 ||
			    iBytes < iFodoOff + 4 + (iCol + 1) * 2) {
				DBG_DEC(iCol);
				DBG_DEC(iFodoOff);
				iInfoLen = 2;
				break;
			}
			if (iCol >= (int)elementsof(pRow->asColumnWidth)) {
				werr(1, "The number of columns is corrupt");
			}
			pRow->ucNumberOfColumns = (unsigned char)iCol;
			pRow->iColumnWidthSum = 0;
			iPosPrev = (int)(short)usGetWord(
					iFodo + iFodoOff + 5,
					aucFpage);
			for (iIndex = 0; iIndex < iCol; iIndex++) {
				iPosCurr = (int)(short)usGetWord(
					iFodo + iFodoOff + 7 + iIndex * 2,
					aucFpage);
				pRow->asColumnWidth[iIndex] =
						(short)(iPosCurr - iPosPrev);
				pRow->iColumnWidthSum +=
					pRow->asColumnWidth[iIndex];
				iPosPrev = iPosCurr;
			}
			bFoundd608 = TRUE;
			break;
		default:
			break;
		}
		if (iInfoLen <= 0) {
			iInfoLen =
				iGet8InfoLength(iFodo + iFodoOff, aucFpage);
			fail(iInfoLen <= 0);
		}
		iFodoOff += iInfoLen;
	}
	if (bFound2416 && bFound2417 && bFoundd608) {
		return found_end_of_row;
	}
	if (bFound2416) {
		return found_a_cell;
	}
	return found_nothing;
} /* end of eGet8RowInfo */

/*
 * Fill the style information block with information
 * from a Word 8/97 file.
 * Returns TRUE when successful, otherwise FALSE
 */
static BOOL
bGet8StyleInfo(int iFodo, const unsigned char *aucFpage,
		style_block_type *pStyle)
{
	int	iBytes, iFodoOff, iInfoLen;
	int	iTmp, iDel, iAdd;
	short	sTmp;

	fail(iFodo <= 0 || aucFpage == NULL || pStyle == NULL);

	iBytes = 2 * (int)ucGetByte(iFodo, aucFpage);
	iFodoOff = 3;
	if (iBytes == 0) {
		iFodo++;
		iBytes = 2 * (int)ucGetByte(iFodo, aucFpage);
	}
	NO_DBG_DEC(iBytes);
	if (iBytes < 2) {
		return FALSE;
	}
	(void)memset(pStyle, 0, sizeof(*pStyle));
	sTmp = (short)usGetWord(iFodo + 1, aucFpage);
	if (sTmp >= 0 && sTmp <= UCHAR_MAX) {
		pStyle->ucStyle = (unsigned char)sTmp;
	}
	NO_DBG_DEC(pStyle->ucStyle);
	while (iBytes >= iFodoOff + 2) {
		iInfoLen = 0;
		switch (usGetWord(iFodo + iFodoOff, aucFpage)) {
		case 0x2403:
			pStyle->ucAlignment = ucGetByte(
					iFodo + iFodoOff + 2, aucFpage);
			break;
		case 0x260a:
			pStyle->bInList = TRUE;
			pStyle->ucListLevel =
				ucGetByte(iFodo + iFodoOff + 2, aucFpage);
			NO_DBG_DEC(pStyle->ucListLevel);
			break;
		case 0x460b:
			NO_DBG_DEC(usGetWord(iFodo + iFodoOff + 2, aucFpage));
			break;
		case 0x4610:
			sTmp = (short)usGetWord(iFodo + iFodoOff + 2, aucFpage);
			pStyle->sLeftIndent += sTmp;
			if (pStyle->sLeftIndent < 0) {
				pStyle->sLeftIndent = 0;
			}
			DBG_DEC(sTmp);
			DBG_DEC(pStyle->sLeftIndent);
			break;
		case 0x6c0d:
			iTmp = (int)ucGetByte(iFodo + iFodoOff + 2, aucFpage);
			if (iTmp < 2) {
				iInfoLen = 1;
				break;
			}
			DBG_DEC(iTmp);
			iDel = (int)ucGetByte(iFodo + iFodoOff + 3, aucFpage);
			if (iTmp < 2 + 2 * iDel) {
				iInfoLen = 1;
				break;
			}
			DBG_DEC(iDel);
			iAdd = (int)ucGetByte(
				iFodo + iFodoOff + 4 + 2 * iDel, aucFpage);
			if (iTmp < 2 + 2 * iDel + 2 * iAdd) {
				iInfoLen = 1;
				break;
			}
			DBG_DEC(iAdd);
			break;
		case 0x840e:
			pStyle->sRightIndent = (short)usGetWord(
					iFodo + iFodoOff + 2, aucFpage);
			NO_DBG_DEC(pStyle->sRightIndent);
			break;
		case 0x840f:
			pStyle->sLeftIndent = (short)usGetWord(
					iFodo + iFodoOff + 2, aucFpage);
			NO_DBG_DEC(pStyle->sLeftIndent);
			break;
		default:
			break;
		}
		if (iInfoLen <= 0) {
			iInfoLen =
				iGet8InfoLength(iFodo + iFodoOff, aucFpage);
			fail(iInfoLen <= 0);
		}
		iFodoOff += iInfoLen;
	}
	return TRUE;
} /* end of bGet8StyleInfo */

/*
 * Build the lists with Paragraph Information for Word 8/97 files
 */
static void
vGet8ParInfo(FILE *pFile, const pps_info_type *pPPS,
	const int *aiBBD, size_t tBBDLen, const int *aiSBD, size_t tSBDLen,
	const unsigned char *aucHeader)
{
	row_block_type		tRow;
	style_block_type	tStyle;
	size_t		*atParfPage;
	const int	*aiBlockDepot;
	unsigned char	*aucBuffer;
	long	lParmOffset, lParmOffsetFirst, lParmOffsetLast, lTableSize;
	size_t	tBeginParfInfo, tParfInfoLen, tBlockDepotLen;
	size_t	tBlockSize, tOffset, tSize, tLen;
	int	iTableStartBlock;
	int	iIndex, iIndex2, iNbr, iFodo;
	unsigned short	usDocStatus;
	unsigned char	aucFpage[BIG_BLOCK_SIZE];

	fail(pFile == NULL || pPPS == NULL || aucHeader == NULL);
	fail(aiBBD == NULL || aiSBD == NULL);

	tBeginParfInfo = (size_t)ulGetLong(0x102, aucHeader);
	NO_DBG_HEX(tBeginParfInfo);
	tParfInfoLen = (size_t)ulGetLong(0x106, aucHeader);
	NO_DBG_DEC(tParfInfoLen);
	if (tParfInfoLen < 4) {
		DBG_DEC(tParfInfoLen);
		return;
	}

	/* Use 0Table or 1Table? */
	usDocStatus = usGetWord(0x0a, aucHeader);
	if (usDocStatus & BIT(9)) {
		iTableStartBlock = pPPS->t1Table.iSb;
		lTableSize = pPPS->t1Table.lSize;
	} else {
		iTableStartBlock = pPPS->t0Table.iSb;
		lTableSize = pPPS->t0Table.lSize;
	}
	DBG_DEC(iTableStartBlock);
	if (iTableStartBlock < 0) {
		DBG_DEC(iTableStartBlock);
		DBG_MSG("No paragraph information");
		return;
	}
	DBG_HEX(lTableSize);
	if (lTableSize < MIN_SIZE_FOR_BBD_USE) {
	  	/* Use the Small Block Depot */
		aiBlockDepot = aiSBD;
		tBlockDepotLen = tSBDLen;
		tBlockSize = SMALL_BLOCK_SIZE;
	} else {
	  	/* Use the Big Block Depot */
		aiBlockDepot = aiBBD;
		tBlockDepotLen = tBBDLen;
		tBlockSize = BIG_BLOCK_SIZE;
	}
	aucBuffer = xmalloc(tParfInfoLen);
	if (!bReadBuffer(pFile, iTableStartBlock,
			aiBlockDepot, tBlockDepotLen, tBlockSize,
			aucBuffer, tBeginParfInfo, tParfInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return;
	}
	NO_DBG_PRINT_BLOCK(aucBuffer, tParfInfoLen);

	tLen = (tParfInfoLen / 4 - 1) / 2;
	tSize = tLen * sizeof(size_t);
	atParfPage = xmalloc(tSize);
	for (iIndex = 0, tOffset = (tLen + 1) * 4;
	     iIndex < (int)tLen;
	     iIndex++, tOffset += 4) {
		 atParfPage[iIndex] = (size_t)ulGetLong(tOffset, aucBuffer);
		 NO_DBG_DEC(atParfPage[iIndex]);
	}
	DBG_HEX(ulGetLong(0, aucBuffer));
	aucBuffer = xfree(aucBuffer);
	aucBuffer = NULL;
	NO_DBG_PRINT_BLOCK(aucHeader, HEADER_SIZE);

	(void)memset(&tStyle, 0, sizeof(tStyle));
	(void)memset(&tRow, 0, sizeof(tRow));
	lParmOffsetFirst = -1;
	for (iIndex = 0; iIndex < (int)tLen; iIndex++) {
		fail(atParfPage[iIndex] > SIZE_T_MAX / BIG_BLOCK_SIZE);
		if (!bReadBuffer(pFile, pPPS->tWordDocument.iSb,
				aiBBD, tBBDLen, BIG_BLOCK_SIZE,
				aucFpage,
				atParfPage[iIndex] * BIG_BLOCK_SIZE,
				BIG_BLOCK_SIZE)) {
			break;
		}
		NO_DBG_PRINT_BLOCK(aucFpage, BIG_BLOCK_SIZE);
		iNbr = (int)ucGetByte(0x1ff, aucFpage);
		NO_DBG_DEC(iNbr);
		for (iIndex2 = 0; iIndex2 < iNbr; iIndex2++) {
			NO_DBG_HEX(ulGetLong(iIndex2 * 4, aucFpage));
			iFodo = 2 * (int)ucGetByte(
				(iNbr + 1) * 4 + iIndex2 * 13, aucFpage);
			if (iFodo <= 0) {
				continue;
			}
			if (bGet8StyleInfo(iFodo, aucFpage, &tStyle)) {
				lParmOffset = (long)ulGetLong(
						iIndex2 * 4, aucFpage);
				NO_DBG_HEX(lParmOffset);
				tStyle.lFileOffset = lTextOffset2FileOffset(
							lParmOffset);
				vAdd2StyleInfoList(&tStyle);
				(void)memset(&tStyle, 0, sizeof(tStyle));
			}
			switch (eGet8RowInfo(iFodo, aucFpage, &tRow)) {
			case found_a_cell:
				if (lParmOffsetFirst >= 0) {
					break;
				}
				lParmOffsetFirst = (long)ulGetLong(
						iIndex2 * 4, aucFpage);
				NO_DBG_HEX(lParmOffsetFirst);
				tRow.lOffsetStart = lTextOffset2FileOffset(
							lParmOffsetFirst);
				DBG_HEX_C(tRow.lOffsetStart < 0,
							lParmOffsetFirst);
				break;
			case found_end_of_row:
				lParmOffsetLast = (long)ulGetLong(
						iIndex2 * 4, aucFpage);
				NO_DBG_HEX(lParmOffsetLast);
				tRow.lOffsetEnd = lTextOffset2FileOffset(
							lParmOffsetLast);
				DBG_HEX_C(tRow.lOffsetEnd < 0, lParmOffsetLast);
				vAdd2RowInfoList(&tRow);
				(void)memset(&tRow, 0, sizeof(tRow));
				lParmOffsetFirst = -1;
				break;
			default:
				break;
			}
		}
	}
	atParfPage = xfree(atParfPage);
} /* end of vGet8ParInfo */

/*
 * Fill the font information block with information
 * from a Word 8/97 file.
 * Returns TRUE when successful, otherwise FALSE
 */
static BOOL
bGet8FontInfo(int iFodo, const unsigned char *aucFpage,
		font_block_type *pFont)
{
	int	iBytes, iFodoOff, iInfoLen;
	unsigned short	usTmp;
	unsigned char	ucTmp;

	fail(iFodo <= 0 || aucFpage == NULL || pFont == NULL);

	iBytes = (int)ucGetByte(iFodo, aucFpage);
	iFodoOff = 1;
	if (iBytes == 0) {
		iFodo++;
		iBytes = (int)ucGetByte(iFodo, aucFpage);
	}
	NO_DBG_DEC(iBytes);
	if (iBytes < 2) {
		return FALSE;
	}
	(void)memset(pFont, 0, sizeof(*pFont));
	pFont->sFontsize = DEFAULT_FONT_SIZE;
	while (iBytes >= iFodoOff + 2) {
		switch (usGetWord(iFodo + iFodoOff, aucFpage)) {
		case 0x0835:
			ucTmp = ucGetByte(iFodo + iFodoOff + 2, aucFpage);
			if (ucTmp == 0x01 || ucTmp == 0x81) {
				pFont->ucFontstyle |= FONT_BOLD;
			}
			DBG_HEX_C(ucTmp != 0x01 && ucTmp != 0x81, ucTmp);
			break;
		case 0x0836:
			ucTmp = ucGetByte(iFodo + iFodoOff + 2, aucFpage);
			if (ucTmp == 0x01 || ucTmp == 0x81) {
				pFont->ucFontstyle |= FONT_ITALIC;
			}
			DBG_HEX_C(ucTmp != 0x01 && ucTmp != 0x81, ucTmp);
			break;
		case 0x0837:
			ucTmp = ucGetByte(iFodo + iFodoOff + 2, aucFpage);
			if (ucTmp == 0x01 || ucTmp == 0x81) {
				NO_DBG_MSG("Strike text");
				pFont->ucFontstyle |= FONT_STRIKE;
			}
			DBG_HEX_C(ucTmp != 0x01 && ucTmp != 0x81, ucTmp);
			break;
		case 0x083a:
			ucTmp = ucGetByte(iFodo + iFodoOff + 2, aucFpage);
			if (ucTmp == 0x01 || ucTmp == 0x81) {
				pFont->ucFontstyle |= FONT_SMALL_CAPITALS;
			}
			DBG_HEX_C(ucTmp != 0x01 && ucTmp != 0x81, ucTmp);
			break;
		case 0x083b:
			ucTmp = ucGetByte(iFodo + iFodoOff + 2, aucFpage);
			if (ucTmp == 0x01 || ucTmp == 0x81) {
				pFont->ucFontstyle |= FONT_CAPITALS;
			}
			DBG_HEX_C(ucTmp != 0x01 && ucTmp != 0x81, ucTmp);
			break;
		case 0x083c:
			ucTmp = ucGetByte(iFodo + iFodoOff + 2, aucFpage);
			if (ucTmp == 0x01 || ucTmp == 0x81) {
				DBG_MSG("Hidden text");
				pFont->ucFontstyle |= FONT_HIDDEN;
			}
			DBG_HEX_C(ucTmp != 0x01 && ucTmp != 0x81, ucTmp);
			break;
		case 0x2a32:
			pFont->ucFontstyle &= FONT_HIDDEN;
			pFont->ucFontcolor = FONT_COLOR_DEFAULT;
			break;
		case 0x2a3e:
			ucTmp = ucGetByte(iFodo + iFodoOff + 2, aucFpage);
			if (ucTmp != 0) {
				DBG_HEX_C(ucTmp > 11, ucTmp);
				NO_DBG_MSG("Underline text");
				pFont->ucFontstyle |= FONT_UNDERLINE;
			}
			break;
		case 0x2a42:
			pFont->ucFontcolor =
				ucGetByte(iFodo + iFodoOff + 2, aucFpage);
			NO_DBG_DEC(pFont->ucFontcolor);
			break;
		case 0x4a43:
			usTmp = usGetWord(iFodo + iFodoOff + 2, aucFpage);
			if (usTmp > SHRT_MAX) {
				pFont->sFontsize = SHRT_MAX;
			} else {
				pFont->sFontsize = (short)usTmp;
			}
			NO_DBG_DEC(pFont->sFontsize);
			break;
		case 0x4a51:
			usTmp = usGetWord(iFodo + iFodoOff + 2, aucFpage);
			if (usTmp <= UCHAR_MAX) {
				pFont->ucFontnumber = (unsigned char)usTmp;
			}
			break;
		default:
			break;
		}
		iInfoLen = iGet8InfoLength(iFodo + iFodoOff, aucFpage);
		fail(iInfoLen <= 0);
		iFodoOff += iInfoLen;
	}
	return TRUE;
} /* end of bGet8FontInfo */

/*
 * Fill the picture information block with information
 * from a Word 8/97 file.
 * Returns TRUE when successful, otherwise FALSE
 */
static BOOL
bGet8PicInfo(int iFodo, const unsigned char *aucFpage,
		picture_block_type *pPicture)
{
	int	iBytes, iFodoOff, iInfoLen;
	BOOL	bFound;
	unsigned char	ucTmp;

	fail(iFodo <= 0 || aucFpage == NULL || pPicture == NULL);

	iBytes = (int)ucGetByte(iFodo, aucFpage);
	iFodoOff = 1;
	if (iBytes == 0) {
		iFodo++;
		iBytes = (int)ucGetByte(iFodo, aucFpage);
	}
	NO_DBG_DEC(iBytes);
	if (iBytes < 2) {
		return FALSE;
	}
	bFound = FALSE;
	(void)memset(pPicture, 0, sizeof(*pPicture));
	while (iBytes >= iFodoOff + 2) {
		switch (usGetWord(iFodo + iFodoOff, aucFpage)) {
		case 0x0806:
			ucTmp = ucGetByte(iFodo + iFodoOff + 2, aucFpage);
			if (ucTmp == 0x01) {
				/* Not a picture, but a form field */
				return FALSE;
			}
			DBG_DEC_C(ucTmp != 0, ucTmp);
			break;
		case 0x080a:
			ucTmp = ucGetByte(iFodo + iFodoOff + 2, aucFpage);
			if (ucTmp == 0x01) {
				/* Not a picture, but an OLE object */
				return FALSE;
			}
			DBG_DEC_C(ucTmp != 0, ucTmp);
			break;
		case 0x6a03:
			pPicture->lPictureOffset = (long)ulGetLong(
					iFodo + iFodoOff + 2, aucFpage);
			bFound = TRUE;
			break;
		default:
			break;
		}
		iInfoLen = iGet8InfoLength(iFodo + iFodoOff, aucFpage);
		fail(iInfoLen <= 0);
		iFodoOff += iInfoLen;
	}
	return bFound;
} /* end of bGet8PicInfo */

/*
 * Build the lists with Character Information for Word 8/97 files
 */
static void
vGet8ChrInfo(FILE *pFile, const pps_info_type *pPPS,
	const int *aiBBD, size_t tBBDLen, const int *aiSBD, size_t tSBDLen,
	const unsigned char *aucHeader)
{
	font_block_type		tFont;
	picture_block_type	tPicture;
	size_t		*atCharPage;
	const int	*aiBlockDepot;
	unsigned char	*aucBuffer;
	long	lFileOffset, lParmOffset, lTableSize;
	size_t	tBeginCharInfo, tCharInfoLen, tBlockDepotLen;
	size_t	tOffset, tBlockSize, tSize, tLen;
	int	iTableStartBlock;
	int	iIndex, iIndex2, iNbr, iFodo;
	unsigned short	usDocStatus;
	unsigned char	aucFpage[BIG_BLOCK_SIZE];

	fail(pFile == NULL || pPPS == NULL || aucHeader == NULL);
	fail(aiBBD == NULL);

	tBeginCharInfo = (size_t)ulGetLong(0xfa, aucHeader);
	NO_DBG_HEX(tBeginCharInfo);
#if defined(__hpux)
	/* Strange: the HPUX cpp can not deal with the number 0xfe */
	tCharInfoLen = (size_t)aucHeader[0xff] << 8 | (size_t)aucHeader[0xfe];
#else
	tCharInfoLen = (size_t)ulGetLong(0xfe, aucHeader);
#endif /* __hpux */
	NO_DBG_DEC(tCharInfoLen);
	if (tCharInfoLen < 4) {
		DBG_DEC(tCharInfoLen);
		return;
	}

	/* Use 0Table or 1Table? */
	usDocStatus = usGetWord(0x0a, aucHeader);
	if (usDocStatus & BIT(9)) {
		iTableStartBlock = pPPS->t1Table.iSb;
		lTableSize = pPPS->t1Table.lSize;
	} else {
		iTableStartBlock = pPPS->t0Table.iSb;
		lTableSize = pPPS->t0Table.lSize;
	}
	DBG_DEC(iTableStartBlock);
	if (iTableStartBlock < 0) {
		DBG_DEC(iTableStartBlock);
		DBG_MSG("No character information");
		return;
	}
	DBG_HEX(lTableSize);
	if (lTableSize < MIN_SIZE_FOR_BBD_USE) {
	  	/* Use the Small Block Depot */
		aiBlockDepot = aiSBD;
		tBlockDepotLen = tSBDLen;
		tBlockSize = SMALL_BLOCK_SIZE;
	} else {
	  	/* Use the Big Block Depot */
		aiBlockDepot = aiBBD;
		tBlockDepotLen = tBBDLen;
		tBlockSize = BIG_BLOCK_SIZE;
	}
	aucBuffer = xmalloc(tCharInfoLen);
	if (!bReadBuffer(pFile, iTableStartBlock,
			aiBlockDepot, tBlockDepotLen, tBlockSize,
			aucBuffer, tBeginCharInfo, tCharInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return;
	}
	NO_DBG_PRINT_BLOCK(aucBuffer, tCharInfoLen);

	tLen = (tCharInfoLen / 4 - 1) / 2;
	tSize = tLen * sizeof(size_t);
	atCharPage = xmalloc(tSize);
	for (iIndex = 0, tOffset = (tLen + 1) * 4;
	     iIndex < (int)tLen;
	     iIndex++, tOffset += 4) {
		 atCharPage[iIndex] = (size_t)ulGetLong(tOffset, aucBuffer);
		 NO_DBG_DEC(atCharPage[iIndex]);
	}
	DBG_HEX(ulGetLong(0, aucBuffer));
	aucBuffer = xfree(aucBuffer);
	aucBuffer = NULL;
	NO_DBG_PRINT_BLOCK(aucHeader, HEADER_SIZE);

	(void)memset(&tFont, 0, sizeof(tFont));
	for (iIndex = 0; iIndex < (int)tLen; iIndex++) {
		fail(atCharPage[iIndex] > SIZE_T_MAX / BIG_BLOCK_SIZE);
		if (!bReadBuffer(pFile, pPPS->tWordDocument.iSb,
				aiBBD, tBBDLen, BIG_BLOCK_SIZE,
				aucFpage,
				atCharPage[iIndex] * BIG_BLOCK_SIZE,
				BIG_BLOCK_SIZE)) {
			break;
		}
		NO_DBG_PRINT_BLOCK(aucFpage, BIG_BLOCK_SIZE);
		iNbr = (int)ucGetByte(0x1ff, aucFpage);
		NO_DBG_DEC(iNbr);
		for (iIndex2 = 0; iIndex2 < iNbr; iIndex2++) {
			lParmOffset = (long)ulGetLong(
						iIndex2 * 4, aucFpage);
			lFileOffset = lTextOffset2FileOffset(lParmOffset);
			iFodo = 2 * (int)ucGetByte(
				(iNbr + 1) * 4 + iIndex2, aucFpage);
			if (iFodo == 0) {
				NO_DBG_HEX(lParmOffset);
				vReset2FontInfoList(lFileOffset);
			} else {
				if (bGet8FontInfo(iFodo, aucFpage, &tFont)) {
					tFont.lFileOffset = lFileOffset;
					vAdd2FontInfoList(&tFont);
				}
			}
			(void)memset(&tFont, 0, sizeof(tFont));
			if (iFodo <= 0) {
				continue;
			}
			if (bGet8PicInfo(iFodo, aucFpage, &tPicture)) {
				tPicture.lFileOffset = lFileOffset;
				tPicture.lFileOffsetPicture =
					lDataOffset2FileOffset(
						tPicture.lPictureOffset);
				vAdd2PicInfoList(&tPicture);
			}
			(void)memset(&tPicture, 0, sizeof(tPicture));
		}
	}
	atCharPage = xfree(atCharPage);
} /* end of vGet8ChrInfo */

/*
 * Build the lists with Paragraph Information
 */
void
vGetParagraphInfo(FILE *pFile, const pps_info_type *pPPS,
	const int *aiBBD, size_t tBBDLen, const int *aiSBD, size_t tSBDLen,
	const unsigned char *aucHeader, int iWordVersion)
{
	options_type	tOptions;

	fail(pFile == NULL || pPPS == NULL || aucHeader == NULL);
	fail(iWordVersion < 6 || iWordVersion > 8);
	fail(aiBBD == NULL || aiSBD == NULL);

	vGetOptions(&tOptions);

	switch (iWordVersion) {
	case 6:
	case 7:
		vGet6ParInfo(pFile, pPPS->tWordDocument.iSb,
			aiBBD, tBBDLen, aucHeader);
		if (tOptions.bUseOutlineFonts) {
			vGet6ChrInfo(pFile, pPPS,
				aiBBD, tBBDLen, aucHeader);
			vCreate6FontTable(pFile, pPPS->tWordDocument.iSb,
				aiBBD, tBBDLen, aucHeader);
		}
		break;
	case 8:
		vGet8ParInfo(pFile, pPPS,
			aiBBD, tBBDLen, aiSBD, tSBDLen, aucHeader);
		if (tOptions.bUseOutlineFonts) {
			vGet8ChrInfo(pFile, pPPS,
				aiBBD, tBBDLen, aiSBD, tSBDLen, aucHeader);
			vCreate8FontTable(pFile, pPPS,
				aiBBD, tBBDLen, aiSBD, tSBDLen, aucHeader);
		}
		break;
	default:
		werr(0, "Sorry, no paragraph information");
		break;
	}
} /* end of vGetParagraphInfo */
