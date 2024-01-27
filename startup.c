/*
 * startup.c
 * Copyright (C) 1998-2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Guarantee a single startup of !AntiWord
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "kernel.h"
#include "swis.h"
#include "wimp.h"
#include "wimpt.h"
#include "antiword.h"


#if !defined(TaskManager_EnumerateTasks)
#define TaskManager_EnumerateTasks	0x042681
#endif /* TaskManager_EnumerateTasks */

/*
 * tGetTaskHandle - get the taskhandle of the given task
 *
 * returns the taskhandle when found, otherwise 0
 */
static wimp_t
tGetTaskHandle(const char *szTaskname, int iOSVersion)
{
	_kernel_swi_regs	regs;
	_kernel_oserror		*e;
	const char	*pcTmp;
	int	iIndex;
	int	aiBuffer[4];
	char	szTmp[12];

	if (iOSVersion < 310) {
		/*
		 * SWI TaskManager_EnumerateTasks does not
		 * exist in this version of RISC-OS
		 */
		return 0;
	}

	(void)memset((void *)&regs, 0, sizeof(regs));
	regs.r[0] = 0;
	do {
		/* Get info on the next task */
		regs.r[1] = (int)aiBuffer;
		regs.r[2] = sizeof(aiBuffer);
		e = _kernel_swi(TaskManager_EnumerateTasks, &regs, &regs);
		if (e != NULL) {
			werr(1, "TaskManager_EnumerateTasks error %d: %s",
				e->errnum, e->errmess);
			return EXIT_FAILURE;
		}
		/* Copy the (control terminated) taskname */
		for (iIndex = 0, pcTmp = (const char *)aiBuffer[1];
		     iIndex < elementsof(szTmp);
		     iIndex++, pcTmp++) {
			if (iscntrl(*pcTmp)) {
				szTmp[iIndex] = '\0';
				break;
			}
			szTmp[iIndex] = *pcTmp;
		}
		szTmp[elementsof(szTmp) - 1] = '\0';
		/* Check for AntiWords' entry */
		if (STREQ(szTmp, szTaskname)) {
			return (wimp_t)aiBuffer[0];
		}
	} while (regs.r[0] >= 0);
	/* Not found */
	return 0;
} /* end of tGetTaskHandle */

int
main(int argc, char **argv)
{
	wimp_msgstr	tMsg;
	wimp_t	tTaskHandle;
	int	iArgLen, iVersion;
	char	szCommand[512];

	iVersion = wimpt_init("StartUp");

	if (argc > 1) {
		iArgLen = strlen(argv[1]);
	} else {
		iArgLen = 0;
	}
	if (iArgLen >= sizeof(tMsg.data.dataload.name)) {
		werr(1, "Input filename too long");
		return EXIT_FAILURE;
	}

	tTaskHandle = tGetTaskHandle("AntiWord", iVersion);

	if (tTaskHandle == 0) {
		/* AntiWord is not active */
		strcpy(szCommand, "chain:<AntiWord$Dir>.!AntiWord");
		if (argc > 1) {
			strcat(szCommand, " ");
			strcat(szCommand, argv[1]);
		}
#if defined(DEBUG)
		strcat(szCommand, " ");
		strcat(szCommand, "2><AntiWord$Dir>.Debug");
#endif /* DEBUG */
		system(szCommand);
		/* If we reach here something has gone wrong */
		return EXIT_FAILURE;
	}
	/* AntiWord is active */
	if (argc > 1) {
		/* Send the argument to AntiWord */
		tMsg.hdr.size = MakeFour(sizeof(tMsg) -
					sizeof(tMsg.data.dataload.name) +
					1 + iArgLen);
		tMsg.hdr.your_ref = 0;
		tMsg.hdr.action = wimp_MDATALOAD;
		tMsg.data.dataload.w = -1;
		tMsg.data.dataload.size = 0;
		tMsg.data.dataload.type = FILETYPE_MSWORD;
		strcpy(tMsg.data.dataload.name, argv[1]);
		wimpt_noerr(wimp_sendmessage(wimp_ESEND,
						&tMsg, tTaskHandle));
	} else {
		/* Give an errormessage and end */
		werr(1, "AntiWord is already running");
	}
	return EXIT_SUCCESS;
} /* end of main */
