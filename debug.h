/*
 * debug.h
 * Copyright (C) 1998-2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Macro's for debuging.
 */

#if !defined(__debug_h)
#define __debug_h 1

#include <stdio.h>

#if defined(DEBUG)

#define DBG_MSG(t)	(void)fprintf(stderr,\
				"%s[%3d]: %.240s\n",\
				__FILE__, __LINE__, t)

#define DBG_DEC(n)	(void)fprintf(stderr,\
				"%s[%3d]: "#n" = %ld\n",\
				__FILE__, __LINE__, (long)(n))

#define DBG_HEX(n)	(void)fprintf(stderr,\
				"%s[%3d]: "#n" = 0x%02lx\n",\
				__FILE__, __LINE__, (unsigned long)(n))

#define DBG_FLT(n)	(void)fprintf(stderr,\
				"%s[%3d]: "#n" = %.3f\n",\
				__FILE__, __LINE__, (double)(n))

#define DBG_PRINT_BLOCK(b,l)	vPrintBlock(__FILE__, __LINE__,b,l)

#define DBG_UNICODE(t)		vPrintUnicode(__FILE__, __LINE__,t)

#define DBG_MSG_C(c,t)	do { if (c) DBG_MSG(t); } while(0)
#define DBG_DEC_C(c,n)	do { if (c) DBG_DEC(n); } while(0)
#define DBG_HEX_C(c,n)	do { if (c) DBG_HEX(n); } while(0)
#define DBG_FLT_C(c,n)	do { if (c) DBG_FLT(n); } while(0)

#else

#define DBG_MSG(t)		/* EMPTY */
#define DBG_DEC(n)		/* EMPTY */
#define DBG_HEX(n)		/* EMPTY */
#define DBG_FLT(n)		/* EMPTY */
#define DBG_PRINT_BLOCK(b,l)	/* EMPTY */
#define DBG_UNICODE(t)		/* EMPTY */

#define DBG_MSG_C(c,t)		/* EMPTY */
#define DBG_DEC_C(c,n)		/* EMPTY */
#define DBG_HEX_C(c,n)		/* EMPTY */
#define DBG_FLT_C(c,n)		/* EMPTY */

#endif /* DEBUG */

#define NO_DBG_MSG(t)		/* EMPTY */
#define NO_DBG_DEC(n)		/* EMPTY */
#define NO_DBG_HEX(n)		/* EMPTY */
#define NO_DBG_FLT(n)		/* EMPTY */
#define NO_DBG_PRINT_BLOCK(b,l)	/* EMPTY */
#define NO_DBG_UNICODE(t)	/* EMPTY */

#define NO_DBG_MSG_C(c,t)	/* EMPTY */
#define NO_DBG_DEC_C(c,n)	/* EMPTY */
#define NO_DBG_HEX_C(c,n)	/* EMPTY */
#define NO_DBG_FLT_C(c,n)	/* EMPTY */

#endif /* !__debug_h */
