/*
 * wordconst.h
 * Copyright (C) 1998-2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Constants and macros for the interpretation of MS Word files
 */

#if !defined(__wordconst_h)
#define __wordconst_h 1

/*
 * A bit odd definition of the type Boolean, but RISC OS insists
 * on this and Unix doesn't mind.
 */
#if !defined(BOOL)
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif /* !BOOL */

/* Block sizes */
#define HEADER_SIZE			768
#define BIG_BLOCK_SIZE			512
#define PROPERTY_SET_STORAGE_SIZE	128
#define SMALL_BLOCK_SIZE		 64
/* Switch size of Depot use */
#define MIN_SIZE_FOR_BBD_USE		0x1000
/* Table sizes */
#define TABLE_COLUMN_MAX		 31
/* Maximum number of tabs positions in a paragraph */
#define NUMBER_OF_TABS_MAX		 64
/* Font sizes (in half-points) */
#define MIN_FONT_SIZE			  8
#define DEFAULT_FONT_SIZE		 20
#define MAX_FONT_SIZE			240
/* Font styles */
#define FONT_REGULAR			0x00
#define FONT_BOLD			0x01
#define FONT_ITALIC			0x02
#define FONT_UNDERLINE			0x04
#define FONT_CAPITALS			0x08
#define FONT_SMALL_CAPITALS		0x10
#define FONT_STRIKE			0x20
#define FONT_HIDDEN			0x40
/* Font colors */
#define FONT_COLOR_DEFAULT		 0
#define FONT_COLOR_BLACK		 1
#define FONT_COLOR_RED			 6
/* End of a block chain */
#define END_OF_CHAIN			(-2)
/* Types of lists */
#define LIST_ARABIC_NUM			0x00
#define LIST_ROMAN_NUM_UPPER		0x01
#define LIST_ROMAN_NUM_LOWER		0x02
#define LIST_UPPER_ALPHA		0x03
#define LIST_LOWER_ALPHA		0x04
#define LIST_BULLETS			0xff
/* Types of paragraph alignment */
#define ALIGNMENT_LEFT			0x00
#define ALIGNMENT_CENTER		0x01
#define ALIGNMENT_RIGHT			0x02
#define ALIGNMENT_JUSTIFY		0x03

/* Macros */
	/* Get macros */
#define ucGetByte(i,a)		((unsigned char)(a[i]))
#define usGetWord(i,a)		((unsigned short)(a[(i)+1])<<8|\
					(unsigned short)(a[i]))
#define ulGetLong(i,a)		((unsigned long)(a[(i)+3])<<24|\
					(unsigned long)(a[(i)+2])<<16|\
					(unsigned long)(a[(i)+1])<<8|\
					(unsigned long)(a[i]))
	/* Font style macros */
#define bIsBold(x)		(((x) & FONT_BOLD) == FONT_BOLD)
#define bIsItalic(x)		(((x) & FONT_ITALIC) == FONT_ITALIC)
#define bIsUnderline(x)		(((x) & FONT_UNDERLINE) == FONT_UNDERLINE)
#define bIsCapitals(x)		(((x) & FONT_CAPITALS) == FONT_CAPITALS)
#define bIsSmallCapitals(x)	(((x) & FONT_SMALL_CAPITALS) == FONT_SMALL_CAPITALS)
#define bIsStrike(x)		(((x) & FONT_STRIKE) == FONT_STRIKE)
#define bIsHidden(x)		(((x) & FONT_HIDDEN) == FONT_HIDDEN)
	/* Computation macros */
/* From Words half-points to draw units (plus a percentage) */
#define lWord2DrawUnits00(x)	((long)(x) * 320)
#define lWord2DrawUnits20(x)	((long)(x) * 384)
#define lToBaseLine(x)		((long)(x) *  45)
/* From twips (1/20 of a point) to millipoints */
#define lTwips2MilliPoints(x)	((long)(x) * 50)
/* From default characters (16 OS units wide) to millipoints */
#define lChar2MilliPoints(x)	((long)(x) * 6400)
#define iMilliPoints2Char(x)	(int)(((long)(x) + 3200) / 6400)
#define iDrawUnits2Char(x)	(int)(((long)(x) + 2048) / 4096)
/* From draw units (1/180*256 inch) to millipoints (1/72*1000 inch) */
#define lDrawUnits2MilliPoints(x)	(((long)(x) * 25 +  8) / 16)
#define lMilliPoints2DrawUnits(x)	(((long)(x) * 16 + 12) / 25)
#define lPoints2DrawUnits(x)		((long)(x) * 640)
#define dDrawUnits2Points(x)		((double)(x) / 640.0)

/* Special characters */
#define IGNORE_CHARACTER	0x00	/* ^@ */
#define PICTURE			0x01	/* ^A */
#define FOOTNOTE_OR_ENDNOTE	0x02	/* ^B */
#define ANNOTATION		0x05	/* ^E */
#define TABLE_SEPARATOR		0x07	/* ^G */
#define FRAME			0x08	/* ^H */
#define TAB			0x09	/* ^I */
/* End of line characters */
#define HARD_RETURN		0x0b	/* ^K */
#define FORM_FEED		0x0c	/* ^L */
#define PAR_END			0x0d	/* ^M */
#define COLUMN_FEED		0x0e	/* ^N */
/* Embedded stuff */
#define START_EMBEDDED		0x13	/* ^S */
#define END_IGNORE		0x14	/* ^T */
#define END_EMBEDDED		0x15	/* ^U */
/* Special characters */
#if defined(DEBUG)
#define FILLER_CHAR		0xb7
#else
#define FILLER_CHAR		' '
#endif /* DEBUG */
#define TABLE_SEPARATOR_CHAR	'|'
#if defined(__dos)
/* Pseudo characters. These must be outside the character range */
#define FOOTNOTE_CHAR		((int)UCHAR_MAX + 1)
#define ENDNOTE_CHAR		((int)UCHAR_MAX + 2)
#define UNKNOWN_NOTE_CHAR	((int)UCHAR_MAX + 3)
#else
/* Pseudo characters. These must be outside the Unicode range */
#define FOOTNOTE_CHAR		((int)USHRT_MAX + 1)
#define ENDNOTE_CHAR		((int)USHRT_MAX + 2)
#define UNKNOWN_NOTE_CHAR	((int)USHRT_MAX + 3)
#endif /* __dos */

/* Charactercodes as used by Word */
#define WORD_UNBREAKABLE_JOIN		0x1e
#define WORD_SOFT_HYPHEN		0x1f

/* Unicode characters */
#define UNICODE_COPYRIGHT_SIGN		0x00a9
#define UNICODE_SMALL_F_HOOK		0x0192
#define UNICODE_MODIFIER_CIRCUMFLEX	0x02c6
#define UNICODE_SMALL_TILDE		0x02dc
#define UNICODE_HYPHEN			0x2010
#define UNICODE_NON_BREAKING_HYPHEN	0x2011
#define UNICODE_FIGURE_DASH		0x2012
#define UNICODE_EN_DASH			0x2013
#define UNICODE_EM_DASH			0x2014
#define UNICODE_HORIZONTAL_BAR		0x2015
#define UNICODE_DOUBLE_VERTICAL_LINE	0x2016
#define UNICODE_DOUBLE_LOW_LINE		0x2017
#define UNICODE_LEFT_SINGLE_QMARK	0x2018
#define UNICODE_RIGHT_SINGLE_QMARK	0x2019
#define UNICODE_SINGLE_LOW_9_QMARK	0x201a
#define UNICODE_SINGLE_HIGH_REV_9_QMARK	0x201b
#define UNICODE_LEFT_DOUBLE_QMARK	0x201c
#define UNICODE_RIGHT_DOUBLE_QMARK	0x201d
#define UNICODE_DOUBLE_LOW_9_QMARK	0x201e
#define UNICODE_DOUBLE_HIGH_REV_9_QMARK	0x201f
#define UNICODE_DAGGER			0x2020
#define UNICODE_DOUBLE_DAGGER		0x2021
#define UNICODE_BULLET			0x2022
#define UNICODE_ONE_DOT_LEADER		0x2024
#define UNICODE_ELLIPSIS		0x2026
#define UNICODE_HYPHENATION_POINT	0x2027
#define UNICODE_PRIME			0x2032
#define UNICODE_DOUBLE_PRIME		0x2033
#define UNICODE_SINGLE_LEFT_ANGLE_QMARK	0x2039
#define UNICODE_SINGLE_RIGHT_ANGLE_QMARK	0x203a
#define UNICODE_UNDERTIE		0x203f
#define UNICODE_FRACTION_SLASH		0x2044
#define UNICODE_EURO_SIGN		0x20ac
#define UNICODE_DIAMOND			0x20df
#define UNICODE_TRADEMARK_SIGN		0x2122
#define UNICODE_MINUS_SIGN		0x2212
#define UNICODE_DIVISION_SLASH		0x2215
#define UNICODE_BLACK_SQUARE		0x25a0
#define UNICODE_MS_DIAMOND		0xf075
#define UNICODE_MS_UNDERTIE		0xf0a4
#define UNICODE_MS_BLACK_SQUARE		0xf0a7
#define UNICODE_MS_BULLET		0xf0b7
#define UNICODE_MS_COPYRIGHT_SIGN	0xf0d3

#if defined(__riscos)
#define OUR_ELLIPSIS			0x8c
#define OUR_BULLET			0x8f
#define OUR_EM_DASH			0x98
#define OUR_UNBREAKABLE_JOIN		0x99
#else
#define OUR_BULLET			'+'
#define OUR_EM_DASH			'-'
#define OUR_UNBREAKABLE_JOIN		'-'
#endif /* __riscos */
#define OUR_DIAMOND			'-'

#endif /* __wordconst_h */
