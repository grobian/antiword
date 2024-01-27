/*
 * wordtypes.h
 * Copyright (C) 1998-2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Typedefs for the interpretation of MS Word files
 */

#if !defined(__wordtypes_h)
#define __wordtypes_h 1

#if defined(__riscos)
typedef struct diagram_tag {
	draw_diag	tInfo;
	wimp_w		tMainWindow;
	wimp_w		tScaleWindow;
	long		lXleft;			/* In DrawUnits */
	long		lYtop;			/* In DrawUnits */
	size_t		tMemorySize;
	int		iScaleFactorCurr;	/* In percentage */
	int		iScaleFactorTemp;	/* In percentage */
	char		szFilename[19+1];
} diagram_type;
#else
typedef struct diagram_tag {
	FILE		*pOutFile;
	long		lXleft;			/* In DrawUnits */
	long		lYtop;			/* In DrawUnits */
} diagram_type;
typedef unsigned char	draw_fontref;
#endif /* __riscos */

typedef struct output_tag {
  	char		*szStorage;
	long		lStringWidth;		/* In millipoints */
	size_t		tStorageSize;
	int		iNextFree;
	int		iColor;
	short		sFontsize;
	unsigned char	ucFontstyle;
	draw_fontref	tFontRef;
	struct output_tag	*pPrev;
	struct output_tag	*pNext;
} output_type;

/* Types of encoding */
typedef enum encoding_tag {
	encoding_neutral = 100,
	encoding_iso_8859_1 = 101,
	encoding_iso_8859_2 = 102
} encoding_type;

/* Font translation table entry */
typedef struct font_table_tag {
	unsigned char	ucWordFontnumber;
	unsigned char	ucFontstyle;
	unsigned char	ucInUse;
	char		szWordFontname[65];
	char		szOurFontname[33];
} font_table_type;

/* Options */
typedef enum image_level_tag {
	level_gs_special = 0,
	level_no_images,
	level_ps_2,
	level_ps_3,
	level_default = level_ps_2
} image_level_enum;

typedef struct options_tag {
	int		iParagraphBreak;
	BOOL		bUseOutlineFonts;
	BOOL		bHideHiddenText;
	BOOL		bUseLandscape;
	encoding_type	eEncoding;
	int		iPageHeight;		/* In points */
	int		iPageWidth;		/* In points */
	image_level_enum	eImageLevel;
#if defined(__riscos)
	BOOL		bAutofiletypeAllowed;
	int		iScaleFactor;		/* As a percentage */
#endif /* __riscos */
} options_type;

/* Property Set Storage */
typedef struct pps_tag {
	long	lSize;
	int	iSb;
} pps_type;
typedef struct pps_info_tag {
	pps_type	tWordDocument;
	pps_type	tData;
	pps_type	t0Table;
	pps_type	t1Table;
} pps_info_type;

/* Record of data block information */
typedef struct data_block_tag {
	long	lFileOffset;
	long	lDataOffset;
	size_t	tLength;
} data_block_type;

/* Record of text block information */
typedef struct text_block_tag {
	long	lFileOffset;
	long	lTextOffset;
	size_t	tLength;
	BOOL	bUsesUnicode;	/* This block uses 16 bits per character */
} text_block_type;

/* Record of table-row block information */
typedef struct row_block_tag {
	long	lOffsetStart;
	long	lOffsetEnd;
	int	iColumnWidthSum;			/* In twips */
	short	asColumnWidth[TABLE_COLUMN_MAX+1];	/* In twips */
	unsigned char	ucNumberOfColumns;
} row_block_type;

/* Linked list of style description information */
typedef struct style_block_tag {
	long	lFileOffset;
	BOOL	bInList;
	BOOL	bUnmarked;
	short	sLeftIndent;	/* Left indentation in twips */
	short	sRightIndent;	/* Right indentation in twips */
	unsigned char	ucStyle;
	unsigned char	ucAlignment;
	unsigned char	ucListType;
	unsigned char	ucListLevel;
	unsigned char	ucListCharacter;
} style_block_type;
typedef struct style_desc_tag {
	style_block_type	tInfo;
	struct style_desc_tag	*pNext;
} style_desc_type;

/* Linked list of font description information */
typedef struct font_block_tag {
	long	lFileOffset;
	short		sFontsize;
	unsigned char	ucFontnumber;
	unsigned char	ucFontcolor;
	unsigned char	ucFontstyle;
} font_block_type;
typedef struct font_desc_tag {
	font_block_type	tInfo;
	struct font_desc_tag	*pNext;
} font_desc_type;

/* Linked list of picture description information */
typedef struct picture_block_tag {
	long	lFileOffset;
	long	lFileOffsetPicture;
	long	lPictureOffset;
} picture_block_type;
typedef struct picture_desc_tag {
	picture_block_type	tInfo;
	struct picture_desc_tag	*pNext;
} picture_desc_type;

/* Types of images */
typedef enum imagetype_tag {
	imagetype_is_unknown = 0,
	imagetype_is_emf,
	imagetype_is_wmf,
	imagetype_is_pict,
	imagetype_is_jpeg,
	imagetype_is_png,
	imagetype_is_dib
} imagetype_enum;

/* Types of compression */
typedef enum compression_tag {
	compression_unknown = 0,
	compression_none,
	compression_rle4,
	compression_rle8,
	compression_jpeg,
	compression_zlib
} compression_enum;

/* Image information */
typedef struct imagedata_tag {
	/* The type of the image */
	imagetype_enum	eImageType;
	/* Information from the Word document */
	int	iPosition;
	int	iLength;
	int	iHorizontalSize;	/* Size in points */
	int	iVerticalSize;		/* Size in points */
	double	dHorizontalScalingFactor;
	double	dVerticalScalingFactor;
	/* Information from the image */
	int	iWidth;			/* Size in pixels */
	int	iHeight;		/* Size in pixels */
	int	iComponents;		/* Number of color components */
	int	iBitsPerComponent;	/* Bits per color component */
	BOOL	bAdobe;	/* Image includes Adobe comment marker */
	compression_enum	eCompression;	/* Type of compression */
	BOOL	bColorImage;	/* Is color image */
	int	iColorsUsed;	/* 0 = uses the maximum number of colors */
	unsigned char aucPalette[256][3];	/* RGB palette */
} imagedata_type;

typedef enum text_info_tag {
	text_success,
	text_failure,
	text_no_information
} text_info_enum;

typedef enum list_id_tag {
	text_list,
	footnote_list,
	endnote_list,
	end_of_lists
} list_id_enum;

typedef enum notetype_tag {
	notetype_is_footnote,
	notetype_is_endnote,
	notetype_is_unknown
} notetype_enum;

typedef enum image_info_tag {
	image_no_information,
	image_minimal_information,
	image_full_information
} image_info_enum;

#endif /* __wordtypes_h */
