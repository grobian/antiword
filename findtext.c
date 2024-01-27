/*
 * findtext.c
 * Copyright (C) 1998-2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Find the blocks that contain the text of MS Word files
 */

#include <stdio.h>
#include <stdlib.h>
#include "antiword.h"


/*
 * bAddTextBlocks - Add the blocks to the text block list
 *
 * Returns TRUE when successful, FALSE if not
 */
BOOL
bAddTextBlocks(long lFirstOffset, size_t tTotalLength, BOOL bUsesUnicode,
	int iStartBlock, const int *aiBBD, size_t tBBDLen)
{
	text_block_type	tTextBlock;
	long	lTextOffset;
	size_t	tToGo, tOffset;
	int	iIndex;

	fail(lFirstOffset < 0);
	fail(iStartBlock < 0);
	fail(aiBBD == NULL);

	NO_DBG_HEX(lFirstOffset);
	NO_DBG_DEC(tTotalLength);

	if (bUsesUnicode) {
		/* One character equals two bytes */
		NO_DBG_MSG("Uses Unicode");
		tToGo = tTotalLength * 2;
	} else {
		/* One character equals one byte */
		NO_DBG_MSG("Uses ASCII");
		tToGo = tTotalLength;
	}

	lTextOffset = lFirstOffset;
	tOffset = (size_t)lFirstOffset;
	for (iIndex = iStartBlock;
	     iIndex != END_OF_CHAIN && tToGo != 0;
	     iIndex = aiBBD[iIndex]) {
		if (iIndex < 0 || iIndex >= (int)tBBDLen) {
			werr(1, "The Big Block Depot is corrupt");
		}
		if (tOffset >= BIG_BLOCK_SIZE) {
			tOffset -= BIG_BLOCK_SIZE;
			continue;
		}
		tTextBlock.lFileOffset =
			((long)iIndex + 1) * BIG_BLOCK_SIZE + (long)tOffset;
		tTextBlock.lTextOffset = lTextOffset;
		tTextBlock.tLength = min(BIG_BLOCK_SIZE - tOffset, tToGo);
		tTextBlock.bUsesUnicode = bUsesUnicode;
		tOffset = 0;
		if (!bAdd2TextBlockList(&tTextBlock)) {
			DBG_HEX(tTextBlock.lFileOffset);
			DBG_HEX(tTextBlock.lTextOffset);
			DBG_DEC(tTextBlock.tLength);
			DBG_DEC(tTextBlock.bUsesUnicode);
			return FALSE;
		}
		lTextOffset += (long)tTextBlock.tLength;
		tToGo -= tTextBlock.tLength;
	}
	return tToGo == 0;
} /* end of bAddTextBlocks */

/*
 * bGet6DocumentText - make a list of the text blocks of Word 6/7 files
 *
 * Code for "fast saved" files.
 */
text_info_enum
eGet6DocumentText(FILE *pFile, BOOL bUsesUnicode, int iStartBlock,
	const int *aiBBD, size_t tBBDLen, const unsigned char *aucHeader)
{
	unsigned char	*aucBuffer;
	long	lOffset;
	size_t	tBeginTextInfo, tTextInfoLen, tTotLength;
	int	iIndex;
	int	iOff, iType, iLen, iPieces;

	DBG_MSG("eGet6DocumentText");

	fail(pFile == NULL);
	fail(aiBBD == NULL);
	fail(aucHeader == NULL);

	tBeginTextInfo = (size_t)ulGetLong(0x160, aucHeader);
	tTextInfoLen = (size_t)ulGetLong(0x164, aucHeader);
	DBG_HEX(tBeginTextInfo);
	DBG_DEC(tTextInfoLen);

	aucBuffer = xmalloc(tTextInfoLen);
	if (!bReadBuffer(pFile, iStartBlock,
			aiBBD, tBBDLen, BIG_BLOCK_SIZE,
			aucBuffer, tBeginTextInfo, tTextInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return text_failure;
	}
	NO_DBG_PRINT_BLOCK(aucBuffer, tTextInfoLen);

	iOff = 0;
	while (iOff < (int)tTextInfoLen) {
		iType = (int)ucGetByte(iOff, aucBuffer);
		iOff++;
		if (iType == 0) {
			iOff++;
			continue;
		}
		iLen = (int)usGetWord(iOff, aucBuffer);
		iOff += 2;
		if (iType == 1) {
			iOff += iLen;
			continue;
		}
		if (iType != 2) {
			werr(0, "Unknown type of 'fastsaved' format");
			aucBuffer = xfree(aucBuffer);
			return text_failure;
		}
		/* Type 2 */
		NO_DBG_DEC(iLen);
		iOff += 2;
		iPieces = (iLen - 4) / 12;
		DBG_DEC(iPieces);
		for (iIndex = 0; iIndex < iPieces; iIndex++) {
			lOffset = (long)ulGetLong(
				iOff + (iPieces + 1) * 4 + iIndex * 8 + 2,
				aucBuffer);
			tTotLength = (size_t)ulGetLong(
						iOff + (iIndex + 1) * 4,
						aucBuffer) -
					(size_t)ulGetLong(
						iOff + iIndex * 4,
						aucBuffer);
			if (!bAddTextBlocks(lOffset, tTotLength, bUsesUnicode,
					iStartBlock,
					aiBBD, tBBDLen)) {
				aucBuffer = xfree(aucBuffer);
				return text_failure;
			}
		}
		break;
	}
	aucBuffer = xfree(aucBuffer);
	return text_success;
} /* end of eGet6DocumentText */

/*
 * eGet8DocumentText - make a list of the text blocks of Word 8/97 files
 */
text_info_enum
eGet8DocumentText(FILE *pFile, const pps_info_type *pPPS,
	const int *aiBBD, size_t tBBDLen, const int *aiSBD, size_t tSBDLen,
	const unsigned char *aucHeader)
{
	const int	*aiBlockDepot;
	unsigned char	*aucBuffer;
	long	lOffset, lTableSize, lLen;
	size_t	tBeginTextInfo, tTextInfoLen, tBlockDepotLen, tBlockSize;
	size_t	tTotLength;
	int	iTableStartBlock;
	int	iIndex, iOff, iType, iLen, iPieces;
	BOOL	bUsesUnicode;
	unsigned short	usDocStatus;

	DBG_MSG("eGet8DocumentText");

	fail(pFile == NULL || pPPS == NULL);
	fail(aiBBD == NULL || aiSBD == NULL);
	fail(aucHeader == NULL);

  	tBeginTextInfo = (size_t)ulGetLong(0x1a2, aucHeader);
	tTextInfoLen = (size_t)ulGetLong(0x1a6, aucHeader);
	DBG_HEX(tBeginTextInfo);
	DBG_DEC(tTextInfoLen);

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
		return text_failure;
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
	aucBuffer = xmalloc(tTextInfoLen);
	if (!bReadBuffer(pFile, iTableStartBlock,
			aiBlockDepot, tBlockDepotLen, tBlockSize,
			aucBuffer, tBeginTextInfo, tTextInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return text_no_information;
	}
	NO_DBG_PRINT_BLOCK(aucBuffer, tTextInfoLen);

	iOff = 0;
	while (iOff < (int)tTextInfoLen) {
		iType = (int)ucGetByte(iOff, aucBuffer);
		iOff++;
		if (iType == 0) {
			iOff++;
			continue;
		}
		if (iType == 1) {
			iLen = (int)usGetWord(iOff, aucBuffer);
			iOff += iLen + 2;
			continue;
		}
		if (iType != 2) {
			werr(0, "Unknown type of 'fastsaved' format");
			aucBuffer = xfree(aucBuffer);
			return text_failure;
		}
		/* Type 2 */
		lLen = (long)ulGetLong(iOff, aucBuffer);
		NO_DBG_DEC(lLen);
		iOff += 4;
		iPieces = (int)((lLen - 4) / 12);
		DBG_DEC(iPieces);
		for (iIndex = 0; iIndex < iPieces; iIndex++) {
			lOffset = (long)ulGetLong(
				iOff + (iPieces + 1) * 4 + iIndex * 8 + 2,
				aucBuffer);
			if ((lOffset & BIT(30)) == 0) {
				bUsesUnicode = TRUE;
			} else {
				bUsesUnicode = FALSE;
				lOffset &= ~BIT(30);
				lOffset /= 2;
			}
			tTotLength = (size_t)ulGetLong(
						iOff + (iIndex + 1) * 4,
						aucBuffer) -
					(size_t)ulGetLong(
						iOff + iIndex * 4,
						aucBuffer);
			if (!bAddTextBlocks(lOffset, tTotLength, bUsesUnicode,
					pPPS->tWordDocument.iSb,
					aiBBD, tBBDLen)) {
				aucBuffer = xfree(aucBuffer);
				return text_failure;
			}
		}
		break;
	}
	aucBuffer = xfree(aucBuffer);
	return text_success;
} /* end of eGet8DocumentText */
