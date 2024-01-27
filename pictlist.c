/*
 * pictlist.c
 * Copyright (C) 2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Build, read and destroy a list of Word picture information
 */

#include <stdlib.h>
#include "antiword.h"


/* Variables needed to write the Picture Information List */
static picture_desc_type	*pAnchor = NULL;
static picture_desc_type	*pPictureLast = NULL;


/*
 * vDestroyPicInfoList - destroy the Picture Information List
 */
void
vDestroyPicInfoList(void)
{
	picture_desc_type	*pCurr, *pNext;

	DBG_MSG("vDestroyPicInfoList");

	/* Free the Picture Information List */
	pCurr = pAnchor;
	while (pCurr != NULL) {
		pNext = pCurr->pNext;
		pCurr = xfree(pCurr);
		pCurr = pNext;
	}
	pAnchor = NULL;
	/* Reset all control variables */
	pPictureLast = NULL;
} /* end of vDestroyPicInfoList */

/*
 * vAdd2PicInfoList - Add an element to the Picture Information List
 */
void
vAdd2PicInfoList(const picture_block_type *pPictureBlock)
{
	picture_desc_type	*pListMember;

	fail(pPictureBlock == NULL);
	fail(pPictureBlock->lFileOffset < -1);
	fail(pPictureBlock->lFileOffsetPicture < -1);

	NO_DBG_MSG("bAdd2PicInfoList");

	if (pPictureBlock->lFileOffset < 0) {
		/*
		 * This offset is really past the end of the file,
		 * so don't waste any memory by storing it.
		 */
		return;
	}
	if (pPictureBlock->lFileOffsetPicture < 0) {
		/*
		 * The place where this picture is supposed to be stored
		 * doesn't exist.
		 */
		return;
	}

	NO_DBG_HEX(pPictureBlock->lFileOffset);
	NO_DBG_HEX(pPictureBlock->lFileOffsetPicture);
	NO_DBG_HEX(pPictureBlock->lPictureOffset);

	/* Create list member */
	pListMember = xmalloc(sizeof(picture_desc_type));
	/* Fill the list member */
	pListMember->tInfo = *pPictureBlock;
	pListMember->pNext = NULL;
	/* Add the new member to the list */
	if (pAnchor == NULL) {
		pAnchor = pListMember;
	} else {
		fail(pPictureLast == NULL);
		pPictureLast->pNext = pListMember;
	}
	pPictureLast = pListMember;
} /* end of vAdd2PicInfoList */

/*
 * Get the info with the given file offset from the Picture Information List
 */
long
lGetPicInfoListItem(long lFileOffset)
{
	picture_desc_type	*pCurr;

	for (pCurr = pAnchor; pCurr != NULL; pCurr = pCurr->pNext) {
		if (pCurr->tInfo.lFileOffset == lFileOffset) {
			return pCurr->tInfo.lFileOffsetPicture;
		}
	}
	return -1;
} /* end of lGetPicInfoListItem */
