/*
 * out2window.c
 * Copyright (C) 1998-2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Output to a text window
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "antiword.h"


/* Used for numbering the chapters */
static int	aiHdrCounter[9];


/*
 * vString2Diagram - put a string into a diagram
 */
static void
vString2Diagram(diagram_type *pDiag, output_type *pAnchor)
{
	output_type	*pOutput;
	long		lWidth;
	int		iMaxFontsize;

	fail(pDiag == NULL);
	fail(pAnchor == NULL);

	/* Compute the maximum fontsize in this string */
	iMaxFontsize = MIN_FONT_SIZE;
	for (pOutput = pAnchor; pOutput != NULL; pOutput = pOutput->pNext) {
		if (pOutput->sFontsize > iMaxFontsize) {
			iMaxFontsize = pOutput->sFontsize;
		}
	}

	/* Goto the next line */
	vMove2NextLine(pDiag, pAnchor->tFontRef, iMaxFontsize);

	/* Output all substrings */
	for (pOutput = pAnchor; pOutput != NULL; pOutput = pOutput->pNext) {
		lWidth = lMilliPoints2DrawUnits(pOutput->lStringWidth);
		vSubstring2Diagram(pDiag, pOutput->szStorage,
			pOutput->iNextFree, lWidth, pOutput->iColor,
			pOutput->ucFontstyle, pOutput->tFontRef,
			pOutput->sFontsize, iMaxFontsize);
	}

	/* Goto the start of the line */
	pDiag->lXleft = 0;
} /* end of vString2Diagram */

/*
 * vSetLeftIndentation - set the left indentation of the given diagram
 */
void
vSetLeftIndentation(diagram_type *pDiag, long lLeftIndentation)
{
	long	lX;

	fail(pDiag == NULL);
	fail(lLeftIndentation < 0);

	lX = lMilliPoints2DrawUnits(lLeftIndentation);
	if (lX > 0) {
		pDiag->lXleft = lX;
	} else {
		pDiag->lXleft = 0;
	}
} /* end of vSetLeftIndentation */

/*
 * lComputeNetWidth - compute the net string width
 */
static long
lComputeNetWidth(output_type *pAnchor)
{
	output_type	*pTmp;
	long		lNetWidth;

	fail(pAnchor == NULL);

	/* Step 1: Count all but the last sub-string */
	lNetWidth = 0;
	for (pTmp = pAnchor; pTmp->pNext != NULL; pTmp = pTmp->pNext) {
		fail(pTmp->lStringWidth < 0);
		lNetWidth += pTmp->lStringWidth;
	}
	fail(pTmp == NULL);
	fail(pTmp->pNext != NULL);

	/* Step 2: remove the white-space from the end of the string */
	while (pTmp->iNextFree > 0 &&
	       isspace(pTmp->szStorage[pTmp->iNextFree - 1])) {
		pTmp->szStorage[pTmp->iNextFree - 1] = '\0';
		pTmp->iNextFree--;
		NO_DBG_DEC(pTmp->lStringWidth);
		pTmp->lStringWidth = lComputeStringWidth(
						pTmp->szStorage,
						pTmp->iNextFree,
						pTmp->tFontRef,
						pTmp->sFontsize);
		NO_DBG_DEC(pTmp->lStringWidth);
	}

	/* Step 3: Count the last sub-string */
	lNetWidth += pTmp->lStringWidth;
	return lNetWidth;
} /* end of lComputeNetWidth */

/*
 * iComputeHoles - compute number of holes
 * (A hole is a number of whitespace characters followed by a
 *  non-whitespace character)
 */
static int
iComputeHoles(output_type *pAnchor)
{
	output_type	*pTmp;
	int	iIndex, iCounter;
	BOOL	bWasSpace, bIsSpace;

	fail(pAnchor == NULL);

	iCounter = 0;
	bIsSpace = FALSE;
	/* Count the holes */
	for (pTmp = pAnchor; pTmp != NULL; pTmp = pTmp->pNext) {
		fail(pTmp->iNextFree != (int)strlen(pTmp->szStorage));
		for (iIndex = 0; iIndex <= pTmp->iNextFree; iIndex++) {
			bWasSpace = bIsSpace;
			bIsSpace = isspace(pTmp->szStorage[iIndex]);
			if (bWasSpace && !bIsSpace) {
				iCounter++;
			}
		}
	}
	return iCounter;
} /* end of iComputeHoles */

/*
 * Align a string and insert it into the text
 */
void
vAlign2Window(diagram_type *pDiag, output_type *pAnchor,
	long lScreenWidth, unsigned char ucAlignment)
{
	long	lNetWidth, lLeftIndentation;

	fail(pDiag == NULL || pAnchor == NULL);
	fail(lScreenWidth < lChar2MilliPoints(MIN_SCREEN_WIDTH));

	NO_DBG_MSG("vAlign2Window");

	lNetWidth = lComputeNetWidth(pAnchor);

	if (lScreenWidth > lChar2MilliPoints(MAX_SCREEN_WIDTH) ||
	    lNetWidth <= 0) {
		/*
		 * Screenwidth is "infinite", so no alignment is possible
		 * Don't bother to align an empty line
		 */
		vString2Diagram(pDiag, pAnchor);
		return;
	}

	switch (ucAlignment) {
	case ALIGNMENT_CENTER:
		lLeftIndentation = (lScreenWidth - lNetWidth) / 2;
		DBG_DEC_C(lLeftIndentation < 0, lLeftIndentation);
		if (lLeftIndentation > 0) {
			vSetLeftIndentation(pDiag, lLeftIndentation);
		}
		break;
	case ALIGNMENT_RIGHT:
		lLeftIndentation = lScreenWidth - lNetWidth;
		DBG_DEC_C(lLeftIndentation < 0, lLeftIndentation);
		if (lLeftIndentation > 0) {
			vSetLeftIndentation(pDiag, lLeftIndentation);
		}
		break;
	case ALIGNMENT_JUSTIFY:
	case ALIGNMENT_LEFT:
	default:
		break;
	}
	vString2Diagram(pDiag, pAnchor);
} /* end of vAlign2Window */

/*
 * vJustify2Window
 */
void
vJustify2Window(diagram_type *pDiag, output_type *pAnchor,
	long lScreenWidth, long lRightIndentation, unsigned char ucAlignment)
{
	output_type	*pTmp;
	char	*pcNew, *pcOld, *szStorage;
	long	lNetWidth, lSpaceWidth, lToAdd;
	int	iFillerLen, iHoles;

	fail(pDiag == NULL || pAnchor == NULL);
	fail(lScreenWidth < MIN_SCREEN_WIDTH);
	fail(lRightIndentation > 0);

	NO_DBG_MSG("vJustify2Window");

	if (ucAlignment != ALIGNMENT_JUSTIFY) {
		vAlign2Window(pDiag, pAnchor, lScreenWidth, ucAlignment);
		return;
	}

	lNetWidth = lComputeNetWidth(pAnchor);

	if (lScreenWidth > lChar2MilliPoints(MAX_SCREEN_WIDTH) ||
	    lNetWidth <= 0) {
		/*
		 * Screenwidth is "infinite", so justify is not possible
		 * Don't bother to align an empty line
		 */
		vString2Diagram(pDiag, pAnchor);
		return;
	}

	/* Justify */
	fail(ucAlignment != ALIGNMENT_JUSTIFY);
	lSpaceWidth = lComputeStringWidth(" ", 1,
				pAnchor->tFontRef, pAnchor->sFontsize);
	lToAdd = lScreenWidth -
			lNetWidth -
			lDrawUnits2MilliPoints(pDiag->lXleft) +
			lRightIndentation;
	DBG_DEC_C(lToAdd < 0, lSpaceWidth);
	DBG_DEC_C(lToAdd < 0, lToAdd);
	DBG_DEC_C(lToAdd < 0, lScreenWidth);
	DBG_DEC_C(lToAdd < 0, lNetWidth);
	DBG_DEC_C(lToAdd < 0, lDrawUnits2MilliPoints(pDiag->lXleft));
	DBG_DEC_C(lToAdd < 0, pDiag->lXleft);
	DBG_DEC_C(lToAdd < 0, lRightIndentation);
	lToAdd /= lSpaceWidth;
	DBG_DEC_C(lToAdd < 0, lToAdd);
	if (lToAdd <= 0) {
		vString2Diagram(pDiag, pAnchor);
		return;
	}

	iHoles = iComputeHoles(pAnchor);
	/* Justify by adding spaces */
	for (pTmp = pAnchor; pTmp != NULL; pTmp = pTmp->pNext) {
		fail(pTmp->iNextFree != (int)strlen(pTmp->szStorage));
		fail(lToAdd < 0);
		szStorage = xmalloc((size_t)(pTmp->iNextFree + lToAdd + 1));
		pcNew = szStorage;
		for (pcOld = pTmp->szStorage; *pcOld != '\0'; pcOld++) {
			*pcNew++ = *pcOld;
			if (*pcOld == ' ' &&
			    *(pcOld + 1) != ' ' &&
			    iHoles > 0) {
				iFillerLen = (int)(lToAdd / iHoles);
				lToAdd -= iFillerLen;
				iHoles--;
				for (; iFillerLen > 0; iFillerLen--) {
					*pcNew++ = ' ';
				}
			}
		}
		*pcNew = '\0';
		pTmp->szStorage = xfree(pTmp->szStorage);
		pTmp->szStorage = szStorage;
		pTmp->tStorageSize = (size_t)(pTmp->iNextFree + lToAdd + 1);
		pTmp->lStringWidth +=
			(pcNew - szStorage - (long)pTmp->iNextFree) *
			lSpaceWidth;
		pTmp->iNextFree = pcNew - szStorage;
		fail(pTmp->iNextFree != (int)strlen(pTmp->szStorage));
	}
	DBG_DEC_C(lToAdd != 0, lToAdd);
	vString2Diagram(pDiag, pAnchor);
} /* end of vJustify2Window */

/*
 * vResetStyles - reset the style information variables
 */
void
vResetStyles(void)
{
	(void)memset(aiHdrCounter, 0, sizeof(aiHdrCounter));
} /* end of vResetStyles */

/*
 * Add the style characters to the line
 */
int
iStyle2Window(char *szLine, const style_block_type *pStyleInfo)
{
	char	*pcTxt;
	int	iIndex, iStyleIndex;

	fail(szLine == NULL || pStyleInfo == NULL);

	pcTxt = szLine;
	if ((int)pStyleInfo->ucStyle >= 1 && (int)pStyleInfo->ucStyle <= 9) {
		iStyleIndex = (int)pStyleInfo->ucStyle - 1;
		for (iIndex = 0; iIndex < 9; iIndex++) {
			if (iIndex == iStyleIndex) {
				aiHdrCounter[iIndex]++;
			} else if (iIndex > iStyleIndex) {
				aiHdrCounter[iIndex] = 0;
			} else if (aiHdrCounter[iIndex] < 1) {
				aiHdrCounter[iIndex] = 1;
			}
			if (iIndex <= iStyleIndex) {
				pcTxt += sprintf(pcTxt, "%d",
						aiHdrCounter[iIndex]);
				if (iIndex < iStyleIndex) {
					*pcTxt++ = '.';
				}
			}
		}
		*pcTxt++ = ' ';
	}
	*pcTxt = '\0';
	NO_DBG_MSG_C(szLine[0] != '\0', szLine);
	return pcTxt - szLine;
} /* end of iStyle2Window */

/*
 * vRemoveRowEnd - remove the end of table row indicator
 *
 * Remove the double TABLE_SEPARATOR characters from the end of the string.
 * Special: remove the TABLE_SEPARATOR, 0x0a sequence
 */
static void
vRemoveRowEnd(char *szRowTxt)
{
	int	iLastIndex;

	fail(szRowTxt == NULL || szRowTxt[0] == '\0');

	iLastIndex = (int)strlen(szRowTxt) - 1;

	if (szRowTxt[iLastIndex] == TABLE_SEPARATOR ||
	    szRowTxt[iLastIndex] == '\n') {
		szRowTxt[iLastIndex] = '\0';
		iLastIndex--;
	} else {
		DBG_HEX(szRowTxt[iLastIndex]);
	}

	if (iLastIndex >= 0 && szRowTxt[iLastIndex] == TABLE_SEPARATOR) {
		szRowTxt[iLastIndex] = '\0';
		return;
	}

	DBG_HEX(szRowTxt[iLastIndex]);
	DBG_MSG(szRowTxt);
} /* end of vRemoveRowEnd */

/*
 * vTableRow2Window - put a table row into a diagram
 */
void
vTableRow2Window(diagram_type *pDiag, output_type *pOutput,
		const row_block_type *pRowInfo)
{
	output_type	tRow;
	char	*aszColTxt[TABLE_COLUMN_MAX];
	char	*szLine, *pcTmp, *pcTxt;
	long	lCharWidthLarge, lCharWidthSmall;
	size_t	tSize;
	int	iIndex, iNbrOfColumns, iColumnWidth, iTmp;
	int	iLen1, iLen2, iLen;
	BOOL	bNotReady;

	fail(pDiag == NULL || pOutput == NULL || pRowInfo == NULL);
	fail(pOutput->szStorage == NULL);
	fail(pOutput->pNext != NULL);

	/* Character sizes */
	lCharWidthLarge = lComputeStringWidth("W", 1,
				pOutput->tFontRef, pOutput->sFontsize);
	NO_DBG_DEC(lCharWidthLarge);
	lCharWidthSmall = lComputeStringWidth("i", 1,
				pOutput->tFontRef, pOutput->sFontsize);
	NO_DBG_DEC(lCharWidthSmall);
	/* For the time being: use a fixed width font */
	fail(lCharWidthLarge != lCharWidthSmall);

	/* Make room for the row */
	tSize = (size_t)(lTwips2MilliPoints(pRowInfo->iColumnWidthSum) /
				lCharWidthSmall +
				(long)pRowInfo->ucNumberOfColumns + 3);
	szLine = xmalloc(tSize);

	vRemoveRowEnd(pOutput->szStorage);

	/* Split the row text into column texts */
	aszColTxt[0] = pOutput->szStorage;
	for (iNbrOfColumns = 1;
	     iNbrOfColumns < TABLE_COLUMN_MAX;
	     iNbrOfColumns++) {
		aszColTxt[iNbrOfColumns] =
				strchr(aszColTxt[iNbrOfColumns - 1],
					TABLE_SEPARATOR);
		if (aszColTxt[iNbrOfColumns] == NULL) {
			break;
		}
		*aszColTxt[iNbrOfColumns] = '\0';
		aszColTxt[iNbrOfColumns]++;
		NO_DBG_DEC(iNbrOfColumns);
		NO_DBG_MSG(aszColTxt[iNbrOfColumns]);
	}

	DBG_DEC_C(iNbrOfColumns != (int)pRowInfo->ucNumberOfColumns,
		iNbrOfColumns);
	DBG_DEC_C(iNbrOfColumns != (int)pRowInfo->ucNumberOfColumns,
		pRowInfo->ucNumberOfColumns);
	if (iNbrOfColumns != (int)pRowInfo->ucNumberOfColumns) {
		werr(0, "Skipping an unmatched table row");
		/* Clean up before you leave */
		szLine = xfree(szLine);
		return;
	}

	do {
		/* Print a table row line */
		bNotReady = FALSE;
		pcTxt = szLine;
		*pcTxt++ = TABLE_SEPARATOR_CHAR;
		for (iIndex = 0; iIndex < iNbrOfColumns; iIndex++) {
			iColumnWidth =
				(int)(lTwips2MilliPoints(
					pRowInfo->asColumnWidth[iIndex]) /
					lCharWidthLarge);
			fail(iColumnWidth < 0);
			if (iColumnWidth < 1) {
				/* Minimum column width */
				iColumnWidth = 1;
			} else if (iColumnWidth > 1) {
				/* Room for the TABLE_SEPARATOR_CHAR */
				iColumnWidth--;
			}
			NO_DBG_DEC(iColumnWidth);
			/* Compute the length of the text for a column */
			if (aszColTxt[iIndex] == NULL) {
				iLen = 0;
			} else {
				pcTmp = strchr(aszColTxt[iIndex], '\n');
				if (pcTmp == NULL) {
					iLen1 = INT_MAX;
				} else {
					iLen1 =
					pcTmp - aszColTxt[iIndex] + 1;
				}
				iLen2 = (int)strlen(aszColTxt[iIndex]);
				if (iLen2 > iColumnWidth) {
					iLen2 = iColumnWidth;
				}
				iLen = min(iLen1, iLen2);
			}
			NO_DBG_DEC(iLen);
			fail(iLen < 0 || iLen > iColumnWidth);
			if (iLen >= 1 &&
			    aszColTxt[iIndex][iLen - 1] == '\n') {
				aszColTxt[iIndex][iLen - 1] = ' ';
			}
			if (iLen == iColumnWidth &&
			    !isspace(aszColTxt[iIndex][iLen])) {
				/* Search for a breaking point */
				for (iTmp = iLen - 1; iTmp >= 0; iTmp--) {
					if (isspace(aszColTxt[iIndex][iTmp])) {
						/* Found a breaking point */
						iLen = iTmp + 1;
						NO_DBG_DEC(iLen);
						break;
					}
				}
			}
			/* Print the text */
			if (iLen <= 0) {
				aszColTxt[iIndex] = NULL;
			} else {
				pcTxt += sprintf(pcTxt,
					"%.*s", iLen, aszColTxt[iIndex]);
				aszColTxt[iIndex] += iLen;
				while (*aszColTxt[iIndex] == ' ') {
					aszColTxt[iIndex]++;
				}
				if (*aszColTxt[iIndex] != '\0') {
					/* The row takes more lines */
					bNotReady = TRUE;
				}
			}
			/* Print the filler */
			for (iTmp = 0; iTmp < iColumnWidth - iLen; iTmp++) {
				*pcTxt++ = (char)FILLER_CHAR;
			}
			*pcTxt++ = TABLE_SEPARATOR_CHAR;
			*pcTxt = '\0';
		}
		/* Output the table row line */
		*pcTxt = '\0';
		tRow = *pOutput;
		tRow.szStorage = szLine;
		tRow.iNextFree = pcTxt - szLine;
		tRow.lStringWidth = lComputeStringWidth(
					tRow.szStorage,
					tRow.iNextFree,
					tRow.tFontRef,
					tRow.sFontsize);
		vString2Diagram(pDiag, &tRow);
	} while (bNotReady);
	/* Clean up before you leave */
	szLine = xfree(szLine);
} /* end of vTableRow2Window */
