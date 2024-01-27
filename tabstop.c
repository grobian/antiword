/*
 * tabstops.c
 * Copyright (C) 1999,2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Read the tab stop information from a MS Word file
 */

#include <stdio.h>
#include "antiword.h"

static long	lDefaultTabWidth = 36000;	/* In millipoints */


/*
 * vSet6DefaultTabWidth -
 */
static void
vSet6DefaultTabWidth(FILE *pFile, int iStartBlock,
	const int *aiBBD, size_t tBBDLen,
	const unsigned char *aucHeader)
{
	unsigned char	*aucBuffer;
	size_t		tBeginDocpInfo, tDocpInfoLen;

	tBeginDocpInfo = (size_t)ulGetLong(0x150, aucHeader);
	DBG_HEX(tBeginDocpInfo);
	tDocpInfoLen = (size_t)ulGetLong(0x154, aucHeader);
	DBG_DEC(tDocpInfoLen);

	aucBuffer = xmalloc(tDocpInfoLen);
	if (!bReadBuffer(pFile, iStartBlock,
			aiBBD, tBBDLen, BIG_BLOCK_SIZE,
			aucBuffer, tBeginDocpInfo, tDocpInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return;
	}
	lDefaultTabWidth = lTwips2MilliPoints(usGetWord(0x0a, aucBuffer));
	DBG_DEC(lDefaultTabWidth);
	aucBuffer = xfree(aucBuffer);
} /* end of vSet6DefaultTabWidth */

/*
 * vSet8DefaultTabWidth -
 */
static void
vSet8DefaultTabWidth(FILE *pFile, const pps_info_type *pPPS,
	const int *aiBBD, size_t tBBDLen, const int *aiSBD, size_t tSBDLen,
	const unsigned char *aucHeader)
{
        const int	*aiBlockDepot;
	unsigned char	*aucBuffer;
	long	lTableSize;
	size_t	tBeginDocpInfo, tDocpInfoLen, tBlockDepotLen, tBlockSize;
	int	iTableStartBlock;
	unsigned short	usDocStatus;

	tBeginDocpInfo = (size_t)ulGetLong(0x192, aucHeader);
	DBG_HEX(tBeginDocpInfo);
	tDocpInfoLen = (size_t)ulGetLong(0x196, aucHeader);
	DBG_DEC(tDocpInfoLen);

	/* Use 0Table or 1Table? */
	usDocStatus = usGetWord(0x0a, aucHeader);
	if (usDocStatus & BIT(9)) {
		iTableStartBlock = pPPS->t1Table.iSb;
		lTableSize = pPPS->t1Table.lSize;
	} else {
		iTableStartBlock = pPPS->t0Table.iSb;
		lTableSize = pPPS->t0Table.lSize;
	}
	DBG_DEC(iTableStartBlock);
	if (iTableStartBlock < 0) {
		DBG_DEC(iTableStartBlock);
		DBG_MSG("No TAB information");
		return;
	}
	DBG_HEX(lTableSize);
	if (lTableSize < MIN_SIZE_FOR_BBD_USE) {
		/* Use the Small Block Depot */
		aiBlockDepot = aiSBD;
		tBlockDepotLen = tSBDLen;
		tBlockSize = SMALL_BLOCK_SIZE;
	} else {
		/* Use the Big Block Depot */
		aiBlockDepot = aiBBD;
		tBlockDepotLen = tBBDLen;
		tBlockSize = BIG_BLOCK_SIZE;
	}
	aucBuffer = xmalloc(tDocpInfoLen);
	if (!bReadBuffer(pFile, iTableStartBlock,
			aiBlockDepot, tBlockDepotLen, tBlockSize,
			aucBuffer, tBeginDocpInfo, tDocpInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return;
	}
	lDefaultTabWidth = lTwips2MilliPoints(usGetWord(0x0a, aucBuffer));
	DBG_DEC(lDefaultTabWidth);
	aucBuffer = xfree(aucBuffer);
} /* end of vSet8DefaultTabWidth */

/*
 * vSetDefaultTabWidth -
 */
void
vSetDefaultTabWidth(FILE *pFile, const pps_info_type *pPPS,
	const int *aiBBD, size_t tBBDLen, const int *aiSBD, size_t tSBDLen,
	const unsigned char *aucHeader, int iWordVersion)
{
	fail(pFile == NULL || pPPS == NULL || aucHeader == NULL);
	fail(iWordVersion < 6 || iWordVersion > 8);
	fail(aiBBD == NULL || aiSBD == NULL);

	switch (iWordVersion) {
	case 6:
	case 7:
		vSet6DefaultTabWidth(pFile, pPPS->tWordDocument.iSb,
				aiBBD, tBBDLen, aucHeader);
		break;
	case 8:
		vSet8DefaultTabWidth(pFile, pPPS,
				aiBBD, tBBDLen, aiSBD, tSBDLen, aucHeader);
		break;
	default:
		werr(0, "Sorry, no TAB information");
		break;
	}
} /* end of vSetDefaultTabWidth */

/*
 * lGetDefaultTabWidth - Get the default tabwidth in millipoints
 */
long
lGetDefaultTabWidth(void)
{
	if (lDefaultTabWidth <= 0) {
		DBG_DEC(lDefaultTabWidth);
		return lTwips2MilliPoints(1);
	}
	return lDefaultTabWidth;
} /* end of lGetDefaultTabWidth */
