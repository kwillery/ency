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

#include <stdio.h>

#define ST_FILE_TYPES st_count_filetypes()

#define ST_FILE_CURR -1
#define ST_FILE_UNKNOWN 254

/* Options to st_find() */
#define ST_SECT_ENCY 0
#define ST_SECT_EPIS 1
#define ST_SECT_CHRO 2
#define ST_OPT_CASE_SENSITIVE 1
#define ST_OPT_RETURN_BODY 2
#define ST_OPT_MATCH_SUBSTRING 4
#define ST_OPT_NO_CACHE 8
#define ST_OPT_NO_FMT 16
#define ST_OPT_TEXTLEN_MAX32 32
#define ST_OPT_SORTEPIS 64
#define ST_OPT_SORTALPHA 128
#define ST_OPT_FT 256

/* enums/structs */
typedef enum
{
	mainfilename,
	data_dir,
	picture_dir,
	video_dir,
	append_char,
	prepend_year,
	append_series
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
	struct st_photo photos[5];
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

/* load the xml file containing file info */
int st_load_xml_file (char *);

/* forces opening of unsupported file */
void st_force_unknown_file (int);

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
int st_find_start (FILE * input);
char st_cleantext (unsigned char c);
void st_cleanstring (char *string);

#endif
