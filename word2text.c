/*
 * word2text.c
 * Copyright (C) 1998-2000 A.J. van Os; Released under GPL
 *
 * Description:
 * MS Word to text functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#if defined(__riscos)
#include "visdelay.h"
#endif /* __riscos */
#include "antiword.h"


#define INITIAL_SIZE		40
#define EXTENTION_SIZE		20


/* Macros to make sure all such statements will be identical */
#define OUTPUT_LINE()		\
	do {\
		vAlign2Window(pDiag, pAnchor, lWidthMax, ucAlignment);\
		pAnchor = pStartNewOutput(pAnchor, NULL);\
		pOutput = pAnchor;\
	} while(0)

#define RESET_LINE()		\
	do {\
		pAnchor = pStartNewOutput(pAnchor, NULL);\
		pOutput = pAnchor;\
	} while(0)

#if defined(__riscos)
/* Length of the document in characters */
static size_t	tDocumentLength;
/* Number of characters processed so far */
static size_t	tCharCounter;
static int	iCurrPct, iPrevPct;
#endif /* __riscos */
/* Special treatment for files from the Apple Macintosh */
static BOOL	bMacFile = FALSE;
/* Needed for reading a complete table row */
static row_block_type	tRowInfo;
static BOOL	bRowInfo = FALSE;
static BOOL	bStartRow = FALSE;
static BOOL	bEndRow = FALSE;
static BOOL	bIsTableRow = FALSE;
/* Needed for finding the start of a style */
static const style_block_type	*pStyleInfo = NULL;
static BOOL	bStartStyle = FALSE;
/* Needed for finding the start of a font */
static const font_block_type	*pFontInfo = NULL;
static BOOL	bStartFont = FALSE;
/* Needed for finding an image */
static long	lFileOffsetImage = -1;


/*
 * vUpdateCounters - Update the counters for the hourglass
 */
static void
vUpdateCounters(void)
{
#if defined(__riscos)
	tCharCounter++;
	iCurrPct = (int)((tCharCounter * 100) / tDocumentLength);
	if (iCurrPct != iPrevPct) {
		visdelay_percent(iCurrPct);
		iPrevPct = iCurrPct;
	}
#endif /* __riscos */
} /* end of vUpdateCounters */

/*
 * bOutputContainsText - see if the output contains more than white-space
 */
static BOOL
bOutputContainsText(output_type *pAnchor)
{
	output_type	*pTmp;
	int	iIndex;

	fail(pAnchor == NULL);

	for (pTmp = pAnchor; pTmp != NULL; pTmp = pTmp->pNext) {
		fail(pTmp->lStringWidth < 0);
		for (iIndex = 0; iIndex < pTmp->iNextFree; iIndex++) {
			if (isspace(pTmp->szStorage[iIndex])) {
				continue;
			}
#if defined(DEBUG)
			if ((unsigned char)pTmp->szStorage[iIndex] ==
			    FILLER_CHAR) {
				continue;
			}
#endif /* DEBUG */
			return TRUE;
		}
	}
	return FALSE;
} /* end of bOutputContainsText */

/*
 * lTotalStringWidth - compute the total width of the output string
 */
static long
lTotalStringWidth(output_type *pAnchor)
{
	output_type	*pTmp;
	long		lTotal;

	lTotal = 0;
	for (pTmp = pAnchor; pTmp != NULL; pTmp = pTmp->pNext) {
		fail(pTmp->lStringWidth < 0);
		lTotal += pTmp->lStringWidth;
	}
	return lTotal;
} /* end of lTotalStringWidth */

/*
 * vStoreCharacter - store one character
 */
static void
vStoreCharacter(int iChar, output_type *pOutput)
{
	fail(pOutput == NULL);

	if (iChar == 0) {
		pOutput->szStorage[pOutput->iNextFree] = '\0';
		return;
	}
	while (pOutput->iNextFree + 2 > (int)pOutput->tStorageSize) {
		pOutput->tStorageSize += EXTENTION_SIZE;
		pOutput->szStorage = xrealloc(pOutput->szStorage,
					pOutput->tStorageSize);
	}
	pOutput->szStorage[pOutput->iNextFree] = (char)iChar;
	pOutput->szStorage[pOutput->iNextFree+1] = '\0';
	pOutput->lStringWidth += lComputeStringWidth(
				pOutput->szStorage + pOutput->iNextFree,
				1,
				pOutput->tFontRef,
				pOutput->sFontsize);
	pOutput->iNextFree++;
} /* end of vStoreCharacter */

/*
 * vStoreString - store a string
 */
static void
vStoreString(const char *szString, int iStringLength, output_type *pOutput)
{
	int	iIndex;

	fail(szString == NULL || iStringLength < 0 || pOutput == NULL);

	for (iIndex = 0; iIndex < iStringLength; iIndex++) {
		vStoreCharacter(szString[iIndex], pOutput);
	}
} /* end of vStoreString */

/*
 * vStoreIntegerAsDecimal - store an integer as a decimal number
 */
static void
vStoreIntegerAsDecimal(int iNumber, output_type *pOutput)
{
	int	iLen;
	char	szString[15];

	fail(pOutput == NULL);

	iLen = sprintf(szString, "%d", iNumber);
	vStoreString(szString, iLen, pOutput);
} /* end of vStoreIntegerAsDecimal */

/*
 * vStoreIntegerAsRoman - store an integer as a roman numerical
 */
static void
vStoreIntegerAsRoman(int iNumber, output_type *pOutput)
{
	int	iLen;
	char	szString[15];

	fail(iNumber <= 0);
	fail(pOutput == NULL);

	iLen = iInteger2Roman(iNumber, FALSE, szString);
	vStoreString(szString, iLen, pOutput);
} /* end of vStoreIntegerAsRoman */

/*
 * vStoreStyle - store a style
 */
static void
vStoreStyle(output_type *pOutput)
{
	int	iLen;
	char	szString[120];

	fail(pOutput == NULL);
	iLen = iStyle2Window(szString, pStyleInfo);
	vStoreString(szString, iLen, pOutput);
} /* end of vStoreStyle */

/*
 * Create an empty line by adding a extra "newline"
 */
static void
vEmptyLine2Diagram(diagram_type *pDiag, draw_fontref tFontRef, int iFontsize)
{
	fail(pDiag == NULL);
	fail(iFontsize < MIN_FONT_SIZE || iFontsize > MAX_FONT_SIZE);

	if (pDiag->lXleft > 0) {
		/* To the start of the line */
		vMove2NextLine(pDiag, tFontRef, iFontsize);
	}
	/* Empty line */
	vMove2NextLine(pDiag, tFontRef, iFontsize);
} /* end of vEmptyLine2Diagram */

/*
 * vPutIndentation - output the given amount of indentation
 */
static void
vPutIndentation(diagram_type *pDiag, output_type *pOutput, BOOL bUnmarked,
	int iListNumber, unsigned char ucListType, char cListCharacter,
	int iLeftIndent)
{
	long	lWidth, lLeftIndentation;
	int	iNextFree;
	char	szLine[30];

	fail(pDiag == NULL || pOutput == NULL);
	fail(iListNumber < 0);
	fail(iLeftIndent < 0);

	if (iLeftIndent <= 0) {
		return;
	}
	lLeftIndentation = lTwips2MilliPoints(iLeftIndent);
	if (bUnmarked) {
		vSetLeftIndentation(pDiag, lLeftIndentation);
		return;
	}
	fail(iListNumber <= 0 || iscntrl(cListCharacter));

	switch (ucListType) {
	case LIST_BULLETS:
		iNextFree = 0;
		break;
	case LIST_ROMAN_NUM_UPPER:
	case LIST_ROMAN_NUM_LOWER:
		iNextFree = iInteger2Roman(iListNumber,
			ucListType == LIST_ROMAN_NUM_UPPER, szLine);
		break;
	case LIST_UPPER_ALPHA:
	case LIST_LOWER_ALPHA:
		iNextFree = iInteger2Alpha(iListNumber,
			ucListType == LIST_UPPER_ALPHA, szLine);
		break;
	case LIST_ARABIC_NUM:
	default:
		iNextFree = sprintf(szLine, "%d", iListNumber);
	}
	szLine[iNextFree++] = cListCharacter;
	szLine[iNextFree++] = ' ';
	szLine[iNextFree] = '\0';
	lWidth = lComputeStringWidth(szLine, iNextFree,
				pOutput->tFontRef, pOutput->sFontsize);
	lLeftIndentation -= lWidth;
	if (lLeftIndentation > 0) {
		vSetLeftIndentation(pDiag, lLeftIndentation);
	}
	vStoreString(szLine, iNextFree, pOutput);
} /* end of vPutIndentation */

/*
 * vPutNoteSeparator - output a note separator
 *
 * A note separator is a horizontal line two inches long.
 * Two inches equals 144000 milliponts.
 */
static void
vPutNoteSeparator(output_type *pOutput)
{
	long	lCharWidth;
	int	iCounter, iChars;
	char	szOne[2];

	fail(pOutput == NULL);

	szOne[0] = OUR_EM_DASH;
	szOne[1] = '\0';
	lCharWidth = lComputeStringWidth(szOne, 1,
				pOutput->tFontRef, pOutput->sFontsize);
	DBG_DEC(lCharWidth);
	iChars = (int)((144000 + lCharWidth / 2) / lCharWidth);
	DBG_DEC(iChars);
	for (iCounter = 0; iCounter < iChars; iCounter++) {
		vStoreCharacter(OUR_EM_DASH, pOutput);
	}
} /* end of vPutNoteSeparator */

/*
 *
 */
static output_type *
pStartNextOutput(output_type *pCurrent)
{
	output_type	*pNew;

	if (pCurrent->iNextFree == 0) {
		/* The current record is empty, re-use */
		fail(pCurrent->szStorage[0] != '\0');
		fail(pCurrent->lStringWidth != 0);
		return pCurrent;
	}
	/* The current record is in use, make a new one */
	pNew = xmalloc(sizeof(*pNew));
	pCurrent->pNext = pNew;
	pNew->tStorageSize = INITIAL_SIZE;
	pNew->szStorage = xmalloc(pNew->tStorageSize);
	pNew->szStorage[0] = '\0';
	pNew->iNextFree = 0;
	pNew->lStringWidth = 0;
	pNew->iColor = FONT_COLOR_DEFAULT;
	pNew->ucFontstyle = FONT_REGULAR;
	pNew->tFontRef = 0;
	pNew->sFontsize = DEFAULT_FONT_SIZE;
	pNew->pPrev = pCurrent;
	pNew->pNext = NULL;
	return pNew;
} /* end of pStartNextOutput */

/*
 * pStartNewOutput
 */
static output_type *
pStartNewOutput(output_type *pAnchor, output_type *pLeftOver)
{
	output_type	*pCurr, *pNext;
	int		iColor;
	short		sFontsize;
	draw_fontref	tFontRef;
	unsigned char	ucFontstyle;

	iColor = FONT_COLOR_DEFAULT;
	ucFontstyle = FONT_REGULAR;
	tFontRef = 0;
	sFontsize = DEFAULT_FONT_SIZE;
	/* Free the old output space */
	pCurr = pAnchor;
	while (pCurr != NULL) {
		pNext = pCurr->pNext;
		pCurr->szStorage = xfree(pCurr->szStorage);
		if (pCurr->pNext == NULL) {
			iColor = pCurr->iColor;
			ucFontstyle = pCurr->ucFontstyle;
			tFontRef = pCurr->tFontRef;
			sFontsize = pCurr->sFontsize;
		}
		pCurr = xfree(pCurr);
		pCurr = pNext;
	}
	if (pLeftOver == NULL) {
		/* Create new output space */
		pLeftOver = xmalloc(sizeof(*pLeftOver));
		pLeftOver->tStorageSize = INITIAL_SIZE;
		pLeftOver->szStorage = xmalloc(pLeftOver->tStorageSize);
		pLeftOver->szStorage[0] = '\0';
		pLeftOver->iNextFree = 0;
		pLeftOver->lStringWidth = 0;
		pLeftOver->iColor = iColor;
		pLeftOver->ucFontstyle = ucFontstyle;
		pLeftOver->tFontRef = tFontRef;
		pLeftOver->sFontsize = sFontsize;
		pLeftOver->pPrev = NULL;
		pLeftOver->pNext = NULL;
	}
	fail(!bCheckDoubleLinkedList(pLeftOver));
	return pLeftOver;
} /* end of pStartNewOutput */

/*
 * iGetChar - get the next character from the given list
 */
static int
iGetChar(FILE *pFile, list_id_enum eListID)
{
	const font_block_type *pCurr;
	long	lFileOffset;
	int	iChar;
	unsigned short usChar;
	BOOL	bSkip;

	fail(pFile == NULL);

	pCurr = pFontInfo;
	bSkip = FALSE;
	for (;;) {
		usChar = usNextChar(pFile, eListID, &lFileOffset);
		if (usChar == (unsigned short)EOF) {
			return EOF;
		}

		vUpdateCounters();

		if (!bStartRow) {
			bStartRow = bRowInfo &&
				lFileOffset == tRowInfo.lOffsetStart;
			NO_DBG_HEX_C(bStartRow, tRowInfo.lOffsetStart);
		}
		if (!bEndRow) {
			bEndRow = bRowInfo &&
				lFileOffset == tRowInfo.lOffsetEnd;
			NO_DBG_HEX_C(bStartRow, tRowInfo.lOffsetEnd);
		}
		if (!bStartStyle) {
			bStartStyle = pStyleInfo != NULL &&
				lFileOffset == pStyleInfo->lFileOffset;
			NO_DBG_HEX_C(bStartStyle, lFileOffset);
		}
		if (pCurr != NULL && lFileOffset == pCurr->lFileOffset) {
			bStartFont = TRUE;
			NO_DBG_HEX(lFileOffset);
			pFontInfo = pCurr;
			pCurr = pGetNextFontInfoListItem(pCurr);
		}

		/* Skip embedded characters */
		if (usChar == START_EMBEDDED) {
			bSkip = TRUE;
			continue;
		}
		if (usChar == END_IGNORE || usChar == END_EMBEDDED) {
			bSkip = FALSE;
			continue;
		}
		if (bSkip) {
			continue;
		}
		iChar = iTranslateCharacters(usChar, lFileOffset, bMacFile);
		if (iChar == IGNORE_CHARACTER) {
			continue;
		}
		if (iChar == PICTURE) {
			lFileOffsetImage = lGetPicInfoListItem(lFileOffset);
		} else {
			lFileOffsetImage = -1;
		}
		return iChar;
	}
} /* end of iGetChar */

/*
 * vWord2Text
 */
void
vWord2Text(diagram_type *pDiag, const char *szFilename)
{
	options_type	tOptions;
	imagedata_type	tImage;
	FILE	*pFile;
	output_type	*pAnchor, *pOutput, *pLeftOver;
	long	lWidthCurr, lWidthMax, lRightIndentation;
	long	lDefaultTabWidth, lTmp;
	list_id_enum 	eListID;
	image_info_enum	eRes;
	int	iFootnoteNumber, iEndnoteNumber, iLeftIndent;
	int	iChar, iListNumber;
	BOOL	bWasTableRow, bTableFontClosed, bUnmarked;
	BOOL	bAllCapitals, bHiddenText, bSuccess;
	short	sFontsize;
	unsigned char	ucFontnumber, ucFontcolor, ucTmp;
	unsigned char	ucFontstyle, ucFontstyleMinimal;
	unsigned char	ucListType, ucAlignment;
	char	cListChar;

	fail(pDiag == NULL || szFilename == NULL || szFilename[0] == '\0');

	DBG_MSG("vWord2Text");

	pFile = pOpenDocument(szFilename);
	if (pFile == NULL) {
		return;
	}
	vAddFonts2Diagram(pDiag);

	/* Initialisation */
#if defined(__riscos)
	tCharCounter = 0;
	iCurrPct = 0;
	iPrevPct = -1;
	tDocumentLength = tGetDocumentLength();
#endif /* __riscos */
	bMacFile = bFileFromTheMac();
	lDefaultTabWidth = lGetDefaultTabWidth();
	DBG_DEC_C(lDefaultTabWidth != 36000, lDefaultTabWidth);
	bRowInfo = bGetNextRowInfoListItem(&tRowInfo);
	DBG_HEX_C(bRowInfo, tRowInfo.lOffsetStart);
	DBG_HEX_C(bRowInfo, tRowInfo.lOffsetEnd);
	bStartRow = FALSE;
	bEndRow = FALSE;
	bIsTableRow = FALSE;
	bWasTableRow = FALSE;
	vResetStyles();
	pStyleInfo = pGetNextStyleInfoListItem();
	bStartStyle = FALSE;
	pAnchor = NULL;
	pFontInfo = pGetNextFontInfoListItem(NULL);
	DBG_HEX_C(pFontInfo != NULL, pFontInfo->lFileOffset);
	DBG_MSG_C(pFontInfo == NULL, "No fonts at all");
	bStartFont = FALSE;
	ucFontnumber = 0;
	ucFontstyleMinimal = FONT_REGULAR;
	ucFontstyle = FONT_REGULAR;
	sFontsize = DEFAULT_FONT_SIZE;
	ucFontcolor = FONT_COLOR_DEFAULT;
	pAnchor = pStartNewOutput(pAnchor, NULL);
	pOutput = pAnchor;
	pOutput->iColor = ucFontcolor;
	pOutput->ucFontstyle = ucFontstyle;
	pOutput->tFontRef = tOpenFont(ucFontnumber, ucFontstyle, sFontsize);
	pOutput->sFontsize = sFontsize;
	bTableFontClosed = TRUE;
	iLeftIndent = 0;
	lRightIndentation = 0;
	bUnmarked = TRUE;
	ucListType = LIST_BULLETS;
	cListChar = OUR_BULLET;
	iListNumber = 0;
	ucAlignment = ALIGNMENT_LEFT;
	bAllCapitals = FALSE;
	bHiddenText = FALSE;
	vGetOptions(&tOptions);
	fail(tOptions.iParagraphBreak < 0);
	if (tOptions.iParagraphBreak == 0) {
		lWidthMax = LONG_MAX;
	} else if (tOptions.iParagraphBreak < MIN_SCREEN_WIDTH) {
		lWidthMax = lChar2MilliPoints(MIN_SCREEN_WIDTH);
	} else if (tOptions.iParagraphBreak > MAX_SCREEN_WIDTH) {
		lWidthMax = lChar2MilliPoints(MAX_SCREEN_WIDTH);
	} else {
		lWidthMax = lChar2MilliPoints(tOptions.iParagraphBreak);
	}
	NO_DBG_DEC(lWidthMax);

	visdelay_begin();

	iFootnoteNumber = 0;
	iEndnoteNumber = 0;
	eListID = text_list;
	for(;;) {
		iChar = iGetChar(pFile, eListID);
		if (iChar == EOF) {
			if (bOutputContainsText(pAnchor)) {
				OUTPUT_LINE();
			} else {
				RESET_LINE();
			}
			switch (eListID) {
			case text_list:
				eListID = footnote_list;
				if (iFootnoteNumber > 0) {
					vPutNoteSeparator(pAnchor);
					OUTPUT_LINE();
					iFootnoteNumber = 0;
				}
				break;
			case footnote_list:
				eListID = endnote_list;
				if (iEndnoteNumber > 0) {
					vPutNoteSeparator(pAnchor);
					OUTPUT_LINE();
					iEndnoteNumber = 0;
				}
				break;
			case endnote_list:
			default:
				eListID = end_of_lists;
				break;
			}
			if (eListID == end_of_lists) {
				break;
			}
			continue;
		}

		if (iChar == UNKNOWN_NOTE_CHAR) {
			switch (eListID) {
			case footnote_list:
				iChar = FOOTNOTE_CHAR;
				break;
			case endnote_list:
				iChar = ENDNOTE_CHAR;
				break;
			default:
				break;
			}
		}

		if (bStartRow) {
			/* Begin of a tablerow found */
			if (bOutputContainsText(pAnchor)) {
				OUTPUT_LINE();
			} else {
				RESET_LINE();
			}
			fail(pAnchor != pOutput);
			if (bTableFontClosed) {
				/* Start special table font */
				vCloseFont();
				/*
				 * Compensate for the fact that Word uses
				 * proportional fonts for its tables and we
				 * only one fixed-width font
				 */
				pOutput->sFontsize =
					(sFontsize <= DEFAULT_FONT_SIZE ?
					 (DEFAULT_FONT_SIZE * 5 + 3) / 6 :
					 (sFontsize * 5 + 3) / 6);
				pOutput->tFontRef =
					tOpenTableFont(pOutput->sFontsize);
				pOutput->ucFontstyle = FONT_REGULAR;
				pOutput->iColor = FONT_COLOR_BLACK;
				bTableFontClosed = FALSE;
			}
			bIsTableRow = TRUE;
			bStartRow = FALSE;
		}

		if (bWasTableRow &&
		    !bIsTableRow &&
		    iChar != PAR_END &&
		    iChar != HARD_RETURN &&
		    iChar != FORM_FEED &&
		    iChar != COLUMN_FEED) {
			/*
			 * The end of a table should be followed by an
			 * empty line, like the end of a paragraph
			 */
			OUTPUT_LINE();
			vEndOfParagraph2Diagram(pDiag,
						pOutput->tFontRef,
						pOutput->sFontsize);
		}

		switch (iChar) {
		case FORM_FEED:
		case COLUMN_FEED:
			if (bIsTableRow) {
				vStoreCharacter('\n', pOutput);
				break;
			}
			if (bOutputContainsText(pAnchor)) {
				OUTPUT_LINE();
			} else {
				RESET_LINE();
			}
			if (iChar == FORM_FEED) {
				vEndOfPage2Diagram(pDiag,
						pOutput->tFontRef,
						pOutput->sFontsize);
			} else {
				vEndOfParagraph2Diagram(pDiag,
						pOutput->tFontRef,
						pOutput->sFontsize);
			}
			break;
		default:
			break;
		}

		if (bStartFont) {
			/* Begin of a font found */
			fail(pFontInfo == NULL);
			bAllCapitals = bIsCapitals(pFontInfo->ucFontstyle);
			bHiddenText = bIsHidden(pFontInfo->ucFontstyle);
			ucTmp = pFontInfo->ucFontstyle &
			(FONT_BOLD|FONT_ITALIC|FONT_UNDERLINE|FONT_STRIKE);
			if (!bIsTableRow &&
			    (sFontsize != pFontInfo->sFontsize ||
			    ucFontnumber != pFontInfo->ucFontnumber ||
			    ucFontstyleMinimal != ucTmp ||
			    ucFontcolor != pFontInfo->ucFontcolor)) {
				pOutput = pStartNextOutput(pOutput);
				vCloseFont();
				pOutput->iColor = pFontInfo->ucFontcolor;
				pOutput->ucFontstyle = pFontInfo->ucFontstyle;
				pOutput->sFontsize = pFontInfo->sFontsize;
				pOutput->tFontRef = tOpenFont(
						pFontInfo->ucFontnumber,
						pFontInfo->ucFontstyle,
						pFontInfo->sFontsize);
				fail(!bCheckDoubleLinkedList(pAnchor));
			}
			ucFontnumber = pFontInfo->ucFontnumber;
			sFontsize = pFontInfo->sFontsize;
			ucFontcolor = pFontInfo->ucFontcolor;
			ucFontstyle = pFontInfo->ucFontstyle;
			ucFontstyleMinimal = ucTmp;
			pFontInfo = pGetNextFontInfoListItem(pFontInfo);
			NO_DBG_HEX_C(pFontInfo != NULL, pFontInfo->lFileOffset);
			DBG_MSG_C(pFontInfo == NULL, "No more fonts");
			bStartFont = FALSE;
		}

		if (bStartStyle) {
			/* Begin of a style found */
			fail(pStyleInfo == NULL);
			if (!bIsTableRow) {
				vStoreStyle(pOutput);
			}
			iLeftIndent = pStyleInfo->sLeftIndent;
			lRightIndentation =
				lTwips2MilliPoints(pStyleInfo->sRightIndent);
			bUnmarked = !pStyleInfo->bInList ||
						pStyleInfo->bUnmarked;
			ucListType = pStyleInfo->ucListType;
			cListChar = (char)pStyleInfo->ucListCharacter;
			ucAlignment = pStyleInfo->ucAlignment;
			if (pStyleInfo->bInList) {
				if (!pStyleInfo->bUnmarked) {
					iListNumber++;
				}
			} else {
				iListNumber = 0;
			}
			pStyleInfo = pGetNextStyleInfoListItem();
			NO_DBG_HEX_C(pStyleInfo != NULL,
						pStyleInfo->lFileOffset);
			bStartStyle = FALSE;
		}

		if (!bIsTableRow &&
		    lTotalStringWidth(pAnchor) == 0) {
			vPutIndentation(pDiag, pAnchor, bUnmarked,
					iListNumber, ucListType, cListChar,
					iLeftIndent);
			/* One mark per paragraph will do */
			bUnmarked = TRUE;
		}

		switch (iChar) {
		case PICTURE:
			(void)memset(&tImage, 0, sizeof(tImage));
			eRes = eExamineImage(pFile, lFileOffsetImage, &tImage);
			switch (eRes) {
			case image_no_information:
				bSuccess = FALSE;
				break;
			case image_minimal_information:
			case image_full_information:
#if 0
				if (bOutputContainsText(pAnchor)) {
					OUTPUT_LINE();
				} else {
					RESET_LINE();
				}
#endif
				bSuccess = bTranslateImage(pDiag, pFile,
					eRes == image_minimal_information,
					lFileOffsetImage, &tImage);
				break;
			default:
				DBG_DEC(eRes);
				bSuccess = FALSE;
				break;
			}
			if (!bSuccess) {
				vStoreString("[pic]", 5, pOutput);
			}
			break;
		case FOOTNOTE_CHAR:
			iFootnoteNumber++;
			vStoreCharacter('[', pOutput);
			vStoreIntegerAsDecimal(iFootnoteNumber, pOutput);
			vStoreCharacter(']', pOutput);
			break;
		case ENDNOTE_CHAR:
			iEndnoteNumber++;
			vStoreCharacter('[', pOutput);
			vStoreIntegerAsRoman(iEndnoteNumber, pOutput);
			vStoreCharacter(']', pOutput);
			break;
		case UNKNOWN_NOTE_CHAR:
			vStoreString("[?]", 3, pOutput);
			break;
		case PAR_END:
			if (bIsTableRow) {
				vStoreCharacter('\n', pOutput);
				break;
			}
			if (bOutputContainsText(pAnchor)) {
				OUTPUT_LINE();
			} else {
				RESET_LINE();
			}
			vEndOfParagraph2Diagram(pDiag,
						pOutput->tFontRef,
						pOutput->sFontsize);
			/*
			 * End of paragraph seen, reset indentation,
			 * marking and alignment
			 */
			iLeftIndent = 0;
			lRightIndentation = 0;
			bUnmarked = TRUE;
			ucAlignment = ALIGNMENT_LEFT;
			break;
		case HARD_RETURN:
			if (bIsTableRow) {
				vStoreCharacter('\n', pOutput);
				break;
			}
			if (bOutputContainsText(pAnchor)) {
				OUTPUT_LINE();
			}
			vEmptyLine2Diagram(pDiag,
					pOutput->tFontRef,
					pOutput->sFontsize);
			break;
		case FORM_FEED:
		case COLUMN_FEED:
			/* Already dealt with */
			break;
		case TABLE_SEPARATOR:
			if (bIsTableRow) {
				vStoreCharacter(iChar, pOutput);
				break;
			}
			vStoreCharacter(' ', pOutput);
			vStoreCharacter(TABLE_SEPARATOR_CHAR, pOutput);
			break;
		case TAB:
			if (bIsTableRow) {
				vStoreCharacter(' ', pOutput);
				break;
			}
			if (tOptions.iParagraphBreak == 0 &&
			    !tOptions.bUseOutlineFonts) {
				/* No logical lines, so no tab expansion */
				vStoreCharacter(TAB, pOutput);
				break;
			}
			lTmp = lTotalStringWidth(pAnchor);
			lTmp += lDrawUnits2MilliPoints(pDiag->lXleft);
			lTmp /= lDefaultTabWidth;
			do {
				vStoreCharacter(FILLER_CHAR, pOutput);
				lWidthCurr = lTotalStringWidth(pAnchor);
				lWidthCurr +=
					lDrawUnits2MilliPoints(pDiag->lXleft);
			} while (lTmp == lWidthCurr / lDefaultTabWidth &&
				 lWidthCurr < lWidthMax + lRightIndentation);
			break;
		default:
			if (bHiddenText && tOptions.bHideHiddenText) {
				continue;
			}
			if (bAllCapitals && iChar < UCHAR_MAX) {
				iChar = iToUpper(iChar);
			}
			vStoreCharacter(iChar, pOutput);
			break;
		}

		if (bWasTableRow && !bIsTableRow) {
			/* End of a table, resume normal font */
			NO_DBG_MSG("End of table font");
			vCloseFont();
			bTableFontClosed = TRUE;
			pOutput->iColor = ucFontcolor;
			pOutput->ucFontstyle = ucFontstyle;
			pOutput->sFontsize = sFontsize;
			pOutput->tFontRef =
				tOpenFont(ucFontnumber, ucFontstyle, sFontsize);
		}
		bWasTableRow = bIsTableRow;

		if (bIsTableRow) {
			fail(pAnchor != pOutput);
			if (!bEndRow) {
				continue;
			}
			/* End of a table row */
			fail(!bRowInfo);
			vTableRow2Window(pDiag, pAnchor, &tRowInfo);
			/* Reset */
			pAnchor = pStartNewOutput(pAnchor, NULL);
			pOutput = pAnchor;
			bRowInfo = bGetNextRowInfoListItem(&tRowInfo);
			bIsTableRow = FALSE;
			bEndRow = FALSE;
			NO_DBG_HEX_C(bRowInfo, tRowInfo.lOffsetStart);
			NO_DBG_HEX_C(bRowInfo, tRowInfo.lOffsetEnd);
			continue;
		}
		lWidthCurr = lTotalStringWidth(pAnchor);
		lWidthCurr += lDrawUnits2MilliPoints(pDiag->lXleft);
		if (lWidthCurr < lWidthMax + lRightIndentation) {
			continue;
		}
		pLeftOver = pSplitList(pAnchor);
		vJustify2Window(pDiag, pAnchor,
				lWidthMax, lRightIndentation, ucAlignment);
		pAnchor = pStartNewOutput(pAnchor, pLeftOver);
		for (pOutput = pAnchor;
		     pOutput->pNext != NULL;
		     pOutput = pOutput->pNext)
			;	/* EMPTY */
		fail(pOutput == NULL);
		if (lTotalStringWidth(pAnchor) > 0) {
			vSetLeftIndentation(pDiag,
					lTwips2MilliPoints(iLeftIndent));
		}
	}

	pAnchor = pStartNewOutput(pAnchor, NULL);
	pAnchor->szStorage = xfree(pAnchor->szStorage);
	pAnchor = xfree(pAnchor);
	vCloseFont();
	vCloseDocument(pFile);
	visdelay_end();
} /* end of vWord2Text */
