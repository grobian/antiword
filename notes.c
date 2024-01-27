/*
 * notes.c
 * Copyright (C) 1998-2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Functions to tell the difference between footnotes and endnotes
 */

#include "antiword.h"

/* Variables needed to write the Footnote and Endnote Lists */
static long	*alFootnoteList = NULL;
static size_t	tFootnoteListLength = 0;
static long	*alEndnoteList = NULL;
static size_t	tEndnoteListLength = 0;


/*
 * Destroy the lists with footnote and endnote information
 */
void
vDestroyNotesInfoLists(void)
{
	DBG_MSG("vDestroyNotesInfoLists");

	/* Free the lists and reset all control variables */
	alEndnoteList = xfree(alEndnoteList);
	alFootnoteList = xfree(alFootnoteList);
	tEndnoteListLength = 0;
	tFootnoteListLength = 0;
} /* end of vDestroyNotesInfoLists */

/*
 * Build the list with footnote information for Word 6/7 files
 */
static void
vGet6FootnotesInfo(FILE *pFile, int iStartBlock,
	const int *aiBBD, size_t tBBDLen,
	const unsigned char *aucHeader)
{
	unsigned char	*aucBuffer;
	long	lFileOffset, lBeginOfText, lOffset;
	size_t	tBeginFootnoteInfo, tFootnoteInfoLen;
	int	iIndex;

	fail(pFile == NULL || aucHeader == NULL);
	fail(iStartBlock < 0);
	fail(aiBBD == NULL);

	lBeginOfText = (long)ulGetLong(0x18, aucHeader);
	DBG_HEX(lBeginOfText);
	tBeginFootnoteInfo = (size_t)ulGetLong(0x68, aucHeader);
	DBG_HEX(tBeginFootnoteInfo);
	tFootnoteInfoLen = (size_t)ulGetLong(0x6c, aucHeader);
	DBG_DEC(tFootnoteInfoLen);

	if (tFootnoteInfoLen < 10) {
		DBG_MSG("No Footnotes in this document");
		return;
	}

	aucBuffer = xmalloc(tFootnoteInfoLen);
	if (!bReadBuffer(pFile, iStartBlock,
			aiBBD, tBBDLen, BIG_BLOCK_SIZE,
			aucBuffer, tBeginFootnoteInfo, tFootnoteInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return;
	}
	NO_DBG_PRINT_BLOCK(aucBuffer, tFootnoteInfoLen);

	fail(tFootnoteListLength != 0);
	tFootnoteListLength = (tFootnoteInfoLen - 4) / 6;
	fail(tFootnoteListLength == 0);

	fail(alFootnoteList != NULL);
	alFootnoteList = xmalloc(sizeof(long) * tFootnoteListLength);

	for (iIndex = 0; iIndex < (int)tFootnoteListLength; iIndex++) {
		lOffset = (long)ulGetLong(iIndex * 4, aucBuffer);
		DBG_HEX(lOffset);
		lFileOffset = lTextOffset2FileOffset(lBeginOfText + lOffset);
		DBG_HEX(lFileOffset);
		alFootnoteList[iIndex] = lFileOffset;
	}
	aucBuffer = xfree(aucBuffer);
} /* end of vGet6FootnotesInfo */

/*
 * Build the list with endnote information for Word 6/7 files
 */
static void
vGet6EndnotesInfo(FILE *pFile, int iStartBlock,
	const int *aiBBD, size_t tBBDLen,
	const unsigned char *aucHeader)
{
	unsigned char	*aucBuffer;
	long	lFileOffset, lBeginOfText, lOffset;
	size_t	tBeginEndnoteInfo, tEndnoteInfoLen;
	int	iIndex;

	fail(pFile == NULL || aucHeader == NULL);
	fail(iStartBlock < 0);
	fail(aiBBD == NULL);

	lBeginOfText = (long)ulGetLong(0x18, aucHeader);
	DBG_HEX(lBeginOfText);
	tBeginEndnoteInfo = (size_t)ulGetLong(0x1d2, aucHeader);
	DBG_HEX(tBeginEndnoteInfo);
	tEndnoteInfoLen = (size_t)ulGetLong(0x1d6, aucHeader);
	DBG_DEC(tEndnoteInfoLen);

	if (tEndnoteInfoLen < 10) {
		DBG_MSG("No Endnotes in this document");
		return;
	}

	aucBuffer = xmalloc(tEndnoteInfoLen);
	if (!bReadBuffer(pFile, iStartBlock,
			aiBBD, tBBDLen, BIG_BLOCK_SIZE,
			aucBuffer, tBeginEndnoteInfo, tEndnoteInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return;
	}
	NO_DBG_PRINT_BLOCK(aucBuffer, tEndnoteInfoLen);

	fail(tEndnoteListLength != 0);
	tEndnoteListLength = (tEndnoteInfoLen - 4) / 6;
	fail(tEndnoteListLength == 0);

	fail(alEndnoteList != NULL);
	alEndnoteList = xmalloc(sizeof(long) * tEndnoteListLength);

	for (iIndex = 0; iIndex < (int)tEndnoteListLength; iIndex++) {
		lOffset = (long)ulGetLong(iIndex * 4, aucBuffer);
		DBG_HEX(lOffset);
		lFileOffset = lTextOffset2FileOffset(lBeginOfText + lOffset);
		DBG_HEX(lFileOffset);
		alEndnoteList[iIndex] = lFileOffset;
	}
	aucBuffer = xfree(aucBuffer);
} /* end of vGet6EndnotesInfo */

/*
 * Build the lists note information for Word 6/7 files
 */
static void
vGet6NotesInfo(FILE *pFile, int iStartBlock,
	const int *aiBBD, size_t tBBDLen,
	const unsigned char *aucHeader)
{
	vGet6FootnotesInfo(pFile, iStartBlock,
			aiBBD, tBBDLen, aucHeader);
	vGet6EndnotesInfo(pFile, iStartBlock,
			aiBBD, tBBDLen, aucHeader);
} /* end of vGet6NotesInfo */

/*
 * Build the list with footnote information for Word 8/97 files
 */
static void
vGet8FootnotesInfo(FILE *pFile, const pps_info_type *pPPS,
	const int *aiBBD, size_t tBBDLen, const int *aiSBD, size_t tSBDLen,
	const unsigned char *aucHeader)
{
	const int	*aiBlockDepot;
	unsigned char	*aucBuffer;
	long	lFileOffset, lBeginOfText, lOffset, lTableSize;
	size_t	tBeginFootnoteInfo, tFootnoteInfoLen;
	size_t	tBlockDepotLen, tBlockSize;
	int	iTableStartBlock;
	int	iIndex;
	unsigned short	usDocStatus;

	lBeginOfText = (long)ulGetLong(0x18, aucHeader);
	DBG_HEX(lBeginOfText);
	tBeginFootnoteInfo = (size_t)ulGetLong(0xaa, aucHeader);
	DBG_HEX(tBeginFootnoteInfo);
	tFootnoteInfoLen = (size_t)ulGetLong(0xae, aucHeader);
	DBG_DEC(tFootnoteInfoLen);

	if (tFootnoteInfoLen < 10) {
		DBG_MSG("No Footnotes in this document");
		return;
	}

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
		DBG_MSG("No notes information");
		return;
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
	aucBuffer = xmalloc(tFootnoteInfoLen);
	if (!bReadBuffer(pFile, iTableStartBlock,
			aiBlockDepot, tBlockDepotLen, tBlockSize,
			aucBuffer, tBeginFootnoteInfo, tFootnoteInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return;
	}
	NO_DBG_PRINT_BLOCK(aucBuffer, tFootnoteInfoLen);

	fail(tFootnoteListLength != 0);
	tFootnoteListLength = (tFootnoteInfoLen - 4) / 6;
	fail(tFootnoteListLength == 0);

	fail(alFootnoteList != NULL);
	alFootnoteList = xmalloc(sizeof(long) * tFootnoteListLength);

	for (iIndex = 0; iIndex < (int)tFootnoteListLength; iIndex++) {
		lOffset = (long)ulGetLong(iIndex * 4, aucBuffer);
		DBG_HEX(lOffset);
		lFileOffset = lTextOffset2FileOffset(lBeginOfText + lOffset);
		DBG_HEX(lFileOffset);
		alFootnoteList[iIndex] = lFileOffset;
	}
	aucBuffer = xfree(aucBuffer);
} /* end of vGet8FootnotesInfo */

/*
 * Build the list with endnote information for Word 8/97 files
 */
static void
vGet8EndnotesInfo(FILE *pFile, const pps_info_type *pPPS,
	const int *aiBBD, size_t tBBDLen, const int *aiSBD, size_t tSBDLen,
	const unsigned char *aucHeader)
{
	const int	*aiBlockDepot;
	unsigned char	*aucBuffer;
	long	lFileOffset, lBeginOfText, lOffset, lTableSize;
	size_t	tBeginEndnoteInfo, tEndnoteInfoLen, tBlockDepotLen, tBlockSize;
	int	iTableStartBlock;
	int	iIndex;
	unsigned short	usDocStatus;

	lBeginOfText = (long)ulGetLong(0x18, aucHeader);
	DBG_HEX(lBeginOfText);
	tBeginEndnoteInfo = (size_t)ulGetLong(0x20a, aucHeader);
	DBG_HEX(tBeginEndnoteInfo);
	tEndnoteInfoLen = (size_t)ulGetLong(0x20e, aucHeader);
	DBG_DEC(tEndnoteInfoLen);

	if (tEndnoteInfoLen < 10) {
		DBG_MSG("No Endnotes in this document");
		return;
	}

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
		DBG_MSG("No notes information");
		return;
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
	aucBuffer = xmalloc(tEndnoteInfoLen);
	if (!bReadBuffer(pFile, iTableStartBlock,
			aiBlockDepot, tBlockDepotLen, tBlockSize,
			aucBuffer, tBeginEndnoteInfo, tEndnoteInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return;
	}
	DBG_PRINT_BLOCK(aucBuffer, tEndnoteInfoLen);

	fail(tEndnoteListLength != 0);
	tEndnoteListLength = (tEndnoteInfoLen - 4) / 6;
	fail(tEndnoteListLength == 0);

	fail(alEndnoteList != NULL);
	alEndnoteList = xmalloc(sizeof(long) * tEndnoteListLength);

	for (iIndex = 0; iIndex < (int)tEndnoteListLength; iIndex++) {
		lOffset = (long)ulGetLong(iIndex * 4, aucBuffer);
		DBG_HEX(lOffset);
		lFileOffset = lTextOffset2FileOffset(lBeginOfText + lOffset);
		DBG_HEX(lFileOffset);
		alEndnoteList[iIndex] = lFileOffset;
	}
	aucBuffer = xfree(aucBuffer);
} /* end of vGet8EndnotesInfo */

/*
 * Build the lists with footnote and endnote information for Word 8/97 files
 */
static void
vGet8NotesInfo(FILE *pFile, const pps_info_type *pPPS,
	const int *aiBBD, size_t tBBDLen, const int *aiSBD, size_t tSBDLen,
	const unsigned char *aucHeader)
{
	vGet8FootnotesInfo(pFile, pPPS,
			aiBBD, tBBDLen, aiSBD, tSBDLen, aucHeader);
	vGet8EndnotesInfo(pFile, pPPS,
			aiBBD, tBBDLen, aiSBD, tSBDLen, aucHeader);
} /* end of vGet8NotesInfo */

/*
 * Build the lists with footnote and endnote information
 */
void
vGetNotesInfo(FILE *pFile, const pps_info_type *pPPS,
	const int *aiBBD, size_t tBBDLen, const int *aiSBD, size_t tSBDLen,
	const unsigned char *aucHeader, int iWordVersion)
{
	fail(pFile == NULL || pPPS == NULL || aucHeader == NULL);
	fail(iWordVersion < 6 || iWordVersion > 8);
	fail(aiBBD == NULL || aiSBD == NULL);

	switch (iWordVersion) {
	case 6:
	case 7:
		vGet6NotesInfo(pFile, pPPS->tWordDocument.iSb,
			aiBBD, tBBDLen, aucHeader);
		break;
	case 8:
		vGet8NotesInfo(pFile, pPPS,
			aiBBD, tBBDLen, aiSBD, tSBDLen, aucHeader);
		break;
	default:
		werr(0, "Sorry, no notes information");
		break;
	}
} /* end of vGetNotesInfo */

/*
 * Get the notetype of the note at the given fileoffset
 */
notetype_enum
eGetNotetype(long lFileOffset)
{
	int	iIndex;

	fail(alFootnoteList == NULL && tFootnoteListLength != 0);
	fail(alEndnoteList == NULL && tEndnoteListLength != 0);

	/* Go for the easy answer first */
	if (tFootnoteListLength == 0 && tEndnoteListLength == 0) {
		return notetype_is_unknown;
	}
	if (tEndnoteListLength == 0) {
		return notetype_is_footnote;
	}
	if (tFootnoteListLength == 0) {
		return notetype_is_endnote;
	}
	/* No easy answer, so we search */
	for (iIndex = 0; iIndex < (int)tFootnoteListLength; iIndex++) {
		if (alFootnoteList[iIndex] == lFileOffset) {
			return notetype_is_footnote;
		}
	}
	for (iIndex = 0; iIndex < (int)tEndnoteListLength; iIndex++) {
		if (alEndnoteList[iIndex] == lFileOffset) {
			return notetype_is_endnote;
		}
	}
	/* Not found */
	return notetype_is_unknown;
} /* end of eGetNotetype */
