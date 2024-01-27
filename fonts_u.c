/*
 * fonts_u.c
 * Copyright (C) 1999,2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Functions to deal with fonts (Unix version)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "antiword.h"
#include "fontinfo.h"

/* Don't use fonts, just plain text */
static BOOL		bUsePlainText = TRUE;
/* Which character set should be used */
static encoding_type	eEncoding = encoding_neutral;


/* Default contents of the fontnames translation table */
static const char *szFonttable[] = {
"# Default fontnames translation table\n",
"# uses only Standard PostScript (TM) fonts\n",
"#\n",
"# MS-Word fontname,	Italic,	Bold,	PostScript fontname,	Special\n",
"Arial,			0,	0,	Helvetica,		0\n",
"Arial,			0,	1,	Helvetica-Bold,		0\n",
"Arial,			1,	0,	Helvetica-Oblique,	0\n",
"Arial,			1,	1,	Helvetica-BoldOblique,	0\n",
"Arial Black,		0,	0,	Helvetica,		0\n",
"Arial Black,		0,	1,	Helvetica-Bold,		0\n",
"Arial Black,		1,	0,	Helvetica-Oblique,	0\n",
"Arial Black,		1,	1,	Helvetica-BoldOblique,	0\n",
"Arial CE,		0,	0,	Helvetica,		0\n",
"Arial CE,		0,	1,	Helvetica-Bold,		0\n",
"Arial CE,		1,	0,	Helvetica-Oblique,	0\n",
"Arial CE,		1,	1,	Helvetica-BoldOblique,	0\n",
"Arial Narrow,		0,	0,	Helvetica-Narrow,	0\n",
"Arial Narrow,		0,	1,	Helvetica-Narrow-Bold,	0\n",
"Arial Narrow,		1,	0,	Helvetica-Narrow-Oblique,	0\n",
"Arial Narrow,		1,	1,	Helvetica-Narrow-BoldOblique,	0\n",
"AvantGarde,		0,	0,	AvantGarde-Book,	0\n",
"AvantGarde,		0,	1,	AvantGarde-Demi,	0\n",
"AvantGarde,		1,	0,	AvantGarde-BookOblique,	0\n",
"AvantGarde,		1,	1,	AvantGarde-DemiOblique,	0\n",
"Bookman Old Style,	0,	0,	Bookman-Light,		0\n",
"Bookman Old Style,	0,	1,	Bookman-Demi,		0\n",
"Bookman Old Style,	1,	0,	Bookman-LightItalic,	0\n",
"Bookman Old Style,	1,	1,	Bookman-DemiItalic,	0\n",
"Century Schoolbook,	0,	0,	NewCenturySchlbk-Roman,	0\n",
"Century Schoolbook,	0,	1,	NewCenturySchlbk-Bold,	0\n",
"Century Schoolbook,	1,	0,	NewCenturySchlbk-Italic,	0\n",
"Century Schoolbook,	1,	1,	NewCenturySchlbk-BoldItalic,	0\n",
"Comic Sans MS,		0,	0,	Helvetica,		0\n",
"Comic Sans MS,		0,	1,	Helvetica-Bold,		0\n",
"Comic Sans MS,		1,	0,	Helvetica-Oblique,	0\n",
"Comic Sans MS,		1,	1,	Helvetica-BoldOblique,	0\n",
"Courier,		0,	0,	Courier,		0\n",
"Courier,		0,	1,	Courier-Bold,		0\n",
"Courier,		1,	0,	Courier-Oblique,	0\n",
"Courier,		1,	1,	Courier-BoldOblique,	0\n",
"Courier New,		0,	0,	Courier,		0\n",
"Courier New,		0,	1,	Courier-Bold,		0\n",
"Courier New,		1,	0,	Courier-Oblique,	0\n",
"Courier New,		1,	1,	Courier-BoldOblique,	0\n",
"Fixedsys,		0,	0,	Courier,		0\n",
"Fixedsys,		0,	1,	Courier-Bold,		0\n",
"Fixedsys,		1,	0,	Courier-Oblique,	0\n",
"Fixedsys,		1,	1,	Courier-BoldOblique,	0\n",
"Helvetica,		0,	0,	Helvetica,		0\n",
"Helvetica,		0,	1,	Helvetica-Bold,		0\n",
"Helvetica,		1,	0,	Helvetica-Oblique,	0\n",
"Helvetica,		1,	1,	Helvetica-BoldOblique,	0\n",
"Helvetica-Narrow,	0,	0,	Helvetica-Narrow,	0\n",
"Helvetica-Narrow,	0,	1,	Helvetica-Narrow-Bold,	0\n",
"Helvetica-Narrow,	1,	0,	Helvetica-Narrow-Oblique,	0\n",
"Helvetica-Narrow,	1,	1,	Helvetica-Narrow-BoldOblique,	0\n",
"ITC Bookman,		0,	0,	Bookman-Light,		0\n",
"ITC Bookman,		0,	1,	Bookman-Demi,		0\n",
"ITC Bookman,		1,	0,	Bookman-LightItalic,	0\n",
"ITC Bookman,		1,	1,	Bookman-DemiItalic,	0\n",
"Lucida Console,	0,	0,	Courier,		0\n",
"Lucida Console,	0,	1,	Courier-Bold,		0\n",
"Lucida Console,	1,	0,	Courier-Oblique,	0\n",
"Lucida Console,	1,	1,	Courier-BoldOblique,	0\n",
"Monotype.com,		0,	0,	Courier,		0\n",
"Monotype.com,		0,	1,	Courier-Bold,		0\n",
"Monotype.com,		1,	0,	Courier-Oblique,	0\n",
"Monotype.com,		1,	1,	Courier-BoldOblique,	0\n",
"MS Sans Serif,		0,	0,	Helvetica,		0\n",
"MS Sans Serif,		0,	1,	Helvetica-Bold,		0\n",
"MS Sans Serif,		1,	0,	Helvetica-Oblique,	0\n",
"MS Sans Serif,		1,	1,	Helvetica-BoldOblique,	0\n",
"New Century Schlbk,	0,	0,	NewCenturySchlbk-Roman,	0\n",
"New Century Schlbk,	0,	1,	NewCenturySchlbk-Bold,	0\n",
"New Century Schlbk,	1,	0,	NewCenturySchlbk-Italic,	0\n",
"New Century Schlbk,	1,	1,	NewCenturySchlbk-BoldItalic,	0\n",
"NewCenturySchlbk,	0,	0,	NewCenturySchlbk-Roman,	0\n",
"NewCenturySchlbk,	0,	1,	NewCenturySchlbk-Bold,	0\n",
"NewCenturySchlbk,	1,	0,	NewCenturySchlbk-Italic,	0\n",
"NewCenturySchlbk,	1,	1,	NewCenturySchlbk-BoldItalic,	0\n",
"Palatino,		0,	0,	Palatino-Roman,		0\n",
"Palatino,		0,	1,	Palatino-Bold,		0\n",
"Palatino,		1,	0,	Palatino-Italic,	0\n",
"Palatino,		1,	1,	Palatino-BoldItalic,	0\n",
"Swiss,			0,	0,	Helvetica,		0\n",
"Swiss,			0,	1,	Helvetica-Bold,		0\n",
"Swiss,			1,	0,	Helvetica-Oblique,	0\n",
"Swiss,			1,	1,	Helvetica-BoldOblique,	0\n",
"Tahoma,		0,	0,	Helvetica,		0\n",
"Tahoma,		0,	1,	Helvetica-Bold,		0\n",
"Tahoma,		1,	0,	Helvetica-Oblique,	0\n",
"Tahoma,		1,	1,	Helvetica-BoldOblique,	0\n",
"Trebuchet MS,		0,	0,	Helvetica,		0\n",
"Trebuchet MS,		0,	1,	Helvetica-Bold,		0\n",
"Trebuchet MS,		1,	0,	Helvetica-Oblique,	0\n",
"Trebuchet MS,		1,	1,	Helvetica-BoldOblique,	0\n",
"Univers,		0,	0,	Helvetica,		0\n",
"Univers,		0,	1,	Helvetica-Bold,		0\n",
"Univers,		1,	0,	Helvetica-Oblique,	0\n",
"Univers,		1,	1,	Helvetica-BoldOblique,	0\n",
"Verdana,		0,	0,	Helvetica,		0\n",
"Verdana,		0,	1,	Helvetica-Bold,		0\n",
"Verdana,		1,	0,	Helvetica-Oblique,	0\n",
"Verdana,		1,	1,	Helvetica-BoldOblique,	0\n",
"# All the other fonts\n",
"*,			0,	0,	Times-Roman,		0\n",
"*,			0,	1,	Times-Bold,		0\n",
"*,			1,	0,	Times-Italic,		0\n",
"*,			1,	1,	Times-BoldItalic,	0\n",
};


/*
 * pOpenFontTableFile - open the Font translation file
 * Copy the file to the proper place if necessary.
 *
 * Returns the file pointer or NULL
 */
FILE *
pOpenFontTableFile(void)
{
	FILE	*pFile;
	const char	*szHome;
	int	iIndex;
	BOOL	bFailed;
	char	szFontNamesFile[PATH_MAX+1];
	
	szHome = szGetHomeDirectory();
	if (strlen(szHome) + sizeof(FONTNAMES_FILE) >=
						sizeof(szFontNamesFile)) {
		werr(0, "The name of your HOME directory is too long");
		return NULL;
	}

	sprintf(szFontNamesFile, "%s%s", szHome, FILE_SEPARATOR FONTNAMES_FILE);
	DBG_MSG(szFontNamesFile);

	pFile = fopen(szFontNamesFile, "r");
	if (pFile != NULL) {
		/* The font table is already in the right directory */
		return pFile;
	}

	if (!bMakeDirectory(szFontNamesFile)) {
		werr(0,
		"I can't make a directory for the fontnames file");
		return NULL;
	}
	/* Here the proper directory is known to exist */

	pFile = fopen(szFontNamesFile, "w");
	if (pFile == NULL) {
		werr(0, "I can't create a default fontnames file");
		return NULL;
	}
	/* Here the proper directory is known to be writeable */

	bFailed = FALSE;
	/* Copy the default fontnames file */
	for (iIndex = 0; iIndex < (int)elementsof(szFonttable); iIndex++) {
		if (fputs(szFonttable[iIndex], pFile) < 0) {
			DBG_MSG("Write error");
			bFailed = TRUE;
			break;
		}
	}
	(void)fclose(pFile);
	if (bFailed) {
		DBG_MSG("Copying the fontnames file failed");
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
	NO_DBG_MSG("vCloseFont");
	/* For safety: to be overwritten at the next call of tOpenfont() */
	bUsePlainText = TRUE;
	eEncoding = encoding_neutral;
} /* end of vCloseFont */

/*
 * tOpenFont - make the given font the current font
 *
 * Returns the font reference number
 */
draw_fontref
tOpenFont(int iWordFontNumber, unsigned char ucFontstyle, int iWordFontsize)
{
	options_type	tOptions;
	const char	*szOurFontname;
	int	iIndex, iFontnumber;

	NO_DBG_MSG("tOpenFont");
	NO_DBG_DEC(iWordFontNumber);
	NO_DBG_HEX(ucFontstyle);
	NO_DBG_DEC(iWordFontsize);

	/* Keep the relevant bits */
	ucFontstyle &= FONT_BOLD|FONT_ITALIC;
	NO_DBG_HEX(ucFontstyle);

	vGetOptions(&tOptions);
	bUsePlainText = !tOptions.bUseOutlineFonts;
	eEncoding = tOptions.eEncoding;

	if (bUsePlainText) {
		/* No outline fonts, no postscript just plain text */
		return (draw_fontref)0;
	}

	iFontnumber = iGetFontByNumber(iWordFontNumber, ucFontstyle);
	szOurFontname = szGetOurFontname(iFontnumber);
	if (szOurFontname == NULL || szOurFontname[0] == '\0') {
		DBG_DEC(iFontnumber);
		return (draw_fontref)0;
	}
	NO_DBG_MSG(szOurFontname);

	for (iIndex = 0; iIndex < (int)elementsof(szFontnames); iIndex++) {
		if (STREQ(szFontnames[iIndex], szOurFontname)) {
			NO_DBG_DEC(iIndex);
			return (draw_fontref)iIndex;
		}
	}
	return (draw_fontref)0;
} /* end of tOpenFont */

/*
 * tOpenTableFont - make the table font the current font
 *
 * Returns the font reference number
 */
draw_fontref
tOpenTableFont(int iWordFontsize)
{
	options_type	tOptions;
	int	iWordFontnumber;

	NO_DBG_MSG("tOpenTableFont");

	vGetOptions(&tOptions);
	bUsePlainText = !tOptions.bUseOutlineFonts;
	if (bUsePlainText) {
		/* No outline fonts, no postscript just plain text */
		return (draw_fontref)0;
	}

	iWordFontnumber = iFontname2Fontnumber(TABLE_FONT, FONT_REGULAR);
	if (iWordFontnumber < 0 || iWordFontnumber > UCHAR_MAX) {
		DBG_DEC(iWordFontnumber);
		return (draw_fontref)0;
	}

	return tOpenFont(iWordFontnumber, FONT_REGULAR, iWordFontsize);
} /* end of tOpenTableFont */

/*
 * szGetFontname - get the fontname
 */
const char *
szGetFontname(draw_fontref tFontRef)
{
	fail((size_t)(unsigned char)tFontRef >= elementsof(szFontnames));
	return szFontnames[(int)(unsigned char)tFontRef];
} /* end of szGetFontname */

/*
 * lComputeStringWidth - compute the string width
 *
 * Note: the fontsize is given in half-points!
 *
 * Returns the string width in millipoints
 */
long
lComputeStringWidth(char *szString, int iStringLength,
		draw_fontref tFontRef, int iFontsize)
{
	unsigned short	*ausCharWidths;
	unsigned char	*pucChar;
	long	lRelWidth;
	int	iIndex, iFontRef;

	fail(szString == NULL || iStringLength < 0);
	fail(iFontsize < MIN_FONT_SIZE || iFontsize > MAX_FONT_SIZE);

	if (szString[0] == '\0' || iStringLength <= 0) {
		/* Empty string */
		return 0;
	}

	if (bUsePlainText) {
		/* No current font, use "systemfont" */
		return lChar2MilliPoints(iStringLength);
	}

	fail(eEncoding != encoding_iso_8859_1 &&
		eEncoding != encoding_iso_8859_2);

	/* Compute the relative string width */
	iFontRef = (int)(unsigned char)tFontRef;
	if (eEncoding == encoding_iso_8859_2) {
		ausCharWidths = ausCharacterWidths2[iFontRef];
	} else {
		ausCharWidths = ausCharacterWidths1[iFontRef];
	}
	lRelWidth = 0;
	for (iIndex = 0, pucChar = (unsigned char *)szString;
	     iIndex < iStringLength;
	     iIndex++, pucChar++) {
		lRelWidth += (long)ausCharWidths[(int)*pucChar];
	}

	/* Compute the absolute string width */
	return (lRelWidth * iFontsize + 1) / 2;
} /* end of lComputeStringWidth */
