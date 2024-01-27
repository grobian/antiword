/*
 * fonts_r.c
 * Copyright (C) 1999,2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Functions to deal with fonts (RiscOs version)
 */

#include <stdlib.h>
#include <string.h>
#include "antiword.h"

static font		tFontCurr = (font)-1;

/*
 * pOpenFontTableFile - open the Font translation file
 * Copy the file to the proper place if necessary.
 *
 * Returns the file pointer or NULL
 */
FILE *
pOpenFontTableFile(void)
{
	FILE	*pFileR, *pFileW;
	char	*szFontNamesFile;
	size_t	tSize;
	BOOL	bFailed;
	char	acBuffer[256];

	pFileR = fopen("<AntiWord$FontNamesFile>", "r");
	if (pFileR != NULL) {
		/* The font table is already in the right directory */
		return pFileR;
	}

	szFontNamesFile = getenv("AntiWord$FontNamesSave");
	if (szFontNamesFile == NULL) {
		werr(0, "Warning: Name of the FontNames file not found");
		return NULL;
	}
	DBG_MSG(szFontNamesFile);

	pFileR = fopen("<AntiWord$Dir>.Resources.Default", "r");
	if (pFileR == NULL) {
		werr(0, "I can't find 'Resources.Default'");
		return NULL;
	}
	/* Here the default font translation table is known to exist */

	if (!bMakeDirectory(szFontNamesFile)) {
		werr(0,
		"I can't make a directory for the FontNames file");
		return NULL;
	}
	/* Here the proper directory is known to exist */

	pFileW = fopen(szFontNamesFile, "w");
	if (pFileW == NULL) {
		(void)fclose(pFileR);
		werr(0, "I can't create a default FontNames file");
		return NULL;
	}
	/* Here the proper directory is known to be writeable */

	/* Copy the default FontNames file */
	bFailed = FALSE;
	while (!feof(pFileR)) {
		tSize = fread(acBuffer, 1, sizeof(acBuffer), pFileR);
		if (ferror(pFileR)) {
			DBG_MSG("Read error");
			bFailed = TRUE;
			break;
		}
		if (fwrite(acBuffer, 1, tSize, pFileW) != tSize) {
			DBG_MSG("Write error");
			bFailed = TRUE;
			break;
		}
	}
	(void)fclose(pFileW);
	(void)fclose(pFileR);
	if (bFailed) {
		DBG_MSG("Copying the FontNames file failed");
		(void)remove(szFontNamesFile);
		return NULL;
	}
	return fopen(szFontNamesFile, "r");
} /* end of pOpenFontTableFile */

/*
 * vCloseFont - close the current font, if any
 */
void
vCloseFont(void)
{
	os_error	*e;

	NO_DBG_MSG("vCloseFont");

	if (tFontCurr == (font)-1) {
		return;
	}
	e = font_lose(tFontCurr);
	if (e != NULL) {
		werr(0, "Close font error %d: %s", e->errnum, e->errmess);
	}
	tFontCurr = -1;
} /* end of vCloseFont */

/*
 * tOpenFont - make the given font the current font
 *
 * Returns the font reference number for use in a draw file
 */
draw_fontref
tOpenFont(int iWordFontNumber, unsigned char ucFontstyle, int iWordFontsize)
{
	os_error	*e;
	const char	*szOurFontname;
	font	tFont;
	int	iFontnumber;

	NO_DBG_MSG("tOpenFont");
	NO_DBG_DEC(iWordFontNumber);
	NO_DBG_HEX(ucFontstyle);
	NO_DBG_DEC(iWordFontsize);

	/* Keep the relevant bits */
	ucFontstyle &= FONT_BOLD|FONT_ITALIC;
	NO_DBG_HEX(ucFontstyle);

	iFontnumber = iGetFontByNumber(iWordFontNumber, ucFontstyle);
	szOurFontname = szGetOurFontname(iFontnumber);
	if (szOurFontname == NULL || szOurFontname[0] == '\0') {
		tFontCurr = (font)-1;
		return (draw_fontref)0;
	}
	NO_DBG_MSG(szOurFontname);
	e = font_find((char *)szOurFontname,
			iWordFontsize * 8, iWordFontsize * 8,
			0, 0, &tFont);
	if (e != NULL) {
		switch (e->errnum) {
		case 523:
			werr(0, "%s", e->errmess);
			break;
		default:
			werr(0, "Open font error %d: %s",
				e->errnum, e->errmess);
			break;
		}
		tFontCurr = (font)-1;
		return (draw_fontref)0;
	}
	tFontCurr = tFont;
	NO_DBG_DEC(tFontCurr);
	return (draw_fontref)(iFontnumber + 1);
} /* end of tOpenFont */

/*
 * tOpenTableFont - make the table font the current font
 *
 * Returns the font reference number for use in a draw file
 */
draw_fontref
tOpenTableFont(int iWordFontsize)
{
	int	iWordFontnumber;

	NO_DBG_MSG("tOpenTableFont");

	iWordFontnumber = iFontname2Fontnumber(TABLE_FONT, FONT_REGULAR);
	if (iWordFontnumber < 0 || iWordFontnumber > UCHAR_MAX) {
		DBG_DEC(iWordFontnumber);
		tFontCurr = (font)-1;
		return (draw_fontref)0;
	}

	return tOpenFont(iWordFontnumber, FONT_REGULAR, iWordFontsize);
} /* end of tOpenTableFont */

/*
 * lComputeStringWidth - compute the string width
 *
 * Returns string width in millipoints
 */
long
lComputeStringWidth(char *szString, int iStringLength,
		draw_fontref tFontRef, int iFontsize)
{
	font_string	tStr;
	os_error	*e;

	fail(szString == NULL || iStringLength < 0);
	fail(iFontsize < MIN_FONT_SIZE || iFontsize > MAX_FONT_SIZE);

	if (szString[0] == '\0' || iStringLength <= 0) {
		/* Empty string */
		return 0;
	}
	if (iStringLength == 1 && szString[0] == TABLE_SEPARATOR) {
		/* Font_strwidth doesn't like control characters */
		return 0;
	}
	if (tFontCurr == (font)-1) {
		/* No current font, use systemfont */
		return lChar2MilliPoints(iStringLength);
	}
	tStr.s = szString;
	tStr.x = INT_MAX;
	tStr.y = INT_MAX;
	tStr.split = -1;
	tStr.term = iStringLength;
	e = font_strwidth(&tStr);
	if (e == NULL) {
		return (long)tStr.x;
	}
	DBG_DEC(e->errnum);
	DBG_MSG(e->errmess);
	DBG_DEC(iStringLength);
	DBG_MSG(szString);
	werr(0, "String width error %d: %s", e->errnum, e->errmess);
	return lChar2MilliPoints(iStringLength);
} /* end of lComputeStringWidth */
