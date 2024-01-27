/*
 * rowlist.c
 * Copyright (C) 1998-2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Build, read and destroy a list of Word table-row information
 */

#include <stdlib.h>
#include <string.h>
#include "antiword.h"

/*
 * Privat structures to hide the way the information
 * is stored from the rest of the program
 */
typedef struct row_unique_tag {
	long	lOffsetStart;
	long	lOffsetEnd;
} row_unique_type;
typedef struct row_common_tag {
	int	iColumnWidthSum;			/* In twips */
	short	asColumnWidth[TABLE_COLUMN_MAX+1];	/* In twips */
	unsigned char	ucNumberOfColumns;
} row_common_type;
typedef struct row_desc_tag {
	row_unique_type		tUnique;
	row_common_type		*pCommon;
	struct row_desc_tag	*pNext;
} row_desc_type;

/* Variables needed to write the Row Information List */
static row_desc_type	*pAnchor = NULL;
static row_desc_type	*pRowLast = NULL;
/* Variable needed to read the Row Information List */
static row_desc_type	*pRowCurrent = NULL;


/*
 * vDestroyRowInfoList - destroy the Row Information List
 */
void
vDestroyRowInfoList(void)
{
	row_desc_type	*pCurr, *pNext;

	DBG_MSG("vDestroyRowInfoList");

	/* Free the Row Information List */
	pCurr = pAnchor;
	while (pCurr != NULL) {
		pNext = pCurr->pNext;
		pCurr->pCommon = xfree(pCurr->pCommon);
		pCurr = xfree(pCurr);
		pCurr = pNext;
	}
	pAnchor = NULL;
	/* Reset all control variables */
	pRowLast = NULL;
	pRowCurrent = NULL;
} /* end of vDestroyRowInfoList */

/*
 * vAdd2RowInfoList - Add an element to the Row Information List
 */
void
vAdd2RowInfoList(const row_block_type *pRowBlock)
{
	row_desc_type	*pNew;
	short		*psTmp;
	int		iIndex;

	fail(pRowBlock == NULL);

	if (pRowBlock->lOffsetStart < 0 || pRowBlock->lOffsetEnd < 0) {
		DBG_HEX_C(pRowBlock->lOffsetStart >= 0,
			pRowBlock->lOffsetStart);
		DBG_HEX_C(pRowBlock->lOffsetEnd >= 0, pRowBlock->lOffsetEnd);
		return;
	}

	NO_DBG_HEX(pRowBlock->lOffsetStart);
	NO_DBG_HEX(pRowBlock->lOffsetEnd);
	NO_DBG_DEC(pRowBlock->ucNumberOfColumns);
	NO_DBG_DEC(pRowBlock->iColumnWidthSum);

	/* Create the new list member */
	pNew = xmalloc(sizeof(row_desc_type));
	pNew->pCommon = xmalloc(sizeof(row_common_type));
	/* Fill the new list member */
	pNew->tUnique.lOffsetStart = pRowBlock->lOffsetStart;
	pNew->tUnique.lOffsetEnd = pRowBlock->lOffsetEnd;
	pNew->pCommon->iColumnWidthSum = pRowBlock->iColumnWidthSum;
	(void)memcpy(pNew->pCommon->asColumnWidth,
		pRowBlock->asColumnWidth,
		sizeof(pNew->pCommon->asColumnWidth));
	pNew->pCommon->ucNumberOfColumns = pRowBlock->ucNumberOfColumns;
	pNew->pNext = NULL;
	/* Correct the values where needed */
	for (iIndex = 0, psTmp = pNew->pCommon->asColumnWidth;
	     iIndex < (int)pNew->pCommon->ucNumberOfColumns;
	     iIndex++, psTmp++) {
		if (*psTmp < 0) {
			*psTmp = 0;
			DBG_MSG("The column width was negative");
		}
	}
	/* Add the new member to the list */
	if (pAnchor == NULL) {
		pAnchor = pNew;
		pRowCurrent = pNew;
	} else {
		fail(pRowLast == NULL);
		pRowLast->pNext = pNew;
	}
	pRowLast = pNew;
} /* end of vAdd2RowInfoList */

/*
 * Get the next item in the Row Information List
 */
BOOL
bGetNextRowInfoListItem(row_block_type *pItem)
{
	fail(pItem == NULL);

	if (pItem == NULL || pRowCurrent == NULL) {
		return FALSE;
	}
	/* Get the information */
	pItem->lOffsetStart = pRowCurrent->tUnique.lOffsetStart;
	pItem->lOffsetEnd = pRowCurrent->tUnique.lOffsetEnd;
	pItem->iColumnWidthSum = pRowCurrent->pCommon->iColumnWidthSum;
	(void)memcpy(pItem->asColumnWidth,
		pRowCurrent->pCommon->asColumnWidth,
		sizeof(pItem->asColumnWidth));
	pItem->ucNumberOfColumns = pRowCurrent->pCommon->ucNumberOfColumns;
	/* Stand by for the next item in the list */
	pRowCurrent = pRowCurrent->pNext;
	return TRUE;
} /* end of bGetNextRowInfoListItem */
