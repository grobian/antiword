/*
 * misc.c
 * Copyright (C) 1998-2000 A.J. van Os; Released under GPL
 *
 * Description:
 * System calls & misc. functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#if defined(__riscos)
#include "kernel.h"
#include "swis.h"
#else
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(__dos)
#define S_ISDIR(x)	(((x) & S_IFMT) == S_IFDIR)
#define S_ISREG(x)	(((x) & S_IFMT) == S_IFREG)
#endif /* __dos */
#endif /* __riscos */
#include "antiword.h"


#if defined(__riscos)
/*
 * iGetFiletype
 * This procedure will get the filetype of the given file.
 * returns the filetype.
 */
int
iGetFiletype(const char *szFilename)
{
	_kernel_swi_regs	regs;
	_kernel_oserror		*e;

	fail(szFilename == NULL || szFilename[0] == '\0');

	(void)memset((void *)&regs, 0, sizeof(regs));
	regs.r[0] = 23;
	regs.r[1] = (int)szFilename;
	e = _kernel_swi(OS_File, &regs, &regs);
	if (e == NULL) {
		return regs.r[6];
	}
	werr(0, "Get Filetype error %d: %s", e->errnum, e->errmess);
	return -1;
} /* end of iGetFiletype */

/*
 * vSetFiletype
 * This procedure will set the filetype of the given file to the given
 * type.
 */
void
vSetFiletype(const char *szFilename, int iFiletype)
{
	_kernel_swi_regs	regs;
	_kernel_oserror		*e;

	fail(szFilename == NULL || szFilename[0] == '\0');

	if (iFiletype < 0x000 || iFiletype > 0xfff) {
		return;
	}
	(void)memset((void *)&regs, 0, sizeof(regs));
	regs.r[0] = 18;
	regs.r[1] = (int)szFilename;
	regs.r[2] = iFiletype;
	e = _kernel_swi(OS_File, &regs, &regs);
	if (e != NULL) {
		switch (e->errnum) {
		case 0x000113:	/* ROM */
		case 0x0104e1:	/* Read-only floppy DOSFS */
		case 0x0108c9:	/* Read-only floppy ADFS */
		case 0x013803:	/* Read-only ArcFS */
		case 0x80344a:	/* CD-ROM */
			break;
		default:
			werr(0, "Set Filetype error %d: %s",
				e->errnum, e->errmess);
			break;
		}
	}
} /* end of vSetFileType */

/*
 * bISO_8859_1_IsCurrent
 * This function checks whether ISO_8859_1 (aka Latin1) is the current
 * character set.
 */
BOOL
bISO_8859_1_IsCurrent(void)
{
	_kernel_swi_regs	regs;
	_kernel_oserror		*e;

	(void)memset((void *)&regs, 0, sizeof(regs));
	regs.r[0] = 71;
	regs.r[1] = 127;
	e = _kernel_swi(OS_Byte, &regs, &regs);
	if (e == NULL) {
		return regs.r[1] == 101;
	}
	werr(0, "Read alphabet error %d: %s", e->errnum, e->errmess);
	return FALSE;
} /* end of bISO_8859_1_IsCurrent */
#else
/*
 * szGetHomeDirectory - get the name of the home directory
 */
const char *
szGetHomeDirectory(void)
{
	const char	*szHome;

	szHome = getenv("HOME");
	if (szHome == NULL || szHome[0] == '\0') {
#if defined(__dos)
		szHome = "C:";
#else
		werr(0, "I can't find the name of your HOME directory");
		szHome = "";
#endif /* __dos */
	}
	return szHome;
} /* end of szGetHomeDirectory */
#endif /* __riscos */

/*
 * Check if the directory part of the given file exists, make the directory
 * if it does not exist yet.
 * Returns TRUE in case of success, otherwise FALSE.
 */
BOOL
bMakeDirectory(const char *szFilename)
{
#if defined(__riscos)
	_kernel_swi_regs	regs;
	_kernel_oserror		*e;
	char	*pcLastDot;
	char	szDirectory[PATH_MAX+1];

	DBG_MSG("bMakeDirectory");
	fail(szFilename == NULL || szFilename[0] == '\0');
	DBG_MSG(szFilename);

	if (strlen(szFilename) >= sizeof(szDirectory)) {
		DBG_DEC(strlen(szFilename));
		return FALSE;
	}
	strcpy(szDirectory, szFilename);
	pcLastDot = strrchr(szDirectory, '.');
	if (pcLastDot == NULL) {
		/* No directory equals current directory */
		DBG_MSG("No directory part given");
		return TRUE;
	}
	*pcLastDot = '\0';
	DBG_MSG(szDirectory);
	/* Check if the name exists */
	(void)memset((void *)&regs, 0, sizeof(regs));
	regs.r[0] = 17;
	regs.r[1] = (int)szDirectory;
	e = _kernel_swi(OS_File, &regs, &regs);
	if (e != NULL) {
		werr(0, "Directory check %d: %s", e->errnum, e->errmess);
		return FALSE;
	}
	if (regs.r[0] == 2) {
		/* The name exists and it is a directory */
		DBG_MSG("The directory already exists");
		return TRUE;
	}
	if (regs.r[0] != 0) {
		/* The name exists and it is not a directory */
		DBG_DEC(regs.r[0]);
		return FALSE;
	}
	/* The name does not exist, make the directory */
	(void)memset((void *)&regs, 0, sizeof(regs));
	regs.r[0] = 8;
	regs.r[1] = (int)szDirectory;
	regs.r[4] = 0;
	e = _kernel_swi(OS_File, &regs, &regs);
	if (e != NULL) {
		werr(0, "I can't make a directory %d: %s",
			e->errnum, e->errmess);
		return FALSE;
	}
	return TRUE;
#else
	struct stat	tBuffer;
	char	*pcLastSeparator;
	char	szDirectory[PATH_MAX+1];

	DBG_MSG("bMakeDirectory");
	fail(szFilename == NULL || szFilename[0] == '\0');
	DBG_MSG(szFilename);

	if (strlen(szFilename) >= sizeof(szDirectory)) {
		DBG_DEC(strlen(szFilename));
		return FALSE;
	}
	strcpy(szDirectory, szFilename);
	pcLastSeparator = strrchr(szDirectory, FILE_SEPARATOR[0]);
	if (pcLastSeparator == NULL) {
		/* No directory equals current directory */
		DBG_MSG("No directory part given");
		return TRUE;
	}
	*pcLastSeparator = '\0';
	if (stat(szDirectory, &tBuffer) == 0) {
		if (S_ISDIR(tBuffer.st_mode)) {
			/* The name exists and it is a directory */
			DBG_MSG("The directory already exists");
			return TRUE;
		}
		/* The name exists and it is not a directory */
		DBG_HEX(tBuffer.st_mode);
		return FALSE;
	}
	/* The name does not exist, make the directory */
	if (mkdir(szDirectory, 0755) < 0) {
		werr(0, "I can't make a directory; errno=%d", errno);
		return FALSE;
	}
	return TRUE;
#endif /* __riscos */
} /* end of bMakeDirectory */

/*
 * Get the size of the given file.
 * Returns -1 if the file does not exist or is not a proper file.
 */
long
lGetFilesize(const char *szFilename)
{
#if defined(__riscos)
	_kernel_swi_regs	regs;
	_kernel_oserror		*e;

	(void)memset(&regs, 0, sizeof(regs));
	regs.r[0] = 17;
	regs.r[1] = (int)szFilename;
	e = _kernel_swi(OS_File, &regs, &regs);
	if (e != NULL) {
		werr(0, "Get Filesize error %d: %s",
			e->errnum, e->errmess);
		return -1;
	}
	if (regs.r[0] != 1) {
		/* It's not a proper file or the file does not exist */
		return -1;
	}
	return (long)regs.r[4];
#else
	struct stat	tBuffer;

	if (stat(szFilename, &tBuffer) != 0) {
		werr(0, "Get Filesize error %d", errno);
		return -1;
	}
	if (!S_ISREG(tBuffer.st_mode)) {
		/* It's not a regular file */
		return -1;
	}
	return (long)tBuffer.st_size;
#endif /* __riscos */
} /* end of lGetFilesize */

#if defined(DEBUG)
void
vPrintBlock(const char	*szFile, int iLine,
		const unsigned char *aucBlock, size_t tLength)
{
	int i, j;

	fail(szFile == NULL || iLine < 0 || aucBlock == NULL);

	fprintf(stderr, "%s[%3d]:\n", szFile, iLine);
	for (i = 0; i < 32; i++) {
		fprintf(stderr, "%03x: ", 16 * i);
		for (j = 0; j < 16; j++) {
			if (16 * i + j >= (int)tLength) {
				if (j > 0) {
					fprintf(stderr, "\n");
				}
				return;
			}
			fprintf(stderr, "%02x ", aucBlock[16 * i + j]);
		}
		fprintf(stderr, "\n");
	}
} /* end of vPrintBlock */

void
vPrintUnicode(const char  *szFile, int iLine, const char *s)
{
	size_t	tLen;
	char	*szASCII;

	tLen = unilen(s) / 2;
	szASCII = xmalloc(tLen + 1);
	(void)unincpy(szASCII, s, tLen);
	szASCII[tLen] = '\0';
	(void)fprintf(stderr, "%s[%3d]: %.240s\n", szFile, iLine, szASCII);
	szASCII = xfree(szASCII);
} /* end of vPrintUnicode */

BOOL
bCheckDoubleLinkedList(output_type *pAnchor)
{
	output_type	*pCurr, *pLast;
	int		iInList;

	pLast = pAnchor;
	iInList = 0;
	for (pCurr = pAnchor; pCurr != NULL; pCurr = pCurr->pNext) {
		pLast = pCurr;
		iInList++;
	}
	NO_DBG_DEC(iInList);
	for (pCurr = pLast; pCurr != NULL; pCurr = pCurr->pPrev) {
		pLast = pCurr;
		iInList--;
	}
	DBG_DEC_C(iInList != 0, iInList);
	return pAnchor == pLast && iInList == 0;
} /* end of bCheckDoubleLinkedList */
#endif /* DEBUG */

/*
 * bReadBytes
 * This function reads the given number of bytes from the given file,
 * starting from the given offset.
 * Returns TRUE when successfull, otherwise FALSE
 */
BOOL
bReadBytes(unsigned char *aucBytes, size_t tMemb, long lOffset, FILE *pFile)
{
	fail(aucBytes == NULL || pFile == NULL || lOffset < 0);

	if (fseek(pFile, lOffset, SEEK_SET) != 0) {
		return FALSE;
	}
	if (fread(aucBytes, sizeof(unsigned char), tMemb, pFile) != tMemb) {
		return FALSE;
	}
	return TRUE;
} /* end of bReadBytes */

/*
 * bReadBuffer
 * This function fills the given buffer with the given number of bytes,
 * starting at the given offset within the Big/Small Block Depot.
 *
 * Returns TRUE when successful, otherwise FALSE
 */
BOOL
bReadBuffer(FILE *pFile, int iStartBlock,
	const int *aiBlockDepot, size_t tBlockDepotLen, size_t tBlockSize,
	unsigned char *aucBuffer, size_t tOffset, size_t tToRead)
{
	long	lBegin;
	size_t	tLen;
	int	iIndex;

	fail(pFile == NULL);
	fail(iStartBlock < 0);
	fail(aiBlockDepot == NULL);
	fail(tBlockSize != BIG_BLOCK_SIZE && tBlockSize != SMALL_BLOCK_SIZE);
	fail(aucBuffer == NULL);
	fail(tToRead == 0);

	for (iIndex = iStartBlock;
	     iIndex != END_OF_CHAIN && tToRead != 0;
	     iIndex = aiBlockDepot[iIndex]) {
		if (iIndex < 0 || iIndex >= (int)tBlockDepotLen) {
			if (tBlockSize >= BIG_BLOCK_SIZE) {
				werr(1, "The Big Block Depot is corrupt");
			} else {
				werr(1, "The Small Block Depot is corrupt");
			}
		}
		if (tOffset >= tBlockSize) {
			tOffset -= tBlockSize;
			continue;
		}
		lBegin = lDepotOffset(iIndex, tBlockSize) + (long)tOffset;
		tLen = min(tBlockSize - tOffset, tToRead);
		tOffset = 0;
		if (!bReadBytes(aucBuffer, tLen, lBegin, pFile)) {
			werr(0, "Read big block %ld not possible", lBegin);
			return FALSE;
		}
		aucBuffer += tLen;
		tToRead -= tLen;
	}
	DBG_DEC_C(tToRead != 0, tToRead);
	return tToRead == 0;
} /* end of bReadBuffer */

/*
 * Translate a Word colornumber into a true color for use in a drawfile
 *
 * Returns the true color
 */
unsigned int
uiColor2Color(int iWordColor)
{
	static const unsigned int	auiColorTable[] = {
		/*  0 */	0x00000000U,	/* Automatic */
		/*  1 */	0x00000000U,	/* Black */
		/*  2 */	0xff000000U,	/* Blue */
		/*  3 */	0xffff0000U,	/* Turquoise */
		/*  4 */	0x00ff0000U,	/* Bright Green */
		/*  5 */	0xff00ff00U,	/* Pink */
		/*  6 */	0x0000ff00U,	/* Red */
		/*  7 */	0x00ffff00U,	/* Yellow */
		/*  8 */	0xffffff00U,	/* White */
		/*  9 */	0x80000000U,	/* Dark Blue */
		/* 10 */	0x80800000U,	/* Teal */
		/* 11 */	0x00800000U,	/* Green */
		/* 12 */	0x80008000U,	/* Violet */
		/* 13 */	0x00008000U,	/* Dark Red */
		/* 14 */	0x00808000U,	/* Dark Yellow */
		/* 15 */	0x80808000U,	/* Gray 50% */
		/* 16 */	0xc0c0c000U,	/* Gray 25% */
	};
	if (iWordColor < 0 ||
	    iWordColor >= (int)elementsof(auiColorTable)) {
		return auiColorTable[0];
	}
	return auiColorTable[iWordColor];
} /* end of uiColor2Color */

/*
 * iFindSplit - find a place to split the string
 *
 * returns the index of the split character or -1 if no split found.
 */
static int
iFindSplit(const char *szString, int iStringLen)
{
	int	iSplit;

	iSplit = iStringLen - 1;
	while (iSplit >= 1) {
		if (szString[iSplit] == ' ' ||
		    (szString[iSplit] == '-' && szString[iSplit - 1] != ' ')) {
			return iSplit;
		}
		iSplit--;
	}
	return -1;
} /* end of iFindSplit */

/*
 * pSplitList - split the given list in a printable part and a leftover part
 *
 * returns the pointer to the leftover part
 */
output_type *
pSplitList(output_type *pAnchor)
{
	output_type	*pCurr, *pLeftOver;
	int		iIndex;

 	fail(pAnchor == NULL);

	for (pCurr = pAnchor; pCurr->pNext != NULL; pCurr = pCurr->pNext)
		;	/* EMPTY */
	iIndex = -1;
	for (; pCurr != NULL; pCurr = pCurr->pPrev) {
		iIndex = iFindSplit(pCurr->szStorage, pCurr->iNextFree);
		if (iIndex >= 0) {
			break;
		}
	}

	if (pCurr == NULL || iIndex < 0) {
		/* No split, no leftover */
		return NULL;
	}
	/* Split over the iIndex-th character */
	NO_DBG_MSG("pLeftOver");
	pLeftOver = xmalloc(sizeof(*pLeftOver));
	pLeftOver->tStorageSize = (size_t)(pCurr->iNextFree - iIndex);
	pLeftOver->szStorage = xmalloc(pLeftOver->tStorageSize);
	pLeftOver->iNextFree = pCurr->iNextFree - iIndex - 1;
	(void)strncpy(pLeftOver->szStorage,
		pCurr->szStorage + iIndex + 1, (size_t)pLeftOver->iNextFree);
	pLeftOver->szStorage[pLeftOver->iNextFree] = '\0';
	NO_DBG_MSG(pLeftOver->szStorage);
	pLeftOver->iColor = pCurr->iColor;
	pLeftOver->ucFontstyle = pCurr->ucFontstyle;
	pLeftOver->tFontRef = pCurr->tFontRef;
	pLeftOver->sFontsize = pCurr->sFontsize;
	pLeftOver->lStringWidth = lComputeStringWidth(
					pLeftOver->szStorage,
					pLeftOver->iNextFree,
					pLeftOver->tFontRef,
					pLeftOver->sFontsize);
	pLeftOver->pPrev = NULL;
	pLeftOver->pNext = pCurr->pNext;
	if (pLeftOver->pNext != NULL) {
		pLeftOver->pNext->pPrev = pLeftOver;
	}
	fail(!bCheckDoubleLinkedList(pLeftOver));

	NO_DBG_MSG("pAnchor");
	NO_DBG_HEX(pCurr->szStorage[iIndex]);
	while (iIndex >= 0 && isspace(pCurr->szStorage[iIndex])) {
		iIndex--;
	}
	pCurr->iNextFree = iIndex + 1;
	pCurr->szStorage[pCurr->iNextFree] = '\0';
	NO_DBG_MSG(pCurr->szStorage);
	pCurr->lStringWidth = lComputeStringWidth(
					pCurr->szStorage,
					pCurr->iNextFree,
					pCurr->tFontRef,
					pCurr->sFontsize);
	pCurr->pNext = NULL;
	fail(!bCheckDoubleLinkedList(pAnchor));

	return pLeftOver;
} /* end of pSplitList */

/*
 * iInteger2Roman - convert an integer to Roman Numerals
 *
 * returns the number of characters written
 */
int
iInteger2Roman(int iNumber, BOOL bUpperCase, char *szOutput)
{
	char *outp, *p, *q;
	int iNextVal, iValue;

	fail(szOutput == NULL);

	if (iNumber <= 0 || iNumber >= 4000) {
		szOutput[0] = '\0';
		return 0;
	}

	outp = szOutput;
	p = bUpperCase ? "M\2D\5C\2L\5X\2V\5I" : "m\2d\5c\2l\5x\2v\5i";
	iValue = 1000;
	for (;;) {
		while (iNumber >= iValue) {
			*outp++ = *p;
			iNumber -= iValue;
		}
		if (iNumber <= 0) {
			*outp = '\0';
			return outp - szOutput;
		}
		q = p + 1;
		iNextVal = iValue / (int)*q;
		if ((int)*q == 2) {		/* magic */
			iNextVal /= (int)*(q += 2);
		}
		if (iNumber + iNextVal >= iValue) {
			*outp++ = *++q;
			iNumber += iNextVal;
		} else {
			p++;
			iValue /= (int)(*p++);
		}
	}
} /* end of iInteger2Roman */

/*
 * iInteger2Alpha - convert an integer to Alphabetic "numbers"
 *
 * returns the number of characters written
 */
int
iInteger2Alpha(int iNumber, BOOL bUpperCase, char *szOutput)
{
	char	*outp;
	int	iTmp;

	fail(szOutput == NULL);

	outp = szOutput;
	iTmp = bUpperCase ? 'A': 'a';
	if (iNumber <= 26) {
		iNumber -= 1;
		*outp++ = (char)(iTmp + iNumber);
	} else if (iNumber <= 26 + 26*26) {
		iNumber -= 26 + 1;
		*outp++ = (char)(iTmp + iNumber / 26);
		*outp++ = (char)(iTmp + iNumber % 26);
	} else if (iNumber <= 26 + 26*26 + 26*26*26) {
		iNumber -= 26 + 26*26 + 1;
		*outp++ = (char)(iTmp + iNumber / (26*26));
		*outp++ = (char)(iTmp + iNumber / 26 % 26);
		*outp++ = (char)(iTmp + iNumber % 26);
	}
	*outp = '\0';
	return outp - szOutput;
} /* end of iInteger2Alpha */

/*
 * unincpy - copy a counted Unicode string to an single-byte string
 */
char *
unincpy(char *s1, const char *s2, size_t n)
{
	char		*dest;
	size_t		len;
	int		iChar;
	unsigned short	usUni;

	for (dest = s1, len = 0; len < n; dest++, len++) {
		usUni = usGetWord(len * 2, s2);
		if (usUni == 0) {
			break;
		}
		iChar = iTranslateCharacters(usUni, 0, FALSE);
		if (iChar == IGNORE_CHARACTER) {
			iChar = '?';
		}
		*dest = (char)iChar;
	}
	for (; len < n; len++) {
		*dest++ = '\0';
	}
	return s1;
} /* end of unincpy */

/*
 * unilen - calculate the length of a Unicode string
 */
size_t
unilen(const char *s)
{
	size_t		tLen;
	unsigned short	usUni;

	tLen = 0;
	for (;;) {
		usUni = usGetWord(tLen * 2, s);
		if (usUni == 0) {
			return tLen;
		}
		tLen += 2;
	}
} /* end of unilen */

/*
 * szBaseName - get the basename of the given filename
 */
const char *
szBasename(const char *szFilename)
{
	const char	*szTmp;

	fail(szFilename == NULL);

	if (szFilename == NULL || szFilename[0] == '\0') {
		return "null";
	}

	szTmp = strrchr(szFilename, FILE_SEPARATOR[0]);
	if (szTmp == NULL) {
		return szFilename;
	}
	return ++szTmp;
} /* end of szBasename */
