/*
 * depot.c
 * Copyright (C) 1998-2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Functions to compute the depot offset
 */

#include "antiword.h"

#define SIZE_RATIO	(BIG_BLOCK_SIZE/SMALL_BLOCK_SIZE)

static int	*aiSmallBlockList = NULL;
static int	iSmallBlockListLen = 0;


/*
 * vDestroySmallBlockList - destroy the small block list
 */
void
vDestroySmallBlockList(void)
{
	DBG_MSG("vDestroySmallBlockList");

	aiSmallBlockList = xfree(aiSmallBlockList);
	iSmallBlockListLen = 0;
} /* end of vDestroySmalBlockList */

/*
 * vCreateSmallBlockList - create the small block list
 *
 * returns: TRUE when successful, otherwise FALSE
 */
BOOL
bCreateSmallBlockList(int iStartblock, const int *aiBBD, size_t tBBDLen)
{
	int	iIndex, iTmp;
	size_t	tSize;

	fail(aiSmallBlockList != NULL || iSmallBlockListLen != 0);
	fail(iStartblock < 0 && iStartblock != END_OF_CHAIN);
	fail(aiBBD == NULL || tBBDLen == 0);

	/* Find the length of the small block list */
	for (iSmallBlockListLen = 0, iTmp = iStartblock;
	     iSmallBlockListLen < (int)tBBDLen && iTmp != END_OF_CHAIN;
	     iSmallBlockListLen++, iTmp = aiBBD[iTmp]) {
		if (iTmp < 0 || iTmp >= (int)tBBDLen) {
			werr(1, "The Big Block Depot is corrupt");
		}
	}
	DBG_DEC(iSmallBlockListLen);

	if (iSmallBlockListLen <= 0) {
		/* There is no small block list */
		fail(iStartblock != END_OF_CHAIN);
		aiSmallBlockList = NULL;
		iSmallBlockListLen = 0;
		return TRUE;
	}

	/* Create the small block list */
	tSize = (size_t)iSmallBlockListLen * sizeof(int);
	aiSmallBlockList = xmalloc(tSize);
	for (iIndex = 0, iTmp = iStartblock;
	     iIndex < (int)tBBDLen && iTmp != END_OF_CHAIN;
	     iIndex++, iTmp = aiBBD[iTmp]) {
		if (iTmp < 0 || iTmp >= (int)tBBDLen) {
			werr(1, "The Big Block Depot is corrupt");
		}
		aiSmallBlockList[iIndex] = iTmp;
		DBG_DEC(aiSmallBlockList[iIndex]);
	}
	return TRUE;
} /* end of bCreateSmallBlockList */

/*
 * lDepotOffset - get the depot offset the block list
 */
long
lDepotOffset(int iIndex, size_t tBlockSize)
{
	int	iTmp1, iTmp2;

	fail(iIndex < 0);

	switch (tBlockSize) {
	case BIG_BLOCK_SIZE:
		return ((long)iIndex + 1) * BIG_BLOCK_SIZE;
	case SMALL_BLOCK_SIZE:
		iTmp1 = iIndex / SIZE_RATIO;
		iTmp2 = iIndex % SIZE_RATIO;
		if (aiSmallBlockList == NULL ||
		    iSmallBlockListLen <= iTmp1) {
			return 0;
		}
		return (((long)aiSmallBlockList[iTmp1] + 1) * SIZE_RATIO +
				iTmp2) * SMALL_BLOCK_SIZE;
	default:
		return 0;
	}
} /* end of lDepotOffset */
