/*
 * main_u.c
 *
 * Released under GPL
 *
 * Copyright (C) 1998-2000 A.J. van Os
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Description:
 * The main program of 'antiword' (Unix version)
 */

#include <stdio.h>
#include "version.h"
#include "antiword.h"

/* The name of this program */
static const char	*szTask = NULL;


static void
vUsage(void)
{
	fprintf(stderr, "\tName: %s\n", szTask);
	fprintf(stderr, "\tPurpose: "PURPOSESTRING"\n");
	fprintf(stderr, "\tAuthor: "AUTHORSTRING"\n");
	fprintf(stderr, "\tVersion: "VERSIONSTRING"\n");
	fprintf(stderr, "\tStatus: "STATUSSTRING"\n");
	fprintf(stderr,
		"\tUsage: %s [switches] wordfile1 [wordfile2 ...]\n", szTask);
	fprintf(stderr,
		"\tSwitches: [-t|-p papersize][-m mapping][-w #][-i #][-X #]"
		"[-Ls]\n");
	fprintf(stderr, "\t\t-t text output (default)\n");
	fprintf(stderr, "\t\t-p <paper size name> PostScript output\n");
	fprintf(stderr, "\t\t   like: a4, letter or legal\n");
	fprintf(stderr, "\t\t-m <mapping> character mapping file\n");
	fprintf(stderr, "\t\t-w <width> in characters of text output\n");
	fprintf(stderr, "\t\t-i <level> image level (PostScript only)\n");
	fprintf(stderr, "\t\t-X <encoding> character set (Postscript only)\n");
	fprintf(stderr, "\t\t-L use landscape mode (PostScript only)\n");
	fprintf(stderr, "\t\t-s Show hidden (by Word) text\n");
} /* end of vUsage */

/*
 * bProcessFile - process a single file
 *
 * returns: TRUE when the given file is a supported Word file, otherwise FALSE
 */
static BOOL
bProcessFile(const char *szFilename)
{
	diagram_type	*pDiag;

	fail(szFilename == NULL || szFilename[0] == '\0');

	DBG_MSG(szFilename);

	if (!bIsSupportedWordFile(szFilename)) {
		if (bIsRtfFile(szFilename)) {
			werr(0, "%s is not a Word Document."
				" It is probably a Rich Text Format file",
				szFilename);
		} else if (bIsWord245File(szFilename)) {
			werr(0, "%s is not in a supported Word format."
				" It is probably from 'Word2, 4 or 5'",
				szFilename);
		} else {
#if defined(__dos)
			werr(0, "%s is not a Word Document or the filename"
				" is not in the 8+3 format.", szFilename);
#else
			werr(0, "%s is not a Word Document.", szFilename);
#endif /* __dos */
		}
		return FALSE;
	}
	pDiag = pCreateDiagram(szTask, szFilename);
	if (pDiag != NULL) {
		vWord2Text(pDiag, szFilename);
		vDestroyDiagram(pDiag);
	}
	return TRUE;
} /* end of bProcessFile */

int
main(int argc, char **argv)
{
	options_type	tOptions;
	const char	*szWordfile;
	int	iFirst, iIndex, iGoodCount;
	BOOL	bUsage, bMultiple, bUsePlainText;

	if (argc <= 0) {
		return 1;
	}

	szTask = szBasename(argv[0]);

	if (argc <= 1) {
		iFirst = 1;
		bUsage = TRUE;
	} else {
		iFirst = iReadOptions(argc, argv);
		bUsage = iFirst <= 0;
	}
	if (bUsage) {
		vUsage();
		return iFirst < 0 ? 1 : 0;
	}

	vGetOptions(&tOptions);

	bMultiple = argc - iFirst > 1;
	bUsePlainText = !tOptions.bUseOutlineFonts;
	iGoodCount = 0;

	for (iIndex = iFirst; iIndex < argc; iIndex++) {
		if (bMultiple && bUsePlainText) {
			szWordfile = szBasename(argv[iIndex]);
			fprintf(stdout, "::::::::::::::\n");
			fprintf(stdout, "%s\n", szWordfile);
			fprintf(stdout, "::::::::::::::\n");
		}
		if (bProcessFile(argv[iIndex])) {
			iGoodCount++;
		}
	}
	DBG_DEC(iGoodCount);
	return iGoodCount <= 0 ? 1 : 0;
} /* end of main */
