/******************************************************************************/
/* Mibus's Ency 98 Reader: Reads the Star Trek Encyclopedia (1998 version)    */
/* Copyright (C) 1998 Robert Mibus                                            */
/*                                                                            */
/* This program is free software; you can redistribute it and/or              */
/* modify it under the terms of the GNU General Public License                */
/* as published by the Free Software Foundation; either version 2             */
/* of the License, or (at your option) any later version.                     */
/*                                                                            */
/* This program is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of             */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              */
/* GNU General Public License for more details.                               */
/*                                                                            */
/* You should have received a copy of the GNU General Public License          */
/* along with this program; if not, write to the Free Software                */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */
/*                                                                            */
/* Author:                                                                    */
/*      Email   mibus@hallett-cove.schools.sa.edu.au                          */
/*              beemer@picknowl.com.au                                        */
/*      webpage www.picknowl.com.au/homepages/beemer/                         */
/******************************************************************************/

#ifndef ENCY_H
#define ENCY_H

#define ST_FILE_TYPES 5
#define ST_FILE_ENCY98 0
#define ST_FILE_OMNI1 1
#define ST_FILE_OMNI2 2
#define ST_FILE_TNG1 3
#define ST_FILE_DS91 4

int ency_init (void);
int ency_initted (void);
int ency_finish (void);

struct ency_titles *ency_find_titles (char[]);
struct ency_titles *ency_find_list (char[], int);
struct ency_titles *ency_get_title (char[]);

struct ency_titles *epis_find_titles (char[]);
struct ency_titles *epis_find_list (char[], int);
struct ency_titles *epis_get_title (char[]);

struct ency_titles *chro_find_titles (char[]);
struct ency_titles *chro_find_list (char[], int);
struct ency_titles *chro_get_title (char[]);

struct st_table *st_get_table (void);
struct st_caption *st_get_captions (void);
struct ency_titles *get_title_at (long);

char *st_nice_error (int);
int st_fingerprint (void);

struct st_media *st_get_media (char *);
char *st_format_filename (char *, char *, int);

struct st_ency_formatting
  {
    int bi;
    int words;
    int firstword;
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

struct st_file_info
  {
    char *name;
    char *filename;
    char *data_dir;
    char *pic_dir;
    char *vid_dir;
    int append_char;
    int fingerprint[16];
    long int filesize;
  };

struct st_photo
  {
    char file[8];
    char caption[50];
  };

struct st_media
  {
    struct st_photo photos[5];
  };

#endif
