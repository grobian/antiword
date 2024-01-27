/*
 * datalist.c
 * Copyright (C) 2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Build, read and destroy a list of Word data blocks
 */

#include <stdlib.h>
#include <errno.h>
#include "antiword.h"

#if defined(__riscos)
#define EIO		42
#endif /* __riscos */


/*
 * Private structure to hide the way the information
 * is stored from the rest of the program
 */
typedef struct data_mem_tag {
	data_block_type		tInfo;
	struct data_mem_tag	*pNext;
} data_mem_type;

/* Variable to describe the start of the data block list */
static data_mem_type	*pAnchor = NULL;
/* Variable needed to read the data block list */
static data_mem_type	*pBlockLast = NULL;
/* Variable needed to read the data block list */
static data_mem_type	*pBlockCurrent = NULL;
static size_t	tBlockOffset = 0;
static size_t	tByteNext = 0;
/* Last block read */
static unsigned char	aucBlock[BIG_BLOCK_SIZE];


/*
 * vDestroyDataBlockList - destroy the data block list
 */
void
vDestroyDataBlockList(void)
{
	data_mem_type	*pCurr, *pNext;

	DBG_MSG("vDestroyDataBlockList");

	pCurr = pAnchor;
	while (pCurr != NULL) {
		pNext = pCurr->pNext;
		pCurr = xfree(pCurr);
		pCurr = pNext;
	}
	pAnchor = NULL;
	/* Reset all the control variables */
	pBlockLast = NULL;
	pBlockCurrent = NULL;
	tBlockOffset = 0;
	tByteNext = 0;
} /* end of vDestroyDataBlockList */

/*
 * bAdd2DataBlockList - add an element to the data block list
 *
 * Returns TRUE when successful, otherwise FALSE
 */
BOOL
bAdd2DataBlockList(data_block_type *pDataBlock)
{
	data_mem_type	*pListMember;

	fail(pDataBlock == NULL);
	fail(pDataBlock->lFileOffset < 0);
	fail(pDataBlock->lDataOffset < 0);
	fail(pDataBlock->tLength == 0);

	NO_DBG_MSG("bAdd2DataBlockList");
	NO_DBG_HEX(pDataBlock->lFileOffset);
	NO_DBG_HEX(pDataBlock->lDataOffset);
	NO_DBG_HEX(pDataBlock->tLength);

	if (pDataBlock->lFileOffset < 0 ||
	    pDataBlock->lDataOffset < 0 ||
	    pDataBlock->tLength == 0) {
		werr(0, "Software (datablock) error");
		return FALSE;
	}
	/* Check for continuous blocks */
	if (pBlockLast != NULL &&
	    pBlockLast->tInfo.lFileOffset +
	     (long)pBlockLast->tInfo.tLength == pDataBlock->lFileOffset &&
	    pBlockLast->tInfo.lDataOffset +
	     (long)pBlockLast->tInfo.tLength == pDataBlock->lDataOffset) {
		/* These are continous blocks */
		pBlockLast->tInfo.tLength += pDataBlock->tLength;
		return TRUE;
	}
	/* Make a new block */
	pListMember = xmalloc(sizeof(data_mem_type));
	/* Add the block to the data list */
	pListMember->tInfo = *pDataBlock;
	pListMember->pNext = NULL;
	if (pAnchor == NULL) {
		pAnchor = pListMember;
	} else {
		fail(pBlockLast == NULL);
		pBlockLast->pNext = pListMember;
	}
	pBlockLast = pListMember;
	return TRUE;
} /* end of bAdd2DataBlockList */

/*
 * bSetDataOffset - set the offset in the data block list
 *
 * Make the given fileofset the current position in the data block list
 */
BOOL
bSetDataOffset(FILE *pFile, long lFileOffset)
{
	data_mem_type	*pCurr;
	size_t	tReadLen;

	DBG_HEX(lFileOffset);

	for (pCurr = pAnchor; pCurr != NULL; pCurr = pCurr->pNext) {
		if (lFileOffset < pCurr->tInfo.lFileOffset ||
		    lFileOffset >= pCurr->tInfo.lFileOffset +
		     (long)pCurr->tInfo.tLength) {
			/* The file offset is not in this block */
			continue;
		}
		/* Compute the maximum number of bytes to read */
		tReadLen = (size_t)(pCurr->tInfo.lFileOffset +
				(long)pCurr->tInfo.tLength -
				lFileOffset);
		/* Compute the real number of bytes to read */
		if (tReadLen > sizeof(aucBlock)) {
			tReadLen = sizeof(aucBlock);
		}
		/* Read the bytes */
		if (!bReadBytes(aucBlock, tReadLen, lFileOffset, pFile)) {
			return FALSE;
		}
		/* Set the control variables */
		pBlockCurrent = pCurr;
		tBlockOffset = (size_t)(lFileOffset - pCurr->tInfo.lFileOffset);
		tByteNext = 0;
		return TRUE;
	}
	return FALSE;
} /* end of bSetDataOffset */

/*
 * iNextByte - get the next byte from the data block list
 */
int
iNextByte(FILE *pFile)
{
	long	lReadOff;
	size_t	tReadLen;

	fail(pBlockCurrent == NULL);

	if (tByteNext >= sizeof(aucBlock) ||
	    tBlockOffset + tByteNext >= pBlockCurrent->tInfo.tLength) {
		if (tBlockOffset + sizeof(aucBlock) <
					pBlockCurrent->tInfo.tLength) {
			/* Same block, next part */
			tBlockOffset += sizeof(aucBlock);
		} else {
			/* Next block, first part */
			pBlockCurrent = pBlockCurrent->pNext;
			tBlockOffset = 0;
		}
		if (pBlockCurrent == NULL) {
			/* Past the last part of the last block */
			errno = EIO;
			return EOF;
		}
		tReadLen = pBlockCurrent->tInfo.tLength - tBlockOffset;
		if (tReadLen > sizeof(aucBlock)) {
			tReadLen = sizeof(aucBlock);
		}
		lReadOff = pBlockCurrent->tInfo.lFileOffset +
				(long)tBlockOffset;
		if (!bReadBytes(aucBlock, tReadLen, lReadOff, pFile)) {
			errno = EIO;
			return EOF;
		}
		tByteNext = 0;
	}
	return (int)aucBlock[tByteNext++];
} /* end of iNextByte */

/*
 * usNextWord - get the next word from the data block list
 *
 * Read a two byte value in Little Endian order, that means MSB last
 *
 * All return values can be valid so errno is set in case of error
 */
unsigned short
usNextWord(FILE *pFile)
{
	unsigned short	usLSB, usMSB;

	usLSB = (unsigned short)iNextByte(pFile);
	if (usLSB == (unsigned short)EOF) {
		errno = EIO;
		return (unsigned short)EOF;
	}
	usMSB = (unsigned short)iNextByte(pFile);
	if (usMSB == (unsigned short)EOF) {
		DBG_MSG("usNextWord: Unexpected EOF");
		errno = EIO;
		return (unsigned short)EOF;
	}
	return (usMSB << 8) | usLSB;
} /* end of usNextWord */

/*
 * ulNextLong - get the next long from the data block list
 *
 * Read a four byte value in Little Endian order, that means MSW last
 *
 * All return values can be valid so errno is set in case of error
 */
unsigned long
ulNextLong(FILE *pFile)
{
	unsigned long	ulLSW, ulMSW;

	ulLSW = usNextWord(pFile);
	if (ulLSW == (unsigned long)EOF) {
		errno = EIO;
		return (unsigned long)EOF;
	}
	ulMSW = usNextWord(pFile);
	if (ulMSW == (unsigned long)EOF) {
		DBG_MSG("ulNextLong: Unexpected EOF");
		errno = EIO;
		return (unsigned long)EOF;
	}
	return (ulMSW << 16) | ulLSW;
} /* end of ulNextLong */

/*
 * usNextWordBE - get the next two byte value
 *
 * Read a two byte value in Big Endian order, that means MSB first
 *
 * All return values can be valid so errno is set in case of error
 */
unsigned short
usNextWordBE(FILE *pFile)
{
	unsigned short usLSB, usMSB;

	usMSB = (unsigned short)iNextByte(pFile);
	if (usMSB == (unsigned short)EOF) {
		errno = EIO;
		return (unsigned short)EOF;
	}
	usLSB = (unsigned short)iNextByte(pFile);
	if (usLSB == (unsigned short)EOF) {
		DBG_MSG("usNextWordBE: Unexpected EOF");
		errno = EIO;
		return (unsigned short)EOF;
	}
	return (usMSB << 8) | usLSB;
} /* end of usNextWordBE */

/*
 * ulNextLongBE - get the next four byte value
 *
 * Read a four byte value in Big Endian order, that means MSW first
 *
 * All return values can be valid so errno is set in case of error
 */
unsigned long
ulNextLongBE(FILE *pFile)
{
	unsigned long ulLSW, ulMSW;

	ulMSW = usNextWordBE(pFile);
	if (ulMSW == (unsigned long)EOF) {
		errno = EIO;
		return (unsigned long)EOF;
	}
	ulLSW = usNextWordBE(pFile);
	if (ulLSW == (unsigned long)EOF) {
		DBG_MSG("ulNextLongBE: Unexpected EOF");
		errno = EIO;
		return (unsigned long)EOF;
	}
	return (ulMSW << 16) | ulLSW;
} /* end of ulNextLongBE */

/*
 * iSkipBytes - skip over the given number of bytes
 *
 * Returns the number of skipped bytes
 */
int
iSkipBytes(FILE *pFile, size_t tToSkip)
{
	size_t	tToGo, tMaxMove, tMove;

	fail(pFile == NULL);
	fail(pBlockCurrent == NULL);

	tToGo = tToSkip;
	while (tToGo != 0) {
		/* Goto the end of the current block */
		tMaxMove = min(sizeof(aucBlock) -
				tByteNext,
				pBlockCurrent->tInfo.tLength -
				tBlockOffset -
				tByteNext);
		tMove = min(tMaxMove, tToGo);
		tByteNext += tMove;
		tToGo -= tMove;
		if (tToGo != 0) {
			/* Goto the next block */
			if (iNextByte(pFile) == EOF) {
				return (int)(tToSkip - tToGo);
			}
			tToGo--;
		}
	}
	return (int)tToSkip;
} /* end of iSkipBytes */

/*
 * Translate the start from the begin of the data to an offset in the file.
 *
 * Returns:	 -1: in case of error
 *		>=0: the computed file offset
 */
long
lDataOffset2FileOffset(long lDataOffset)
{
	data_mem_type	*pCurr;

	fail(lDataOffset < 0);

	for (pCurr = pAnchor; pCurr != NULL; pCurr = pCurr->pNext) {
		if (lDataOffset < pCurr->tInfo.lDataOffset ||
		    lDataOffset >= pCurr->tInfo.lDataOffset +
		     (long)pCurr->tInfo.tLength) {
			/* The data offset is not in this block, try the next */
			continue;
		}
		/* The data offset is in the current block */
		return pCurr->tInfo.lFileOffset +
				lDataOffset -
				pCurr->tInfo.lDataOffset;
	}
	/* Passed beyond the end of the list */
	DBG_HEX_C(lDataOffset != 0, lDataOffset);
	return -1;
} /* end of lDataOffset2FileOffset */
