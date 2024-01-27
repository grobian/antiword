/*
 * xmalloc.c
 * Copyright (C) 1998,1999 A.J. van Os
 *
 * Description:
 * Extended malloc and friends
 */

#include <stdlib.h>
#include <string.h>
#include "antiword.h"

static char *szWarning = "Memory allocation failed, unable to continue";


/*
 * xmalloc - Allocates dynamic memory
 *
 * See malloc(3), but unlike malloc(3) xmalloc does not return in case
 * of error.
 */
void *
xmalloc(size_t tSize)
{
	void	*pvTmp;

	pvTmp = malloc(tSize);
	if (pvTmp == NULL) {
		werr(1, szWarning);
	}
	return pvTmp;
} /* end of xmalloc */

/*
 * xrealloc - Changes the size of a memory object
 *
 * See realloc(3), but unlike realloc(3) xrealloc does not return in case
 * of error.
 */
void *
xrealloc(void *pvArg, size_t tSize)
{
	void	*pvTmp;

	pvTmp = realloc(pvArg, tSize);
	if (pvTmp == NULL) {
		werr(1, szWarning);
	}
	return pvTmp;
} /* end of xrealloc */

/*
 * xstrdup - Duplicate a string
 *
 * See strdup(3), but unlike strdup(3) xstrdup does not return in case
 * of error.
 *
 * NOTE:
 * Does not use strdup(3), because some systems don't have it.
 */
char *
xstrdup(const char *szArg)
{
	char	*szTmp;

	szTmp = xmalloc(strlen(szArg) + 1);
	strcpy(szTmp, szArg);
	return szTmp;
} /* end of xstrdup */

/*
 * xfree - Deallocates dynamic memory
 *
 * See free(3).
 *
 * returns NULL;
 * This makes p=xfree(p) possible, free memory and overwrite the pointer to it.
 */
void *
xfree(void *pvArg)
{
	if (pvArg != NULL) {
		free(pvArg);
	}
	return NULL;
} /* end of xfree */
