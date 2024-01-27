/*
 * antiword.h
 * Copyright (C) 1998-2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Generic include file for project 'Antiword'
 */

#if !defined(__antiword_h)
#define __antiword_h 1

#if defined(DEBUG) == defined(NDEBUG)
#error Exactly one of the DEBUG and NDEBUG flags MUST be set
#endif /* DEBUG == NDEBUG */

#include <stdio.h>
#include <limits.h>
#if defined(__riscos)
#include "wimp.h"
#include "drawfobj.h"
#include "font.h"
#include "werr.h"
#else
#include <sys/types.h>
#endif /* __riscos */
#include "wordconst.h"
#include "wordtypes.h"
#include "fail.h"
#include "debug.h"

/* Constants */
#if !defined(PATH_MAX)
 #if defined(__riscos)
 #define PATH_MAX		 255
 #else
  #if defined(MAXPATHLEN)
  #define PATH_MAX		MAXPATHLEN
  #else
  #define PATH_MAX		1024
  #endif /* MAXPATHLEN */
 #endif /* __riscos */
#endif /* !PATH_MAX */

#define SIZE_T_MAX		(size_t)UINT_MAX

#if defined(__riscos)
#define FILE_SEPARATOR		"."
#elif defined(__dos)
#define FILE_SEPARATOR		"\\"
#else
#define FILE_SEPARATOR		"/"
#endif /* __riscos */

/* PNG chunk names */
#define PNG_CN_IDAT		0x49444154
#define PNG_CN_IEND		0x49454e44
#define PNG_CN_IHDR		0x49484452
#define PNG_CN_PLTE		0x504c5445

/* The screen width */
#define MIN_SCREEN_WIDTH	 45
#define DEFAULT_SCREEN_WIDTH	 76
#define MAX_SCREEN_WIDTH	145

/* The scale factors as percentages */
#define MIN_SCALE_FACTOR	 25
#define DEFAULT_SCALE_FACTOR	100
#define MAX_SCALE_FACTOR	400

/* Filetypes */
#define FILETYPE_MSWORD		0xae6
#define FILETYPE_IMPDOC		0xbc5
#define FILETYPE_DRAW		0xaff
#define FILETYPE_POSCRIPT	0xff5
#define FILETYPE_SPRITE		0xff9
#define FILETYPE_TEXT		0xfff

/* The button numbers in the choices window */
#define CHOICES_DEFAULT_BUTTON		 3
#define CHOICES_SAVE_BUTTON		 2
#define CHOICES_CANCEL_BUTTON		 1
#define CHOICES_APPLY_BUTTON		 0
#define CHOICES_BREAK_BUTTON		 6
#define CHOICES_BREAK_WRITEABLE		 7
#define CHOICES_BREAK_UP_BUTTON		 8
#define CHOICES_BREAK_DOWN_BUTTON	 9
#define CHOICES_NO_BREAK_BUTTON		11
#define CHOICES_AUTOFILETYPE_BUTTON	14
#define CHOICES_HIDDEN_TEXT_BUTTON	22
#define CHOICES_WITH_IMAGES_BUTTON	17
#define CHOICES_NO_IMAGES_BUTTON	18
#define CHOICES_TEXTONLY_BUTTON		19
#define CHOICES_SCALE_WRITEABLE		25
#define CHOICES_SCALE_UP_BUTTON		26
#define CHOICES_SCALE_DOWN_BUTTON	27

/* The button numbers in the scale view window */
#define SCALE_CANCEL_BUTTON		 1
#define SCALE_SCALE_BUTTON		 0
#define SCALE_SCALE_WRITEABLE		 3
#define SCALE_50_PCT			 5
#define SCALE_75_PCT			 6
#define SCALE_100_PCT			 7
#define SCALE_150_PCT			 8

/* Margin for the PostScript version */
#define	PS_LEFT_MARGIN			(72 * 640L)
#define PS_RIGHT_MARGIN			(43 * 640L)
#define PS_TOP_MARGIN			(72 * 640L)
#define PS_BOTTOM_MARGIN		(72 * 640L)

/* Macros */
#define STREQ(x,y)	(*(x) == *(y) && strcmp(x,y) == 0)
#define STRNEQ(x,y,n)	(*(x) == *(y) && strncmp(x,y,n) == 0)
#define elementsof(a)	(sizeof(a) / sizeof(a[0]))
#define odd(x)		((x)&0x01)
#define BIT(x)		((unsigned int)1 << (x))
#define MakeFour(x)	(((x)+3)&~0x03)
#if !defined(max)
#define max(x,y)	((x)>(y)?(x):(y))
#endif /* !max */
#if !defined(min)
#define min(x,y)	((x)<(y)?(x):(y))
#endif /* !min */

#if defined(__riscos)
/* The name of the table font */
#define TABLE_FONT		"Corpus.Medium"
#else
/* The name of the table font */
#define TABLE_FONT		"Courier"
/* The name of the antiword directory */
#if defined(__dos)
#define ANTIWORD_DIR		"antiword"
#define FONT_BASENAME		"fontname.txt"
#else
#define ANTIWORD_DIR		".antiword"
#define FONT_BASENAME		"fontnames"
#endif /* __dos */
/* The name of the font information file */
#define FONTNAMES_FILE		ANTIWORD_DIR FILE_SEPARATOR FONT_BASENAME
/* The name of the default mapping file */
#define MAPPING_FILE_DEFAULT_1	ANTIWORD_DIR FILE_SEPARATOR "8859-1.txt"
#define MAPPING_FILE_DEFAULT_2	ANTIWORD_DIR FILE_SEPARATOR "8859-2.txt"
#endif /* __riscos */

/* Prototypes */

/* asc85enc.c */
extern void	vASCII85EncodeByte(FILE *, int);
extern void	vASCII85EncodeArray(FILE *, FILE *, int);
extern void	vASCII85EncodeFile(FILE *, FILE *, int);
/* blocklist.c */
extern void	vDestroyTextBlockList(void);
extern BOOL	bAdd2TextBlockList(text_block_type *);
extern void	vSplitBlockList(size_t, size_t, size_t, size_t, size_t, BOOL);
extern unsigned short	usNextChar(FILE *, list_id_enum, long *);
extern long	lTextOffset2FileOffset(long);
extern long	lGetSeqNumber(long);
#if defined(__riscos)
extern size_t	tGetDocumentLength(void);
#endif /* __riscos */
/* chartrans.c */
extern BOOL	bReadCharacterMappingTable(const char *);
extern int	iTranslateCharacters(unsigned short, long, BOOL);
extern int	iToUpper(int);
/* datalist.c */
extern void	vDestroyDataBlockList(void);
extern BOOL	bAdd2DataBlockList(data_block_type *);
extern BOOL	bSetDataOffset(FILE *, long);
extern int	iNextByte(FILE *);
extern unsigned short	usNextWord(FILE *);
extern unsigned long	ulNextLong(FILE *);
extern unsigned short	usNextWordBE(FILE *);
extern unsigned long	ulNextLongBE(FILE *);
extern int	iSkipBytes(FILE *, size_t);
extern long	lDataOffset2FileOffset(long);
/* depot.c */
extern void	vDestroySmallBlockList(void);
extern BOOL	bCreateSmallBlockList(int, const int *, size_t);
extern long	lDepotOffset(int, size_t);
/* dib2eps & dib2sprt.c */
extern BOOL	bTranslateDIB(diagram_type *,
			FILE *, long, const imagedata_type *);
/* draw.c & postscript.c */
extern BOOL	bAddDummyImage(diagram_type *, const imagedata_type *);
extern diagram_type *pCreateDiagram(const char *, const char *);
extern void	vAddFonts2Diagram(diagram_type *);
extern void	vSubstring2Diagram(diagram_type *,
			char *, int, long, int, unsigned char,
			draw_fontref, int, int);
extern void	vMove2NextLine(diagram_type *, draw_fontref, int);
extern void	vEndOfParagraph2Diagram(diagram_type *, draw_fontref, int);
extern void	vEndOfPage2Diagram(diagram_type *, draw_fontref, int);
#if defined(__riscos)
extern void	vImage2Diagram(diagram_type *,
			long, long, unsigned char *, size_t);
extern BOOL	bVerifyDiagram(diagram_type *);
extern void	vShowDiagram(diagram_type *);
extern void	vMainEventHandler(wimp_eventstr *, void *);
extern void	vScaleOpenAction(diagram_type *);
extern void	vSetTitle(diagram_type *);
extern void	vScaleEventHandler(wimp_eventstr *, void *);
#else
extern void	vImagePrologue(diagram_type *, const imagedata_type *);
extern void	vImageEpilogue(diagram_type *);
extern void	vDestroyDiagram(diagram_type *);
#endif /* __riscos */
/* finddata.c */
extern BOOL	bAddDataBlocks(long , size_t, int, const int *, size_t);
extern BOOL	bGet6DocumentData(FILE *, int,
				const int *, size_t, const unsigned char *);
/* findtext.c */
extern BOOL	bAddTextBlocks(long , size_t, BOOL, int, const int *, size_t);
extern text_info_enum	eGet6DocumentText(FILE *, BOOL, int,
				const int *, size_t, const unsigned char *);
extern text_info_enum	eGet8DocumentText(FILE *, const pps_info_type *,
				const int *, size_t, const int *, size_t,
				const unsigned char *);
/* fontlist.c */
extern void	vDestroyFontInfoList(void);
extern void	vAdd2FontInfoList(const font_block_type *);
extern void	vReset2FontInfoList(long);
extern const font_block_type	*pGetNextFontInfoListItem(
					const font_block_type *);
/* fonts.c */
extern int	iGetFontByNumber(int, unsigned char);
extern const char	*szGetOurFontname(int);
extern int	iFontname2Fontnumber(const char *, unsigned char);
extern void	vCreate6FontTable(FILE *, int,
			const int *, size_t, const unsigned char *);
extern void	vCreate8FontTable(FILE *, const pps_info_type *,
			const int *, size_t, const int *, size_t,
			const unsigned char *);
extern void	vDestroyFontTable(void);
extern const font_table_type	*pGetNextFontTableRecord(
						const font_table_type *);
extern size_t	tGetFontTableLength(void);
/* fonts_r.c & fonts_u.c */
extern FILE	*pOpenFontTableFile(void);
extern void	vCloseFont(void);
extern draw_fontref	tOpenFont(int, unsigned char, int);
extern draw_fontref	tOpenTableFont(int);
extern long	lComputeStringWidth(char *, int, draw_fontref, int);
/* fonts_u.c */
#if !defined(__riscos)
extern const char	*szGetFontname(draw_fontref);
#endif /* !__riscos */
#if defined(__riscos)
/* icons.c */
extern void	vUpdateIcon(wimp_w, wimp_icon *);
extern void	vUpdateRadioButton(wimp_w, wimp_i, BOOL);
extern void	vUpdateWriteable(wimp_w, wimp_i, char *);
extern void	vUpdateWriteableNumber(wimp_w, wimp_i, int);
#endif /* __riscos */
/* imgexam.c */
extern image_info_enum	eExamineImage(FILE *, long, imagedata_type *);
/* imgtrans */
extern BOOL	bTranslateImage(diagram_type *,
			FILE *, BOOL, long, const imagedata_type *);
/* jpeg2eps.c & jpeg2spr.c */
extern BOOL	bTranslateJPEG(diagram_type *,
			FILE *, long, int, const imagedata_type *);
/* misc.c */
#if defined(__riscos)
extern int	iGetFiletype(const char *);
extern void	vSetFiletype(const char *, int);
extern BOOL	bISO_8859_1_IsCurrent(void);
#else
extern const char	*szGetHomeDirectory(void);
#endif /* __riscos */
extern BOOL	bMakeDirectory(const char *);
extern long	lGetFilesize(const char *);
#if defined(DEBUG)
extern void	vPrintBlock(const char *, int, const unsigned char *, size_t);
extern void	vPrintUnicode(const char *, int, const char *);
extern BOOL	bCheckDoubleLinkedList(output_type *);
#endif /* DEBUG */
extern BOOL	bReadBytes(unsigned char *, size_t, long, FILE *);
extern BOOL	bReadBuffer(FILE *, int, const int *, size_t, size_t,
			unsigned char *, size_t, size_t);
extern unsigned int	uiColor2Color(int);
extern output_type *pSplitList(output_type *);
extern int	iInteger2Roman(int, BOOL, char *);
extern int	iInteger2Alpha(int, BOOL, char *);
extern char	*unincpy(char *, const char *, size_t);
extern size_t	unilen(const char *);
extern const char	*szBasename(const char *);
/* notes.c */
extern void	vDestroyNotesInfoLists(void);
extern void	vGetNotesInfo(FILE *, const pps_info_type *,
			const int *, size_t, const int *, size_t,
			const unsigned char *, int);
extern notetype_enum eGetNotetype(long);
/* options.c */
extern int	iReadOptions(int, char **);
extern void	vGetOptions(options_type *);
#if defined(__riscos)
extern void	vChoicesOpenAction(wimp_w);
extern void	vChoicesMouseClick(wimp_mousestr *);
extern void	vChoicesKeyPressed(wimp_caretstr *);
#endif /* __riscos */
/* out2window.c */
extern void	vSetLeftIndentation(diagram_type *, long);
extern void	vAlign2Window(diagram_type *, output_type *,
			long, unsigned char);
extern void	vJustify2Window(diagram_type *, output_type *,
			long, long, unsigned char);
extern void	vResetStyles(void);
extern int	iStyle2Window(char *, const style_block_type *);
extern void	vTableRow2Window(diagram_type *,
			output_type *, const row_block_type *);
/* paragraph.c */
extern void	vGetParagraphInfo(FILE *, const pps_info_type *,
			const int *, size_t, const int *, size_t,
			const unsigned char *, int);
/* pictlist.c */
extern void	vDestroyPicInfoList(void);
extern void	vAdd2PicInfoList(const picture_block_type *);
extern long	lGetPicInfoListItem(long);
/* png2eps.c & png2spr.c */
extern BOOL	bTranslatePNG(diagram_type *,
			FILE *, long, int, const imagedata_type *);
/* rowlist.c */
extern void	vDestroyRowInfoList(void);
extern void	vAdd2RowInfoList(const row_block_type *);
extern BOOL	bGetNextRowInfoListItem(row_block_type *);
/* stylelist.c */
extern void	vDestroyStyleInfoList(void);
extern void	vAdd2StyleInfoList(const style_block_type *);
extern const style_block_type	*pGetNextStyleInfoListItem(void);
/* tabstop.c */
extern void	vSetDefaultTabWidth(FILE *, const pps_info_type *,
			const int *, size_t, const int *, size_t,
			const unsigned char *, int);
extern long	lGetDefaultTabWidth(void);
/* unix.c */
#if !defined(__riscos)
extern void	werr(int, const char *, ...);
extern void	visdelay_begin(void);
extern void	visdelay_end(void);
#endif /* !__riscos */
/* saveas.c */
#if defined(__riscos)
extern void	vSaveTextfile(diagram_type *);
extern void	vSaveDrawfile(diagram_type *);
#endif /* __riscos */
/* word2text.c */
extern void	vWord2Text(diagram_type *, const char *);
/* wordlib.c */
extern FILE	*pOpenDocument(const char *);
extern void	vCloseDocument(FILE *);
extern BOOL	bIsSupportedWordFile(const char *);
extern BOOL	bIsWord245File(const char *);
extern BOOL	bIsRtfFile(const char *);
extern BOOL	bFileFromTheMac(void);
/* xmalloc.c */
extern void 	*xmalloc(size_t);
extern void 	*xrealloc(void *, size_t);
extern char	*xstrdup(const char *);
extern void 	*xfree(void *);

#endif /* __antiword_h */
