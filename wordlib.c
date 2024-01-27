/*
 * wordlib.c
 * Copyright (C) 1998-2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Deal with the internals of a MS Word file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "antiword.h"

typedef struct pps_entry_tag {
	char	szName[32];
	int	iType;
	int	iNext;
	int	iPrev;
	int	iDir;
	int	iSb;
	long	lSize;
	int	iLevel;
} pps_entry_type;

static BOOL	bMacFile = FALSE;

/* Macro to make sure all such statements will be identical */
#define FREE_ALL()		\
	do {\
		vDestroySmallBlockList();\
		aiRootList = xfree(aiRootList);\
		aiSbdList = xfree(aiSbdList);\
		aiBbdList = xfree(aiBbdList);\
		aiSBD = xfree(aiSBD);\
		aiBBD = xfree(aiBBD);\
	} while(0)


/*
 * ulReadLong - read four bytes from the given file and offset
 */
static unsigned long
ulReadLong(FILE *pFile, long lOffset)
{
	unsigned char	aucBytes[4];

	fail(pFile == NULL || lOffset < 0);

	if (!bReadBytes(aucBytes, 4, lOffset, pFile)) {
		werr(1, "Read long %ld not possible", lOffset);
	}
	return ulGetLong(0, aucBytes);
} /* end of ulReadLong */

/*
 * vName2String - turn the name into a proper string.
 */
static void
vName2String(char *szName, const unsigned char *aucBytes, int iNameSize)
{
	char	*pcChar;
	int	iIndex;

	fail(aucBytes == NULL || szName == NULL);

	if (iNameSize <= 0) {
		szName[0] = '\0';
		return;
	}
	for (iIndex = 0, pcChar = szName;
	     iIndex < 2 * iNameSize;
	     iIndex += 2, pcChar++) {
		*pcChar = (char)aucBytes[iIndex];
	}
	szName[iNameSize - 1] = '\0';
} /* end of vName2String */

/*
 * tReadBlockIndices - read the Big/Small Block Depot indices
 *
 * Returns the number of indices read
 */
static size_t
tReadBlockIndices(FILE *pFile, int *aiBlockDepot, size_t tMaxRec, long lOffset)
{
	size_t	tDone;
	int	iIndex;
	unsigned char	aucBytes[BIG_BLOCK_SIZE];

	fail(pFile == NULL || aiBlockDepot == NULL);
	fail(tMaxRec == 0);
	fail(lOffset < 0);

	/* Read a big block with BBD or SBD indices */
	if (!bReadBytes(aucBytes, BIG_BLOCK_SIZE, lOffset, pFile)) {
		werr(0, "Reading big block from %ld is not possible", lOffset);
		return 0;
	}
	/* Split the big block into indices, an index is four bytes */
	tDone = min(tMaxRec, BIG_BLOCK_SIZE / 4);
	for (iIndex = 0; iIndex < (int)tDone; iIndex++) {
		aiBlockDepot[iIndex] = (int)ulGetLong(4 * iIndex, aucBytes);
		NO_DBG_DEC(aiBlockDepot[iIndex]);
	}
	return tDone;
} /* end of tReadBlockIndices */

/*
 * bGetBBD - get the Big Block Depot indices from the index-blocks
 */
static BOOL
bGetBBD(FILE *pFile, const int *aiDepot, size_t tDepotLen,
			int *aiBBD, size_t tBBDLen)
{
	long	lBegin;
	size_t	tToGo, tDone;
	int	iIndex;

	fail(pFile == NULL || aiDepot == NULL || aiBBD == NULL);

	DBG_MSG("bGetBBD");

	tToGo = tBBDLen;
	for (iIndex = 0; iIndex < (int)tDepotLen && tToGo != 0; iIndex++) {
		lBegin = ((long)aiDepot[iIndex] + 1) * BIG_BLOCK_SIZE;
		NO_DBG_HEX(lBegin);
		tDone = tReadBlockIndices(pFile, aiBBD, tToGo, lBegin);
		fail(tDone > tToGo);
		if (tDone == 0) {
			return FALSE;
		}
		aiBBD += tDone;
		tToGo -= tDone;
	}
	return tToGo == 0;
} /* end of bGetBBD */

/*
 * bGetSBD - get the Small Block Depot indices from the index-blocks
 */
static BOOL
bGetSBD(FILE *pFile, const int *aiDepot, size_t tDepotLen,
			int *aiSBD, size_t tSBDLen)
{
	long	lBegin;
	size_t	tToGo, tDone;
	int	iIndex;

	fail(pFile == NULL || aiDepot == NULL || aiSBD == NULL);

	DBG_MSG("bGetSBD");

	tToGo = tSBDLen;
	for (iIndex = 0; iIndex < (int)tDepotLen && tToGo != 0; iIndex++) {
		lBegin = ((long)aiDepot[iIndex] + 1) * BIG_BLOCK_SIZE;
		NO_DBG_HEX(lBegin);
		tDone = tReadBlockIndices(pFile, aiSBD, tToGo, lBegin);
		fail(tDone > tToGo);
		if (tDone == 0) {
			return FALSE;
		}
		aiSBD += tDone;
		tToGo -= tDone;
	}
	return tToGo == 0;
} /* end of bGetSBD */

/*
 * vComputePPSlevels - compute the levels of the Property Set Storage entries
 */
static void
vComputePPSlevels(pps_entry_type *atPPSlist, pps_entry_type *pNode,
			int iLevel, int iRecursionLevel)
{
	fail(atPPSlist == NULL || pNode == NULL);
	fail(iLevel < 0 || iRecursionLevel < 0);

	if (iRecursionLevel > 25) {
		/* This removes the possibility of an infinite recursion */
		DBG_DEC(iRecursionLevel);
		return;
	}
	if (pNode->iLevel <= iLevel) {
		/* Avoid entering a loop */
		DBG_DEC(iLevel);
		DBG_DEC(pNode->iLevel);
		return;
	}

	pNode->iLevel = iLevel;

	if (pNode->iDir != -1) {
		vComputePPSlevels(atPPSlist,
				&atPPSlist[pNode->iDir],
				iLevel + 1,
				iRecursionLevel + 1);
	}
	if (pNode->iNext != -1) {
		vComputePPSlevels(atPPSlist,
				&atPPSlist[pNode->iNext],
				iLevel,
				iRecursionLevel + 1);
	}
	if (pNode->iPrev != -1) {
		vComputePPSlevels(atPPSlist,
				&atPPSlist[pNode->iPrev],
				iLevel,
				iRecursionLevel + 1);
	}
} /* end of vComputePPSlevels */

/*
 * bGetPPS - search the Property Set Storage for three sets
 *
 * Return TRUE if the WordDocument PPS is found
 */
static BOOL
bGetPPS(FILE *pFile,
		const int *aiRootList, size_t tRootListLen,
		pps_info_type *pPPS)
{
	pps_entry_type	*atPPSlist;
	long	lBegin;
	size_t	tNbrOfPPS;
	int	iTmp, iIndex, iStartBlock, iOffset;
	int	iNameSize, iRootIndex;
	BOOL	bWord, bExcel;
	unsigned char	aucBytes[PROPERTY_SET_STORAGE_SIZE];

	fail(pFile == NULL || pPPS == NULL || aiRootList == NULL);

	DBG_MSG("bGetPPS");
	NO_DBG_DEC(tRootListLen);

	bWord = FALSE;
	bExcel = FALSE;
	(void)memset(pPPS, 0, sizeof(*pPPS));

	/* Read and store all the Property Set Storage entries */
	tNbrOfPPS = tRootListLen * BIG_BLOCK_SIZE / PROPERTY_SET_STORAGE_SIZE;
	atPPSlist = xmalloc(tNbrOfPPS * sizeof(pps_entry_type));
	iRootIndex = 0;
	for (iIndex = 0; iIndex < (int)tNbrOfPPS; iIndex++) {
		iTmp = iIndex * PROPERTY_SET_STORAGE_SIZE;
		iStartBlock = iTmp / BIG_BLOCK_SIZE;
		iOffset = iTmp % BIG_BLOCK_SIZE;
		lBegin = ((long)aiRootList[iStartBlock] + 1) * BIG_BLOCK_SIZE +
			iOffset;
		NO_DBG_HEX(lBegin);
		if (!bReadBytes(aucBytes, PROPERTY_SET_STORAGE_SIZE,
							lBegin, pFile)) {
			werr(0, "Reading PPS %d is not possible", iIndex);
			atPPSlist = xfree(atPPSlist);
			return FALSE;
		}
		iNameSize = (int)usGetWord(0x40, aucBytes);
		iNameSize = (iNameSize + 1) / 2;
		vName2String(atPPSlist[iIndex].szName, aucBytes, iNameSize);
		atPPSlist[iIndex].iType = (int)ucGetByte(0x42, aucBytes);
		if (atPPSlist[iIndex].iType == 5) {
			iRootIndex = iIndex;
		}
		atPPSlist[iIndex].iPrev = (int)ulGetLong(0x44, aucBytes);
		atPPSlist[iIndex].iNext = (int)ulGetLong(0x48, aucBytes);
		atPPSlist[iIndex].iDir = (int)ulGetLong(0x4c, aucBytes);
		atPPSlist[iIndex].iSb = (int)ulGetLong(0x74, aucBytes);
		atPPSlist[iIndex].lSize = (long)ulGetLong(0x78, aucBytes);
		atPPSlist[iIndex].iLevel = INT_MAX;
		if (atPPSlist[iIndex].iPrev < -1 ||
		    atPPSlist[iIndex].iPrev >= (int)tNbrOfPPS ||
		    atPPSlist[iIndex].iNext < -1 ||
		    atPPSlist[iIndex].iNext >= (int)tNbrOfPPS ||
		    atPPSlist[iIndex].iDir < -1 ||
		    atPPSlist[iIndex].iDir >= (int)tNbrOfPPS) {
			DBG_DEC(iIndex);
			DBG_DEC(atPPSlist[iIndex].iPrev);
			DBG_DEC(atPPSlist[iIndex].iNext);
			DBG_DEC(atPPSlist[iIndex].iDir);
			DBG_DEC(tNbrOfPPS);
			werr(0, "The Property Set Storage is damaged");
			atPPSlist = xfree(atPPSlist);
			return FALSE;
		}
	}

#if 0 /* defined(DEBUG) */
	DBG_MSG("Before");
	for (iIndex = 0; iIndex < (int)tNbrOfPPS; iIndex++) {
		DBG_MSG(atPPSlist[iIndex].szName);
		DBG_HEX(atPPSlist[iIndex].iDir);
		DBG_HEX(atPPSlist[iIndex].iPrev);
		DBG_HEX(atPPSlist[iIndex].iNext);
		DBG_DEC(atPPSlist[iIndex].iSb);
		DBG_HEX(atPPSlist[iIndex].lSize);
		DBG_DEC(atPPSlist[iIndex].iLevel);
	}
#endif /* DEBUG */

	/* Add level information to each entry */
	vComputePPSlevels(atPPSlist, &atPPSlist[iRootIndex], 0, 0);

	/* Check the entries on level 1 for the required information */
	NO_DBG_MSG("After");
	for (iIndex = 0; iIndex < (int)tNbrOfPPS; iIndex++) {
#if 0 /* defined(DEBUG) */
		DBG_MSG(atPPSlist[iIndex].szName);
		DBG_HEX(atPPSlist[iIndex].iDir);
		DBG_HEX(atPPSlist[iIndex].iPrev);
		DBG_HEX(atPPSlist[iIndex].iNext);
		DBG_DEC(atPPSlist[iIndex].iSb);
		DBG_HEX(atPPSlist[iIndex].lSize);
		DBG_DEC(atPPSlist[iIndex].iLevel);
#endif /* DEBUG */
		if (atPPSlist[iIndex].iLevel != 1 ||
		    atPPSlist[iIndex].iType != 2 ||
		    atPPSlist[iIndex].szName[0] == '\0' ||
		    atPPSlist[iIndex].lSize <= 0) {
			continue;
		}
		if (pPPS->tWordDocument.lSize <= 0 &&
		    STREQ(atPPSlist[iIndex].szName, "WordDocument")) {
			pPPS->tWordDocument.iSb = atPPSlist[iIndex].iSb;
			pPPS->tWordDocument.lSize = atPPSlist[iIndex].lSize;
			bWord = TRUE;
		} else if (pPPS->tData.lSize <= 0 &&
			   STREQ(atPPSlist[iIndex].szName, "Data")) {
			pPPS->tData.iSb = atPPSlist[iIndex].iSb;
			pPPS->tData.lSize = atPPSlist[iIndex].lSize;
		} else if (pPPS->t0Table.lSize <= 0 &&
			   STREQ(atPPSlist[iIndex].szName, "0Table")) {
			pPPS->t0Table.iSb = atPPSlist[iIndex].iSb;
			pPPS->t0Table.lSize = atPPSlist[iIndex].lSize;
		} else if (pPPS->t1Table.lSize <= 0 &&
			   STREQ(atPPSlist[iIndex].szName, "1Table")) {
			pPPS->t1Table.iSb = atPPSlist[iIndex].iSb;
			pPPS->t1Table.lSize = atPPSlist[iIndex].lSize;
		} else if (STREQ(atPPSlist[iIndex].szName, "Book") ||
			   STREQ(atPPSlist[iIndex].szName, "Workbook")) {
			bExcel = TRUE;
		}
	}

	/* Free the space for the Property Set Storage entries */
	atPPSlist = xfree(atPPSlist);

	/* Draw your conclusions */
	if (bWord) {
		return TRUE;
	}
	if (bExcel) {
		werr(0, "Sorry, but this is an Excel spreadsheet");
	} else {
		werr(0, "This OLE file does not contain a Word document");
	}
	return FALSE;
} /* end of bGetPPS */

/*
 * bGetDocumentText - make a list of the text blocks of a Word document
 *
 * Return TRUE when succesful, otherwise FALSE
 */
static BOOL
bGetDocumentText(FILE *pFile, const pps_info_type *pPPS,
		const int *aiBBD, size_t tBBDLen,
		const int *aiSBD, size_t tSBDLen,
		const unsigned char *aucHeader, int iWordVersion)
{
	long	lBeginOfText;
	size_t	tTextLen, tFootnoteLen, tEndnoteLen;
	size_t	tHeaderLen, tMacroLen, tAnnotationLen;
	size_t	tTextBoxLen, tHdrTextBoxLen;
	BOOL	bFarEastWord, bFastSaved, bEncrypted, bSuccess;
	text_info_enum	eResult;
	unsigned short	usDocStatus, usIdent;

	fail(pFile == NULL || pPPS == NULL);
	fail(aiBBD == NULL);
	fail(aiSBD == NULL);

	DBG_MSG("bGetDocumentText");

	/* Get the "magic number" from the header */
	usIdent = usGetWord(0x00, aucHeader);
	DBG_HEX(usIdent);
	bFarEastWord = usIdent == 0x8098 || usIdent == 0x8099 ||
			usIdent == 0xa697 || usIdent == 0xa699;
	/* Get the status flags from the header */
	usDocStatus = usGetWord(0x0a, aucHeader);
	DBG_HEX(usDocStatus);
	bFastSaved = (usDocStatus & BIT(2)) != 0;
	DBG_MSG_C(bFastSaved, "This document is Fast Saved");
	DBG_DEC_C(bFastSaved, (usDocStatus & 0x00f0) >> 4);
	bEncrypted = (usDocStatus & BIT(8)) != 0;
	if (bEncrypted) {
		werr(0, "Encrypted documents are not supported");
		return FALSE;
	}

	/* Get length information */
	lBeginOfText = (long)ulGetLong(0x18, aucHeader);
	DBG_HEX(lBeginOfText);
	if (iWordVersion == 6 || iWordVersion == 7) {
		tTextLen = (size_t)ulGetLong(0x34, aucHeader);
		tFootnoteLen = (size_t)ulGetLong(0x38, aucHeader);
		tHeaderLen = (size_t)ulGetLong(0x3c, aucHeader);
		tMacroLen = (size_t)ulGetLong(0x40, aucHeader);
		tAnnotationLen = (size_t)ulGetLong(0x44, aucHeader);
		tEndnoteLen = (size_t)ulGetLong(0x48, aucHeader);
		tTextBoxLen = (size_t)ulGetLong(0x4c, aucHeader);
		tHdrTextBoxLen = (size_t)ulGetLong(0x50, aucHeader);
	} else {
		tTextLen = (size_t)ulGetLong(0x4c, aucHeader);
		tFootnoteLen = (size_t)ulGetLong(0x50, aucHeader);
		tHeaderLen = (size_t)ulGetLong(0x54, aucHeader);
		tMacroLen = (size_t)ulGetLong(0x58, aucHeader);
		tAnnotationLen = (size_t)ulGetLong(0x5c, aucHeader);
		tEndnoteLen = (size_t)ulGetLong(0x60, aucHeader);
		tTextBoxLen = (size_t)ulGetLong(0x64, aucHeader);
		tHdrTextBoxLen = (size_t)ulGetLong(0x68, aucHeader);
	}
	DBG_DEC(tTextLen);
	DBG_DEC(tFootnoteLen);
	DBG_DEC(tHeaderLen);
	DBG_DEC(tMacroLen);
	DBG_DEC(tAnnotationLen);
	DBG_DEC(tEndnoteLen);
	DBG_DEC(tTextBoxLen);
	DBG_DEC(tHdrTextBoxLen);

	/* Make a list of the text blocks */
	switch (iWordVersion) {
	case 6:
	case 7:
		if (bFastSaved) {
			eResult = eGet6DocumentText(pFile,
					bFarEastWord,
					pPPS->tWordDocument.iSb,
					aiBBD, tBBDLen,
					aucHeader);
			bSuccess = eResult == text_success;
		} else {
		  	bSuccess = bAddTextBlocks(lBeginOfText,
				tTextLen +
				tFootnoteLen +
				tHeaderLen + tMacroLen + tAnnotationLen +
				tEndnoteLen +
				tTextBoxLen + tHdrTextBoxLen,
				bFarEastWord,
				pPPS->tWordDocument.iSb,
				aiBBD, tBBDLen);
		}
		break;
	case 8:
		eResult = eGet8DocumentText(pFile,
				pPPS,
				aiBBD, tBBDLen, aiSBD, tSBDLen,
				aucHeader);
		if (eResult == text_no_information && !bFastSaved) {
			/* Assume there is only one block */
			bSuccess = bAddTextBlocks(lBeginOfText,
				tTextLen +
				tFootnoteLen +
				tHeaderLen + tMacroLen + tAnnotationLen +
				tEndnoteLen +
				tTextBoxLen + tHdrTextBoxLen,
				FALSE,
				pPPS->tWordDocument.iSb,
				aiBBD, tBBDLen);
		} else {
			bSuccess = eResult == text_success;
		}
		break;
	default:
		werr(0, "This version of Word is not supported");
		bSuccess = FALSE;
		break;
	}

	if (bSuccess) {
		vSplitBlockList(tTextLen,
				tFootnoteLen,
				tHeaderLen + tMacroLen + tAnnotationLen,
				tEndnoteLen,
				tTextBoxLen + tHdrTextBoxLen,
				!bFastSaved && iWordVersion == 8);
	} else {
		vDestroyTextBlockList();
		werr(0, "I can't find the text of this document");
	}
	return bSuccess;
} /* end of bGetDocumentText */

/*
 * vGetDocumentData - make a list of the data blocks of a Word document
 */
static void
vGetDocumentData(FILE *pFile, const pps_info_type *pPPS,
		const int *aiBBD, size_t tBBDLen,
		const unsigned char *aucHeader, int iWordVersion)
{
	options_type	tOptions;
	long	lBeginOfText;
	BOOL	bFastSaved, bHasImages, bSuccess;
	unsigned short	usDocStatus;

	fail(pFile == NULL);
	fail(pPPS == NULL);
	fail(aiBBD == NULL);

	/* Get the options */
	vGetOptions(&tOptions);

	/* Get the status flags from the header */
	usDocStatus = usGetWord(0x0a, aucHeader);
	DBG_HEX(usDocStatus);
	bFastSaved = (usDocStatus & BIT(2)) != 0;
	bHasImages = (usDocStatus & BIT(3)) != 0;

	if (!bHasImages ||
	    !tOptions.bUseOutlineFonts ||
	    tOptions.eImageLevel == level_no_images) {
		/*
		 * No images in the document or text-only output or
		 * no images wanted, so no data blocks will be needed
		 */
		vDestroyDataBlockList();
		return;
	}

	/* Get length information */
	lBeginOfText = (long)ulGetLong(0x18, aucHeader);
	DBG_HEX(lBeginOfText);

	/* Make a list of the data blocks */
	switch (iWordVersion) {
	case 6:
	case 7:
		/*
		 * The data blocks are in the text stream. The text stream
		 * is in "fast saved" format or "normal saved" format
		 */
		if (bFastSaved) {
			bSuccess = bGet6DocumentData(pFile,
					pPPS->tWordDocument.iSb,
					aiBBD, tBBDLen,
					aucHeader);
		} else {
		  	bSuccess = bAddDataBlocks(lBeginOfText, SIZE_T_MAX,
				pPPS->tWordDocument.iSb, aiBBD, tBBDLen);
		}
		break;
	case 8:
		/*
		 * The data blocks are in the data stream. The data stream
		 * is always in "normal saved" format
		 */
		bSuccess = bAddDataBlocks(0, SIZE_T_MAX,
				pPPS->tData.iSb, aiBBD, tBBDLen);
		break;
	default:
		werr(0, "This version of Word is not supported");
		bSuccess = FALSE;
		break;
	}

	if (!bSuccess) {
		vDestroyDataBlockList();
		werr(0, "I can't find the data of this document");
	}
} /* end of vGetDocumentData */

/*
 * pInitFile - open and check the file
 */
static FILE *
pInitFile(const char *szFilename)
{
	FILE	*pFile;

	fail(szFilename == NULL || szFilename[0] == '\0');

	pFile = fopen(szFilename, "rb");
	if (pFile == NULL) {
		werr(0, "I can't open '%s' for reading", szFilename);
		return NULL;
	}
	return pFile;
} /* end of pInitFile */

static BOOL
bInitDoc(FILE *pFile, long lFilesize)
{
	pps_info_type	PPS_info;
	int	*aiBBD, *aiRootList, *aiBbdList;
	int	*aiSBD, *aiSbdList;
	long	lMaxBlock;
	size_t	tBBDLen, tSBDLen, tNumBbdBlocks, tRootListLen, tSize;
	int	iRootStartblock;
	int	iSbdStartblock, iSBLstartblock;
	int	iWordVersion, iIndex, iTmp;
	int	iMaxSmallBlock;
	BOOL	bResult;
	unsigned short	usIdent, usFib, usChse;
	unsigned char	aucHeader[HEADER_SIZE];

	fail(pFile == NULL);

	lMaxBlock = lFilesize / BIG_BLOCK_SIZE - 2;
	DBG_DEC(lMaxBlock);
	if (lMaxBlock < 1) {
		return FALSE;
	}
	tBBDLen = (size_t)(lMaxBlock + 1);
	tNumBbdBlocks = (size_t)ulReadLong(pFile, 0x2c);
	DBG_DEC(tNumBbdBlocks);
	if (tNumBbdBlocks > 109) {
		DBG_DEC(lFilesize);
		if (lFilesize > 109 * (BIG_BLOCK_SIZE/4) * BIG_BLOCK_SIZE) {
			werr(0, "I'm afraid this file is too big to handle.");
		} else {
			werr(0, "This file is probably damaged. Sorry.");
		}
		return FALSE;
	}
	iRootStartblock = (int)ulReadLong(pFile, 0x30);
	DBG_DEC(iRootStartblock);
	iSbdStartblock = (int)ulReadLong(pFile, 0x3c);
	DBG_DEC(iSbdStartblock);
	iSBLstartblock = (int)ulReadLong(pFile,
		((long)iRootStartblock + 1) * BIG_BLOCK_SIZE + 0x74);
	DBG_DEC(iSBLstartblock);
	iMaxSmallBlock = (int)(ulReadLong(pFile,
		((long)iRootStartblock + 1) *
		BIG_BLOCK_SIZE + 0x78) / SMALL_BLOCK_SIZE) - 1;
	DBG_DEC(iMaxSmallBlock);
	tSBDLen = (size_t)(iMaxSmallBlock + 1);
	/* All to be xmalloc-ed pointers to NULL */
	aiRootList = NULL;
	aiSbdList = NULL;
	aiBbdList = NULL;
	aiSBD = NULL;
	aiBBD = NULL;
/* Big Block Depot */
	tSize = tNumBbdBlocks * sizeof(int);
	aiBbdList = xmalloc(tSize);
	tSize = tBBDLen * sizeof(int);
	aiBBD = xmalloc(tSize);
	for (iIndex = 0; iIndex < (int)tNumBbdBlocks; iIndex++) {
		aiBbdList[iIndex] =
			(int)ulReadLong(pFile, 0x4c + 4 * (long)iIndex);
		NO_DBG_HEX(aiBbdList[iIndex]);
	}
	if (!bGetBBD(pFile, aiBbdList, tNumBbdBlocks, aiBBD, tBBDLen)) {
		FREE_ALL();
		return FALSE;
	}
	aiBbdList = xfree(aiBbdList);
/* Small Block Depot */
	tSize = tBBDLen * sizeof(int);
	aiSbdList = xmalloc(tSize);
	tSize = tSBDLen * sizeof(int);
	aiSBD = xmalloc(tSize);
	for (iIndex = 0, iTmp = iSbdStartblock;
	     iIndex < (int)tBBDLen && iTmp != END_OF_CHAIN;
	     iIndex++, iTmp = aiBBD[iTmp]) {
		if (iTmp < 0 || iTmp >= (int)tBBDLen) {
			werr(1, "The Big Block Depot is damaged");
		}
		aiSbdList[iIndex] = iTmp;
		NO_DBG_HEX(aiSbdList[iIndex]);
	}
	if (!bGetSBD(pFile, aiSbdList, tBBDLen, aiSBD, tSBDLen)) {
		FREE_ALL();
		return FALSE;
	}
	aiSbdList = xfree(aiSbdList);
/* Root list */
	for (tRootListLen = 0, iTmp = iRootStartblock;
	     tRootListLen < tBBDLen && iTmp != END_OF_CHAIN;
	     tRootListLen++, iTmp = aiBBD[iTmp]) {
		if (iTmp < 0 || iTmp >= (int)tBBDLen) {
			werr(1, "The Big Block Depot is damaged");
		}
	}
	if (tRootListLen == 0) {
		werr(0, "No Rootlist found");
		FREE_ALL();
		return FALSE;
	}
	tSize = tRootListLen * sizeof(int);
	aiRootList = xmalloc(tSize);
	for (iIndex = 0, iTmp = iRootStartblock;
	     iIndex < (int)tBBDLen && iTmp != END_OF_CHAIN;
	     iIndex++, iTmp = aiBBD[iTmp]) {
		if (iTmp < 0 || iTmp >= (int)tBBDLen) {
			werr(1, "The Big Block Depot is damaged");
		}
		aiRootList[iIndex] = iTmp;
		NO_DBG_DEC(aiRootList[iIndex]);
	}
	fail(tRootListLen != (size_t)iIndex);
	bResult = bGetPPS(pFile, aiRootList, tRootListLen, &PPS_info);
	aiRootList = xfree(aiRootList);
	if (!bResult) {
		FREE_ALL();
		return FALSE;
	}
/* Small block list */
	if (!bCreateSmallBlockList(iSBLstartblock, aiBBD, tBBDLen)) {
		FREE_ALL();
		return FALSE;
	}

	if (PPS_info.tWordDocument.lSize < MIN_SIZE_FOR_BBD_USE) {
		DBG_DEC(PPS_info.tWordDocument.lSize);
		FREE_ALL();
		werr(0, "I'm afraid the text stream of this file "
			"is too small to handle.");
		return FALSE;
	}
	/* Read the headerblock */
	if (!bReadBuffer(pFile, PPS_info.tWordDocument.iSb,
			aiBBD, tBBDLen, BIG_BLOCK_SIZE,
			aucHeader, 0, HEADER_SIZE)) {
		FREE_ALL();
		return FALSE;
	}
	usIdent = usGetWord(0x00, aucHeader);
	DBG_HEX(usIdent);
	fail(usIdent != 0x8098 &&	/* Word 7 for oriental languages */
	     usIdent != 0x8099 &&	/* Word 7 for oriental languages */
	     usIdent != 0xa5dc &&	/* Word 6 & 7 */
	     usIdent != 0xa5ec &&	/* Word 7 & 97 & 98 */
	     usIdent != 0xa697 &&	/* Word 7 for oriental languages */
	     usIdent != 0xa699);	/* Word 7 for oriental languages */
	usFib = usGetWord(0x02, aucHeader);
	DBG_DEC(usFib);
	if (usFib < 101) {
		FREE_ALL();
		werr(0, "This file is from a version of Word before Word 6.");
		return FALSE;
	}
	usChse = usGetWord(0x14, aucHeader);
	DBG_DEC(usChse);
	bMacFile = FALSE;
	/* Get the version of Word */
	switch (usFib) {
	case 101:
	case 102:
		iWordVersion = 6;
		DBG_MSG("Word 6 for Windows");
		break;
	case 103:
	case 104:
		switch (usChse) {
		case 0:
			iWordVersion = 7;
			DBG_MSG("Word 7 for Win95");
			break;
		case 256:
			iWordVersion = 6;
			DBG_MSG("Word 6 for Macintosh");
			bMacFile = TRUE;
			break;
		default:
			if (ucGetByte(0x05, aucHeader) == 0xe0) {
				iWordVersion = 7;
				DBG_MSG("Word 7 for Win95");
			} else {
				iWordVersion = 6;
				DBG_MSG("Word 6 for Macintosh");
				bMacFile = TRUE;
			}
		}
		break;
	default:
		iWordVersion = 8;
		DBG_MSG_C(usChse != 256, "Word97 for Win95/98/NT");
		DBG_MSG_C(usChse == 256, "Word98 for Macintosh");
		/* NOTE: No Macintosh character set !! */
		break;
	}
	bResult = bGetDocumentText(pFile, &PPS_info,
			aiBBD, tBBDLen, aiSBD, tSBDLen,
			aucHeader, iWordVersion);
	if (bResult) {
		vGetDocumentData(pFile, &PPS_info,
			aiBBD, tBBDLen, aucHeader, iWordVersion);
		vSetDefaultTabWidth(pFile, &PPS_info,
			aiBBD, tBBDLen, aiSBD, tSBDLen,
			aucHeader, iWordVersion);
		vGetParagraphInfo(pFile, &PPS_info,
			aiBBD, tBBDLen, aiSBD, tSBDLen,
			aucHeader, iWordVersion);
		vGetNotesInfo(pFile, &PPS_info,
			aiBBD, tBBDLen, aiSBD, tSBDLen,
			aucHeader, iWordVersion);
	}
	FREE_ALL();
	return bResult;
} /* end of bInitDoc */

/*
 * pOpenDocument - open and initialize the document
 *
 * Returns the pointer to the document file, or NULL in case of failure
 */
FILE *
pOpenDocument(const char *szFilename)
{
	FILE	*pFile;
	long	lFilesize;

	lFilesize = lGetFilesize(szFilename);
	if (lFilesize <= 0) {
		return NULL;
	}
	pFile = pInitFile(szFilename);
	if (pFile == NULL) {
		return NULL;
	}
	if (bInitDoc(pFile, lFilesize)) {
		return pFile;
	}
	(void)fclose(pFile);
	return NULL;
} /* end of pOpenDocument*/

/*
 * vCloseDocument
 */
void
vCloseDocument(FILE *pFile)
{
	DBG_MSG("vCloseDocument");

	/* Free the memory */
	vDestroyTextBlockList();
	vDestroyDataBlockList();
	vDestroyRowInfoList();
	vDestroyStyleInfoList();
	vDestroyFontInfoList();
	vDestroyPicInfoList();
	vDestroyNotesInfoLists();
	vDestroyFontTable();
	/* Close the file */
	(void)fclose(pFile);
} /* end of vCloseDocument */

/*
 * Common part of the file checking functions
 */
static BOOL
bCheckBytes(const char *szFilename,
	const unsigned char *aucBytes, size_t tBytes)
{
	FILE	*pFile;
	int	iIndex, iChar;
	BOOL	bResult;

	fail(aucBytes == NULL || tBytes == 0);

	if (szFilename == NULL || szFilename[0] == '\0') {
		return FALSE;
	}
	pFile = fopen(szFilename, "rb");
	if (pFile == NULL) {
		return FALSE;
	}
	bResult = TRUE;
	for (iIndex = 0; iIndex < (int)tBytes; iIndex++) {
		iChar = getc(pFile);
		if (iChar == EOF || iChar != (int)aucBytes[iIndex]) {
			DBG_HEX(iChar);
			DBG_HEX(aucBytes[iIndex]);
			bResult = FALSE;
			break;
		}
	}
	(void)fclose(pFile);
	return bResult;
} /* end of bCheckBytes */

/*
 * This function checks whether the given file is or is not a Word6 (or later)
 * document
 */
BOOL
bIsSupportedWordFile(const char *szFilename)
{
	static unsigned char	aucBytes[] =
		{ 0xd0, 0xcf, 0x11, 0xe0, 0xa1, 0xb1, 0x1a, 0xe1 };
	long	lSize;

	if (szFilename == NULL || szFilename[0] == '\0') {
		DBG_MSG("No file given");
		return FALSE;
	}
	lSize = lGetFilesize(szFilename);
	if (lSize < 3 * BIG_BLOCK_SIZE || lSize % BIG_BLOCK_SIZE != 0) {
		DBG_DEC(lSize);
		DBG_MSG("Size mismatch");
		return FALSE;
	}
	return bCheckBytes(szFilename, aucBytes, elementsof(aucBytes));
} /* end of bIsSupportedWordFile */

/*
 * This function checks whether the given file is or is not a "Word2, 4, 5"
 * document
 */
BOOL
bIsWord245File(const char *szFilename)
{
	static unsigned char	aucBytes[6][8] = {
		{ 0x31, 0xbe, 0x00, 0x00, 0x00, 0xab, 0x00, 0x00 },
		{ 0xdb, 0xa5, 0x2d, 0x00, 0x00, 0x00, 0x09, 0x04 },
		{ 0xdb, 0xa5, 0x2d, 0x00, 0x31, 0x40, 0x09, 0x08 },
		{ 0xdb, 0xa5, 0x2d, 0x00, 0x31, 0x40, 0x09, 0x0c },
		{ 0xfe, 0x37, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00 },
		{ 0xfe, 0x37, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00 },
	};
	int	iIndex;

	for (iIndex = 0; iIndex < (int)elementsof(aucBytes); iIndex++) {
		if (bCheckBytes(szFilename,
				aucBytes[iIndex],
				elementsof(aucBytes[iIndex]))) {
			return TRUE;
		}
	}
	return FALSE;
} /* end of bIsWord245File */

/*
 * This function checks whether the given file is or is not a RTF document
 */
BOOL
bIsRtfFile(const char *szFilename)
{
	static unsigned char	aucBytes[] =
		{ '{', '\\', 'r', 't', 'f', '1' };

	return bCheckBytes(szFilename, aucBytes, elementsof(aucBytes));
} /* end of bIsRtfFile */

/*
 * TRUE if the current file was made on an Apple Macintosh, otherwise FALSE.
 * This function hides the methode of how to find out from the rest of the
 * program.
 *
 * note: this should grow into a real function
 */
BOOL
bFileFromTheMac(void)
{
	return bMacFile;
} /* end of bOriginatesOnMac */
