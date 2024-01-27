/*
 * postscript.c
 * Copyright (C) 1999,2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Functions to deal with the PostScript format
 *
 *================================================================
 * The function vImagePrologue is based on:
 * jpeg2ps - convert JPEG compressed images to PostScript Level 2
 * Copyright (C) 1994-99 Thomas Merz (tm@muc.de)
 *================================================================
 * The credit should go to him, but all the bugs are mine.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "version.h"
#include "antiword.h"

/* The output must be in PostScript */
static BOOL		bUsePostScript = FALSE;
/* The character set */
static encoding_type	eEncoding = encoding_neutral;
/* The image level */
static image_level_enum	eImageLevel = level_default;
/* The output must use landscape orientation */
static BOOL		bUseLandscape = FALSE;
/* The height of a PostScript page (in DrawUnits) */
static long		lPageHeight = LONG_MAX;
/* The net width of a PostScript page (in points) */
static double		dNetPageWidth = (double)ULONG_MAX;
/* Current time for a PS header */
static const char	*szCreationDate = NULL;
/* Current creator for a PS header */
static const char	*szCreator = NULL;
/* Current font information */
static draw_fontref	tFontRefCurr = (draw_fontref)-1;
static int		iFontsizeCurr = -1;
static int		iColorCurr = -1;
/* Current vertical position information */
static long		lYtopCurr = -1;
/* PostScript page counter */
static int	iPageCount = 0;
/* Image counter */
static int	iImageCount = 0;

static char iso_8859_1_data[] = { "\
/newcodes	% ISO-8859-1 character encodings\n\
[\n\
160/space 161/exclamdown 162/cent 163/sterling 164/currency\n\
165/yen 166/brokenbar 167/section 168/dieresis 169/copyright\n\
170/ordfeminine 171/guillemotleft 172/logicalnot 173/hyphen 174/registered\n\
175/macron 176/degree 177/plusminus 178/twosuperior 179/threesuperior\n\
180/acute 181/mu 182/paragraph 183/periodcentered 184/cedilla\n\
185/onesuperior 186/ordmasculine 187/guillemotright 188/onequarter\n\
189/onehalf 190/threequarters 191/questiondown 192/Agrave 193/Aacute\n\
194/Acircumflex 195/Atilde 196/Adieresis 197/Aring 198/AE 199/Ccedilla\n\
200/Egrave 201/Eacute 202/Ecircumflex 203/Edieresis 204/Igrave 205/Iacute\n\
206/Icircumflex 207/Idieresis 208/Eth 209/Ntilde 210/Ograve 211/Oacute\n\
212/Ocircumflex 213/Otilde 214/Odieresis 215/multiply 216/Oslash\n\
217/Ugrave 218/Uacute 219/Ucircumflex 220/Udieresis 221/Yacute 222/Thorn\n\
223/germandbls 224/agrave 225/aacute 226/acircumflex 227/atilde\n\
228/adieresis 229/aring 230/ae 231/ccedilla 232/egrave 233/eacute\n\
234/ecircumflex 235/edieresis 236/igrave 237/iacute 238/icircumflex\n\
239/idieresis 240/eth 241/ntilde 242/ograve 243/oacute 244/ocircumflex\n\
245/otilde 246/odieresis 247/divide 248/oslash 249/ugrave 250/uacute\n\
251/ucircumflex 252/udieresis 253/yacute 254/thorn 255/ydieresis\n\
] bind def\n\
\n\
/reencdict 12 dict def\n\
\n\
" };

static char iso_8859_2_data[] = { "\
/newcodes	% ISO-8859-2 character encodings\n\
[\n\
160/space 161/Aogonek 162/breve 163/Lslash 164/currency 165/Lcaron\n\
166/Sacute 167/section 168/dieresis 169/Scaron 170/Scommaaccent\n\
171/Tcaron 172/Zacute 173/hyphen 174/Zcaron 175/Zdotaccent 176/degree\n\
177/aogonek 178/ogonek 179/lslash 180/acute 181/lcaron 182/sacute\n\
183/caron 184/cedilla 185/scaron 186/scommaaccent 187/tcaron\n\
188/zacute 189/hungarumlaut 190/zcaron 191/zdotaccent 192/Racute\n\
193/Aacute 194/Acircumflex 195/Abreve 196/Adieresis 197/Lacute\n\
198/Cacute 199/Ccedilla 200/Ccaron 201/Eacute 202/Eogonek\n\
203/Edieresis 204/Ecaron 205/Iacute 206/Icircumflex 207/Dcaron\n\
208/Dcroat 209/Nacute 210/Ncaron 211/Oacute 212/Ocircumflex\n\
213/Ohungarumlaut 214/Odieresis 215/multiply 216/Rcaron 217/Uring\n\
218/Uacute 219/Uhungarumlaut 220/Udieresis 221/Yacute 222/Tcommaaccent\n\
223/germandbls 224/racute 225/aacute 226/acircumflex 227/abreve\n\
228/adieresis 229/lacute 230/cacute 231/ccedilla 232/ccaron 233/eacute\n\
234/eogonek 235/edieresis 236/ecaron 237/iacute 238/icircumflex\n\
239/dcaron 240/dcroat 241/nacute 242/ncaron 243/oacute 244/ocircumflex\n\
245/ohungarumlaut 246/odieresis 247/divide 248/rcaron 249/uring\n\
250/uacute 251/uhungarumlaut 252/udieresis 253/yacute 254/tcommaaccent\n\
255/dotaccent\n\
] bind def\n\
\n\
/reencdict 12 dict def\n\
\n\
" };

static char iso_8859_x_func[] = { "\
% change fonts using ISO-8859-x characters\n\
/ChgFnt		% size psname natname => font\n\
{\n\
	dup FontDirectory exch known		% is re-encoded name known?\n\
	{ exch pop }				% yes, get rid of long name\n\
	{ dup 3 1 roll ReEncode } ifelse	% no, re-encode it\n\
	findfont exch scalefont setfont\n\
} bind def\n\
\n\
/ReEncode\n\
{\n\
reencdict begin\n\
	/newname exch def\n\
	/basename exch def\n\
	/basedict basename findfont def\n\
	/newfont basedict maxlength dict def\n\
	basedict\n\
	{ exch dup /FID ne\n\
		{ dup /Encoding eq\n\
			{ exch dup length array copy newfont 3 1 roll put }\n\
			{ exch newfont 3 1 roll put } ifelse\n\
		}\n\
		{ pop pop } ifelse\n\
	} forall\n\
	newfont /FontName newname put\n\
	newcodes aload pop newcodes length 2 idiv\n\
	{ newfont /Encoding get 3 1 roll put } repeat\n\
	newname newfont definefont pop\n\
end\n\
} bind def\n\
\n\
" };

static char misc_func[] = { "\
% draw a line and show the string\n\
/LineShow	% string linewidth movement\n\
{\n\
	gsave\n\
		0 exch rmoveto\n\
		setlinewidth\n\
		dup\n\
		stringwidth pop\n\
		0 rlineto stroke\n\
	grestore\n\
	show\n\
} bind def\n\
\n\
% begin an EPS file (level 2 and up)\n\
/BeginEPSF\n\
{\n\
	/b4_Inc_state save def\n\
	/dict_count countdictstack def\n\
	/op_count count 1 sub def\n\
	userdict begin\n\
		/showpage { } def\n\
		0 setgray 0 setlinecap\n\
		1 setlinewidth 0 setlinejoin\n\
		10 setmiterlimit [ ] 0 setdash newpath\n\
		false setstrokeadjust false setoverprint\n\
} bind def\n\
\n\
% end an EPS file\n\
/EndEPSF {\n\
	count op_count sub { pop } repeat\n\
	countdictstack dict_count sub { end } repeat\n\
	b4_Inc_state restore\n\
} bind def\n\
\n\
" };


/*
 * vAddPageSetup - add the page setup
 */
static void
vAddPageSetup(FILE *pOutFile)
{
	if (bUseLandscape) {
		fprintf(pOutFile, "%%%%BeginPageSetup\n");
		fprintf(pOutFile, "90 rotate\n");
		fprintf(pOutFile, "0.00 %.2f translate\n",
					-dDrawUnits2Points(lPageHeight));
		fprintf(pOutFile, "%%%%EndPageSetup\n");
	}
} /* end of vAddPageSetup */

/*
 * vMove2NextPage - move to the start of the next page
 */
static void
vMove2NextPage(diagram_type *pDiag)
{
	fail(pDiag == NULL);
	fail(!bUsePostScript);

	fprintf(pDiag->pOutFile, "showpage\n");
	iPageCount++;
	fprintf(pDiag->pOutFile, "%%%%Page: %d %d\n", iPageCount, iPageCount);
	vAddPageSetup(pDiag->pOutFile);
	pDiag->lYtop = lPageHeight - PS_TOP_MARGIN;
	lYtopCurr = -1;
} /* end of vMove2NextPage */

/*
 * vMoveToPS - move to the given X,Y coordinates (Postscript)
 *
 * Move the current position of the given diagram to its X,Y coordianates,
 * start on a new page if needed
 */
static void
vMoveToPS(diagram_type *pDiag, long lLastVerticalMovement)
{
	fail(pDiag == NULL);
	fail(pDiag->pOutFile == NULL);

	if (pDiag->lYtop < PS_BOTTOM_MARGIN) {
		vMove2NextPage(pDiag);
		/* Repeat the last vertical movement on the new page */
		pDiag->lYtop -= lLastVerticalMovement;
	}
	fail(pDiag->lYtop < PS_BOTTOM_MARGIN);

	if (pDiag->lYtop != lYtopCurr) {
		fprintf(pDiag->pOutFile, "%.2f %.2f moveto\n",
			dDrawUnits2Points(pDiag->lXleft + PS_LEFT_MARGIN),
			dDrawUnits2Points(pDiag->lYtop));
		lYtopCurr = pDiag->lYtop;
	}
} /* end of vMoveToPS */

/*
 * vPrologue - perform the PostScript initialisation
 */
static void
vPrologue(FILE *pOutFile, const char *szTask, const char *szFilename)
{
	options_type	tOptions;
	const char	*szTmp;
	time_t	tTime;

	fail(pOutFile == NULL);
	fail(szTask == NULL || szTask[0] == '\0');

	vGetOptions(&tOptions);
	lPageHeight = lPoints2DrawUnits(tOptions.iPageHeight);
	DBG_DEC(lPageHeight);
	dNetPageWidth = dDrawUnits2Points(
		lPoints2DrawUnits(tOptions.iPageWidth) -
		(PS_LEFT_MARGIN + PS_RIGHT_MARGIN));
	DBG_FLT(dNetPageWidth);
	bUsePostScript = tOptions.bUseOutlineFonts;
	bUseLandscape = tOptions.bUseLandscape;
	eEncoding = tOptions.eEncoding;
	eImageLevel = tOptions.eImageLevel;
	tFontRefCurr = (draw_fontref)-1;
	iFontsizeCurr = -1;
	iColorCurr = -1;
	lYtopCurr = -1;
	iImageCount = 0;

	if (!bUsePostScript) {
		return;
	}

	szCreator = szTask;

	fprintf(pOutFile, "%%!PS-Adobe-2.0\n");
	fprintf(pOutFile, "%%%%Title: %s\n", szBasename(szFilename));
	fprintf(pOutFile, "%%%%Creator: %s %s\n", szCreator, VERSIONSTRING);
	szTmp = getenv("LOGNAME");
	if (szTmp == NULL || szTmp[0] == '\0') {
		szTmp = getenv("USER");
		if (szTmp == NULL || szTmp[0] == '\0') {
			szTmp = "unknown";
		}
	}
	fprintf(pOutFile, "%%%%For: %s\n", szTmp);
	errno = 0;
	tTime = time(NULL);
	if (tTime == (time_t)-1 && errno != 0) {
		szCreationDate = NULL;
	} else {
		szCreationDate = ctime(&tTime);
	}
	if (szCreationDate == NULL || szCreationDate[0] == '\0') {
		szCreationDate = "unknown\n";
	}
	fprintf(pOutFile, "%%%%CreationDate: %s", szCreationDate);
	if (bUseLandscape) {
		fprintf(pOutFile, "%%%%Orientation: Landscape\n");
		fprintf(pOutFile, "%%%%BoundingBox: 0 0 %d %d\n",
				tOptions.iPageHeight, tOptions.iPageWidth);
	} else {
		fprintf(pOutFile, "%%%%Orientation: Portrait\n");
		fprintf(pOutFile, "%%%%BoundingBox: 0 0 %d %d\n",
				tOptions.iPageWidth, tOptions.iPageHeight);
	}
} /* end of vPrologue */

/*
 * vEpilogue - clean up after everything is done
 */
static void
vEpilogue(FILE *pFile)
{
	if (!bUsePostScript) {
		fprintf(pFile, "\n");
		return;
	}

	fprintf(pFile, "%%%%Trailer\n");
	fprintf(pFile, "%%%%Pages: %d\n", iPageCount);
	fprintf(pFile, "%%%%EOF\n");
	szCreationDate = NULL;
	szCreator = NULL;
} /* end of vEpilogue */

/*
 * vPrintPalette - print a postscript palette
 */
static void
vPrintPalette(FILE *pOutFile, const imagedata_type *pImg)
{
	int	iIndex;

	fail(pOutFile == NULL);
	fail(pImg == NULL);
	fail(pImg->iColorsUsed < 2);
	fail(pImg->iColorsUsed > 256);

	fprintf(pOutFile, "[ /Indexed\n");
	fprintf(pOutFile, "\t/Device%s %d\n",
		pImg->bColorImage ? "RGB" : "Gray", pImg->iColorsUsed - 1);
	fprintf(pOutFile, "<");
	for (iIndex = 0; iIndex < pImg->iColorsUsed; iIndex++) {
		fprintf(pOutFile, "%02x", pImg->aucPalette[iIndex][0]);
		if (pImg->bColorImage) {
			fprintf(pOutFile, "%02x%02x",
				pImg->aucPalette[iIndex][1],
				pImg->aucPalette[iIndex][2]);
		}
		if (iIndex % 8 == 7) {
			fprintf(pOutFile, "\n");
		} else {
			fprintf(pOutFile, " ");
		}
	}
	fprintf(pOutFile, ">\n");
	fprintf(pOutFile, "] setcolorspace\n");
} /* end of vPrintPalette */

/*
 * vImagePrologue - perform the Encapsulated PostScript initialisation
 */
void
vImagePrologue(diagram_type *pDiag, const imagedata_type *pImg)
{
	FILE	*pOutFile;
	double	dHor, dVer;
	int	iHorSize, iVerSize;

	fail(pDiag == NULL);
	fail(pDiag->pOutFile == NULL);
	fail(pImg == NULL);

	if (!bUsePostScript) {
		return;
	}

	fail(szCreationDate == NULL);
	fail(szCreator == NULL);
	fail(eImageLevel == level_no_images);

	iImageCount++;

	/* Scaling */
	dHor = pImg->iHorizontalSize * pImg->dHorizontalScalingFactor;
	dVer = pImg->iVerticalSize * pImg->dVerticalScalingFactor;

	/* Make sure the image fits on the paper */
	if (dHor < dNetPageWidth) {
		iHorSize = (int)(dHor + 0.5);
		iVerSize = (int)(dVer + 0.5);
	} else {
		iHorSize = (int)(dNetPageWidth + 0.5);
		iVerSize = (int)(dVer * dNetPageWidth / dHor + 0.5);
		DBG_DEC(iVerSize);
	}

	DBG_DEC_C(pDiag->lXleft != 0, pDiag->lXleft);
	pDiag->lYtop -= lPoints2DrawUnits(iVerSize);
	vMoveToPS(pDiag, lPoints2DrawUnits(iVerSize));

	pOutFile = pDiag->pOutFile;

	fprintf(pOutFile, "BeginEPSF\n");
	fprintf(pOutFile, "%%%%BeginDocument: image%03d.eps\n", iImageCount);
	fprintf(pOutFile, "%%!PS-Adobe-2.0 EPSF-2.0\n");
	fprintf(pOutFile, "%%%%Creator: %s %s\n", szCreator, VERSIONSTRING);
	fprintf(pOutFile, "%%%%Title: Image %03d\n", iImageCount);
	fprintf(pOutFile, "%%%%CreationDate: %s", szCreationDate);
	fprintf(pOutFile, "%%%%BoundingBox: 0 0 %d %d\n", iHorSize, iVerSize);
	fprintf(pOutFile, "%%%%DocumentData: Clean7Bit\n");
	fprintf(pOutFile, "%%%%LanguageLevel: 2\n");
	fprintf(pOutFile, "%%%%EndComments\n");
	fprintf(pOutFile, "%%%%BeginProlog\n");
	fprintf(pOutFile, "%%%%EndProlog\n");
	fprintf(pOutFile, "%%%%Page: 1 1\n");

	fprintf(pOutFile, "save\n");

	switch (pImg->eImageType) {
	case imagetype_is_jpeg:
		fprintf(pOutFile, "/Data1 currentfile ");
		fprintf(pOutFile, "/ASCII85Decode filter def\n");
		fprintf(pOutFile, "/Data Data1 << ");
		fprintf(pOutFile, ">> /DCTDecode filter def\n");
		switch (pImg->iComponents) {
		case 1:
			fprintf(pOutFile, "/DeviceGray setcolorspace\n");
			break;
		case 3:
			fprintf(pOutFile, "/DeviceRGB setcolorspace\n");
			break;
		case 4:
			fprintf(pOutFile, "/DeviceCMYK setcolorspace\n");
			break;
		default:
			DBG_DEC(pImg->iComponents);
			break;
		}
		break;
	case imagetype_is_png:
		if (eImageLevel == level_gs_special) {
			fprintf(pOutFile,
			"/Data2 currentfile /ASCII85Decode filter def\n");
			fprintf(pOutFile,
			"/Data1 Data2 << >> /FlateDecode filter def\n");
			fprintf(pOutFile, "/Data Data1 <<\n");
			fprintf(pOutFile, "\t/Colors %d\n", pImg->iComponents);
			fprintf(pOutFile, "\t/BitsPerComponent %d\n",
						pImg->iBitsPerComponent);
			fprintf(pOutFile, "\t/Columns %d\n", pImg->iWidth);
			fprintf(pOutFile,
				">> /PNGPredictorDecode filter def\n");
		} else {
			fprintf(pOutFile,
			"/Data1 currentfile /ASCII85Decode filter def\n");
			fprintf(pOutFile,
			"/Data Data1 << >> /FlateDecode filter def\n");
		}
		if (pImg->iComponents == 3) {
			fprintf(pOutFile, "/DeviceRGB setcolorspace\n");
		} else if (pImg->iColorsUsed > 0) {
			vPrintPalette(pOutFile, pImg);
		} else {
			fprintf(pOutFile, "/DeviceGray setcolorspace\n");
		}
		break;
	case imagetype_is_dib:
		fprintf(pOutFile, "/Data currentfile ");
		fprintf(pOutFile, "/ASCII85Decode filter def\n");
		if (pImg->iBitsPerComponent <= 8) {
			vPrintPalette(pOutFile, pImg);
		} else {
			fprintf(pOutFile, "/DeviceRGB setcolorspace\n");
		}
		break;
	default:
		fprintf(pOutFile, "/Data currentfile ");
		fprintf(pOutFile, "/ASCIIHexDecode filter def\n");
		fprintf(pOutFile, "/Device%s setcolorspace\n",
			pImg->bColorImage ? "RGB" : "Gray");
		break;
	}

	/* Translate to lower left corner of image */
	fprintf(pOutFile, "%.2f %.2f translate\n",
			dDrawUnits2Points(pDiag->lXleft + PS_LEFT_MARGIN),
			dDrawUnits2Points(pDiag->lYtop));

	fprintf(pOutFile, "%d %d scale\n",
			iHorSize, iVerSize);

	fprintf(pOutFile, "{ <<\n");
	fprintf(pOutFile, "\t/ImageType 1\n");
	fprintf(pOutFile, "\t/Width %d\n", pImg->iWidth);
	fprintf(pOutFile, "\t/Height %d\n", pImg->iHeight);
	if (pImg->eImageType == imagetype_is_dib) {
		/* Scanning from left to right and bottom to top */
		fprintf(pOutFile, "\t/ImageMatrix [ %d 0 0 %d 0 0 ]\n",
			pImg->iWidth, pImg->iHeight);
	} else {
		/* Scanning from left to right and top to bottom */
		fprintf(pOutFile, "\t/ImageMatrix [ %d 0 0 %d 0 %d ]\n",
			pImg->iWidth, -pImg->iHeight, pImg->iHeight);
	}
	fprintf(pOutFile, "\t/DataSource Data\n");

	switch (pImg->eImageType) {
	case imagetype_is_jpeg:
		fprintf(pOutFile, "\t/BitsPerComponent 8\n");
		switch (pImg->iComponents) {
		case 1:
			fprintf(pOutFile, "\t/Decode [0 1]\n");
			break;
		case 3:
			fprintf(pOutFile, "\t/Decode [0 1 0 1 0 1]\n");
			break;
		case 4:
			if (pImg->bAdobe) {
				/*
				 * Adobe-conforming CMYK file
				 * applying workaround for color inversion
				 */
				fprintf(pOutFile,
					"\t/Decode [1 0 1 0 1 0 1 0]\n");
			} else {
				fprintf(pOutFile,
					"\t/Decode [0 1 0 1 0 1 0 1]\n");
			}
			break;
		default:
			DBG_DEC(pImg->iComponents);
			break;
		}
		break;
	case imagetype_is_png:
		if (pImg->iComponents == 3) {
			fprintf(pOutFile, "\t/BitsPerComponent 8\n");
			fprintf(pOutFile, "\t/Decode [0 1 0 1 0 1]\n");
		} else if (pImg->iColorsUsed > 0) {
			fail(pImg->iBitsPerComponent > 8);
			fprintf(pOutFile, "\t/BitsPerComponent %d\n",
					pImg->iBitsPerComponent);
			fprintf(pOutFile, "\t/Decode [0 %d]\n",
					(1 << pImg->iBitsPerComponent) - 1);
		} else {
			fprintf(pOutFile, "\t/BitsPerComponent 8\n");
			fprintf(pOutFile, "\t/Decode [0 1]\n");
		}
		break;
	case imagetype_is_dib:
		fprintf(pOutFile, "\t/BitsPerComponent 8\n");
		if (pImg->iBitsPerComponent <= 8) {
			fprintf(pOutFile, "\t/Decode [0 255]\n");
		} else {
			fprintf(pOutFile, "\t/Decode [0 1 0 1 0 1]\n");
		}
		break;
	default:
		fprintf(pOutFile, "\t/BitsPerComponent 8\n");
		if (pImg->bColorImage) {
			fprintf(pOutFile, "\t/Decode [0 1 0 1 0 1]\n");
		} else {
			fprintf(pOutFile, "\t/Decode [0 1]\n");
		}
		break;
	}

	fprintf(pOutFile, "  >> image\n");
	fprintf(pOutFile, "  Data closefile\n");
	fprintf(pOutFile, "  showpage\n");
	fprintf(pOutFile, "  restore\n");
	fprintf(pOutFile, "} exec\n");
} /* end of vImagePrologue */

/*
 * vImageEpilogue - clean up after Encapsulated PostScript
 */
void
vImageEpilogue(diagram_type *pDiag)
{
	FILE	*pOutFile;

	if (!bUsePostScript) {
		return;
	}

	fail(pDiag == NULL);
	fail(pDiag->pOutFile == NULL);

	pOutFile = pDiag->pOutFile;

	fprintf(pOutFile, "%%%%EOF\n");
	fprintf(pOutFile, "%%%%EndDocument\n");
	fprintf(pOutFile, "EndEPSF\n");

	pDiag->lXleft = 0;
} /* end of vImageEpilogue */

/*
 * bAddDummyImage - add a dummy image
 *
 * return TRUE when sucessful, otherwise FALSE
 */
BOOL
bAddDummyImage(diagram_type *pDiag, const imagedata_type *pImg)
{
	FILE	*pOutFile;
	double	dHor, dVer;
	int	iHorSize, iVerSize;

	fail(pDiag == NULL);
	fail(pDiag->pOutFile == NULL);
	fail(pImg == NULL);

	if (!bUsePostScript) {
		return FALSE;
	}

	if (pImg->iVerticalSize <= 0 || pImg->iHorizontalSize <= 0) {
		return FALSE;
	}

	iImageCount++;

	/* Scaling */
	dHor = pImg->iHorizontalSize * pImg->dHorizontalScalingFactor;
	dVer = pImg->iVerticalSize * pImg->dVerticalScalingFactor;

	/* Make sure the dummy fits on the paper */
	if (dHor < dNetPageWidth) {
		iHorSize = (int)(dHor + 0.5);
		iVerSize = (int)(dVer + 0.5);
	} else {
		iHorSize = (int)(dNetPageWidth + 0.5);
		iVerSize = (int)(dVer * dNetPageWidth / dHor + 0.5);
		DBG_DEC(iVerSize);
	}

	DBG_DEC_C(pDiag->lXleft != 0, pDiag->lXleft);
	pDiag->lYtop -= lPoints2DrawUnits(iVerSize);
	vMoveToPS(pDiag, lPoints2DrawUnits(iVerSize));

	pOutFile = pDiag->pOutFile;

	fprintf(pOutFile, "gsave %% Image %03d\n", iImageCount);
	fprintf(pOutFile, "\tnewpath\n");
	fprintf(pOutFile, "\t%.2f %.2f moveto\n",
			dDrawUnits2Points(pDiag->lXleft + PS_LEFT_MARGIN),
			dDrawUnits2Points(pDiag->lYtop));
	fprintf(pOutFile, "\t1.0 setlinewidth\n");
	fprintf(pOutFile, "\t0.3 setgray\n");
	fprintf(pOutFile, "\t0 %d rlineto\n", iVerSize);
	fprintf(pOutFile, "\t%d 0 rlineto\n", iHorSize);
	fprintf(pOutFile, "\t0 %d rlineto\n", -iVerSize);
	fprintf(pOutFile, "\tclosepath\n");
	fprintf(pOutFile, "\tstroke\n");
	fprintf(pOutFile, "grestore\n");

	pDiag->lXleft = 0;

	return TRUE;
} /* end of bAddDummyImage */

/*
 * pCreateDiagram - create and initialize a diagram
 *
 * remark: does not return if the diagram can't be created
 */
diagram_type *
pCreateDiagram(const char *szTask, const char *szFilename)
{
	diagram_type	*pDiag;

	fail(szTask == NULL || szTask[0] == '\0');
	DBG_MSG("pCreateDiagram");

	/* Get the necessary memory */
	pDiag = xmalloc(sizeof(diagram_type));
	/* Initialisation */
	pDiag->pOutFile = stdout;
	vPrologue(pDiag->pOutFile, szTask, szFilename);
	iPageCount = 0;
	pDiag->lXleft = 0;
	if (bUsePostScript) {
		pDiag->lYtop = lPageHeight - PS_TOP_MARGIN;
	} else {
		pDiag->lYtop = 0;
	}
	/* Return success */
	return pDiag;
} /* end of pCreateDiagram */

/*
 * vDestroyDiagram - remove a diagram by freeing the memory it uses
 */
void
vDestroyDiagram(diagram_type *pDiag)
{
	DBG_MSG("vDestroyDiagram");

	fail(pDiag == NULL);

	if (pDiag == NULL) {
		return;
	}
	if (bUsePostScript && pDiag->lYtop < lPageHeight - PS_TOP_MARGIN) {
		fprintf(pDiag->pOutFile, "showpage\n");
	}
	vEpilogue(pDiag->pOutFile);
	pDiag = xfree(pDiag);
} /* end of vDestroyDiagram */

/*
 * vAddFonts2Diagram - add the list of fonts and complete the prologue
 */
void
vAddFonts2Diagram(diagram_type *pDiag)
{
	FILE	*pOutFile;
	const font_table_type *pTmp, *pTmp2;
	int	iLineLen;
	BOOL	bFound;

	fail(pDiag == NULL);
	fail(pDiag->pOutFile == NULL);

	if (!bUsePostScript) {
		return;
	}

	pOutFile = pDiag->pOutFile;
	iLineLen = fprintf(pOutFile, "%%%%DocumentFonts:");

	if (tGetFontTableLength() == 0) {
		iLineLen += fprintf(pOutFile, " Courier");
	} else {
		pTmp = NULL;
		while ((pTmp = pGetNextFontTableRecord(pTmp)) != NULL) {
			/* Print the document fonts */
			bFound = FALSE;
			pTmp2 = NULL;
			while ((pTmp2 = pGetNextFontTableRecord(pTmp2))
					!= NULL && pTmp2 < pTmp) {
				bFound = STREQ(pTmp2->szOurFontname,
						pTmp->szOurFontname);
				if (bFound) {
					break;
				}
			}
			if (bFound) {
				continue;
			}
			if (iLineLen + (int)strlen(pTmp->szOurFontname) > 78) {
				fprintf(pOutFile, "\n%%%%+");
				iLineLen = 3;
			}
			iLineLen += fprintf(pOutFile,
					" %s", pTmp->szOurFontname);
		}
	}
	fprintf(pOutFile, "\n");
	fprintf(pOutFile, "%%%%Pages: (atend)\n");
	fprintf(pOutFile, "%%%%EndComments\n");
	fprintf(pOutFile, "%%%%BeginProlog\n");

	switch (eEncoding) {
	case encoding_iso_8859_1:
		fprintf(pOutFile, "%s\n%s", iso_8859_1_data, iso_8859_x_func);
		break;
	case encoding_iso_8859_2:
		fprintf(pOutFile, "%s\n%s", iso_8859_2_data, iso_8859_x_func);
		break;
	default:
		DBG_DEC(eEncoding);
		break;
	}

	/* The rest of the functions */
	fprintf(pOutFile, "%s", misc_func);
	fprintf(pOutFile, "%%%%EndProlog\n");
	iPageCount = 1;
	fprintf(pDiag->pOutFile, "%%%%Page: %d %d\n", iPageCount, iPageCount);
	vAddPageSetup(pDiag->pOutFile);
} /* end of vAddFonts2Diagram */

/*
 * vPrintPS - print a PostScript string
 */
static void
vPrintPS(FILE *pFile, const char *szString, int iStringLength,
		unsigned char ucFontstyle)
{
	int iCount;
	const unsigned char *ucBytes;

	fail(szString == NULL || iStringLength < 0);

	if (szString == NULL || szString[0] == '\0' || iStringLength <= 0) {
		return;
	}

	ucBytes = (unsigned char *)szString;
	(void)putc('(', pFile);
	for (iCount = 0; iCount < iStringLength ; iCount++) {
		switch (ucBytes[iCount]) {
		case '(':
		case ')':
		case '\\':
			(void)putc('\\', pFile);
			(void)putc(szString[iCount], pFile);
			break;
		default:
			if (ucBytes[iCount] < 0x20 ||
			    (ucBytes[iCount] >= 0x7f &&
			     ucBytes[iCount] < 0xa0)) {
				DBG_HEX(ucBytes[iCount]);
				(void)putc(' ', pFile);
			} else if (ucBytes[iCount] >= 0x80) {
				fprintf(pFile, "\\%03o", ucBytes[iCount]);
			} else {
				(void)putc(szString[iCount], pFile);
			}
			break;
		}
	}
	fprintf(pFile, ") ");
	if (bIsStrike(ucFontstyle) && iFontsizeCurr > 0) {
		fprintf(pFile, "%.2f %.2f LineShow\n",
			iFontsizeCurr * 0.02, iFontsizeCurr * 0.12);
	} else if (bIsUnderline(ucFontstyle) && iFontsizeCurr > 0) {
		fprintf(pFile, "%.2f %.2f LineShow\n",
			iFontsizeCurr * 0.02, iFontsizeCurr * -0.06);
	} else {
		fprintf(pFile, "show\n");
	}
} /* end of vPrintPS */

/*
 * vSetColor - move to the given color
 */
static void
vSetColor(FILE *pFile, int iColor)
{
	unsigned int	uiTmp, uiRed, uiGreen, uiBlue;

	uiTmp = uiColor2Color(iColor);
	uiRed   = (uiTmp & 0x0000ff00) >> 8;
	uiGreen = (uiTmp & 0x00ff0000) >> 16;
	uiBlue  = (uiTmp & 0xff000000) >> 24;
	fprintf(pFile, "%.3f %.3f %.3f setrgbcolor\n",
				uiRed / 255.0, uiGreen / 255.0, uiBlue / 255.0);
} /* end of vSetColor */

/*
 * vMoveToASCII - move to the given X,Y coordinates (ASCII)
 *
 * Move the current position of the given diagram to its X,Y coordianates,
 * start on a new page if needed
 */
static void
vMoveToASCII(diagram_type *pDiag)
{
	int	iCount, iNbr;

	fail(pDiag == NULL);
	fail(pDiag->pOutFile == NULL);

	if (pDiag->lYtop != lYtopCurr) {
		iNbr = iDrawUnits2Char(pDiag->lXleft);
		for (iCount = 0; iCount < iNbr; iCount++) {
			fprintf(pDiag->pOutFile, "%c", FILLER_CHAR);
		}
		lYtopCurr = pDiag->lYtop;
	}
} /* end of vMoveToASCII */

/*
 * vMove2NextLine - move to the next line
 */
void
vMove2NextLine(diagram_type *pDiag, draw_fontref tFontRef, int iFontsize)
{
	fail(pDiag == NULL);
	fail(pDiag->pOutFile == NULL);
	fail(iFontsize < MIN_FONT_SIZE || iFontsize > MAX_FONT_SIZE);

	pDiag->lYtop -= lWord2DrawUnits20(iFontsize);
	if (!bUsePostScript) {
		(void)fprintf(pDiag->pOutFile, "\n");
	}
} /* end of vMove2NextLine */

/*
 * vSubstring2Diagram - put a substring into a diagram
 */
void
vSubstring2Diagram(diagram_type *pDiag,
	char *szString, int iStringLength, long lStringWidth,
	int iColor, unsigned char ucFontstyle, draw_fontref tFontRef,
	int iFontsize, int iMaxFontsize)
{
	const char	*szOurFontname;

	fail(pDiag == NULL || szString == NULL);
	fail(pDiag->pOutFile == NULL);
	fail(pDiag->lXleft < 0);
	fail(iStringLength != (int)strlen(szString));
	fail(iFontsize < MIN_FONT_SIZE || iFontsize > MAX_FONT_SIZE);
	fail(iMaxFontsize < MIN_FONT_SIZE || iMaxFontsize > MAX_FONT_SIZE);
	fail(iFontsize > iMaxFontsize);

	if (szString[0] == '\0' || iStringLength <= 0) {
		return;
	}

	if (bUsePostScript) {
		if (tFontRef != tFontRefCurr || iFontsize != iFontsizeCurr) {
			szOurFontname = szGetFontname(tFontRef);
			fail(szOurFontname == NULL);
			fprintf(pDiag->pOutFile,
				"%.1f /%s /%s-ISO-8859-x ChgFnt\n",
				(double)iFontsize / 2.0,
				szOurFontname, szOurFontname);
			tFontRefCurr = tFontRef;
			iFontsizeCurr = iFontsize;
		}
		if (iColor != iColorCurr) {
			vSetColor(pDiag->pOutFile, iColor);
			iColorCurr = iColor;
		}
		vMoveToPS(pDiag, lWord2DrawUnits20(iMaxFontsize));
		vPrintPS(pDiag->pOutFile, szString, iStringLength, ucFontstyle);
	} else {
		vMoveToASCII(pDiag);
		fprintf(pDiag->pOutFile, "%.*s", iStringLength, szString);
	}
	pDiag->lXleft += lStringWidth;
} /* end of vSubstring2Diagram */

/*
 * Create an end of paragraph by moving the y-high mark 1/3 line down
 */
void
vEndOfParagraph2Diagram(diagram_type *pDiag,
			draw_fontref tFontRef, int iFontsize)
{
	fail(pDiag == NULL);
	fail(pDiag->pOutFile == NULL);
	fail(iFontsize < MIN_FONT_SIZE || iFontsize > MAX_FONT_SIZE);

	if (pDiag->lXleft > 0) {
		/* To the start of the line */
		vMove2NextLine(pDiag, tFontRef, iFontsize);
	}
	/* Empty line */
	if (bUsePostScript) {
		pDiag->lXleft = 0;
		pDiag->lYtop -= lWord2DrawUnits20(iFontsize) / 3;
	} else {
		vMove2NextLine(pDiag, tFontRef, iFontsize);
	}
} /* end of vEndOfParagraph2Diagram */

/*
 * Create an end of page
 */
void
vEndOfPage2Diagram(diagram_type *pDiag,
			draw_fontref tFontRef, int iFontsize)
{
	if (bUsePostScript) {
		vMove2NextPage(pDiag);
	} else {
		vEndOfParagraph2Diagram(pDiag, tFontRef, iFontsize);
	}
} /* end of vEndOfPage2Diagram */
