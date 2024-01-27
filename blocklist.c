/*
 * blocklist.c
 * Copyright (C) 1998-2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Build, read and destroy a list of Word text blocks
 */

#include <stdlib.h>
#include "antiword.h"


/*
 * Privat structure to hide the way the information
 * is stored from the rest of the program
 */
typedef struct list_mem_tag {
	text_block_type		tInfo;
	struct list_mem_tag	*pNext;
} list_mem_type;

/* Variables to describe the start of the five block lists */
static list_mem_type	*pTextAnchor = NULL;
static list_mem_type	*pFootAnchor = NULL;
static list_mem_type	*pUnused1Anchor = NULL;
static list_mem_type	*pEndAnchor = NULL;
static list_mem_type	*pUnused2Anchor = NULL;
/* Variable needed to read the block list */
static list_mem_type	*pBlockLast = NULL;
/* Variable needed to read the text block list */
static list_mem_type	*pTextBlockCurrent = NULL;
/* Variable needed to read the footnote block list */
static list_mem_type	*pFootBlockCurrent = NULL;
/* Variable needed to read the endnote block list */
static list_mem_type	*pEndBlockCurrent = NULL;
/* Last block read */
static unsigned char	aucBlock[BIG_BLOCK_SIZE];


/*
 * vDestroyTextBlockList - destroy the text block list
 */
void
vDestroyTextBlockList(void)
{
	list_mem_type	*apAnchor[5];
	list_mem_type	*pCurr, *pNext;
	int	iIndex;

	DBG_MSG("vDestroyTextBlockList");

	apAnchor[0] = pTextAnchor;
	apAnchor[1] = pFootAnchor;
	apAnchor[2] = pUnused1Anchor;
	apAnchor[3] = pEndAnchor;
	apAnchor[4] = pUnused2Anchor;

	for (iIndex = 0; iIndex < 5; iIndex++) {
		pCurr = apAnchor[iIndex];
		while (pCurr != NULL) {
			pNext = pCurr->pNext;
			pCurr = xfree(pCurr);
			pCurr = pNext;
		}
	}
	/* Show that there are no lists any more */
	pTextAnchor = NULL;
	pFootAnchor = NULL;
	pUnused1Anchor = NULL;
	pEndAnchor = NULL;
	pUnused2Anchor = NULL;
	/* Reset all the controle variables */
	pBlockLast = NULL;
	pTextBlockCurrent = NULL;
	pFootBlockCurrent = NULL;
} /* end of vDestroyTextBlockList */

/*
 * bAdd2TextBlockList - add an element to the text block list
 *
 * returns: TRUE when successful, otherwise FALSE
 */
BOOL
bAdd2TextBlockList(text_block_type *pTextBlock)
{
	list_mem_type	*pListMember;

	fail(pTextBlock == NULL);
	fail(pTextBlock->lFileOffset < 0);
	fail(pTextBlock->lTextOffset < 0);
	fail(pTextBlock->tLength == 0);
	fail(pTextBlock->bUsesUnicode && odd(pTextBlock->tLength));

	NO_DBG_MSG("bAdd2TextBlockList");
	NO_DBG_HEX(pTextBlock->lFileOffset);
	NO_DBG_HEX(pTextBlock->lTextOffset);
	NO_DBG_HEX(pTextBlock->tLength);
	NO_DBG_DEC(pTextBlock->bUsesUnicode);

	if (pTextBlock->lFileOffset < 0 ||
	    pTextBlock->lTextOffset < 0 ||
	    pTextBlock->tLength == 0) {
		werr(0, "Software (textblock) error");
		return FALSE;
	}
	/* Check for continuous blocks of the same character size */
	if (pBlockLast != NULL &&
	    pBlockLast->tInfo.lFileOffset +
	     (long)pBlockLast->tInfo.tLength == pTextBlock->lFileOffset &&
	    pBlockLast->tInfo.lTextOffset +
	     (long)pBlockLast->tInfo.tLength == pTextBlock->lTextOffset &&
	    pBlockLast->tInfo.bUsesUnicode == pTextBlock->bUsesUnicode) {
		/* These are continous blocks */
		pBlockLast->tInfo.tLength += pTextBlock->tLength;
		return TRUE;
	}
	/* Make a new block */
	pListMember = xmalloc(sizeof(list_mem_type));
	/* Add the block to the list */
	pListMember->tInfo = *pTextBlock;
	pListMember->pNext = NULL;
	if (pTextAnchor == NULL) {
		pTextAnchor = pListMember;
	} else {
		fail(pBlockLast == NULL);
		pBlockLast->pNext = pListMember;
	}
	pBlockLast = pListMember;
	return TRUE;
} /* end of bAdd2TextBlockList */


/*
 * vSplitBlockList - split the block list in three parts
 *
 * Split the blocklist in a Text block list, a Footnote block list, a
 * Endnote block list and two Unused lists.
 *
 * NOTE:
 * The various i*Len input parameters are given in characters, but the
 * length of the blocks are in bytes.
 */
void
vSplitBlockList(size_t tTextLen, size_t tFootnoteLen, size_t tUnused1Len,
	size_t tEndnoteLen, size_t tUnused2Len, BOOL bMustExtend)
{
	list_mem_type	*apAnchors[5];
	list_mem_type	*pGarbageAnchor, *pCurr, *pNext;
	int		iIndex, iCharsToGo, iBytesTooFar;
#if defined(DEBUG)
	size_t		tTotal;
#endif /* DEBUG */

	DBG_MSG("vSplitBlockList");

/* Text block list */
	pCurr = NULL;
	iCharsToGo = (int)tTextLen;
	iBytesTooFar = -1;
	if (tTextLen != 0) {
		DBG_MSG("Text block list");
		DBG_DEC(tTextLen);
		for (pCurr = pTextAnchor;
		     pCurr != NULL;
		     pCurr = pCurr->pNext) {
			NO_DBG_DEC(pCurr->tInfo.tLength);
			fail(pCurr->tInfo.tLength == 0);
			if (pCurr->tInfo.bUsesUnicode) {
				fail(odd(pCurr->tInfo.tLength));
				iCharsToGo -= (int)(pCurr->tInfo.tLength / 2);
				if (iCharsToGo < 0) {
					iBytesTooFar = -2 * iCharsToGo;
				}
			} else {
				iCharsToGo -= (int)pCurr->tInfo.tLength;
				if (iCharsToGo < 0) {
					iBytesTooFar = -iCharsToGo;
				}
			}
			if (iCharsToGo <= 0) {
				break;
			}
		}
	}
/* Split the list */
	if (tTextLen == 0) {
		/* Empty text blocks list */
		pFootAnchor = pTextAnchor;
		pTextAnchor = NULL;
	} else if (pCurr == NULL) {
		/* No footnote blocks */
		pFootAnchor = NULL;
	} else if (iCharsToGo == 0) {
		/* Move the integral number of footnote blocks */
		pFootAnchor = pCurr->pNext;
		pCurr->pNext = NULL;
	} else {
		/* Split the part-text block, part-footnote block */
		DBG_DEC(iBytesTooFar);
		fail(iBytesTooFar <= 0);
		pFootAnchor = xmalloc(sizeof(list_mem_type));
		DBG_HEX(pCurr->tInfo.lFileOffset);
		pFootAnchor->tInfo.lFileOffset =
				pCurr->tInfo.lFileOffset +
				(long)pCurr->tInfo.tLength -
				iBytesTooFar;
		DBG_HEX(pFootAnchor->tInfo.lFileOffset);
		DBG_HEX(pCurr->tInfo.lTextOffset);
		pFootAnchor->tInfo.lTextOffset =
				pCurr->tInfo.lTextOffset +
				(long)pCurr->tInfo.tLength -
				iBytesTooFar;
		DBG_HEX(pFootAnchor->tInfo.lTextOffset);
		pFootAnchor->tInfo.tLength = (size_t)iBytesTooFar;
		pCurr->tInfo.tLength -= (size_t)iBytesTooFar;
		pFootAnchor->tInfo.bUsesUnicode = pCurr->tInfo.bUsesUnicode;
		/* Move the integral number of footnote blocks */
		pFootAnchor->pNext = pCurr->pNext;
		pCurr->pNext = NULL;
	}
/* Footnote block list */
	pCurr = NULL;
	iCharsToGo = (int)tFootnoteLen;
	iBytesTooFar = -1;
	if (tFootnoteLen != 0) {
		DBG_MSG("Footnote block list");
		DBG_DEC(tFootnoteLen);
		for (pCurr = pFootAnchor;
		     pCurr != NULL;
		     pCurr = pCurr->pNext) {
			DBG_DEC(pCurr->tInfo.tLength);
			fail(pCurr->tInfo.tLength == 0);
			if (pCurr->tInfo.bUsesUnicode) {
				fail(odd(pCurr->tInfo.tLength));
				iCharsToGo -= (int)(pCurr->tInfo.tLength / 2);
				if (iCharsToGo < 0) {
					iBytesTooFar = -2 * iCharsToGo;
				}
			} else {
				iCharsToGo -= (int)pCurr->tInfo.tLength;
				if (iCharsToGo < 0) {
					iBytesTooFar = -iCharsToGo;
				}
			}
			if (iCharsToGo <= 0) {
				break;
			}
		}
	}
/* Split the list */
	if (tFootnoteLen == 0) {
		/* Empty footnote list */
		pUnused1Anchor = pFootAnchor;
		pFootAnchor = NULL;
	} else if (pCurr == NULL) {
		/* No unused1 blocks */
		pUnused1Anchor = NULL;
	} else if (iCharsToGo == 0) {
		/* Move the integral number of unused1-list blocks */
		pUnused1Anchor = pCurr->pNext;
		pCurr->pNext = NULL;
	} else {
	  	/* Split the part-footnote block, part-unused1 block */
		DBG_DEC(iBytesTooFar);
		fail(iBytesTooFar <= 0);
		pUnused1Anchor = xmalloc(sizeof(list_mem_type));
		DBG_HEX(pCurr->tInfo.lFileOffset);
		pUnused1Anchor->tInfo.lFileOffset =
				pCurr->tInfo.lFileOffset +
				(long)pCurr->tInfo.tLength -
				iBytesTooFar;
		DBG_HEX(pUnused1Anchor->tInfo.lFileOffset);
		DBG_HEX(pCurr->tInfo.lTextOffset);
		pUnused1Anchor->tInfo.lTextOffset =
				pCurr->tInfo.lTextOffset +
				(long)pCurr->tInfo.tLength -
				iBytesTooFar;
		DBG_HEX(pUnused1Anchor->tInfo.lTextOffset);
		pUnused1Anchor->tInfo.tLength = (size_t)iBytesTooFar;
		pCurr->tInfo.tLength -= (size_t)iBytesTooFar;
		pUnused1Anchor->tInfo.bUsesUnicode =
				pCurr->tInfo.bUsesUnicode;
		/* Move the integral number of unused1 blocks */
		pUnused1Anchor->pNext = pCurr->pNext;
		pCurr->pNext = NULL;
	}
/* Unused1 block list */
	pCurr = NULL;
	iCharsToGo = (int)tUnused1Len;
	iBytesTooFar = -1;
	if (tUnused1Len != 0) {
		DBG_MSG("Unused1 block list");
		DBG_DEC(tUnused1Len);
		for (pCurr = pUnused1Anchor;
		     pCurr != NULL;
		     pCurr = pCurr->pNext) {
			DBG_DEC(pCurr->tInfo.tLength);
			fail(pCurr->tInfo.tLength == 0);
			if (pCurr->tInfo.bUsesUnicode) {
				fail(odd(pCurr->tInfo.tLength));
				iCharsToGo -= (int)(pCurr->tInfo.tLength / 2);
				if (iCharsToGo < 0) {
					iBytesTooFar = -2 * iCharsToGo;
				}
			} else {
				iCharsToGo -= (int)pCurr->tInfo.tLength;
				if (iCharsToGo < 0) {
					iBytesTooFar = -iCharsToGo;
				}
			}
			if (iCharsToGo <= 0) {
				break;
			}
		}
	}
/* Split the list */
	if (tUnused1Len == 0) {
		/* Empty unused1 list */
		pEndAnchor = pUnused1Anchor;
		pUnused1Anchor = NULL;
	} else if (pCurr == NULL) {
		/* No endnote blocks */
		pEndAnchor = NULL;
	} else if (iCharsToGo == 0) {
		/* Move the intergral number of endnote blocks */
		pEndAnchor = pCurr->pNext;
		pCurr->pNext = NULL;
	} else {
		/* Split the part-unused1-list block, part-endnote block */
		DBG_DEC(iBytesTooFar);
		fail(iBytesTooFar <= 0);
		pEndAnchor = xmalloc(sizeof(list_mem_type));
		DBG_HEX(pCurr->tInfo.lFileOffset);
		pEndAnchor->tInfo.lFileOffset =
				pCurr->tInfo.lFileOffset +
				(long)pCurr->tInfo.tLength -
				iBytesTooFar;
		DBG_HEX(pEndAnchor->tInfo.lFileOffset);
		DBG_HEX(pCurr->tInfo.lTextOffset);
		pEndAnchor->tInfo.lTextOffset =
				pCurr->tInfo.lTextOffset +
				(long)pCurr->tInfo.tLength -
				iBytesTooFar;
		DBG_HEX(pEndAnchor->tInfo.lTextOffset);
		pEndAnchor->tInfo.tLength = (size_t)iBytesTooFar;
		pCurr->tInfo.tLength -= (size_t)iBytesTooFar;
		pEndAnchor->tInfo.bUsesUnicode = pCurr->tInfo.bUsesUnicode;
		/* Move the integral number of endnote blocks */
		pEndAnchor->pNext = pCurr->pNext;
		pCurr->pNext = NULL;
	}
/* Endnote block list */
	pCurr = NULL;
	iCharsToGo = (int)tEndnoteLen;
	iBytesTooFar = -1;
	if (tEndnoteLen != 0) {
		DBG_MSG("Endnote block list");
		DBG_DEC(tEndnoteLen);
		for (pCurr = pEndAnchor;
		     pCurr != NULL;
		     pCurr = pCurr->pNext) {
			DBG_DEC(pCurr->tInfo.tLength);
			fail(pCurr->tInfo.tLength == 0);
			if (pCurr->tInfo.bUsesUnicode) {
				fail(odd(pCurr->tInfo.tLength));
				iCharsToGo -= (int)(pCurr->tInfo.tLength / 2);
				if (iCharsToGo <= 0) {
					iBytesTooFar = -2 * iCharsToGo;
				}
			} else {
				iCharsToGo -= (int)pCurr->tInfo.tLength;
				if (iCharsToGo <= 0) {
					iBytesTooFar = -iCharsToGo;
				}
			}
			if (iCharsToGo <= 0) {
				break;
			}
		}
	}
/* Split the list */
	if (tEndnoteLen == 0) {
		/* Empty endnote list */
		pUnused2Anchor = pEndAnchor;
		pEndAnchor = NULL;
	} else if (pCurr == NULL) {
		/* No unused2 blocks */
		pUnused2Anchor = NULL;
	} else if (iCharsToGo == 0) {
		/* Move the intergral number of unused2 blocks */
		pUnused2Anchor = pCurr->pNext;
		pCurr->pNext = NULL;
	} else {
		/* Split the part-endnote block, part-unused2 block */
		DBG_DEC(iBytesTooFar);
		fail(iBytesTooFar <= 0);
		pUnused2Anchor = xmalloc(sizeof(list_mem_type));
		DBG_HEX(pCurr->tInfo.lFileOffset);
		pUnused2Anchor->tInfo.lFileOffset =
				pCurr->tInfo.lFileOffset +
				(long)pCurr->tInfo.tLength -
				iBytesTooFar;
		DBG_HEX(pUnused2Anchor->tInfo.lFileOffset);
		DBG_HEX(pCurr->tInfo.lTextOffset);
		pUnused2Anchor->tInfo.lTextOffset =
				pCurr->tInfo.lTextOffset +
				(long)pCurr->tInfo.tLength -
				iBytesTooFar;
		DBG_HEX(pUnused2Anchor->tInfo.lTextOffset);
		pUnused2Anchor->tInfo.tLength = (size_t)iBytesTooFar;
		pCurr->tInfo.tLength -= (size_t)iBytesTooFar;
		pUnused2Anchor->tInfo.bUsesUnicode =
				pCurr->tInfo.bUsesUnicode;
		/* Move the integral number of unused2 blocks */
		pUnused2Anchor->pNext = pCurr->pNext;
		pCurr->pNext = NULL;
	}
/* Unused2 block list */
	pCurr = NULL;
	iCharsToGo = (int)tUnused2Len;
	iBytesTooFar = -1;
	if (tUnused2Len != 0) {
		DBG_MSG("Unused2 block list");
		DBG_DEC(tUnused2Len);
		for (pCurr = pUnused2Anchor;
		     pCurr != NULL;
		     pCurr = pCurr->pNext) {
			DBG_DEC(pCurr->tInfo.tLength);
			fail(pCurr->tInfo.tLength == 0);
			if (pCurr->tInfo.bUsesUnicode) {
				fail(odd(pCurr->tInfo.tLength));
				iCharsToGo -= (int)(pCurr->tInfo.tLength / 2);
				if (iCharsToGo < 0) {
					iBytesTooFar = -2 * iCharsToGo;
				}
			} else {
				iCharsToGo -= (int)pCurr->tInfo.tLength;
				if (iCharsToGo < 0) {
					iBytesTooFar = -iCharsToGo;
				}
			}
			if (iCharsToGo <= 0) {
				break;
			}
		}
	}
/* Split the list */
	if (tUnused2Len == 0) {
		/* Empty unused2 list */
		pGarbageAnchor = pUnused2Anchor;
		pUnused2Anchor = NULL;
	} else if (pCurr == NULL) {
		/* No garbage block list */
		pGarbageAnchor = NULL;
	} else if (iCharsToGo == 0) {
		/* Move the intergral number of garbage blocks */
		pGarbageAnchor = pCurr->pNext;
	} else {
		/* Reduce the part-endnote block */
		DBG_DEC(iBytesTooFar);
		fail(iBytesTooFar <= 0);
		pCurr->tInfo.tLength -= (size_t)iBytesTooFar;
		/* Move the integral number of garbage blocks */
		pGarbageAnchor = pCurr->pNext;
		pCurr->pNext = NULL;
	}
/* Free the garbage block list, this should never be needed */
	pCurr = pGarbageAnchor;
	while (pCurr != NULL) {
		DBG_HEX(pCurr->tInfo.lFileOffset);
		DBG_DEC(pCurr->tInfo.tLength);
		pNext = pCurr->pNext;
		pCurr = xfree(pCurr);
		pCurr = pNext;
	}

#if defined(DEBUG)
	/* Check the number of bytes in the block lists */
	tTotal = 0;
	for (pCurr = pTextAnchor; pCurr != NULL; pCurr = pCurr->pNext) {
		NO_DBG_HEX(pCurr->tInfo.lFileOffset);
		NO_DBG_HEX(pCurr->tInfo.lTextOffset);
		NO_DBG_DEC(pCurr->tInfo.tLength);
		fail(pCurr->tInfo.tLength == 0);
		if (pCurr->tInfo.bUsesUnicode) {
			fail(odd(pCurr->tInfo.tLength));
			tTotal += pCurr->tInfo.tLength / 2;
		} else {
			tTotal += pCurr->tInfo.tLength;
		}
	}
	DBG_DEC(tTotal);
	if (tTotal != tTextLen) {
		DBG_DEC(tTextLen);
		werr(1, "Software error (Text)");
	}
	tTotal = 0;
	for (pCurr = pFootAnchor; pCurr != NULL; pCurr = pCurr->pNext) {
		DBG_HEX(pCurr->tInfo.lFileOffset);
		NO_DBG_HEX(pCurr->tInfo.lTextOffset);
		DBG_DEC(pCurr->tInfo.tLength);
		fail(pCurr->tInfo.tLength == 0);
		if (pCurr->tInfo.bUsesUnicode) {
			fail(odd(pCurr->tInfo.tLength));
			tTotal += pCurr->tInfo.tLength / 2;
		} else {
			tTotal += pCurr->tInfo.tLength;
		}
	}
	DBG_DEC(tTotal);
	if (tTotal != tFootnoteLen) {
		DBG_DEC(tFootnoteLen);
		werr(1, "Software error (Footnotes)");
	}
	tTotal = 0;
	for (pCurr = pUnused1Anchor; pCurr != NULL; pCurr = pCurr->pNext) {
		DBG_HEX(pCurr->tInfo.lFileOffset);
		NO_DBG_HEX(pCurr->tInfo.lTextOffset);
		DBG_DEC(pCurr->tInfo.tLength);
		fail(pCurr->tInfo.tLength == 0);
		if (pCurr->tInfo.bUsesUnicode) {
			fail(odd(pCurr->tInfo.tLength));
			tTotal += pCurr->tInfo.tLength / 2;
		} else {
			tTotal += pCurr->tInfo.tLength;
		}
	}
	DBG_DEC(tTotal);
	if (tTotal != tUnused1Len) {
		DBG_DEC(tUnused1Len);
		werr(1, "Software error (Unused1-list)");
	}
	tTotal = 0;
	for (pCurr = pEndAnchor; pCurr != NULL; pCurr = pCurr->pNext) {
		DBG_HEX(pCurr->tInfo.lFileOffset);
		NO_DBG_HEX(pCurr->tInfo.lTextOffset);
		DBG_DEC(pCurr->tInfo.tLength);
		fail(pCurr->tInfo.tLength == 0);
		if (pCurr->tInfo.bUsesUnicode) {
			fail(odd(pCurr->tInfo.tLength));
			tTotal += pCurr->tInfo.tLength / 2;
		} else {
			tTotal += pCurr->tInfo.tLength;
		}
	}
	DBG_DEC(tTotal);
	if (tTotal != tEndnoteLen) {
		DBG_DEC(tEndnoteLen);
		werr(1, "Software error (Endnotes)");
	}
	tTotal = 0;
	for (pCurr = pUnused2Anchor; pCurr != NULL; pCurr = pCurr->pNext) {
		DBG_HEX(pCurr->tInfo.lFileOffset);
		NO_DBG_HEX(pCurr->tInfo.lTextOffset);
		DBG_DEC(pCurr->tInfo.tLength);
		fail(pCurr->tInfo.tLength == 0);
		if (pCurr->tInfo.bUsesUnicode) {
			fail(odd(pCurr->tInfo.tLength));
			tTotal += pCurr->tInfo.tLength / 2;
		} else {
			tTotal += pCurr->tInfo.tLength;
		}
	}
	DBG_DEC(tTotal);
	if (tTotal != tUnused2Len) {
		DBG_DEC(tUnused2Len);
		werr(1, "Software error (Unused2-list)");
	}
#endif /* DEBUG */

	if (!bMustExtend) {
		return;
	}
	/*
	 * All blocks (except the last one) must have a length that
	 * is a multiple of the Big Block Size
	 */

	apAnchors[0] = pTextAnchor;
	apAnchors[1] = pFootAnchor;
	apAnchors[2] = pUnused1Anchor;
	apAnchors[3] = pEndAnchor;
	apAnchors[4] = pUnused2Anchor;

	for (iIndex = 0; iIndex < 5; iIndex++) {
		for (pCurr = apAnchors[iIndex];
		     pCurr != NULL;
		     pCurr = pCurr->pNext) {
			if (pCurr->pNext != NULL &&
			    pCurr->tInfo.tLength % BIG_BLOCK_SIZE != 0) {
				DBG_HEX(pCurr->tInfo.lFileOffset);
				DBG_HEX(pCurr->tInfo.lTextOffset);
				DBG_DEC(pCurr->tInfo.tLength);
				pCurr->tInfo.tLength /= BIG_BLOCK_SIZE;
				pCurr->tInfo.tLength++;
				pCurr->tInfo.tLength *= BIG_BLOCK_SIZE;
				DBG_DEC(pCurr->tInfo.tLength);
			}
		}
	}
} /* end of vSplitBlockList */

#if defined(__riscos)
/*
 * tGetDocumentLength - get the combined character length of the three lists
 *
 * returns: The total number of characters
 */
size_t
tGetDocumentLength(void)
{
	list_mem_type	*apAnchors[3];
	list_mem_type	*pCurr;
	size_t		tTotal;
	int		iIndex;

	DBG_MSG("uiGetDocumentLength");

	apAnchors[0] = pTextAnchor;
	apAnchors[1] = pFootAnchor;
	apAnchors[2] = pEndAnchor;

	tTotal = 0;
	for (iIndex = 0; iIndex < 3; iIndex++) {
		for (pCurr = apAnchors[iIndex];
		     pCurr != NULL;
		     pCurr = pCurr->pNext) {
			fail(pCurr->tInfo.tLength == 0);
			if (pCurr->tInfo.bUsesUnicode) {
				fail(odd(pCurr->tInfo.tLength));
				tTotal += pCurr->tInfo.tLength / 2;
			} else {
				tTotal += pCurr->tInfo.tLength;
			}
		}
	}
	DBG_DEC(tTotal);
	return tTotal;
} /* end of tGetDocumentLength */
#endif /* __riscos */

/*
 * usNextTextByte - get the next byte from the text block list
 */
static unsigned short
usNextTextByte(FILE *pFile, long *plOffset)
{
	static size_t	tByteNext = 0;
	static size_t	tBlockOffset = 0;
	long	lReadOff;
	size_t	tReadLen;

	if (pTextBlockCurrent == NULL ||
	    tByteNext >= sizeof(aucBlock) ||
	    tBlockOffset + tByteNext >= pTextBlockCurrent->tInfo.tLength) {
		if (pTextBlockCurrent == NULL) {
			/* First block, first part */
			pTextBlockCurrent = pTextAnchor;
			tBlockOffset = 0;
		} else if (tBlockOffset + sizeof(aucBlock) <
				pTextBlockCurrent->tInfo.tLength) {
			/* Same block, next part */
			tBlockOffset += sizeof(aucBlock);
		} else {
			/* Next block, first part */
			pTextBlockCurrent = pTextBlockCurrent->pNext;
			tBlockOffset = 0;
		}
		if (pTextBlockCurrent == NULL) {
			/* Past the last part of the last block */
			return (unsigned short)EOF;
		}
		tReadLen = pTextBlockCurrent->tInfo.tLength - tBlockOffset;
		if (tReadLen > sizeof(aucBlock)) {
			tReadLen = sizeof(aucBlock);
		}
		lReadOff = pTextBlockCurrent->tInfo.lFileOffset +
				(long)tBlockOffset;
		if (!bReadBytes(aucBlock, tReadLen, lReadOff, pFile)) {
			return (unsigned short)EOF;
		}
		tByteNext = 0;
	}
	if (plOffset != NULL) {
		*plOffset = pTextBlockCurrent->tInfo.lFileOffset +
			(long)tBlockOffset +
			(long)tByteNext;
	}
	return aucBlock[tByteNext++];
} /* end of usNextTextByte */

/*
 * usNextFootByte - get the next byte from the footnote block list
 */
static unsigned short
usNextFootByte(FILE *pFile, long *plOffset)
{
	static size_t	tByteNext = 0;
	static size_t	tBlockOffset = 0;
	long	lReadOff;
	size_t	tReadLen;

	if (pFootBlockCurrent == NULL ||
	    tByteNext >= sizeof(aucBlock) ||
	    tBlockOffset + tByteNext >= pFootBlockCurrent->tInfo.tLength) {
		if (pFootBlockCurrent == NULL) {
			/* First block, first part */
			pFootBlockCurrent = pFootAnchor;
			tBlockOffset = 0;
		} else if (tBlockOffset + sizeof(aucBlock) <
				pFootBlockCurrent->tInfo.tLength) {
			/* Same block, next part */
			tBlockOffset += sizeof(aucBlock);
		} else {
			/* Next block, first part */
			pFootBlockCurrent = pFootBlockCurrent->pNext;
			tBlockOffset = 0;
		}
		if (pFootBlockCurrent == NULL) {
			/* Past the last part of the last block */
			return (unsigned short)EOF;
		}
		tReadLen = pFootBlockCurrent->tInfo.tLength - tBlockOffset;
		if (tReadLen > sizeof(aucBlock)) {
			tReadLen = sizeof(aucBlock);
		}
		lReadOff = pFootBlockCurrent->tInfo.lFileOffset +
				(long)tBlockOffset;
		if (!bReadBytes(aucBlock, tReadLen, lReadOff, pFile)) {
			return (unsigned short)EOF;
		}
		tByteNext = 0;
	}
	if (plOffset != NULL) {
		*plOffset = pFootBlockCurrent->tInfo.lFileOffset +
			(long)tBlockOffset +
			(long)tByteNext;
	}
	return aucBlock[tByteNext++];
} /* end of usNextFootByte */

/*
 * usNextEndByte - get the next byte from the endnote block list
 */
static unsigned short
usNextEndByte(FILE *pFile, long *plOffset)
{
	static size_t	tByteNext = 0;
	static size_t	tBlockOffset = 0;
	long	lReadOff;
	size_t	tReadLen;

	if (pEndBlockCurrent == NULL ||
	    tByteNext >= sizeof(aucBlock) ||
	    tBlockOffset + tByteNext >= pEndBlockCurrent->tInfo.tLength) {
		if (pEndBlockCurrent == NULL) {
			/* First block, first part */
			pEndBlockCurrent = pEndAnchor;
			tBlockOffset = 0;
		} else if (tBlockOffset + sizeof(aucBlock) <
				pEndBlockCurrent->tInfo.tLength) {
			/* Same block, next part */
			tBlockOffset += sizeof(aucBlock);
		} else {
			/* Next block, first part */
			pEndBlockCurrent = pEndBlockCurrent->pNext;
			tBlockOffset = 0;
		}
		if (pEndBlockCurrent == NULL) {
			/* Past the last part of the last block */
			return (unsigned short)EOF;
		}
		tReadLen = pEndBlockCurrent->tInfo.tLength - tBlockOffset;
		if (tReadLen > sizeof(aucBlock)) {
			tReadLen = sizeof(aucBlock);
		}
		lReadOff = pEndBlockCurrent->tInfo.lFileOffset +
				(long)tBlockOffset;
		if (!bReadBytes(aucBlock, tReadLen, lReadOff, pFile)) {
			return (unsigned short)EOF;
		}
		tByteNext = 0;
	}
	if (plOffset != NULL) {
		*plOffset = pEndBlockCurrent->tInfo.lFileOffset +
			(long)tBlockOffset +
			(long)tByteNext;
	}
	return aucBlock[tByteNext++];
} /* end of usNextEndByte */

/*
 * usNextTextChar - get the next character from the text block list
 */
static unsigned short
usNextTextChar(FILE *pFile, long *plOffset)
{
	unsigned short	usLSB, usMSB;

	usLSB = usNextTextByte(pFile, plOffset);
	if (usLSB == (unsigned short)EOF) {
		return (unsigned short)EOF;
	}
	if (pTextBlockCurrent->tInfo.bUsesUnicode) {
		usMSB = usNextTextByte(pFile, NULL);
	} else {
		usMSB = 0x00;
	}
	if (usMSB == (unsigned short)EOF) {
		DBG_MSG("usNextTextChar: Unexpected EOF");
		DBG_HEX_C(plOffset != NULL, *plOffset);
		return (unsigned short)EOF;
	}
	return (usMSB << 8) | usLSB;
} /* end of usNextTextChar */

/*
 * usNextFootChar - get the next character from the footnote block list
 */
static unsigned short
usNextFootChar(FILE *pFile, long *plOffset)
{
	unsigned short	usLSB, usMSB;

	usLSB = usNextFootByte(pFile, plOffset);
	if (usLSB == (unsigned short)EOF) {
		return (unsigned short)EOF;
	}
	if (pFootBlockCurrent->tInfo.bUsesUnicode) {
		usMSB = usNextFootByte(pFile, NULL);
	} else {
		usMSB = 0x00;
	}
	if (usMSB == (unsigned short)EOF) {
		DBG_MSG("usNextFootChar: Unexpected EOF");
		DBG_HEX_C(plOffset != NULL, *plOffset);
		return (unsigned short)EOF;
	}
	return (usMSB << 8) | usLSB;
} /* end of usNextFootChar */

/*
 * usNextEndChar - get the next character from the endnote block list
 */
static unsigned short
usNextEndChar(FILE *pFile, long *plOffset)
{
	unsigned short	usLSB, usMSB;

	usLSB = usNextEndByte(pFile, plOffset);
	if (usLSB == (unsigned short)EOF) {
		return (unsigned short)EOF;
	}
	if (pEndBlockCurrent->tInfo.bUsesUnicode) {
		usMSB = usNextEndByte(pFile, NULL);
	} else {
		usMSB = 0x00;
	}
	if (usMSB == (unsigned short)EOF) {
		DBG_MSG("usNextEndChar: Unexpected EOF");
		DBG_HEX_C(plOffset != NULL, *plOffset);
		return (unsigned short)EOF;
	}
	return (usMSB << 8) | usLSB;
} /* end of usNextEndChar */

/*
 * usNextChar - get the next character from the given block list
 */
unsigned short
usNextChar(FILE *pFile, list_id_enum eListID, long *plOffset)
{
	fail(pFile == NULL);

	switch (eListID) {
	case text_list:
		return usNextTextChar(pFile, plOffset);
	case footnote_list:
		return usNextFootChar(pFile, plOffset);
	case endnote_list:
		return usNextEndChar(pFile, plOffset);
	default:
		if (plOffset != NULL) {
			*plOffset = -1;
		}
		return (unsigned short)EOF;
	}
} /* end of usNextChar */

/*
 * Translate the start from the begin of the text to an offset in the file.
 *
 * Returns:	 -1: in case of error
 *		>=0: the computed file offset
 */
long
lTextOffset2FileOffset(long lTextOffset)
{
	list_mem_type	*apAnchors[5];
	list_mem_type	*pCurr;
	int		iIndex;
	BOOL		bStartOfList;

	apAnchors[0] = pTextAnchor;
	apAnchors[1] = pFootAnchor;
	apAnchors[2] = pUnused1Anchor;
	apAnchors[3] = pEndAnchor;
	apAnchors[4] = pUnused2Anchor;

	bStartOfList = FALSE;

	for (iIndex = 0; iIndex < 5; iIndex++) {
		pCurr = apAnchors[iIndex];
		while (pCurr != NULL) {
			if (bStartOfList) {
				NO_DBG_DEC(iIndex);
				NO_DBG_HEX(pCurr->tInfo.lFileOffset);
				if (iIndex >= 4) {
					/* Past the last used byte */
					return -1;
				}
				return pCurr->tInfo.lFileOffset;
			}
			if (lTextOffset < pCurr->tInfo.lTextOffset ||
			    lTextOffset >= pCurr->tInfo.lTextOffset +
			     (long)pCurr->tInfo.tLength) {
				pCurr = pCurr->pNext;
				continue;
			}
			switch (iIndex) {
			case 0:
			case 1:
			case 3:
			/* The textoffset is in the current block */
				return pCurr->tInfo.lFileOffset +
					lTextOffset -
					pCurr->tInfo.lTextOffset;
			case 2:
			/* Use the start of the next non-empty list */
				bStartOfList = TRUE;
				break;
			case 4:
			/* In Unused2, means after the last used byte */
				return -1;
			default:
			/* This should not happen */
				return -1;
			}
			/* To the start of the next list */
			break;
		}
	}
	/* Passed beyond the end of the last list */
	NO_DBG_HEX(lTextOffset);
	return -1;
} /* end of lTextOffset2FileOffset */

/*
 * Get the sequence number beloning to the given file offset
 *
 * Returns the sequence number
 */
long
lGetSeqNumber(long lFileOffset)
{
	list_mem_type	*pCurr;
	long		lSeq;

	if (lFileOffset < 0) {
		return -1;
	}

	lSeq = 0;
	for (pCurr = pTextAnchor; pCurr != NULL; pCurr = pCurr->pNext) {
		if (lFileOffset >= pCurr->tInfo.lFileOffset &&
		    lFileOffset < pCurr->tInfo.lFileOffset +
		     (long)pCurr->tInfo.tLength) {
			/* The file offset is within the current textblock */
			return lSeq + lFileOffset - pCurr->tInfo.lFileOffset;
		}
		lSeq += (long)pCurr->tInfo.tLength;
	}
	return -1;
} /* end of lGetSeqNumber */
