/*
 * stylelist.c
 * Copyright (C) 1998-2000 A.J. van Os
 *
 * Description:
 * Build, read and destroy a list of Word style information
 */

#include <stdlib.h>
#include <ctype.h>
#include "antiword.h"


/* Variables needed to write the Style Information List */
static style_desc_type	*pAnchor = NULL;
static style_desc_type	*pStyleLast = NULL;
/* Variable needed to read the Style Information List */
static style_desc_type	*pStyleCurrent = NULL;


/*
 * vDestroyStyleInfoList - destroy the Style Information List
 */
void
vDestroyStyleInfoList(void)
{
	style_desc_type	*pCurr, *pNext;

	DBG_MSG("vDestroyStyleInfoList");

	/* Free the Style Information List */
	pCurr = pAnchor;
	while (pCurr != NULL) {
		pNext = pCurr->pNext;
		pCurr = xfree(pCurr);
		pCurr = pNext;
	}
	pAnchor = NULL;
	/* Reset all control variables */
	pStyleLast = NULL;
	pStyleCurrent = NULL;
} /* end of vDestroyStyleInfoList */

/*
 * ucChooseListCharacter - choose our list character
 */
static unsigned char
ucChooseListCharacter(unsigned char ucListType, unsigned char ucListChar)
{
	if (isprint(ucListChar)) {
		return ucListChar;
	}
	if (ucListType == LIST_BULLETS) {
		switch (ucListChar) {
		case 0xa8:
			return OUR_DIAMOND;
		case 0xb7:
			return OUR_BULLET;
		case 0xde:
			return '=';
		case 0xe0:
			return 'o';
		case 0xfe:
			return ' ';
		default:
			DBG_HEX(ucListChar);
			return OUR_BULLET;
		}
	}
	return '.';
} /* end of ucChooseListCharacter */

/*
 * vAdd2StyleInfoList - Add an element to the Style Information List
 */
void
vAdd2StyleInfoList(const style_block_type *pStyleBlock)
{
	static BOOL	bMustStore = FALSE;
	style_desc_type	*pListMember;

	fail(pStyleBlock == NULL);
	fail(pStyleBlock->lFileOffset < -1);

	NO_DBG_MSG("bAdd2StyleInfoList");

	if (pStyleBlock->lFileOffset < 0) {
		return;
	}

	switch (pStyleBlock->ucStyle) {
	case 1: case 2: case 3:
	case 4: case 5: case 6:
	case 7: case 8: case 9:
		/* These styles are the nine header levels */
		break;
	default:
		if (pStyleBlock->sLeftIndent > 0 ||
		    pStyleBlock->sRightIndent < 0 ||
		    pStyleBlock->ucAlignment != 0x00 ||
		    bMustStore) {
			break;
		}
		/*
		 * When reached here, the information is not useful to
		 * the current implementation, so we save memory by
		 * not storing them.
		 */
		return;
	}

	NO_DBG_HEX(pStyleBlock->lFileOffset);
	NO_DBG_DEC_C(pStyleBlock->sLeftIndent != 0,
		pStyleBlock->sLeftIndent);
	NO_DBG_DEC_C(pStyleBlock->sRightIndent != 0,
		pStyleBlock->sRightIndent);
	NO_DBG_DEC_C(pStyleBlock->bInList, pStyleBlock->bInList);
	NO_DBG_DEC_C(pStyleBlock->bUnmarked, pStyleBlock->bUnmarked);
	NO_DBG_DEC_C(pStyleBlock->ucStyle != 0, pStyleBlock->ucStyle);
	NO_DBG_DEC_C(pStyleBlock->ucAlignment != 0, pStyleBlock->ucAlignment);
	NO_DBG_DEC_C(pStyleBlock->bInList, pStyleBlock->ucListType);
	NO_DBG_HEX_C(pStyleBlock->bInList, pStyleBlock->ucListCharacter);

	/*
	 * Always store the information about the paragraph
	 * following a member of a list.
	 */
	bMustStore = pStyleBlock->bInList;

	if (pStyleLast != NULL &&
	    pStyleLast->tInfo.lFileOffset == pStyleBlock->lFileOffset) {
		/*
		 * If two consecutive styles share the same
		 * offset, remember only the last style
		 */
		fail(pStyleLast->pNext != NULL);
		pStyleLast->tInfo = *pStyleBlock;
		return;
	}

	/* Create list member */
	pListMember = xmalloc(sizeof(style_desc_type));
	/* Fill the list member */
	pListMember->tInfo = *pStyleBlock;
	pListMember->pNext = NULL;
	/* Correct the values where needed */
	if (pListMember->tInfo.sLeftIndent < 0) {
		pListMember->tInfo.sLeftIndent = 0;
	}
	if (pListMember->tInfo.sRightIndent > 0) {
		pListMember->tInfo.sRightIndent = 0;
	}
	if (pListMember->tInfo.ucListLevel > 8) {
		pListMember->tInfo.ucListLevel = 8;
	}
	pListMember->tInfo.ucListCharacter =
		ucChooseListCharacter(pListMember->tInfo.ucListType,
				pListMember->tInfo.ucListCharacter);
	/* Add the new member to the list */
	if (pAnchor == NULL) {
		pAnchor = pListMember;
		pStyleCurrent = pListMember;
	} else {
		fail(pStyleLast == NULL);
		pStyleLast->pNext = pListMember;
	}
	pStyleLast = pListMember;
} /* end of vAdd2StyleInfoList */

/*
 * Get the next item in the Style Information List
 */
const style_block_type *
pGetNextStyleInfoListItem(void)
{
	const style_block_type	*pItem;

	if (pStyleCurrent == NULL) {
		return NULL;
	}
	pItem = &pStyleCurrent->tInfo;
	pStyleCurrent = pStyleCurrent->pNext;
	return pItem;
} /* end of pGetNextStyleInfoListItem */
