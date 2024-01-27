/*
 * fonts.c
 * Copyright (C) 1998-2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Functions to deal with fonts (generic)
 */

#include <ctype.h>
#include <string.h>
#include "antiword.h"

/* Font Translation Table */
static size_t		tFontTableRecords = 0;
static font_table_type	*pFontTable = NULL;

/*
 * Find the given font in the font table
 *
 * returns the index into the FontTable, -1 if not found
 */
int
iGetFontByNumber(int iWordFontnumber, unsigned char ucFontstyle)
{
	int	iIndex;

	for (iIndex = 0; iIndex < (int)tFontTableRecords; iIndex++) {
		if (iWordFontnumber ==
				(int)pFontTable[iIndex].ucWordFontnumber &&
		    ucFontstyle == pFontTable[iIndex].ucFontstyle &&
		    pFontTable[iIndex].szOurFontname[0] != '\0') {
			return iIndex;
		}
	}
	return -1;
} /* end of iGetFontByNumber */

/*
 * szGetOurFontName - Get our font name
 *
 * return our font name from the given index, NULL if not found
 */
const char *
szGetOurFontname(int iIndex)
{
	if (iIndex < 0 || iIndex >= (int)tFontTableRecords) {
		return NULL;
	}
	return pFontTable[iIndex].szOurFontname;
} /* end of szGetOurFontname */

/*
 * Find the given font in the font table
 *
 * returns the Word font number, -1 if not found
 */
int
iFontname2Fontnumber(const char *szOurFontname, unsigned char ucFontstyle)
{
	int	iIndex;

	for (iIndex = 0; iIndex < (int)tFontTableRecords; iIndex++) {
		if (pFontTable[iIndex].ucFontstyle == ucFontstyle &&
		    STREQ(pFontTable[iIndex].szOurFontname, szOurFontname)) {
			return (int)pFontTable[iIndex].ucWordFontnumber;
		}
	}
	return -1;
} /* end of iGetFontByName */

/*
 * See if the fontname from the Word file matches the fontname from the
 * font translation file.
 * If iBytesPerChar is one than szWord is in ISO-8859-x (Word 6/7),
 * if iBytesPerChar is two than szWord is in Unicode (Word 8/97).
 */
static BOOL
bFontEqual(const char *szWord, const char *szTable, int iBytesPerChar)
{
	const char	*pcTmp1, *pcTmp2;

	fail(szWord == NULL || szTable == NULL);
	fail(iBytesPerChar != 1 && iBytesPerChar != 2);

	for (pcTmp1 = szWord, pcTmp2 = szTable;
	     *pcTmp1 != '\0';
	     pcTmp1 += iBytesPerChar, pcTmp2++) {
		if (iToUpper(*pcTmp1) != iToUpper(*pcTmp2)) {
			return FALSE;
		}
	}
	return *pcTmp2 == '\0';
} /* end of bFontEqual */

/*
 * vCreateFontTable - Create and initialize the internal font table
 */
static void
vCreateFontTable(void)
{
	font_table_type	*pTmp;
	size_t	tSize;
	int	iNbr;

	if (tFontTableRecords == 0) {
		pFontTable = NULL;
		return;
	}

	/* Create the font table */
	tSize = tFontTableRecords * sizeof(*pFontTable);
	pFontTable = xmalloc(tSize);
	/* Initialize the font table */
	(void)memset(pFontTable, 0, tSize);
	for (iNbr = 0, pTmp = pFontTable;
	     pTmp < pFontTable + tFontTableRecords;
	     iNbr++, pTmp++) {
		pTmp->ucWordFontnumber = (unsigned char)(iNbr / 4);
		switch (iNbr % 4) {
		case 0:
			pTmp->ucFontstyle = FONT_REGULAR;
			break;
		case 1:
			pTmp->ucFontstyle = FONT_BOLD;
			break;
		case 2:
			pTmp->ucFontstyle = FONT_ITALIC;
			break;
		case 3:
			pTmp->ucFontstyle = FONT_BOLD|FONT_ITALIC;
			break;
		default:
			DBG_DEC(iNbr);
			break;
		}
	}
} /* end of vCreateFontTable */

/*
 * vMinimizeFontTable - make the font table as small as possible
 */
static void
vMinimizeFontTable(void)
{
	const font_block_type	*pFont;
	font_table_type		*pTmp;
	int	iUnUsed;
	BOOL	bMustAddTableFont;

	NO_DBG_MSG("vMinimizeFontTable");

	if (pFontTable == NULL || tFontTableRecords == 0) {
		return;
	}

#if 0
	DBG_MSG("Before");
	DBG_DEC(tFontTableRecords);
	for (pTmp = pFontTable;
	     pTmp < pFontTable + tFontTableRecords;
	     pTmp++) {
		DBG_DEC(pTmp->ucWordFontnumber);
		DBG_HEX(pTmp->ucFontstyle);
		DBG_MSG(pTmp->szWordFontname);
		DBG_MSG(pTmp->szOurFontname);
	}
#endif /* DEBUG */

	/* Default font/style is by definition in use */
	pFontTable[0].ucInUse = 1;

	/* See which fonts/styles are really being used */
	bMustAddTableFont = TRUE;
	pFont = NULL;
	while((pFont = pGetNextFontInfoListItem(pFont)) != NULL) {
		pTmp = pFontTable + 4 * (int)pFont->ucFontnumber;
		if (bIsBold(pFont->ucFontstyle)) {
			pTmp++;
		}
		if (bIsItalic(pFont->ucFontstyle)) {
			pTmp += 2;
		}
		if (pTmp >= pFontTable + tFontTableRecords) {
			continue;
		}
		if (STREQ(pTmp->szOurFontname, TABLE_FONT)) {
			/* The table font is already present */
			bMustAddTableFont = FALSE;
		}
		pTmp->ucInUse = 1;
	}

	/* Remove the unused font entries from the font table */
	iUnUsed = 0;
	for (pTmp = pFontTable;
	     pTmp < pFontTable + tFontTableRecords;
	     pTmp++) {
		if (pTmp->ucInUse == 0) {
			iUnUsed++;
			continue;
		}
		if (iUnUsed > 0) {
			fail(pTmp - iUnUsed <= pFontTable);
			*(pTmp - iUnUsed) = *pTmp;
		}
	}
	tFontTableRecords -= (size_t)iUnUsed;
	if (bMustAddTableFont) {
		pTmp = pFontTable + tFontTableRecords;
	  	pTmp->ucWordFontnumber = (pTmp - 1)->ucWordFontnumber + 1;
		pTmp->ucFontstyle = FONT_REGULAR;
		pTmp->ucInUse = 1;
		strcpy(pTmp->szWordFontname, "Extra Table Font");
		strcpy(pTmp->szOurFontname, TABLE_FONT);
		tFontTableRecords++;
		iUnUsed--;
	}
	if (iUnUsed > 0) {
		/* Resize the font table */
		pFontTable = xrealloc(pFontTable,
				tFontTableRecords * sizeof(*pFontTable));
	}
#if 0
	/* Report the used but untranslated fontnames */
	for (pTmp = pFontTable;
	     pTmp < pFontTable + tFontTableRecords;
	     pTmp++) {
		if (pTmp->szOurFontname[0] == '\0') {
			DBG_MSG(pTmp->szWordFontname);
			werr(0, "%s is not in the FontNames file",
				pTmp->szWordFontname);
		}
	}
#endif /* 0 */
#if defined(DEBUG)
	DBG_MSG("After");
	DBG_DEC(tFontTableRecords);
	for (pTmp = pFontTable;
	     pTmp < pFontTable + tFontTableRecords;
	     pTmp++) {
		DBG_DEC(pTmp->ucWordFontnumber);
		DBG_HEX(pTmp->ucFontstyle);
		DBG_MSG(pTmp->szWordFontname);
		DBG_MSG(pTmp->szOurFontname);
	}
#endif /* DEBUG */
} /* end of vMinimizeFontTable */

/*
 * bReadFontFile - read and check a line from the font translation file
 *
 * returns TRUE when a correct line has been read, otherwise FALSE
 */
static BOOL
bReadFontFile(FILE *pFontTableFile, char *szWordFont,
	int *piItalic, int *piBold, char *szOurFont, int *piSpecial)
{
	char	*pcTmp;
	int	iFields;
	char	szLine[81];

	fail(szWordFont == NULL || szOurFont == NULL);
	fail(piItalic == NULL || piBold == NULL || piSpecial == NULL);

	while (fgets(szLine, (int)sizeof(szLine), pFontTableFile) != NULL) {
		if (szLine[0] == '#' ||
		    szLine[0] == '\n' ||
		    szLine[0] == '\r') {
			continue;
		}
		iFields = sscanf(szLine, "%[^,],%d,%d,%1s%[^,],%d",
		    	szWordFont, piItalic, piBold,
		    	&szOurFont[0], &szOurFont[1], piSpecial);
		if (iFields != 6) {
			pcTmp = strchr(szLine, '\r');
			if (pcTmp != NULL) {
				*pcTmp = '\0';
			}
			pcTmp = strchr(szLine, '\n');
			if (pcTmp != NULL) {
				*pcTmp = '\0';
			}
			DBG_DEC(iFields);
			werr(0, "Syntax error in: '%s'", szLine);
			continue;
		}
		if (strlen(szWordFont) >=
		    		sizeof(pFontTable[0].szWordFontname) ||
		    strlen(szOurFont) >=
		    		sizeof(pFontTable[0].szOurFontname)) {
			continue;
		}
		/* The current line passed all the tests */
		return TRUE;
	}
	return FALSE;
} /* end of bReadFontFile */

/*
 * vCreate6FontTable - create a font table from Word 6/7
 */
void
vCreate6FontTable(FILE *pFile, int iStartBlock,
	const int *aiBBD, size_t tBBDLen,
	const unsigned char *aucHeader)
{
	FILE	*pFontTableFile;
	font_table_type	*pTmp;
	char	*szFont, *szAltFont;
	unsigned char	*aucBuffer;
	size_t	tBeginFontInfo, tFontInfoLen;
	int	iPos, iRecLen, iOffsetAltName;
	int	iBold, iItalic, iSpecial;
	char	szWordFont[81], szOurFont[81];

	fail(pFile == NULL || aucHeader == NULL);
	fail(iStartBlock < 0);
	fail(aiBBD == NULL);

	pFontTableFile = pOpenFontTableFile();
	if (pFontTableFile == NULL) {
		/* No translation table file, no translation table */
		return;
	}

	tBeginFontInfo = (size_t)ulGetLong(0xd0, aucHeader);
	DBG_HEX(tBeginFontInfo);
	tFontInfoLen = (size_t)ulGetLong(0xd4, aucHeader);
	DBG_DEC(tFontInfoLen);
	fail(tFontInfoLen < 9);

	aucBuffer = xmalloc(tFontInfoLen);
	if (!bReadBuffer(pFile, iStartBlock,
			aiBBD, tBBDLen, BIG_BLOCK_SIZE,
			aucBuffer, tBeginFontInfo, tFontInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		(void)fclose(pFontTableFile);
		return;
	}
	DBG_DEC(usGetWord(0, aucBuffer));

	/* Compute the maximum number of entries in the font table */
	tFontTableRecords = 0;
	iPos = 2;
	while (iPos + 6 < (int)tFontInfoLen) {
		iRecLen = (int)ucGetByte(iPos, aucBuffer);
		NO_DBG_DEC(iRecLen);
		iOffsetAltName = (int)ucGetByte(iPos + 5, aucBuffer);
		NO_DBG_MSG(aucBuffer + iPos + 6);
		NO_DBG_MSG_C(iOffsetAltName > 0,
				aucBuffer + iPos + 6 + iOffsetAltName);
		iPos += iRecLen + 1;
		tFontTableRecords++;
	}
	tFontTableRecords *= 4;	/* Regular, Bold, Italic and Bold/italic */
	tFontTableRecords++;	/* One extra for the table-font */
	vCreateFontTable();

	/* Read the font translation file */
	while (bReadFontFile(pFontTableFile, szWordFont,
			&iItalic, &iBold, szOurFont, &iSpecial)) {
		pTmp = pFontTable;
		if (iBold != 0) {
			pTmp++;
		}
		if (iItalic != 0) {
			pTmp += 2;
		}
		iPos = 2;
		while (iPos + 6 < (int)tFontInfoLen) {
			iRecLen = (int)ucGetByte(iPos, aucBuffer);
			szFont = (char *)aucBuffer + iPos + 6;
			iOffsetAltName = (int)ucGetByte(iPos + 5, aucBuffer);
			if (iOffsetAltName <= 0) {
				szAltFont = NULL;
			} else {
				szAltFont = szFont + iOffsetAltName;
				NO_DBG_MSG(szFont);
				NO_DBG_MSG(szAltFont);
			}
			if (bFontEqual(szFont, szWordFont, 1) ||
			    (szAltFont != NULL &&
			     bFontEqual(szAltFont, szWordFont, 1)) ||
			    (pTmp->szWordFontname[0] == '\0' &&
			     szWordFont[0] == '*' &&
			     szWordFont[1] == '\0')) {
			  	strncpy(pTmp->szWordFontname, szFont,
			  		sizeof(pTmp->szWordFontname) - 1);
			  	pTmp->szWordFontname[sizeof(
			  		pTmp->szWordFontname) - 1] = '\0';
				strcpy(pTmp->szOurFontname, szOurFont);
			}
			pTmp += 4;
			iPos += iRecLen + 1;
		}
	}
	vMinimizeFontTable();
	aucBuffer = xfree(aucBuffer);
	(void)fclose(pFontTableFile);
} /* end of vCreate6FontTable */

/*
 * vCreate8FontTable - create a font table from Word 8/97
 */
void
vCreate8FontTable(FILE *pFile, const pps_info_type *pPPS,
	const int *aiBBD, size_t tBBDLen, const int *aiSBD, size_t tSBDLen,
	const unsigned char *aucHeader)
{
	FILE	*pFontTableFile;
	font_table_type	*pTmp;
	const int	*aiBlockDepot;
	char		*szFont, *szAltFont;
	unsigned char	*aucBuffer;
	long	lTableSize;
	size_t	tBeginFontInfo, tFontInfoLen, tBlockDepotLen, tBlockSize;
	int	iTableStartBlock;
	int	iPos, iRecLen, iOffsetAltName;
	int	iBold, iItalic, iSpecial;
	unsigned short	usDocStatus;
	char	szWordFont[81], szOurFont[81];

	fail(pFile == NULL || pPPS == NULL || aucHeader == NULL);
	fail(aiBBD == NULL || aiSBD == NULL);

	pFontTableFile = pOpenFontTableFile();
	if (pFontTableFile == NULL) {
		/* No translation table file, no translation table */
		return;
	}

	tBeginFontInfo = (size_t)ulGetLong(0x112, aucHeader);
	DBG_HEX(tBeginFontInfo);
	tFontInfoLen = (size_t)ulGetLong(0x116, aucHeader);
	DBG_DEC(tFontInfoLen);
	fail(tFontInfoLen < 46);

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
		DBG_MSG("No fontname table");
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
	aucBuffer = xmalloc(tFontInfoLen);
	if (!bReadBuffer(pFile, iTableStartBlock,
			aiBlockDepot, tBlockDepotLen, tBlockSize,
			aucBuffer, tBeginFontInfo, tFontInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		(void)fclose(pFontTableFile);
		return;
	}
	NO_DBG_PRINT_BLOCK(aucBuffer, tFontInfoLen);

	/* Get the maximum number of entries in the font table */
	tFontTableRecords = (size_t)usGetWord(0, aucBuffer);
	tFontTableRecords *= 4;	/* Regular, Bold, Italic and Bold/italic */
	tFontTableRecords++;	/* One extra for the table-font */
	vCreateFontTable();

	/* Read the font translation file */
	while (bReadFontFile(pFontTableFile, szWordFont,
			&iItalic, &iBold, szOurFont, &iSpecial)) {
		pTmp = pFontTable;
		if (iBold != 0) {
			pTmp++;
		}
		if (iItalic != 0) {
			pTmp += 2;
		}
		iPos = 4;
		while (iPos + 40 < (int)tFontInfoLen) {
			iRecLen = (int)ucGetByte(iPos, aucBuffer);
			szFont = (char *)aucBuffer + iPos + 40;
			iOffsetAltName = (int)unilen(szFont);
			if (iPos + 40 + iOffsetAltName + 4 >= iRecLen) {
				szAltFont = NULL;
			} else {
				szAltFont = szFont + iOffsetAltName + 2;
				NO_DBG_UNICODE(szFont);
				NO_DBG_UNICODE(szAltFont);
			}
			if (bFontEqual(szFont, szWordFont, 2) ||
			    (szAltFont != NULL &&
			     bFontEqual(szAltFont, szWordFont, 2)) ||
			    (pTmp->szWordFontname[0] == '\0' &&
			     szWordFont[0] == '*' &&
			     szWordFont[1] == '\0')) {
				(void)unincpy(pTmp->szWordFontname, szFont,
					sizeof(pTmp->szWordFontname) - 1);
				pTmp->szWordFontname[sizeof(
					pTmp->szWordFontname) - 1] = '\0';
				(void)strcpy(pTmp->szOurFontname, szOurFont);
			}
			pTmp += 4;
			iPos += iRecLen + 1;
		}
	}
	vMinimizeFontTable();
	aucBuffer = xfree(aucBuffer);
	(void)fclose(pFontTableFile);
} /* end of vCreate8FontTable */

void
vDestroyFontTable(void)
{
	DBG_MSG("vDestroyFontTable");

	pFontTable = xfree(pFontTable);
	tFontTableRecords = 0;
} /* end of vDestroyFontTable */

/*
 * pGetNextFontTableRecord
 *
 * returns the next record in the table or NULL if there is no next record
 */
const font_table_type *
pGetNextFontTableRecord(const font_table_type *pRecordCurr)
{
	int	iIndexCurr;

	if (pRecordCurr == NULL) {
		/* No current record, so start with the first */
		return &pFontTable[0];
	}

	iIndexCurr = pRecordCurr - pFontTable;
	if (iIndexCurr + 1 < (int)tFontTableRecords) {
		/* There is a next record */
		return &pFontTable[iIndexCurr + 1];
	}
	return NULL;
} /* end of pGetNextFontTableRecord */

/*
 * tGetFontTableLength
 *
 * returns the number of records in the font table
 */
size_t
tGetFontTableLength(void)
{
	return tFontTableRecords;
} /* end of tGetFontTableLength */
