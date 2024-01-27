/*
 * finddata.c
 * Copyright (C) 2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Find the blocks that contain the data of MS Word files
 */

#include <stdio.h>
#include <stdlib.h>
#include "antiword.h"


/*
 * bAddDataBlocks - Add the blocks to the data block list
 *
 * Returns TRUE when successful, otherwise FALSE
 */
BOOL
bAddDataBlocks(long lFirstOffset, size_t tTotalLength,
	int iStartBlock, const int *aiBBD, size_t tBBDLen)
{
	data_block_type	tDataBlock;
	long	lDataOffset;
	size_t	tToGo, tOffset;
	int	iIndex;
	BOOL	bResult;

	fail(lFirstOffset < 0);
	fail(iStartBlock < 0);
	fail(aiBBD == NULL);

	NO_DBG_HEX(lFirstOffset);
	NO_DBG_DEC(tTotalLength);

	tToGo = tTotalLength;
	lDataOffset = lFirstOffset;
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
		tDataBlock.lFileOffset =
			((long)iIndex + 1) * BIG_BLOCK_SIZE + (long)tOffset;
		tDataBlock.lDataOffset = lDataOffset;
		tDataBlock.tLength = min(BIG_BLOCK_SIZE - tOffset, tToGo);
		tOffset = 0;
		if (!bAdd2DataBlockList(&tDataBlock)) {
			DBG_HEX(tDataBlock.lFileOffset);
			DBG_HEX(tDataBlock.lDataOffset);
			DBG_DEC(tDataBlock.tLength);
			return FALSE;
		}
		lDataOffset += (long)tDataBlock.tLength;
		tToGo -= tDataBlock.tLength;
	}
	bResult = tToGo == 0 ||
		(tTotalLength == SIZE_T_MAX && iIndex == END_OF_CHAIN);
	DBG_DEC_C(!bResult, tToGo);
	DBG_DEC_C(!bResult, tTotalLength);
	DBG_DEC_C(!bResult, iIndex);
	return bResult;
} /* end of bAddDataBlocks */

/*
 * bGet6DocumentData - make a list of the data blocks of Word 6/7 files
 *
 * Code for "fast saved" files.
 *
 * Returns TRUE when successful, otherwise FALSE
 */
BOOL
bGet6DocumentData(FILE *pFile, int iStartBlock,
	const int *aiBBD, size_t tBBDLen, const unsigned char *aucHeader)
{
	unsigned char	*aucBuffer;
	long	lOffset;
	size_t	tBeginTextInfo, tTextInfoLen, tTotLength;
	int	iIndex;
	int	iOff, iType, iLen, iPieces;

	DBG_MSG("bGet6DocumentData");

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
		return FALSE;
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
			return FALSE;
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
			if (!bAddDataBlocks(lOffset, tTotLength,
					iStartBlock,
					aiBBD, tBBDLen)) {
				aucBuffer = xfree(aucBuffer);
				return FALSE;
			}
		}
		break;
	}
	aucBuffer = xfree(aucBuffer);
	return TRUE;
} /* end of bGet6DocumentData */
