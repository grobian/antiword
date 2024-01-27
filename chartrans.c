/*
 * chartrans.c
 * Copyright (C) 1999,2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Translate Word characters to local representation
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "antiword.h"

static const unsigned short usCp1252[] = {
	0x20ac,    '?', 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
	0x02c6, 0x2030, 0x0160, 0x2039, 0x0152,    '?', 0x017D,    '?',
	   '?', 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
	0x02dc, 0x2122, 0x0161, 0x203a, 0x0153,    '?', 0x017e, 0x0178,
};

static const unsigned short usMacRoman[] = {
	0x00c4, 0x00c5, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1,
	0x00e0, 0x00e2, 0x00e4, 0x00e3, 0x00e5, 0x00e7, 0x00e9, 0x00e8,
	0x00ea, 0x00eb, 0x00ed, 0x00ec, 0x00ee, 0x00ef, 0x00f1, 0x00f3,
	0x00f2, 0x00f4, 0x00f6, 0x00f5, 0x00fa, 0x00f9, 0x00fb, 0x00fc,
	0x2020, 0x00b0, 0x00a2, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x00df,
	0x00ae, 0x00a9, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x00c6, 0x00d8,
	0x221e, 0x00b1, 0x2264, 0x2265, 0x00a5, 0x00b5, 0x2202, 0x2211,
	0x220f, 0x03c0, 0x222b, 0x00aa, 0x00ba, 0x2126, 0x00e6, 0x00f8,
	0x00bf, 0x00a1, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab,
	0x00bb, 0x2026, 0x00a0, 0x00c0, 0x00c3, 0x00d5, 0x0152, 0x0153,
	0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca,
	0x00ff, 0x0178, 0x2044, 0x00a4, 0x2039, 0x203a, 0xfb01, 0xfb02,
	0x2021, 0x00b7, 0x201a, 0x201e, 0x2030, 0x00c2, 0x00ca, 0x00c1,
	0x00cb, 0x00c8, 0x00cd, 0x00ce, 0x00cf, 0x00cc, 0x00d3, 0x00d4,
	   '?', 0x00d2, 0x00da, 0x00db, 0x00d9, 0x0131, 0x02c6, 0x02dc,
	0x00af, 0x02d8, 0x02d9, 0x02da, 0x00b8, 0x02dd, 0x02db, 0x02c7,
};

typedef struct char_table_tag {
	int		iLocal;
	unsigned short	usUnicode;
} char_table_type;

static char_table_type atCharTable[128];


/*
 * iCompare - compare two records
 *
 * Compares two records. For use by qsort(3C) and bsearch(3C).
 *
 * returns -1 if rec1 < rec2, 0 if rec1 == rec2, 1 if rec1 > rec2
 */
static int
iCompare(const void *vpRecord1, const void *vpRecord2)
{
	unsigned short	usUnicode1, usUnicode2;

	usUnicode1 = ((char_table_type *)vpRecord1)->usUnicode;
	usUnicode2 = ((char_table_type *)vpRecord2)->usUnicode;

	if (usUnicode1 < usUnicode2) {
		return -1;
	}
	if (usUnicode1 > usUnicode2) {
		return 1;
	}
	return 0;
} /* end of iCompare */

/*
 * bReadCharacterMappingTable - read the mapping table
 *
 * Read the character mapping table from file and have the contents sorted
 *
 * returns TRUE if successful, otherwise FALSE
 */
BOOL
bReadCharacterMappingTable(const char *szFilename)
{
	FILE	*pFile;
	char	*pcTmp;
	long	lUnicode;
	int	iFields, iLocal;
	char	szLine[81];

	DBG_MSG(szFilename);

	fail(szFilename == NULL);

	if (szFilename == NULL || szFilename[0] == '\0') {
		return FALSE;
	}
	pFile = fopen(szFilename, "r");
	if (pFile == NULL) {
		DBG_MSG(szFilename);
		werr(0, "I can't open your mapping file (%s)", szFilename);
		return FALSE;
	}
	(void)memset(atCharTable, 0, sizeof(atCharTable));

	while (fgets(szLine, (int)sizeof(szLine), pFile)) {
		if (szLine[0] == '#' ||
		    szLine[0] == '\r' ||
		    szLine[0] == '\n') {
			/* Comment or empty line */
			continue;
		}
		iFields = sscanf(szLine, "%x %lx %*s", &iLocal, &lUnicode);
		if (iFields != 2) {
			pcTmp = strchr(szLine, '\r');
			if (pcTmp != NULL) {
				*pcTmp = '\0';
			}
			pcTmp = strchr(szLine, '\n');
			if (pcTmp != NULL) {
				*pcTmp = '\0';
			}
			werr(0, "Syntax error in: '%s'", szLine);
			continue;
		}
		if (iLocal > 0xff || lUnicode > 0xffff) {
			werr(0, "Syntax error in: '%02x %04x'",
					iLocal, lUnicode);
			continue;
		}
		if (iLocal >= 0x80) {
			atCharTable[iLocal - 0x80].iLocal = iLocal;
			atCharTable[iLocal - 0x80].usUnicode =
						(unsigned short)lUnicode;
		}
	}
	(void)fclose(pFile);

	DBG_HEX(atCharTable[0].usUnicode);
	DBG_HEX(atCharTable[elementsof(atCharTable)-1].usUnicode);

	qsort(atCharTable,
		elementsof(atCharTable), sizeof(atCharTable[0]),
		iCompare);

	DBG_HEX(atCharTable[0].usUnicode);
	DBG_HEX(atCharTable[elementsof(atCharTable)-1].usUnicode);

	return TRUE;
} /* end of bReadCharacterMappingTable */

/*
 * iTranslateCharacters - Translate characters to local representation
 *
 * Translate all characters to local representation
 *
 * returns the translated character
 */
int
iTranslateCharacters(unsigned short usChar, long lFileOffset, BOOL bMacWord6)
{
	char_table_type	tKey;
	char_table_type	*pTmp;

	if (bMacWord6) {
		/* Translate special Macintosh characters */
		if (usChar >= 0x80 && usChar <= 0xff) {
			usChar = usMacRoman[usChar - 0x80];
		}
	} else {
		/* Translate implementation defined characters */
		if (usChar >= 0x80 && usChar <= 0x9f) {
			usChar = usCp1252[usChar - 0x80];
		}
	}

	/* Microsoft Unicode to real Unicode */
	switch (usChar) {
	case UNICODE_MS_DIAMOND:
		usChar = UNICODE_DIAMOND;
		break;
	case UNICODE_MS_UNDERTIE:
		usChar = UNICODE_UNDERTIE;
		break;
	case UNICODE_MS_BLACK_SQUARE:
		usChar = UNICODE_BLACK_SQUARE;
		break;
	case UNICODE_MS_BULLET:
		usChar = UNICODE_BULLET;
		break;
	case UNICODE_MS_COPYRIGHT_SIGN:
		usChar = UNICODE_COPYRIGHT_SIGN;
		break;
	default:
		break;
	}

	/* Characters with a special meaning in Word */
	switch (usChar) {
	case IGNORE_CHARACTER:
	case ANNOTATION:
	case FRAME:
	case WORD_SOFT_HYPHEN:
	case UNICODE_HYPHENATION_POINT:
		return IGNORE_CHARACTER;
	case PICTURE:
	case TABLE_SEPARATOR:
	case TAB:
	case HARD_RETURN:
	case FORM_FEED:
	case PAR_END:
	case COLUMN_FEED:
		return (int)usChar;
	case FOOTNOTE_OR_ENDNOTE:
		NO_DBG_HEX(lFileOffset);
		switch (eGetNotetype(lFileOffset)) {
		case notetype_is_footnote:
			return FOOTNOTE_CHAR;
		case notetype_is_endnote:
			return ENDNOTE_CHAR;
		default:
			return UNKNOWN_NOTE_CHAR;
		}
	case WORD_UNBREAKABLE_JOIN:
		return OUR_UNBREAKABLE_JOIN;
	default:
		break;
	}

	/* Latin characters in an oriental text */
	if (usChar >= 0xff01 && usChar <= 0xff5e) {
		usChar -= 0xfee0;
	}

	/* US ASCII */
	if (usChar < 0x80) {
		if (usChar < 0x20 || usChar == 0x7f) {
			/* A control character slipped through */
			return IGNORE_CHARACTER;
		}
		return (int)usChar;
	}

	/* Unicode to local representation */
	tKey.usUnicode = usChar;
	tKey.iLocal = 0;
	pTmp = (char_table_type *)bsearch(&tKey,
			atCharTable,
			elementsof(atCharTable), sizeof(atCharTable[0]),
			iCompare);
	if (pTmp != NULL) {
		return pTmp->iLocal;
	}

	/* Fancy characters to simple US ASCII */
	switch (usChar) {
	case UNICODE_SMALL_F_HOOK:
		return 'f';
	case UNICODE_MODIFIER_CIRCUMFLEX:
		return '^';
	case UNICODE_SMALL_TILDE:
		return '~';
	case UNICODE_LEFT_DOUBLE_QMARK:
	case UNICODE_RIGHT_DOUBLE_QMARK:
	case UNICODE_DOUBLE_LOW_9_QMARK:
	case UNICODE_DOUBLE_HIGH_REV_9_QMARK:
	case UNICODE_DOUBLE_PRIME:
		return '"';
	case UNICODE_LEFT_SINGLE_QMARK:
	case UNICODE_RIGHT_SINGLE_QMARK:
	case UNICODE_SINGLE_LOW_9_QMARK:
	case UNICODE_SINGLE_HIGH_REV_9_QMARK:
	case UNICODE_PRIME:
		return '\'';
	case UNICODE_HYPHEN:
	case UNICODE_NON_BREAKING_HYPHEN:
	case UNICODE_FIGURE_DASH:
	case UNICODE_EN_DASH:
	case UNICODE_EM_DASH:
	case UNICODE_HORIZONTAL_BAR:
	case UNICODE_MINUS_SIGN:
		return '-';
	case UNICODE_DOUBLE_VERTICAL_LINE:
		return '|';
	case UNICODE_DOUBLE_LOW_LINE:
		return '_';
	case UNICODE_DAGGER:
		return '-';
	case UNICODE_DOUBLE_DAGGER:
		return '=';
	case UNICODE_BULLET:
		return OUR_BULLET;
	case UNICODE_ONE_DOT_LEADER:
		return '.';
	case UNICODE_ELLIPSIS:
		return '.';
	case UNICODE_SINGLE_LEFT_ANGLE_QMARK:
		return '<';
	case UNICODE_SINGLE_RIGHT_ANGLE_QMARK:
		return '>';
	case UNICODE_UNDERTIE:
		return '-';
	case UNICODE_EURO_SIGN:
		return 'E';
	case UNICODE_DIAMOND:
		return '-';
	case UNICODE_FRACTION_SLASH:
	case UNICODE_DIVISION_SLASH:
		return '/';
	case UNICODE_BLACK_SQUARE:
		return '+';
	default:
		break;
	}

	if (usChar == UNICODE_TRADEMARK_SIGN) {
		/*
		 * No local representation, it doesn't look like anything in
		 * US-ASCII and a question mark does more harm than good.
		 */
		return IGNORE_CHARACTER;
	}

	DBG_HEX_C(usChar < 0x3000 || usChar >= 0xd800, lFileOffset);
	DBG_HEX_C(usChar < 0x3000 || usChar >= 0xd800, usChar);

	/* Untranslated Unicode character */
	return '?';
} /* end of iTranslateCharacters */

/*
 * iToUpper -  convert letter to upper case
 *
 * This function converts a letter to upper case. Unlike toupper(3) this
 * function is independent from the settings of locale. This comes in handy
 * for people who have to read Word documents in more than one language or
 * contain more than one language.
 *
 * returns the converted letter, or iChar if the conversion was not possible.
 */
int iToUpper(int iChar)
{
	if ((iChar & ~0x7f) == 0) {
		/* US ASCII: use standard function */
		return toupper(iChar);
	}
	if (iChar >= 0xe0 && iChar <= 0xfe && iChar != 0xf7) {
		/*
		 * Lower case accented characters
		 * 0xf7 is Division sign; 0xd7 is Multiplication sign
		 * 0xff is y with diaeresis; 0xdf is Sharp s
		 */
		return iChar & ~0x20;
	}
	return iChar;
} /* end of iToUpper */
