/*
 * fontlist.c
 * Copyright (C) 1998-2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Build, read and destroy a list of Word font information
 */

#include <stdlib.h>
#include <stddef.h>
#include "antiword.h"


/* Variables needed to write the Font Information List */
static font_desc_type	*pAnchor = NULL;
static font_desc_type	*pFontLast = NULL;


/*
 * vDestroyFontInfoList - destroy the Font Information List
 */
void
vDestroyFontInfoList(void)
{
	font_desc_type	*pCurr, *pNext;

	DBG_MSG("vDestroyFontInfoList");

	/* Free the Font Information List */
	pCurr = pAnchor;
	while (pCurr != NULL) {
		pNext = pCurr->pNext;
		pCurr = xfree(pCurr);
		pCurr = pNext;
	}
	pAnchor = NULL;
	/* Reset all control variables */
	pFontLast = NULL;
} /* end of vDestroyFontInfoList */

/*
 * vAdd2FontInfoList - Add an element to the Font Information List
 */
void
vAdd2FontInfoList(const font_block_type *pFontBlock)
{
	font_desc_type	*pListMember;
	int		iRealSize;
	unsigned char	ucRealStyle;

	fail(pFontBlock == NULL);
	fail(pFontBlock->lFileOffset < -1);

	NO_DBG_MSG("bAdd2FontInfoList");

	if (pFontBlock->lFileOffset < 0) {
		/*
		 * This offset is really past the end of the file,
		 * so don't waste any memory by storing it.
		 */
		return;
	}

	iRealSize = pFontBlock->sFontsize;
	ucRealStyle = pFontBlock->ucFontstyle;
	if (bIsSmallCapitals(pFontBlock->ucFontstyle)) {
		/* Small capitals become normal capitals in a smaller font */
		iRealSize = (iRealSize * 8 + 5) / 10;
		ucRealStyle &= ~FONT_SMALL_CAPITALS;
		ucRealStyle |= FONT_CAPITALS;
	}
	if (iRealSize < MIN_FONT_SIZE) {
		iRealSize = MIN_FONT_SIZE;
	} else if (iRealSize > MAX_FONT_SIZE) {
		iRealSize = MAX_FONT_SIZE;
	}

	if (pFontLast != NULL &&
	    pFontLast->tInfo.ucFontnumber == pFontBlock->ucFontnumber &&
	    pFontLast->tInfo.sFontsize == iRealSize &&
	    pFontLast->tInfo.ucFontcolor == pFontBlock->ucFontcolor &&
	    pFontLast->tInfo.ucFontstyle == ucRealStyle) {
		/*
		 * The new record would be the same as the one last added
		 * to the list and is therefore redundant
		 */
		return;
	}

	NO_DBG_HEX(pFontBlock->iFileOffset);
	NO_DBG_DEC_C(pFontBlock->ucFontnumber != 0,
					pFontBlock->ucFontnumber);
	NO_DBG_DEC_C(pFontBlock->sFontsize != DEFAULT_FONT_SIZE,
					pFontBlock->sFontsize);
	NO_DBG_DEC_C(pFontBlock->ucFontcolor != 0,
					pFontBlock->ucFontcolor);
	NO_DBG_HEX_C(pFontBlock->ucFontstyle != 0x00,
					pFontBlock->ucFontstyle);

	if (pFontLast != NULL &&
	    pFontLast->tInfo.lFileOffset == pFontBlock->lFileOffset) {
		/*
		 * If two consecutive fonts share the same
		 * offset, remember only the last font
		 */
		fail(pFontLast->pNext != NULL);
		pFontLast->tInfo = *pFontBlock;
		return;
	}

	/* Create list member */
	pListMember = xmalloc(sizeof(font_desc_type));
	/* Fill the list member */
	pListMember->tInfo = *pFontBlock;
	pListMember->pNext = NULL;
	/* Correct the values where needed */
	pListMember->tInfo.sFontsize = (short)iRealSize;
	pListMember->tInfo.ucFontstyle = ucRealStyle;
	/* Add the new member to the list */
	if (pAnchor == NULL) {
		pAnchor = pListMember;
	} else {
		fail(pFontLast == NULL);
		pFontLast->pNext = pListMember;
	}
	pFontLast = pListMember;
} /* end of vAdd2FontInfoList */

/*
 * vReset2FontInfoList - Add a reset element to the Font Information List
 */
void
vReset2FontInfoList(long lFileOffset)
{
	font_block_type	tFontBlock;

	fail(lFileOffset < -1);

	NO_DBG_MSG("bReset2FontInfoList");

	if (lFileOffset < 0) {
		/*
		 * This offset is really past the end of the file,
		 * so don't waste any memory by storing it.
		 */
		return;
	}

	if (pFontLast == NULL) {
		/* There are no values to reset */
		return;
	}
	if (pFontLast->tInfo.ucFontstyle == FONT_REGULAR &&
	    pFontLast->tInfo.sFontsize == DEFAULT_FONT_SIZE &&
	    pFontLast->tInfo.ucFontnumber == 0 &&
	    pFontLast->tInfo.ucFontcolor == FONT_COLOR_DEFAULT) {
		/* All values are at their defaults, no reset is needed */
		return;
	}
	/* Copy and set the default values */
	tFontBlock = pFontLast->tInfo;
	tFontBlock.lFileOffset = lFileOffset;
	tFontBlock.ucFontnumber = 0;
	tFontBlock.sFontsize = DEFAULT_FONT_SIZE;
	tFontBlock.ucFontcolor = FONT_COLOR_DEFAULT;
	tFontBlock.ucFontstyle = FONT_REGULAR;
	/* Add the block to the list */
	vAdd2FontInfoList(&tFontBlock);
} /* end of vReset2FontInfoList */

/*
 * Get the record that follows the given recored in the Font Information List
 */
const font_block_type *
pGetNextFontInfoListItem(const font_block_type *pCurr)
{
	const font_desc_type	*pRecord;
	size_t	tOffset;

	if (pCurr == NULL) {
		if (pAnchor == NULL) {
			/* There are no records */
			return NULL;
		}
		/* The first record is the only one without a predecessor */
		return &pAnchor->tInfo;
	}
	tOffset = offsetof(font_desc_type, tInfo);
	/* Many casts to prevent alignment warnings */
	pRecord = (font_desc_type *)(void *)((char *)pCurr - tOffset);
	fail(pCurr != &pRecord->tInfo);
	if (pRecord->pNext == NULL) {
		/* The last record has no successor */
		return NULL;
	}
	return &pRecord->pNext->tInfo;
} /* end of pGetNextFontInfoListItem */
