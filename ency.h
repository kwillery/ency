/*****************************************************************************/
/* star trek ency reader: Reads the Star Trek Encyclopedia (1998 version)    */
/* Also reads the various Omnipedias & Episode guides                        */
/* Copyright (C) 1998 Robert Mibus                                           */
/*                                                                           */
/* This program is free software; you can redistribute it and/or             */
/* modify it under the terms of the GNU General Public License               */
/* as published by the Free Software Foundation; either version 2            */
/* of the License, or (at your option) any later version.                    */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with this program; if not, write to the Free Software               */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */
/*                                                                           */
/* Author:                                                                   */
/*      Email   mibus@bigpond.com                                            */
/*      Webpage http://users.bigpond.com/mibus/                              */
/*****************************************************************************/

#ifndef ENCY_H
#define ENCY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#define ST_FILE_TYPES st_count_filetypes()

#define ST_FILE_CURR -1
#define ST_FILE_UNKNOWN 254

/* Options to st_find() */
/* NB. ST_SECT_EPIS_SORTED is equivalent to
   ST_SECT_EPIS w/ ST_OPT_EPIS_SORTED as an
   option */
#define ST_SECT_ENCY 0
#define ST_SECT_EPIS 1
#define ST_SECT_CHRO 2
#define ST_SECT_EPIS_SORTED 3
#define ST_OPT_CASE_SENSITIVE 1
#define ST_OPT_RETURN_BODY 2
#define ST_OPT_MATCH_SUBSTRING 4
#define ST_OPT_NO_FMT 8
#define ST_OPT_SORTEPIS 16
#define ST_OPT_FT 32
#define ST_OPT_NO_FILEPOS 64

/* Data file types */
#define ST_DFILE_TYPES 4
#define ST_DFILE_UNKNOWN 0
#define ST_DFILE_ENCY 1 /* e.g. Ency99.dxr */
#define ST_DFILE_DATA 2 /* e.g. Data99.cxt */
#define ST_DFILE_PICON 3 /* e.g. Picons.cxt */

/* enums/structs */
typedef enum
{
	mainfilename,
	data_dir,
	picture_dir,
	video_dir,
	append_char,
} st_filename_type;

typedef enum
{
	picture,
	video,
	audio,
	swf
} media_type;

struct st_ency_formatting
{
	int firstword;
	int words;
	int bold;
	int italic;
	int underline;
	struct st_ency_formatting *next;
};

struct ency_titles
{
	char *name;
	char *title;
	char *text;
	struct st_ency_formatting *fmt;
	struct ency_titles *next;
	int err;
	int section;
	int block_id;
	int id;
	long filepos;
	int length; /* N.B. Not the real length! */
	float score;
};

struct st_table
{
	char *title;
	char *fnbase;
	char *audio;
	int section;
	int block_id;
	int id;
	struct st_table *next;
};

struct st_caption
{
	char *fnbasen;
	char *caption;
	struct st_caption *next;
};

struct st_photo
{
	char file[13];
	char caption[50];
};

struct st_media
{
	struct st_photo photos[6];
	struct st_photo video;
	struct st_photo audio;
	struct st_photo swf;
};

/* Initialisation & De-init */
int st_init (void);
int st_finish (void);

/* For controlling what file is opened */
int st_set_filename (char *);
char *st_get_filename (void);
int st_count_filetypes(void);

/* load/unload the rc file containing file info */
int st_load_rc_file (char *);
void st_unload_data(void);

/* For checking a directory to see if a data file is there */
char *st_autofind (int, char *);

/* For getting a description of a file type */
char *st_fileinfo_get_name (int);

/* For media handling */
int st_load_media (void);
int st_loaded_media (void);
void st_unload_media (void);

struct st_media *st_get_media (char *);
char *st_format_filename (char *, char *, media_type);

/* Save a thumbnail or pic to a PPM */
int st_get_thumbnail(char *name, char *file);
int st_get_picture(char *name, char *file, int dfile_type, long width, long height);

/* For the actual searches */
struct ency_titles *st_find (char *, int, int);
struct ency_titles *st_get_title_at (long);
struct ency_titles *st_read_title_at (long, int options);

/* Takes an error # & returns a string */
char *st_nice_error (int);
char *st_autofind (int, char *);

/* dealing w/ structs */
void st_free_entry (struct ency_titles *);
void st_free_entry_tree (struct ency_titles *);
void st_free_entry_and_advance (struct ency_titles **);
void st_free_fmt (struct st_ency_formatting *);
void st_free_fmt_tree (struct st_ency_formatting *);
void st_free_fmt_and_advance (struct st_ency_formatting **);

void st_copy_part_entry (struct ency_titles **, struct ency_titles *);

/* lower-level functions */
char st_cleantext (unsigned char c);
unsigned char *st_cleanstring (unsigned char *string);
void st_ultraclean_string (unsigned char *string);
char *st_lcase (char *mcase);

#ifdef __cplusplus
}
#endif
#endif
