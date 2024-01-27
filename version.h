/*
 * version.h
 * Copyright (C) 1998-2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Version and release information
 */

#if !defined(__version_h)
#define __version_h 1

/* Strings for the info box */
#define PURPOSESTRING	"Display text from MS-Word files"

#if defined(__riscos)
#define AUTHORSTRING	"© 1998-2000 Adri van Os"
#else
#define AUTHORSTRING	"(C) 1998-2000 Adri van Os"
#endif /* __riscos */

#define VERSIONSTRING	"0.31  (08 Dec 2000)"

#if defined(DEBUG)
#define STATUSSTRING	"DEBUG version"
#else
#define STATUSSTRING	"GNU General Public License"
#endif /* DEBUG */

#endif /* __version_h */
