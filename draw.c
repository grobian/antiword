/*
 * draw.c
 * Copyright (C) 1998-2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Functions to deal with the Draw format
 */

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "akbd.h"
#include "flex.h"
#include "wimp.h"
#include "template.h"
#include "wimpt.h"
#include "win.h"
#include "antiword.h"

/* The work area must be a litte bit larger than the diagram */
#define WORKAREA_EXTENTION	    5
/* Diagram memory */
#define INITIAL_SIZE		32768	/* 32k */
#define EXTENTION_SIZE		 4096	/*  4k */
/* Main window title */
#define WINDOW_TITLE_LEN	   28
#define FILENAME_TITLE_LEN	(WINDOW_TITLE_LEN - 10)


/*
 * vCreateMainWindow - create the MainWindow
 *
 * remark: does not return if the MainWindow can't be created
 */
static wimp_w
tCreateMainWindow(void)
{
	static int	iY = 0;
	template	*pTemplate;
	wimp_w		tMainWindow;

	/* Find and check the template */
	pTemplate = template_find("MainWindow");
	if (pTemplate == NULL) {
		werr(1, "The 'MainWindow' template can't be found");
	}
	pTemplate = template_copy(pTemplate);
	if (pTemplate == NULL) {
		werr(1, "I can't copy the 'MainWindow' template");
	}
	if ((pTemplate->window.titleflags & wimp_INDIRECT) !=
							wimp_INDIRECT) {
		werr(1,
	"The title of the 'MainWindow' template must be indirected text");
	}
	if (pTemplate->window.title.indirecttext.bufflen < WINDOW_TITLE_LEN) {
		werr(1, "The 'MainWindow' title needs %d characters",
			WINDOW_TITLE_LEN);
	}
	/*
	 * Leave 48 OS units between two windows, as recommanded by the
	 * Style guide. And try to stay away from the iconbar.
	 */
	if (pTemplate->window.box.y0 < iY + 130) {
		iY = 48;
	} else {
		pTemplate->window.box.y0 -= iY;
		pTemplate->window.box.y1 -= iY;
		iY += 48;
	}
	/* Create the window */
	wimpt_noerr(wimp_create_wind(&pTemplate->window, &tMainWindow));
	return tMainWindow;
} /* end of tCreateMainWindow */

/*
 * vCreateScaleWindow - create the Scale view Window
 *
 * remark: does not return if the Scale view Window can't be created
 */
static wimp_w
tCreateScaleWindow(void)
{
	wimp_wind	*pw;
	wimp_w		tScaleWindow;

	pw = template_syshandle("ScaleView");
	if (pw == NULL) {
		werr(1, "Template 'ScaleView' can't be found");
	}
	wimpt_noerr(wimp_create_wind(pw, &tScaleWindow));
	return tScaleWindow;
} /* end of tCreateScaleWindow */

/*
 * pCreateDiagram - create and initialize a diagram
 *
 * remark: does not return if the diagram can't be created
 */
diagram_type *
pCreateDiagram(const char *szTask, const char *szFilename)
{
	diagram_type	*pDiag;
	options_type	tOptions;
	wimp_w		tMainWindow, tScaleWindow;
	draw_box	tBox;

	DBG_MSG("pCreateDiagram");

	fail(szTask == NULL || szTask[0] == '\0');

	/* Create the mainwindow */
	tMainWindow = tCreateMainWindow();
	/* Create the scale view window */
	tScaleWindow = tCreateScaleWindow();

	/* Get the necessary memory */
	pDiag = xmalloc(sizeof(diagram_type));
	if (flex_alloc((flex_ptr)&pDiag->tInfo.data, INITIAL_SIZE) != 1) {
		werr(1, "Memory allocation failed, unable to continue");
	}
	vGetOptions(&tOptions);
	/* Initialize the diagram */
	pDiag->tMainWindow = tMainWindow;
	pDiag->tScaleWindow = tScaleWindow;
	pDiag->iScaleFactorCurr = tOptions.iScaleFactor;
	pDiag->iScaleFactorTemp = tOptions.iScaleFactor;
	pDiag->tMemorySize = INITIAL_SIZE;
	tBox.x0 = 0;
	tBox.y0 = -(draw_screenToDraw(32 + 3) * 8 + 1);
	tBox.x1 = draw_screenToDraw(16) * MIN_SCREEN_WIDTH + 1;
	tBox.y1 = 0;
	draw_create_diag(&pDiag->tInfo, (char *)szTask, tBox);
	DBG_DEC(pDiag->tInfo.length);
	pDiag->lXleft = 0;
	pDiag->lYtop = 0;
	strncpy(pDiag->szFilename,
			szBasename(szFilename), sizeof(pDiag->szFilename));
	pDiag->szFilename[sizeof(pDiag->szFilename) - 1] = '\0';
	/* Return success */
	return pDiag;
} /* end of pCreateDiagram */

/*
 * vDestroyDiagram - remove a diagram by freeing the memory it uses
 */
static void
vDestroyDiagram(wimp_w tWindow, diagram_type *pDiag)
{
	DBG_MSG("vDestroyDiagram");

	fail(pDiag != NULL && pDiag->tMainWindow != tWindow);

	wimpt_noerr(wimp_close_wind(tWindow));
	if (pDiag == NULL) {
		return;
	}
	if (pDiag->tInfo.data != NULL && pDiag->tMemorySize != 0) {
		flex_free((flex_ptr)&pDiag->tInfo.data);
	}
	pDiag = xfree(pDiag);
} /* end of vDestroyDiagram */

/*
 * vPrintDrawError - print an error reported by a draw function
 */
static void
vPrintDrawError(draw_error *pError)
{
	DBG_MSG("vPrintDrawError");

	fail(pError == NULL);

	switch (pError->type) {
	case DrawOSError:
		DBG_DEC(pError->err.os.errnum);
		DBG_MSG(pError->err.os.errmess);
		werr(1, "DrawOSError: %d: %s",
			pError->err.os.errnum, pError->err.os.errmess);
		break;
	case DrawOwnError:
		DBG_DEC(pError->err.draw.code);
		DBG_HEX(pError->err.draw.location);
		werr(1, "DrawOwnError: Code %d - Location &%x",
			pError->err.draw.code, pError->err.draw.location);
		break;
	case None:
	default:
		break;
	}
} /* end of vPrintDrawError */

/*
 * vExtendDiagramSize - make sure the diagram is big enough
 */
static void
vExtendDiagramSize(diagram_type *pDiag, size_t tSize)
{
	fail(pDiag == NULL || tSize % 4 != 0);

	while (pDiag->tInfo.length + tSize > pDiag->tMemorySize) {
		if (flex_extend((flex_ptr)&pDiag->tInfo.data,
				pDiag->tMemorySize + EXTENTION_SIZE) != 1) {
			werr(1, "Memory extend failed, unable to continue");
		}
		pDiag->tMemorySize += EXTENTION_SIZE;
		DBG_DEC(pDiag->tMemorySize);
	}
} /* end of vExtendDiagramSize */

/*
 * vAddFonts2Diagram - add a fontlist to a diagram
 */
void
vAddFonts2Diagram(diagram_type *pDiag)
{
	draw_objectType	tNew;
	draw_error	tError;
	draw_object	tHandle;
	const font_table_type	*pTmp;
	char	*pcTmp;
	size_t	tRealSize, tSize;
	int	iCount;

	fail(pDiag == NULL);

	if (tGetFontTableLength() == 0) {
		return;
	}
	tRealSize = sizeof(draw_fontliststrhdr);
	pTmp = NULL;
	while ((pTmp = pGetNextFontTableRecord(pTmp)) != NULL) {
		tRealSize += 2 + strlen(pTmp->szOurFontname);
	}
	tSize = MakeFour(tRealSize);
	vExtendDiagramSize(pDiag, tSize);
	tNew.fontList = xmalloc(tSize);
	tNew.fontList->tag = draw_OBJFONTLIST;
	tNew.fontList->size = tSize;
	pcTmp = (char *)&tNew.fontList->fontref;
	iCount = 0;
	pTmp = NULL;
	while ((pTmp = pGetNextFontTableRecord(pTmp)) != NULL) {
		*pcTmp = ++iCount;
		pcTmp++;
		strcpy(pcTmp, pTmp->szOurFontname);
		pcTmp += 1 + strlen(pTmp->szOurFontname);
	}
	for (iCount = (int)tRealSize,
	      pcTmp = (char *)tNew.fontList + tRealSize;
	     iCount < (int)tSize;
	     iCount++, pcTmp++) {
		*pcTmp = '\0';
	}
	if (draw_createObject(&pDiag->tInfo, tNew, draw_LastObject,
						TRUE, &tHandle, &tError)) {
		draw_translateText(&pDiag->tInfo);
	} else {
		DBG_MSG("draw_createObject() failed");
		vPrintDrawError(&tError);
	}
	tNew.fontList = xfree(tNew.fontList);
} /* end of vAddFonts2Diagram */

/*
 * vSubstring2Diagram - put a substring into a diagram
 */
void
vSubstring2Diagram(diagram_type *pDiag,
	char *szString, int iStringLength, long lStringWidth,
	int iColor, unsigned char ucFontstyle, draw_fontref tFontRef,
	int iFontsize, int iMaxFontsize)
{
	draw_objectType	tNew;
	draw_error	tError;
	draw_object	tHandle;
	char	*pcTmp;
	long	lSizeX, lSizeY, lOffset, l20;
	size_t	tRealSize, tSize;
	int	iCount;

	fail(pDiag == NULL || szString == NULL);
	fail(pDiag->lXleft < 0);
	fail(iStringLength != strlen(szString));
	fail(iFontsize < MIN_FONT_SIZE || iFontsize > MAX_FONT_SIZE);
	fail(iMaxFontsize < MIN_FONT_SIZE || iMaxFontsize > MAX_FONT_SIZE);
	fail(iFontsize > iMaxFontsize);

	if (szString[0] == '\0' || iStringLength <= 0) {
		return;
	}

	if (tFontRef == 0) {
		lOffset = draw_screenToDraw(2);
		l20 = draw_screenToDraw(32 + 3);
		lSizeX = draw_screenToDraw(16);
		lSizeY = draw_screenToDraw(32);
	} else {
		lOffset = lToBaseLine(iMaxFontsize);
		l20 = lWord2DrawUnits20(iMaxFontsize);
		lSizeX = lWord2DrawUnits00(iFontsize);
		lSizeY = lWord2DrawUnits00(iFontsize);
	}

	tRealSize = sizeof(draw_textstr) + iStringLength;
	tSize = MakeFour(tRealSize);
	vExtendDiagramSize(pDiag, tSize);
	tNew.text = xmalloc(tSize);
	tNew.text->tag = draw_OBJTEXT;
	tNew.text->size = tSize;
	tNew.text->bbox.x0 = (int)pDiag->lXleft;
	tNew.text->bbox.y0 = (int)pDiag->lYtop;
	tNew.text->bbox.x1 = (int)(pDiag->lXleft + lStringWidth);
	tNew.text->bbox.y1 = (int)(pDiag->lYtop + l20);
	tNew.text->textcolour = uiColor2Color(iColor);
	tNew.text->background = 0xffffff00;	/* White */
	tNew.text->textstyle.fontref = tFontRef;
	tNew.text->textstyle.reserved8 = 0;
	tNew.text->textstyle.reserved16 = 0;
	tNew.text->fsizex = (int)lSizeX;
	tNew.text->fsizey = (int)lSizeY;
	tNew.text->coord.x = (int)pDiag->lXleft;
	tNew.text->coord.y = (int)(pDiag->lYtop + lOffset);
	strncpy(tNew.text->text, szString, iStringLength);
	tNew.text->text[iStringLength] = '\0';
	for (iCount = (int)tRealSize, pcTmp = (char *)tNew.text + tRealSize;
	     iCount < (int)tSize;
	     iCount++, pcTmp++) {
		*pcTmp = '\0';
	}
	if (!draw_createObject(&pDiag->tInfo, tNew, draw_LastObject,
						TRUE, &tHandle, &tError)) {
		DBG_MSG("draw_createObject() failed");
		vPrintDrawError(&tError);
	}
	tNew.text = xfree(tNew.text);
	draw_translateText(&pDiag->tInfo);
	pDiag->lXleft += lStringWidth;
} /* end of vSubstring2Diagram */

/*
 * vImage2Diagram - put an image into a diagram
 */
void
vImage2Diagram(diagram_type *pDiag, long lWidth, long lHeight,
	unsigned char *pucSprite, size_t tSpriteSize)
{
  	draw_objectType	tNew;
	draw_error	tError;
	draw_object	tHandle;
	char	*pcTmp;
	size_t	tRealSize, tSize;
	int	iCount;

	DBG_MSG("vImage2Diagram");

	fail(pDiag == NULL);
	fail(pDiag->lXleft < 0);

	DBG_DEC_C(pDiag->lXleft != 0, pDiag->lXleft);
	pDiag->lYtop -= lHeight;

	tRealSize = sizeof(draw_spristrhdr) + tSpriteSize;
	tSize = MakeFour(tRealSize);
	vExtendDiagramSize(pDiag, tSize);
	tNew.sprite = xmalloc(tSize);
	tNew.sprite->tag = draw_OBJSPRITE;
	tNew.sprite->size = tSize;
	tNew.sprite->bbox.x0 = (int)pDiag->lXleft;
	tNew.sprite->bbox.y0 = (int)pDiag->lYtop;
	tNew.sprite->bbox.x1 = (int)(pDiag->lXleft + lWidth);
	tNew.sprite->bbox.y1 = (int)(pDiag->lYtop + lHeight);
	memcpy(&tNew.sprite->sprite, pucSprite, tSpriteSize);
	for (iCount = (int)tRealSize, pcTmp = (char *)tNew.sprite + tRealSize;
	     iCount < (int)tSize;
	     iCount++, pcTmp++) {
		*pcTmp = '\0';
	}
	if (!draw_createObject(&pDiag->tInfo, tNew, draw_LastObject,
						TRUE, &tHandle, &tError)) {
		DBG_MSG("draw_createObject() failed");
		vPrintDrawError(&tError);
	}
	tNew.sprite = xfree(tNew.sprite);
	pDiag->lXleft = 0;
} /* end of vImage2Diagram */

/*
 * bAddDummyImage - add a dummy image
 *
 * return TRUE when sucessful, otherwise FALSE
 */
BOOL
bAddDummyImage(diagram_type *pDiag, const imagedata_type *pImg)
{
  	draw_objectType	tNew;
	draw_error	tError;
	draw_object	tHandle;
	int	*piTmp;
	char	*pcTmp;
	size_t	tRealSize, tSize;
	int	iCount;

	DBG_MSG("bAddDummyImage");

	fail(pDiag == NULL);
	fail(pImg == NULL);
	fail(pDiag->lXleft < 0);

	if (pImg->iVerticalSize <= 0 || pImg->iHorizontalSize <= 0) {
		return FALSE;
	}

	DBG_DEC_C(pDiag->lXleft != 0, pDiag->lXleft);
	pDiag->lYtop -= lPoints2DrawUnits(pImg->iVerticalSize);

	tRealSize = sizeof(draw_pathstrhdr) + 14 * sizeof(int);
	tSize = MakeFour(tRealSize);
	vExtendDiagramSize(pDiag, tSize);
	tNew.path = xmalloc(tSize);
	tNew.path->tag = draw_OBJPATH;
	tNew.path->size = tSize;
	tNew.path->bbox.x0 = (int)pDiag->lXleft;
	tNew.path->bbox.y0 = (int)pDiag->lYtop;
	tNew.path->bbox.x1 =
		(int)(pDiag->lXleft +
		lPoints2DrawUnits(pImg->iHorizontalSize));
	tNew.path->bbox.y1 =
		(int)(pDiag->lYtop +
		lPoints2DrawUnits(pImg->iVerticalSize));
	tNew.path->fillcolour = -1;
	tNew.path->pathcolour = 0x4d4d4d00;	/* Gray 70 % */
	tNew.path->pathwidth = (int)lMilliPoints2DrawUnits(500);
	tNew.path->pathstyle.joincapwind = 0;
	tNew.path->pathstyle.reserved8 = 0;
	tNew.path->pathstyle.tricapwid = 0;
	tNew.path->pathstyle.tricaphei = 0;
	piTmp = (int *)((char *)tNew.path + sizeof(draw_pathstrhdr));
	*piTmp++ = draw_PathMOVE;
	*piTmp++ = tNew.path->bbox.x0;
	*piTmp++ = tNew.path->bbox.y0;
	*piTmp++ = draw_PathLINE;
	*piTmp++ = tNew.path->bbox.x0;
	*piTmp++ = tNew.path->bbox.y1;
	*piTmp++ = draw_PathLINE;
	*piTmp++ = tNew.path->bbox.x1;
	*piTmp++ = tNew.path->bbox.y1;
	*piTmp++ = draw_PathLINE;
	*piTmp++ = tNew.path->bbox.x1;
	*piTmp++ = tNew.path->bbox.y0;
	*piTmp++ = draw_PathCLOSE;
	*piTmp++ = draw_PathTERM;
	for (iCount = (int)tRealSize, pcTmp = (char *)tNew.path + tRealSize;
	     iCount < (int)tSize;
	     iCount++, pcTmp++) {
		*pcTmp = '\0';
	}
	if (!draw_createObject(&pDiag->tInfo, tNew, draw_LastObject,
						TRUE, &tHandle, &tError)) {
		DBG_MSG("draw_createObject() failed");
		vPrintDrawError(&tError);
	}
	tNew.path = xfree(tNew.path);
	pDiag->lXleft = 0;
	return TRUE;
} /* end of bAddDummyImage */

/*
 * vMove2NextLine - move to the next line
 */
void
vMove2NextLine(diagram_type *pDiag, draw_fontref tFontRef, int iFontsize)
{
	long	l20;

	fail(pDiag == NULL);
	fail(iFontsize < MIN_FONT_SIZE || iFontsize > MAX_FONT_SIZE);

	if (tFontRef == 0) {
		l20 = draw_screenToDraw(32 + 3);
	} else {
		l20 = lWord2DrawUnits20(iFontsize);
	}
	pDiag->lYtop -= l20;
} /* end of vMove2NextLine */

/*
 * Create an end of paragraph by moving the y-high mark 1/3 line down
 */
void
vEndOfParagraph2Diagram(diagram_type *pDiag,
			draw_fontref tFontRef, int iFontsize)
{
	long	l20;

	fail(pDiag == NULL);
	fail(iFontsize < MIN_FONT_SIZE || iFontsize > MAX_FONT_SIZE);

	if (tFontRef == 0) {
		l20 = draw_screenToDraw(32 + 3);
	} else {
		l20 = lWord2DrawUnits20(iFontsize);
	}
	pDiag->lYtop -= l20 / 3;	/* Line spacing */
	pDiag->lXleft = 0;
} /* end of vEndOfParagraph2Diagram */

/*
 * Create an end of page
 */
void
vEndOfPage2Diagram(diagram_type *pDiag,
			draw_fontref tFontRef, int iFontsize)
{
	vEndOfParagraph2Diagram(pDiag, tFontRef, iFontsize);
} /* end of vEndOfPage2Diagram */

/*
 * bVerifyDiagram - Verify the diagram generated from the Word file
 *
 * returns TRUE if the diagram is correct
 */
BOOL
bVerifyDiagram(diagram_type *pDiag)
{
	draw_error	tError;

	fail(pDiag == NULL);
	DBG_MSG("bVerifyDiagram");

	if (draw_verify_diag(&pDiag->tInfo, &tError)) {
		return TRUE;
	}
	DBG_MSG("draw_verify_diag() failed");
	vPrintDrawError(&tError);
	return FALSE;
} /* end of bVerifyDiagram */

void
vShowDiagram(diagram_type *pDiag)
{
	wimp_wstate	tWindowState;
	wimp_redrawstr	tRedraw;

	fail(pDiag == NULL);

	DBG_MSG("vShowDiagram");

	wimpt_noerr(wimp_get_wind_state(pDiag->tMainWindow, &tWindowState));
	tWindowState.o.behind = -1;
	wimpt_noerr(wimp_open_wind(&tWindowState.o));

	draw_queryBox(&pDiag->tInfo, (draw_box *)&tRedraw.box, TRUE);
	tRedraw.w = pDiag->tMainWindow;
	/* Work area extention */
	tRedraw.box.x0 -= WORKAREA_EXTENTION;
	tRedraw.box.y0 -= WORKAREA_EXTENTION;
	tRedraw.box.x1 += WORKAREA_EXTENTION;
	tRedraw.box.y1 += WORKAREA_EXTENTION;
	wimpt_noerr(wimp_set_extent(&tRedraw));
} /* end of vShowDiagram */

/*
 * vMainButtonClick - handle mouse buttons clicks for the main screen
 */
static void
vMainButtonClick(wimp_mousestr *m)
{
	wimp_caretstr	c;
	wimp_wstate	ws;

	fail(m == NULL);

	DBG_HEX(m->bbits);
	DBG_DEC(m->i);

	if (m->w >= 0 &&
	    m->i == -1 &&
	    ((m->bbits & wimp_BRIGHT) == wimp_BRIGHT ||
	     (m->bbits & wimp_BLEFT) == wimp_BLEFT)) {
		/* Get the input focus */
		wimpt_noerr(wimp_get_wind_state(m->w, &ws));
		c.w = m->w;
		c.i = -1;
		c.x = m->x - ws.o.box.x0;
		c.y = m->y - ws.o.box.y1;
		c.height = BIT(25);
		c.index = 0;
		wimpt_noerr(wimp_set_caret_pos(&c));
	}
} /* end of vMainButtonClick */

/*
 * vMainKeyPressed - handle pressed keys for the main screen
 */
static void
vMainKeyPressed(int chcode, wimp_caretstr *c, diagram_type *pDiag)
{
	fail(c == NULL || pDiag == NULL);
	fail(c->w != pDiag->tMainWindow);

	switch (chcode) {
	case akbd_Ctl+akbd_Fn+2:	/* Ctrl F2 */
		vDestroyDiagram(c->w, pDiag);
		break;
	case akbd_Fn+3:			/* F3 */
		vSaveDrawfile(pDiag);
		break;
	case akbd_Sh+akbd_Fn+3:		/* Shift F3 */
		vSaveTextfile(pDiag);
		break;
	default:
		DBG_DEC(chcode);
		wimpt_noerr(wimp_processkey(chcode));
	}
} /* end of vMainKeyPressed */

/*
 * vRedrawMainWindow - redraw the main window
 */
static void
vRedrawMainWindow(wimp_w tWindow, diagram_type *pDiag)
{
	wimp_redrawstr	r;
	draw_error	tError;
	double		dScaleFactor;
	draw_diag	*pInfo;
	BOOL		bMore;

	fail(pDiag == NULL);
	fail(pDiag->tMainWindow != tWindow);
	fail(pDiag->iScaleFactorCurr < MIN_SCALE_FACTOR);
	fail(pDiag->iScaleFactorCurr > MAX_SCALE_FACTOR);

	dScaleFactor = (double)pDiag->iScaleFactorCurr / 100.0;
	pInfo = &pDiag->tInfo;

	r.w = tWindow;
	wimpt_noerr(wimp_redraw_wind(&r, &bMore));

	while (bMore) {
		if (pInfo->data != NULL) {
			if (!draw_render_diag(pInfo,
					(draw_redrawstr *)&r,
					dScaleFactor,
					&tError)) {
				DBG_MSG("draw_render_diag() failed");
				DBG_DEC(r.box.x0 - r.scx);
				DBG_DEC(r.box.y1 - r.scy);
				vPrintDrawError(&tError);
			}
		}
		wimp_get_rectangle(&r, &bMore);
	}
} /* end of vRedrawMainWindow */

/*
 * vMainEventHandler - event handler for the main screen
 */
void
vMainEventHandler(wimp_eventstr *pEvent, void *pvHandle)
{
	diagram_type	*pDiag;

	fail(pEvent == NULL);

	pDiag = (diagram_type *)pvHandle;

	switch (pEvent->e) {
	case wimp_ENULL:
		break;
	case wimp_EREDRAW:
		vRedrawMainWindow(pEvent->data.o.w, pDiag);
		break;
	case wimp_EOPEN:
		wimpt_noerr(wimp_open_wind(&pEvent->data.o));
		break;
	case wimp_ECLOSE:
		vDestroyDiagram(pEvent->data.o.w, pDiag);
		break;
	case wimp_EBUT:
		vMainButtonClick(&pEvent->data.but.m);
		break;
	case wimp_EKEY:
		vMainKeyPressed(pEvent->data.key.chcode,
				&pEvent->data.key.c, pDiag);
		break;
	default:
		break;
	}
} /* end of vMainEventHandler */

/*
 * vScaleOpenAction - action to be taken when the Scale view window opens
 */
void
vScaleOpenAction(diagram_type *pDiag)
{
	wimp_wstate	tWindowState;

	fail(pDiag == NULL);

	wimpt_noerr(wimp_get_wind_state(pDiag->tScaleWindow, &tWindowState));
	if ((tWindowState.flags & wimp_WOPEN) == wimp_WOPEN) {
		/* The window is already open */
		return;
	}

	DBG_MSG("vScaleOpenAction");

	pDiag->iScaleFactorTemp = pDiag->iScaleFactorCurr;
	vUpdateWriteableNumber(pDiag->tScaleWindow,
			SCALE_SCALE_WRITEABLE, pDiag->iScaleFactorTemp);
	tWindowState.o.behind = -1;
	wimpt_noerr(wimp_open_wind(&tWindowState.o));
} /* end of vScaleOpenAction */

/*
 * vSetTitle - set the titel of a window
 */
void
vSetTitle(diagram_type *pDiag)
{
	char	szTitle[WINDOW_TITLE_LEN];

	fail(pDiag == NULL);
	fail(pDiag->szFilename[0] == '\0');

	(void)sprintf(szTitle, "%.*s at %d%%",
				FILENAME_TITLE_LEN,
				pDiag->szFilename,
				pDiag->iScaleFactorCurr % 1000);
	if (strlen(pDiag->szFilename) > FILENAME_TITLE_LEN) {
		szTitle[FILENAME_TITLE_LEN - 1] = OUR_ELLIPSIS;
	}

	win_settitle(pDiag->tMainWindow, szTitle);
} /* end of vSetTitle */

/*
 * vForceRedraw - force a redraw of the main window
 */
static void
vForceRedraw(diagram_type *pDiag)
{
	wimp_wstate	tWindowState;
	wimp_redrawstr	tRedraw;

	DBG_MSG("vForceRedraw");

	fail(pDiag == NULL);

	DBG_DEC(pDiag->iScaleFactorCurr);

	/* Read the size of the current diagram */
	draw_queryBox(&pDiag->tInfo, (draw_box *)&tRedraw.box, TRUE);
	tRedraw.w = pDiag->tMainWindow;
	/* Adjust the size of the work area */
	tRedraw.box.x0 = tRedraw.box.x0 * pDiag->iScaleFactorCurr / 100 - 1;
	tRedraw.box.y0 = tRedraw.box.y0 * pDiag->iScaleFactorCurr / 100 - 1;
	tRedraw.box.x1 = tRedraw.box.x1 * pDiag->iScaleFactorCurr / 100 + 1;
	tRedraw.box.y1 = tRedraw.box.y1 * pDiag->iScaleFactorCurr / 100 + 1;
	/* Work area extention */
	tRedraw.box.x0 -= WORKAREA_EXTENTION;
	tRedraw.box.y0 -= WORKAREA_EXTENTION;
	tRedraw.box.x1 += WORKAREA_EXTENTION;
	tRedraw.box.y1 += WORKAREA_EXTENTION;
	wimpt_noerr(wimp_set_extent(&tRedraw));
	/* Widen the box slightly to be sure all the edges are drawn */
	tRedraw.box.x0 -= 5;
	tRedraw.box.y0 -= 5;
	tRedraw.box.x1 += 5;
	tRedraw.box.y1 += 5;
	/* Force the redraw */
	wimpt_noerr(wimp_force_redraw(&tRedraw));
	/* Reopen the window to show the correct size */
	wimpt_noerr(wimp_get_wind_state(pDiag->tMainWindow, &tWindowState));
	tWindowState.o.behind = -1;
	wimpt_noerr(wimp_open_wind(&tWindowState.o));
} /* end of vForceRedraw */

/*
 * vScaleButtonClick - handle a mouse button click in the Scale view window
 */
static void
vScaleButtonClick(wimp_mousestr *m, diagram_type *pDiag)
{
	BOOL	bCloseWindow, bRedraw;

	fail(m == NULL || pDiag == NULL);
	fail(m->w != pDiag->tScaleWindow);

	bCloseWindow = FALSE;
	bRedraw = FALSE;
	switch (m->i) {
	case SCALE_CANCEL_BUTTON:
		bCloseWindow = TRUE;
		pDiag->iScaleFactorTemp = pDiag->iScaleFactorCurr;
		break;
	case SCALE_SCALE_BUTTON:
		bCloseWindow = TRUE;
		bRedraw = pDiag->iScaleFactorCurr != pDiag->iScaleFactorTemp;
		pDiag->iScaleFactorCurr = pDiag->iScaleFactorTemp;
		break;
	case SCALE_50_PCT:
		pDiag->iScaleFactorTemp = 50;
		break;
	case SCALE_75_PCT:
		pDiag->iScaleFactorTemp = 75;
		break;
	case SCALE_100_PCT:
		pDiag->iScaleFactorTemp = 100;
		break;
	case SCALE_150_PCT:
		pDiag->iScaleFactorTemp = 150;
		break;
	default:
		DBG_DEC(m->i);
		break;
	}
	if (bCloseWindow) {
		/* Close the scale window */
		wimpt_noerr(wimp_close_wind(m->w));
		if (bRedraw) {
			/* Redraw the main window */
			vSetTitle(pDiag);
			vForceRedraw(pDiag);
		}
	} else {
		vUpdateWriteableNumber(m->w,
				SCALE_SCALE_WRITEABLE,
				pDiag->iScaleFactorTemp);
	}
} /* end of vScaleButtonClick */

static void
vScaleKeyPressed(int chcode, wimp_caretstr *c, diagram_type *pDiag)
{
	wimp_icon	tIcon;
	char		*pcChar;
	int		iTmp;

	DBG_MSG("vScaleKeyPressed");

	fail(c == NULL || pDiag == NULL);
	fail(c->w != pDiag->tScaleWindow);

	DBG_DEC_C(c->i != SCALE_SCALE_WRITEABLE, c->i);
	DBG_DEC_C(c->i == SCALE_SCALE_WRITEABLE, chcode);

	if (chcode != '\r' ||
	    c->w != pDiag->tScaleWindow ||
	    c->i != SCALE_SCALE_WRITEABLE) {
		wimpt_noerr(wimp_processkey(chcode));
		return;
	}

	wimpt_noerr(wimp_get_icon_info(c->w, c->i, &tIcon));
	if ((tIcon.flags & (wimp_ITEXT|wimp_INDIRECT)) !=
	    (wimp_ITEXT|wimp_INDIRECT)) {
		werr(1, "Icon %d must be indirected text", (int)c->i);
		return;
	}
	iTmp = (int)strtol(tIcon.data.indirecttext.buffer, &pcChar, 10);
	if (*pcChar != '\0' && *pcChar != '\r') {
		DBG_DEC(*pcChar);
	} else if (iTmp < MIN_SCALE_FACTOR) {
		pDiag->iScaleFactorTemp = MIN_SCALE_FACTOR;
	} else if (iTmp > MAX_SCALE_FACTOR) {
		pDiag->iScaleFactorTemp = MAX_SCALE_FACTOR;
	} else {
		pDiag->iScaleFactorTemp = iTmp;
	}
	pDiag->iScaleFactorCurr = pDiag->iScaleFactorTemp;
	/* Close the scale window */
	wimpt_noerr(wimp_close_wind(c->w));
	/* Redraw the main window */
	vSetTitle(pDiag);
	vForceRedraw(pDiag);
} /* end of vScaleKeyPressed */

/*
 * vScaleEventHandler - event handler for the scale view screen
 */
void
vScaleEventHandler(wimp_eventstr *pEvent, void *pvHandle)
{
	diagram_type	*pDiag;

	fail(pEvent == NULL);

	pDiag = (diagram_type *)pvHandle;

	switch (pEvent->e) {
	case wimp_ENULL:
		break;
	case wimp_EREDRAW:
		/* handled by the WIMP */
		break;
	case wimp_EOPEN:
		wimpt_noerr(wimp_open_wind(&pEvent->data.o));
		break;
	case wimp_ECLOSE:
		wimpt_noerr(wimp_close_wind(pEvent->data.o.w));
		break;
	case wimp_EBUT:
		vScaleButtonClick(&pEvent->data.but.m, pDiag);
		break;
	case wimp_EKEY:
		vScaleKeyPressed(pEvent->data.key.chcode,
				&pEvent->data.key.c, pDiag);
		break;
	default:
		break;
	}
} /* end of vScaleEventHandler */
