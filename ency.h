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

#define ST_FILE_TYPES 5
#define ST_FILE_ENCY98 0
#define ST_FILE_OMNI1 1
#define ST_FILE_OMNI2 2
#define ST_FILE_TNG1 3
#define ST_FILE_DS91 4

#define ST_FILE_CURR -1
#define ST_FILE_UNKNOWN 254

/* Options to st_find() */
#define ST_SECT_ENCY 0
#define ST_SECT_EPIS 1
#define ST_SECT_CHRO 2
#define ST_OPT_CASE_SENSITIVE 1
#define ST_OPT_RETURN_BODY 2
#define ST_OPT_MATCH_SUBSTRING 4

/* enums/structs */
typedef enum
{
	mainfilename,
	data,
	picture,
	video,
	append_char
} st_filename_type;

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
	char *title;
	char *text;
	struct st_ency_formatting *fmt;
	struct ency_titles *next;
	int err;
	long filepos;
};

struct st_table
{
	char *title;
	char *fnbase;
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
	char file[8];
	char caption[50];
};

struct st_media
{
	struct st_photo photos[5];
	struct st_photo video;
};

struct st_part
{
	long start;
	long count;
};

/* Initialisation & De-init */
int st_init (void);
int st_finish (void);

/* For controlling what file is opened */
int st_set_filename (char *);
char *st_get_filename (void);

/* forces opening of unsupported file */
void st_force_unknown_file (int);

/* For checking a directory to see if a data file is there */
char *st_autofind (int, char *);

/* For getting a description of a file type */
char *st_fileinfo_get_name (int);

/* For media handling */
int st_load_media (void);
int st_loaded_media (void);
int st_unload_media (void);

struct st_media *st_get_media (char *);
char *st_format_filename (char *, char *, int);

/* For the actual searches */
struct ency_titles *st_find (char *, int, int);
struct ency_titles *ency_find_list (char[], int);
struct ency_titles *epis_find_list (char[], int);
struct ency_titles *chro_find_list (char[], int);
struct ency_titles *st_get_title_at (long);

/* Takes an error # & returns a string */
char *st_nice_error (int);
int st_fingerprint (void);
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

#endif
