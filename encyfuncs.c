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
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.*/
/*                                                                           */
/* Author:                                                                   */
/*      Email   mibus@bigpond.com                                            */
/*      Webpage http://users.bigpond.com/mibus/                              */
/*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#define __USE_ISOC99 /* For isblank() in ctype.h */
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "ency.h"
#include "encyfuncs.h"
#include "data.h"

#define free(A) {if (A) free (A); A=NULL;}

static char *ency_filename = NULL;

static int st_file_type = 0;

struct st_wl
{
	int word;
	struct st_wl *next;
};

struct st_ftlist
{
	int section;
	char *word;
	char *fnbase;
	struct st_wl *words;
	struct st_ftlist *next;
};

struct st_ftlist *ftlist=NULL;

/* internal */
#define ST_SECT_PTBL 10
#define ST_SECT_VTBL 11
#define ST_SECT_PCPT 12
#define ST_SECT_VCPT 13

/* the attribs table */
static struct st_table *entrylist_head=NULL;

/* for pictures */
static struct st_table *st_ptbls = NULL;
static struct st_caption *st_pcpts = NULL;
static struct st_caption *st_pcpts_quick = NULL;

/* for videos */
static struct st_table *st_vtbls = NULL;
static struct st_caption *st_vcpts = NULL;

/* cache */
static struct ency_titles *cache = NULL;
static struct ency_titles *cache_last = NULL;
static struct ency_titles *cache_quick = NULL;

static void st_clear_cache (void);
static void st_clear_entry_list (void);

/* init/de-init stuff */
int st_init (void)
{
	if (count_files() == 0) /* If there isnt an RC file already loaded... */
		load_rc_file_info(NULL);
	if (!count_files())
		fprintf (stderr, "ency: Warning - could not load RC file\n");
	st_file_type = st_fingerprint ();
	return (st_file_type >= 254 ? 0 : 1);
}

int st_finish (void)
{
	if (st_loaded_media ())
		st_unload_media ();
	if (ency_filename)
		free (ency_filename);
	st_clear_cache ();
	st_data_clear();

	return (1);
}

/* useful bits */
static int safe_strlen (const char *t)
{
	if (t)
		return strlen (t);
	else 
		return 0;
}

/* change characters to 'real' ones.
(eg. smart dbl quotes -> normal dbl quotes */
char st_cleantext (unsigned char c)
{
	switch (c)
	{
	case 13:
		return ('\n');
	case 0x88:
		return ('a');
	case 0x8E:
		return ('e');
	case 0x8F:
		return ('e');
	case 0x92:
		return ('\'');
	case 0x93:
		return ('\"');
	case 0x94:
		return ('\"');
	case 0x95:
		return ('*');
	case 0x96:
		return ('-');
	case 0x97:
		return ('-');
	case 0xA5:
		return ('*');
	case 0xD0:
		return ('-');
	case 0xD1:
		return ('-');
	case 0xD2:
		return ('\"');
	case 0xD3:
		return ('\"');
	case 0xD4:
		return ('\'');
	case 0xD5:
		return ('\'');
	case 0xE0:
		return ('a');
	case 0xE9:
		return ('e');
	default:
		return (c);
	}
}

void st_cleanstring (char *string)
{
	while ((*string = st_cleantext (*string)))
		string++;
}

static char *st_lcase (char *mcase)
{
	char *lcase = NULL;
	int i = 0;
	int length = 0;

	if (!mcase)
		return NULL;

	length = strlen (mcase) + 1;
	lcase = (char *) malloc (length);

	for (i = 0; i < length; i++)
		lcase[i] = tolower (mcase[i]);

	return (lcase);
}

int st_count_filetypes(void)
{
	return count_files();
}

void st_unload_data(void)
{
	st_data_clear();
}

/* struct manipulation */
void st_free_fmt (struct st_ency_formatting *fmt)
{
	if (fmt)
		free (fmt);
}

void st_free_fmt_tree (struct st_ency_formatting *fmt)
{
	struct st_ency_formatting *last = NULL;
	while (fmt)
	{
		last = fmt;
		fmt = fmt->next;
		free (last);
	}
}

void st_free_fmt_and_advance (struct st_ency_formatting **fmt)
{
	struct st_ency_formatting *last;
	if (fmt)
		if (*fmt)
		{
			last = *fmt;
			*fmt = (*fmt)->next;
			free (last);
		}
}

static struct ency_titles *st_new_entry (void)
{
	struct ency_titles *entry=NULL;

	entry = (struct ency_titles *) malloc (sizeof (struct ency_titles));

	entry->name = NULL;
	entry->title = NULL;
	entry->text = NULL;
	entry->fmt = NULL;
	entry->next = NULL;
	entry->err = 0;
	entry->section = 0;
	entry->block_id = entry->id = 0;
	entry->filepos = 0;
	entry->length = 0;
	entry->score = 0;

	return entry;
}

void st_free_entry (struct ency_titles *entry)
{
	if (entry)
	{
		if (entry->title)
			free (entry->title);
		if (entry->text)
			free (entry->text);
		if (entry->fmt)
			st_free_fmt_tree (entry->fmt);
		if (entry->name)
			free (entry->name);
		free (entry);
	}
}

void st_free_entry_tree (struct ency_titles *entry)
{
	struct ency_titles *last;

	while (entry)
	{
		last = entry;
		entry = entry->next;
		st_free_entry (last);
	}
}

void st_free_entry_and_advance (struct ency_titles **entry)
{
	struct ency_titles *last;

	last = *entry;
	*entry = (*entry)->next;
	st_free_entry (last);
}

void st_copy_part_entry (struct ency_titles **to, struct ency_titles *from)
{
	if (from)
	{
		*to = malloc (sizeof (struct ency_titles));
		if (*to)
		{
			(*to)->title = strdup (from->title);
			(*to)->filepos = from->filepos;
			(*to)->fmt = NULL;
			(*to)->text = NULL;
			(*to)->next = NULL;
			(*to)->err = 0;
		}
	}
}

static struct st_table *st_new_table ()
{
	struct st_table *tbl=NULL;

	tbl = (struct st_table *) malloc (sizeof (struct st_table));

	if (!tbl)
		return NULL;

	tbl->title = NULL;
	tbl->fnbase = NULL;
	tbl->audio = NULL;
	tbl->section = 0;
	tbl->block_id = 0;
	tbl->id = 0;
	tbl->next = NULL;

	return tbl;
}

/* cache stuff */
static void st_clear_cache ()
{
	st_free_entry_tree (cache);
	cache = NULL;
	cache_last = NULL;
	cache_quick = NULL;
}

/* clean out the attrib list - we are either
	- finishing, or, 
	- done with this file */
static void st_clear_entry_list ()
{
	struct st_table *tbl;

	while (entrylist_head)
	{
		tbl = entrylist_head;
		entrylist_head = entrylist_head->next;

		if (tbl->title)
			free (tbl->title);
		if (tbl->fnbase)
			free (tbl->fnbase);
		if (tbl->audio)
			free (tbl->audio);

		free (tbl);
	}

	st_vtbls = NULL;
}

/* try and set the filename libency will use
 * NB this will fail & return 0 if the file
 * can't be identified by st_fingerprint() */
int st_set_filename (char *filename)
{
	int type;

	if (ency_filename)
		free (ency_filename);

	ency_filename = strdup (filename);

	if (ency_filename)
	{
		type = st_fingerprint();
		st_file_type = type;
		if ((type >= 0) && (type < ST_FILE_TYPES))
		{
			st_clear_cache ();
			st_clear_entry_list ();
			return (1);
		}
		else
		{
			free (ency_filename);
			ency_filename = NULL;
			return (0);
		}
	}
	return (0);
}

char *st_get_filename (void)
{
	return (ency_filename);
}

int st_load_rc_file (char *filename)
{
	st_data_clear ();
	return (load_rc_file_info (filename));
}

/* get the descriptive name of the file
 * currently loaded (eg. "Encyclopedia 3.0"). */
char *st_fileinfo_get_name (int file_type)
{
	if (file_type > ST_FILE_TYPES)
		return "Unknown";

	if (file_type == -1)
		file_type = st_file_type;

	return get_name_of_file (file_type);
}

/* try to open the currently selected file, and
 * seek to 'start'. if ency_filename is NULL, it will
 * try to find an encyclopedia to load.
 * It returns 0 for failure, the file pointer otherwise */
FILE *curr_open (long start)
{
	FILE *inp;
	int i = 0;
	char *temp_fn = NULL;

	if (ency_filename == NULL)
	{
		temp_fn = getenv ("ENCY_FILENAME");
		if (temp_fn == NULL)
		{
			for (i=0;i<ST_FILE_TYPES;i++)
			{
				if ((temp_fn = st_autofind (i,".")))
				{
					st_set_filename (temp_fn);
					free (temp_fn);
					break;
				}
			}
		}
		else
			st_set_filename (temp_fn);

		if (ency_filename == NULL) return (0);
	}
	inp = fopen (ency_filename, "rb");

	i = 0;
	if (inp)
       	{
		i = fseek (inp, start, SEEK_SET);
		if (i)
		{
			fclose (inp);
			inp = 0;
		}
	}

	return (inp);
}

/* Look for a given encyclopedia version in a directory.
 * Try lowering the case of the directory, etc. etc.
 * This will also work if 'base_dir' is the actual
 * filename */
char *st_autofind (int st_file_version, char *base_dir)
{
	char *test_filename = NULL;
	char *ency_fn_backup = NULL;
	const char *datadir = NULL, *filename = NULL;
	char *lc_data_dir = NULL, *lc_filename = NULL;

	if ((st_file_version < ST_FILE_TYPES) && (st_file_version >= 0))
	{

		datadir = st_fileinfo_get_data (st_file_version,data_dir);
		filename = st_fileinfo_get_data (st_file_version,mainfilename);

		lc_data_dir = st_lcase ((char *)datadir);
		lc_filename = st_lcase ((char *)filename);

		ency_fn_backup = ency_filename;
		test_filename = malloc (safe_strlen (base_dir) + safe_strlen (datadir) + safe_strlen (filename) + 3);
		ency_filename = test_filename;

		sprintf (test_filename, "%s", base_dir);
		if (st_fingerprint () == st_file_version)
		{
			free (lc_data_dir);
			free (lc_filename);
			ency_filename = ency_fn_backup;
			return (test_filename);
		}

		sprintf (test_filename, "%s/%s", base_dir, filename);
		if (st_fingerprint () == st_file_version)
		{
			free (lc_data_dir);
			free (lc_filename);
			ency_filename = ency_fn_backup;
			return (test_filename);
		}

		sprintf (test_filename, "%s/%s/%s", base_dir, datadir, filename);
		if (st_fingerprint () == st_file_version)
		{
			free (lc_data_dir);
			free (lc_filename);
			ency_filename = ency_fn_backup;
			return (test_filename);
		}

		sprintf (test_filename, "%s/%s", base_dir, lc_filename);
		if (st_fingerprint () == st_file_version)
		{
			free (lc_data_dir);
			free (lc_filename);
			ency_filename = ency_fn_backup;
			return (test_filename);
		}

		sprintf (test_filename, "%s/%s/%s", base_dir, lc_data_dir, lc_filename);
		if (st_fingerprint () == st_file_version)
		{
			free (lc_data_dir);
			free (lc_filename);
			ency_filename = ency_fn_backup;
			return (test_filename);
		}

		sprintf (test_filename, "%s/%s/%s", base_dir, datadir, lc_filename);
		if (st_fingerprint () == st_file_version)
		{
			free (lc_data_dir);
			free (lc_filename);
			ency_filename = ency_fn_backup;
			return (test_filename);
		}

		sprintf (test_filename, "%s/%s/%s", base_dir, lc_data_dir, lc_filename);
		if (st_fingerprint () == st_file_version)
		{
			free (lc_data_dir);
			free (lc_filename);
			ency_filename = ency_fn_backup;
			return (test_filename);
		}

		free (lc_filename);
		free (lc_data_dir);
	}
	return (NULL);
}

/* Try to find the beginning of the next STXT block.
 * This is very simple and could be easily
 * fooled as it doesn't actually use the file's block
 * format. (It just looks for "STXT" or "TXTS").*/
static void find_next_stxt_block (FILE *inp)
{
	char d[5]="    ";
	
	while (!feof (inp))
	{
		d[3] = getc (inp);

		if ((!strcmp (d, "STXT")) || (!strcmp (d, "TXTS")))
			return;
		d[0] = d[1];
		d[1] = d[2];
		d[2] = d[3];
	}
}

/* Read in a small block of text.
 * It handles quoted text properly and
 * everything! :-D */
static inline char *get_text_from_file (FILE *inp)
{
	char c, *text;
	int size;
	long start_pos;
	int quotes=0;

	if (!inp)
		return NULL;
	if (feof (inp))
		return NULL;

	start_pos = ftell (inp);
	while (!feof (inp))
	{
		c = getc (inp);
		if (c == '\"')
		{
			if (quotes)
				break;
			else
			{
				quotes = 1;
				start_pos++;
			}
		} else if (((c == ':') || (c == ',') || (c == ']')) && !quotes)
			break;
	}
	size = ftell (inp) - start_pos;
	if (!size)
		return NULL;
	text = malloc (sizeof (char) * size);
	if (!text)
		return NULL;
	fseek (inp, start_pos, SEEK_SET);
	fread (text, size, sizeof (char), inp);
	text[size-1] = 0;

	return text;
}

/* this is the same as get_text_from_file(), but
 * has a maximum length and simpler error checking */
static inline char *get_text_from_file_max_length (FILE *inp, int length)
{
	char c, text[length+1];
	char *t;
	int quotes=0;

	t = text;
	if ((c = getc(inp)) == '\"')
		quotes = 1;
	else
		ungetc (c, inp);

	while (1)
	{
		c = getc (inp);

		if (c == '\"')
		{
			if (quotes)
				break;
		} else if (((c == ':') || (c == ',') || (c == ']')) && !quotes)
			break;

		*t++ = c;
	}

	*t = 0;

	return strdup (text);
}

/* Reads an attrib table from 'inp' and appends
 * it to 'root' */
static struct st_table *read_table (FILE *inp, struct st_table *root)
{
	struct st_table *root_tbl = NULL;
	struct st_table *curr_tbl = NULL;
	struct st_table *last_tbl = NULL;
	char c;

	c = getc (inp);

	if (c == 0)
		return root;

	if (ungetc (getc (inp), inp) == ':')
		return root;

	if ((c != '[') && (c != '\"'))
		return root;

	if (c != '[')
		ungetc (c, inp);

	if (root)
	{
		root_tbl = root;
		last_tbl = root;
		while (last_tbl->next)
			last_tbl = last_tbl->next;
	}

	while (!feof(inp))
	{
		curr_tbl = st_new_table ();

		if (curr_tbl == NULL)
			return NULL;

		curr_tbl->fnbase = get_text_from_file_max_length (inp, 10);

		do {
			c = getc (inp);
		} while ((c == ' ') || (c == ':'));
		ungetc (c, inp);

		curr_tbl->title = get_text_from_file (inp);

		if (last_tbl)
			last_tbl->next = curr_tbl;
		else
			root_tbl = curr_tbl;

		last_tbl = curr_tbl;
		curr_tbl = NULL;

		do {
			c = getc (inp);
		} while ((c == ' ') || (c == ','));
		ungetc (c, inp);
		if (c == ']' || c == 0)
			break;
	}
	return root_tbl;
}

/* Loads all of the photo/picture lookup tables
 * for the current file */
static struct st_table *st_get_table ()
{
	FILE *inp;
	int i;
	int count = 0;
	struct st_table *root_tbl = NULL;
	struct st_part *part;

	while ((part = get_part (st_file_type, ST_SECT_PTBL, 0, count, 0)))
	{
		if ((inp = curr_open (part->start)) == 0)
			return (NULL);
		else
		{

			for (i = 0; i < part->count; i++)
			{
				if (i)
				{
					find_next_stxt_block (inp);
					fseek (inp, 16, SEEK_CUR);
				}
				root_tbl = read_table (inp, root_tbl);
			}
		}
		fclose (inp);
		count++;
	}
	return (root_tbl);
}


/* Reads a caption table from 'inp', appends to 'root' */
static struct st_caption *read_captions (FILE *inp, struct st_caption *root)
{
	struct st_caption *root_cpt = NULL, *curr_cpt = NULL, *last_cpt = NULL;
	char c = 0;

	c = getc (inp);

	if (c == 0)
		return root;

	if (ungetc (getc (inp), inp) == ':')
		return root;

	if (c != '[')
		ungetc (c, inp);

	if (root)
	{
		root_cpt = root;
		last_cpt = root;
		while (last_cpt->next)
			last_cpt = last_cpt->next;
	}

	while (!feof (inp))
	{	/* main loop */
		curr_cpt = (struct st_caption *) malloc (sizeof (struct st_caption));

		if (curr_cpt == NULL)
			return (NULL);

		curr_cpt->fnbasen = get_text_from_file_max_length (inp, 10);

		do {
			c = getc (inp);
		} while ((c == ' ') || (c == ':'));
		ungetc (c, inp);

		curr_cpt->caption = get_text_from_file (inp);
		st_cleanstring (curr_cpt->caption);

		curr_cpt->next = NULL;
		if (last_cpt)
			last_cpt->next = curr_cpt;
		else
			root_cpt = curr_cpt;
		last_cpt = curr_cpt;
		curr_cpt = NULL;

		do {
			c = getc (inp);
		} while ((c == ' ') || (c == ','));
		ungetc (c, inp);

		if (ungetc (getc (inp), inp) == ']')
			break;

	}	/* end main loop */
	return (root_cpt);
}

/* Loads all of the media caption lookup tables
 * for the current file. 'section' is either
 * ST_SECT_PCPT or ST_SECT_VCPT for photo or video
 * captions respectively */
static struct st_caption *st_get_captions (int section)
{
	FILE *inp;
	int i;
	int count = 0;
	struct st_caption *root_cpt = NULL;
	struct st_part *part;

	while ((part = get_part (st_file_type, section, 0, count, 0)))
	{
		if ((inp = curr_open (part->start)) == 0)
			return (NULL);
		else
			for (i = 0; i < part->count; i++)
				root_cpt = read_captions (inp, root_cpt);

		count++;
		fclose (inp);
	}
	return (root_cpt);
}

/* Loads 'count' attribs tables in 'inp'. The
 * section is given so it can be put into the
 * entry's data. */
static struct st_table *read_attribs_table (FILE *inp, int section, int count)
{
	struct st_table *root_tbl = NULL, *curr_tbl = NULL, *last_tbl = NULL;
	char c;
	int i;
	int in_quote;
	int level, commas;

	for (i = 0; i < count; i++)
	{
		if (i)
		{
			find_next_stxt_block (inp);
			fseek (inp, 16, SEEK_CUR);
		}

		c = getc (inp);

		if (c == 0)
			return NULL;

		if (ungetc (getc (inp), inp) == ']')
			return NULL;

		if (ungetc (getc (inp), inp) == ':')
			return NULL;

		if (c != '[')
			ungetc (c, inp);

		while (!feof (inp))
		{	/* main loop */

			curr_tbl = st_new_table ();

			if (curr_tbl == NULL)
				return (NULL);

			curr_tbl->section = section;

			curr_tbl->title = get_text_from_file (inp);

			c = getc (inp);
			if (c == ':')
			{
				c = getc (inp);
				c = getc (inp);
				level = 1;
				commas = 0;
				in_quote = 0;
				while (level)
				{
					c = getc (inp);
					if (!in_quote)
					{
						if (c == '[')
							level++;
						if (c == ']')
							level--;
						if ((level == 1) && (c == ','))
							commas++;
						/* commas == 4 is video fnbase, 5 is aif FN */
						if ((c == '\"') && ((commas == 4) || (commas == 5)))
						{
							ungetc (c, inp);
							if (commas == 4)
								curr_tbl->fnbase = get_text_from_file_max_length (inp, 20);
							else
								curr_tbl->audio = get_text_from_file_max_length (inp, 20);
							c=0;
						}
						if (commas == 8) /* Block ID */
							fscanf (inp, "%d", &(curr_tbl->block_id));
						if (commas == 9) /* Entry ID in block*/
							fscanf (inp, "%d", &(curr_tbl->id));
					}
					if (c == '\"')
						in_quote = !in_quote;
				}
			}

			if (curr_tbl->fnbase)
				st_cleanstring (curr_tbl->fnbase);
			st_cleanstring (curr_tbl->title);
			curr_tbl->next = NULL;
			if (last_tbl)
				last_tbl->next = curr_tbl;
			else
				root_tbl = curr_tbl;
			last_tbl = curr_tbl;
			curr_tbl = NULL;

			do {
				c = getc (inp);
			} while ((c == ' ') || (c == ','));
			ungetc (c, inp);

			if (ungetc (getc (inp), inp) == ']')
				break;

		}
	}
	return root_tbl;
}

/* Load the attribs tables for a given section */
static struct st_table *st_get_video_table (int section)
{
	FILE *inp;
	struct st_table *root_tbl = NULL, *curr_tbl=NULL;
	struct st_part *part;
	int count=0;

	while ((part = get_part (st_file_type, ST_BLOCK_ATTRIB, section, count, 0)))
	{
		if ((inp = curr_open (part->start)) == 0)
			return (NULL);
		else
		{
			if (curr_tbl)
			{
				while (curr_tbl->next)
					curr_tbl = curr_tbl->next;

				curr_tbl->next = read_attribs_table (inp, part->section, part->count);
			}
			else
				curr_tbl = root_tbl = read_attribs_table (inp, part->section, part->count);

			fclose (inp);
			count++;
		}
	}
	return (root_tbl);
}

/* Load all of the bits & pieces needed for
 * st_get_media() to work properly. */
int st_load_media (void)
{
	/*
	 * Get the table & captions (if we don't already have them)
	 */
	if (!st_ptbls)
		st_ptbls = st_get_table ();
	if (!st_pcpts)
		st_pcpts = st_get_captions (ST_SECT_PCPT);
	/* Note we assume here that
	 * - the entry list has been loaded
	 * - that it contains all of the entries
	 *   that we want.
	 * This *should* be OK as long as a search has loaded
	 * the relevant part of the entry list. (As it would
	 * have needed to to search it). But if the entry list
	 * is free()'d in between the search and now, there
	 * will be no videos in the media list.
	 * (10 lines of comments for two of code! :-) */
	if (!st_vtbls)
		st_vtbls = entrylist_head;
	if (!st_vcpts)
		st_vcpts = st_get_captions (ST_SECT_VCPT);

	return (1);
}

/* Have we already run st_load_media()? */
int st_loaded_media (void)
{
	if (((st_pcpts) && (st_ptbls)) || ((st_vtbls) && (st_vcpts)))
		return (1);
	else
		return (0);
}

/* Unload all of the photo tables, the photo captions,
 * and the video captions. We don't unload the video
 * tables 'cos it's really just the entrylist */
void st_unload_media (void)
{
	static struct st_table *st_oldptbls = NULL;
	static struct st_caption *st_oldpcpts = NULL;
	static struct st_caption *st_oldvcpts = NULL;

/* Free the caption & table info for the pictures */
	while (st_pcpts)
	{
		st_oldpcpts = st_pcpts;
		st_pcpts = st_pcpts->next;
		if (st_oldpcpts)
		{
			if (st_oldpcpts->fnbasen)
				free (st_oldpcpts->fnbasen);
			if (st_oldpcpts->caption)
				free (st_oldpcpts->caption);
			free (st_oldpcpts);
		}
	}

	while (st_ptbls)
	{
		st_oldptbls = st_ptbls;
		st_ptbls = st_ptbls->next;
		if (st_oldptbls)
		{
			if (st_oldptbls->fnbase)
				free (st_oldptbls->fnbase);
			if (st_oldptbls->title)
				free (st_oldptbls->title);
			free (st_oldptbls);
		}
	}

/* & for the videos */
	while (st_vcpts)
	{
		st_oldvcpts = st_vcpts;
		st_vcpts = st_vcpts->next;
		if (st_oldvcpts)
		{
			if (st_oldvcpts->fnbasen)
				free (st_oldvcpts->fnbasen);
			if (st_oldvcpts->caption)
				free (st_oldvcpts->caption);
			free (st_oldvcpts);
		}
	}

	/* clean up... */
	st_ptbls = NULL;
	st_pcpts = NULL;
	st_pcpts_quick = NULL;
	st_vtbls = NULL;
	st_vcpts = NULL;
}


/* Compare a title against a search string, taking
 * into account any options set. Sort of an advanced
 * strcmp() */
static int check_match (char *search_string, char *title, int options)
{
	int found = 0;

	if (!(options & ST_OPT_CASE_SENSITIVE))
	{
		title = st_lcase (title);
		search_string = st_lcase (search_string);

	}

	if ((options & ST_OPT_MATCH_SUBSTRING) && strstr (title, search_string))
		found = 1;
	else if (!strcmp (title, search_string))
		found = 1;

	if (!(options & ST_OPT_CASE_SENSITIVE))
	{
		free (title);
		free (search_string);
	}

	return found;
}

/* Read the formatting info from 'inp' that lies
 * at the start of an entry.*/
static struct st_ency_formatting *st_return_fmt (FILE *inp)
{
	struct st_ency_formatting *root_fmt = NULL, *last_fmt = NULL, *curr_fmt = NULL;
	char c = 0;
	int i = 0;
	char fmt[8]="";

	while (c != '@')
	{
		curr_fmt = (struct st_ency_formatting *) malloc (sizeof (struct st_ency_formatting));
		curr_fmt->next = NULL;
		curr_fmt->bold = 0;
		curr_fmt->italic = 0;
		curr_fmt->underline = 0;

		if (curr_fmt == NULL)
		{
			printf ("Memory allocation failed\n");
			exit (1);
		}

		fscanf (inp,"%d : [ %d , %[#buiBUI] ] ",&curr_fmt->firstword,&curr_fmt->words,fmt);

		c=getc(inp);

		for (i=0;fmt[i];i++)
		{
			switch (fmt[i])
			{
			case 'b':
			case 'B':
				curr_fmt->bold = 1;
				break;
			case 'u':
			case 'U':
				curr_fmt->underline = 1;
				break;
			case 'i':
			case 'I':
				curr_fmt->italic = 1;
				break;
			}
		}


		if (!root_fmt)
			root_fmt = curr_fmt;
		curr_fmt->next = NULL;
		if (last_fmt != NULL)
			last_fmt->next = curr_fmt;
		last_fmt = curr_fmt;
		curr_fmt = NULL;
	}

	c = getc (inp);

	return (root_fmt);
}

/* Read the body of an entry in from 'inp'. We
 * don't know how long it will be before we start,
 * so we run over it to get the length first. Then
 * we fseek() back to the start, and finally read
 * the text in. */
static char *st_return_text (FILE *inp)
{
	long text_starts_at = 0;
	int text_size = 1;
	int bye = 0, i;
	char c = 0;
	char old_c = 0;
	char *temp_text = NULL;

	text_starts_at = ftell (inp);

	while (!bye && !feof (inp))
	{
		c = getc (inp);
		if (c == 0)
			bye = 1;
		else if ((old_c == 0x0D) && (c == 0x7E))
		{
			c = ungetc (c, inp);
			text_size--;
			bye = 1;
		}
		else
		{
			old_c = c;
			text_size++;
		}
	}

	temp_text = malloc (text_size + 1);
	fseek (inp, text_starts_at, SEEK_SET);
	fread (temp_text, 1, text_size, inp);

	for (i = 0; i < text_size; i++)
		temp_text[i] = st_cleantext (temp_text[i]);
	temp_text[i] = 0;

	return (temp_text);
}

/* Read an entry's title in from 'inp' */
static char *st_return_title (FILE *inp)
{
	char c;
	char *title = NULL;
	int title_size = 0;

/* TODO: make this 70 autodetected ala st_return_text */
	title = malloc (70);

	while ((c = st_cleantext (getc (inp))) != '@')
	{
		if (title == NULL)
		{
			printf ("Oh, ^$#%%!\n");
			return (NULL);
		}
		title[title_size++] = c;
	}

	title[title_size] = 0;
	return (title);
}

/* This formats ency_titles' ->err to be a
 * string. (->err sucks, BTW, and will
 * probably disappear as soon as I have
 * something better . :-) */
char *st_nice_error (int error_no)
{
	switch (error_no)
	{
	case 1:
		return ("The data file was not found");
		break;
	case 2:
		return ("Memory allocation error");
		break;
	default:
		return ("An error has occurred");
		break;
	}
}

/* This creates an empty (bogus) entry, and
 * puts an error number into it.
 * (This also sucks, and should also disappear */
static struct ency_titles *st_title_error (int error_no)
{

	struct ency_titles *return_error = NULL;
	return_error = st_new_entry ();

	if (return_error == NULL)
	{
		return (NULL);
	}
	return_error->err = error_no;

	return (return_error);
}

/* Check to see if we have loaded a given
 * section into the entry list.
 * NB. if the section isn't present, it will
 * return 0, but that shouldn't matter */
static int entry_list_has_section (section)
{
	struct st_table *lst;
	lst = entrylist_head;

	while (lst)
		if (lst->section == section)
			return 1;
		else
			lst = lst->next;
	return 0;
}

/* Load just one section's entry list. Use
 * section = -1 to load every section. */
static int load_entry_list (int section)
{
	struct st_table *lst=entrylist_head;

	if (!entrylist_head)
	{
		entrylist_head = st_get_video_table (section);
		st_vtbls = entrylist_head;
	}
	else
	{
		while (lst->next)
			lst = lst->next;

		lst->next = st_get_video_table (section);
	}


	return 1;
}


/* Get the fnbase from a list when given
 * the entry's title. This checks the
 * exception list first, so it can be
 * overridden from the rcfile */
static char *get_fnbase (struct st_table *tbl, char *title)
{
	char *exception;
	if (!title)
		return NULL;
	if ((exception = get_exception (st_file_type, "LU title", title)))
		title = exception;
	while (tbl)
	{
		if ((!strcmp (tbl->title, title)))
			return tbl->fnbase;
		tbl = tbl->next;
	}
	return NULL;
}

/* Get the title from a list when given
 * the entry's fnbase. This checks the
 * exception list first, so it can be
 * overridden from the rcfile */
static char *get_title (struct st_table *tbl, char *fnbase)
{
	char *exception;
	if (!fnbase)
		return NULL;
	if ((exception = get_exception (st_file_type, "LU fnbase", fnbase)))
		fnbase = exception;
	while (tbl)
	{
		if (tbl->fnbase)
			if (!strcasecmp (tbl->fnbase, fnbase))
				return tbl->title;
		tbl = tbl->next;
	}

	return NULL;
}

/* Get the table entry from a list when given
 * the entry's fnbase. This checks the
 * exception list first, so it can be
 * overridden from the rcfile */
static struct st_table *get_table_entry_by_fnbase (struct st_table *tbl, char *fnbase)
{
	char *exception;
	if (!fnbase)
		return NULL;
	if ((exception = get_exception (st_file_type, "LU fnbase", fnbase)))
		fnbase = exception;
	while (tbl)
	{
		if (!strcasecmp (tbl->fnbase, fnbase))
			return tbl;
		tbl = tbl->next;
	}
	return NULL;
}

/* Get the table entry from a list when given
 * the entry's title. This checks the
 * exception list first, so it can be
 * overridden from the rcfile */
static struct st_table *get_table_entry_by_title (struct st_table *tbl, char *title)
{
	char *exception;

	if (!title)
		return NULL;
	if ((exception = get_exception (st_file_type, "LU title", title)))
		title = exception;
	while (tbl)
	{
		if (!strcasecmp (tbl->title, title))
			return tbl;
		tbl = tbl->next;
	}

	return NULL;
}

/* Read an entry from 'inp'. The retrieval
 * of the formatting & the body. is dependant
 * on what options are set (in 'options' :-). */
static struct ency_titles *read_entry (FILE *inp, int options)
{
	char c;
	char *temp_text = NULL;
	struct ency_titles *root_title = NULL;
	char *ttl = NULL;
	struct st_ency_formatting *text_fmt = NULL;
	long filepos;

	root_title = NULL;

	filepos = ftell (inp);

	root_title = st_new_entry ();

	if (root_title == NULL)
	{
		return (st_title_error (2));
	}

	if (options & ST_OPT_NO_FMT)
	{
		while ((getc (inp) != '@'));
		getc (inp);
	} else
		text_fmt = st_return_fmt (inp);

	ttl = st_return_title (inp);

	c = getc (inp);

	if (options & ST_OPT_RETURN_BODY)
		temp_text = st_return_text (inp);

/* copy pointer stuff over */
	root_title->filepos = filepos;
	root_title->title = ttl;
	root_title->text = temp_text;
	root_title->next = NULL;
	root_title->name = NULL;
	root_title->fmt = text_fmt;
	root_title->err = 0;

	return (root_title);
}

/* Read an entry located at 'filepos' from
 * the current file using 'options'. */
struct ency_titles *st_read_title_at (long filepos, int options)
{
	FILE *inp;
	struct ency_titles *ret=NULL;

	inp = curr_open (filepos);
	if (!inp)
	{
		return (st_title_error (1));
	}

	ret = read_entry (inp, options);

	fclose (inp);

	return (ret);
}

/* Read an entry located at 'filepos' from
 * the current file using the default options. */
struct ency_titles *st_get_title_at (long filepos)
{
	return (st_read_title_at (filepos, ST_OPT_RETURN_BODY));
}

/* Add an entry's offset to the (sorted)
 * block cache.
 * The block cache associates the block ID &
 * entry ID in the attribs table to a point in
 * the actual file */
static void add_to_block_cache (int block_id, int id, long filepos)
{
	struct ency_titles *new_entry=NULL, *tmp=NULL, *otmp=NULL;

	if (cache)
	{
		if ((block_id < cache_last->block_id) || ((block_id == cache_last->block_id) && (id < cache_last->id)))
			tmp = cache;
		else
			tmp = cache_last;

		while (tmp)
		{
			if (((block_id == tmp->block_id) && (id < tmp->id)) || (block_id < tmp->block_id))
				break;
			otmp = tmp;
			tmp = tmp->next;
		}
		new_entry = st_new_entry ();
		new_entry->next = tmp;
		if (otmp)
			otmp->next = new_entry;
		else
			cache = new_entry;
		cache_last = new_entry;
	}
	else
		cache_last = cache = st_new_entry ();

	cache_last->filepos = filepos;
	cache_last->block_id = block_id;
	cache_last->id = id;
}

/* Load the block cache for block 'block_id' in
 * the current file. */
static void load_block_cache (int block_id)
{
	FILE *inp;
	char c;
	int i;
	int id;
	struct st_part *part;

	part = get_part_by_id (st_file_type, block_id);

	if (!part) /* just in case... */
		return;

	inp = curr_open (part->start);

	if (inp)
	{
		for (i=part->start_id;i<part->start_id + part->bcount;i++)
		{
			if (i > part->start_id)
			{
				find_next_stxt_block (inp);
				fseek (inp, 16, SEEK_CUR);
			}
			add_to_block_cache (i, id=1, ftell (inp));
			while ((c = getc (inp)))
			{
				if (c == '~')
				{
					id++;
					add_to_block_cache (i, id, ftell (inp));
				}
			}
				/* So we can determine the size of the last entry */
				/* in a block later. This is *not* a real entry,  */
				/* and thus should never be read.                 */
			add_to_block_cache (i, ++id, ftell (inp)-1);
		}
		fclose (inp);
	}
}

/* This scans along the block cache for an
 * entry with the matching block & entry IDs.
 * If one isn't found and allow_recursion is set,
 * it will try to create the block cache for that
 * block, and then call itself. */
static long get_block_pos_from_cache (int block_id, int id, int allow_recursion)
{
	struct ency_titles *curr=NULL;

	if (cache_quick)
		if (cache_quick->block_id <= block_id && cache_quick->id <= id)
			while (cache_quick)
			{
				if ((cache_quick->block_id == block_id) && (cache_quick->id == id))
					return (cache_quick->filepos);
				cache_quick = cache_quick->next;
			}
	
	curr = cache;
	
	while (curr)
	{
		if ((curr->block_id == block_id) && (curr->id == id))
			return (curr->filepos);
		cache_quick = curr = curr->next;
	}

	if (allow_recursion)
	{
		load_block_cache (block_id);
		return (get_block_pos_from_cache (block_id, id, 0));
	} else
		return -1;
}

/* Read in the entry from the current file
 * given the block & entry IDs from the attribs
 * table. */
static struct ency_titles *get_entry_by_id (int block_id, int id, int options)
{
	static struct ency_titles *ret;
	FILE *inp;
	long filepos;

	if (!block_id || !id)
		return NULL;

	if (((options & ST_OPT_RETURN_BODY) == 0) && (options & ST_OPT_NO_FILEPOS))
		filepos = 1; /* can't be 0 'cos of the if() below */
	else
		filepos = get_block_pos_from_cache (block_id, id, 1);

	if (filepos >= 0)
	{
		if (options & ST_OPT_RETURN_BODY)
		{
			inp = curr_open (filepos);
			if (!inp)
				fprintf (stderr, "Oh damn! curr_open() failed for %ld (entry %d:%d)\n(%s)\n", filepos, block_id, id, strerror (errno));
			if (!inp)
				return NULL;

			ret = read_entry (inp, options);
			fclose (inp);
		} else {
			ret = st_new_entry ();
			if (!ret)
				return NULL;
			ret->filepos = filepos;
			ret->title = NULL;
			ret->name = NULL;
			ret->text = NULL;
			ret->fmt = NULL;
			ret->next = NULL;
			ret->block_id = block_id;
			ret->id = id;
		}

		/* Determine the length in bytes used by the entry */
		/* (Formatting & all). This is used when scoring in*/
		/* FT searches */
		if (cache_quick && cache_quick->next)
		{
			ret->length = (cache_quick->next->filepos - cache_quick->filepos);
		} else
			ret->length = 1;

		return (ret);
	}

	fprintf (stderr, "Uhoh - entry not found @ %d:%d\n", block_id, id);

	return NULL;
}

/* Search a file for entrys matching 'search_string'.
 * This will load the entry lists if they aren't already
 * loaded */
static struct ency_titles *st_find_in_file (int file, int section, char *search_string, int options)
{
	struct ency_titles *root = NULL, *curr = NULL;
	struct st_table *tmp = NULL;
	struct st_table *tbl=NULL;

	if (!entry_list_has_section (section))
		if (!st_get_video_table (section))
			return NULL;

	tmp = entrylist_head;
	if (!tmp)
		return NULL;

	while (tmp)
	{
		if (tmp->section == section)
		{
			if (check_match (search_string, tmp->title, options))
			{
				if (tbl)
				{
					if (strcmp (tmp->title, tbl->title) >= 0)
						tbl = get_table_entry_by_title (tbl, tmp->title);
					else
						tbl = NULL;
				}

				if (!tbl)
					tbl = get_table_entry_by_title (entrylist_head, tmp->title);
				if (curr)
				{
					curr->next = get_entry_by_id (tbl->block_id, tbl->id, options);
					if (!curr->next)
					{
						tmp = tmp->next;
						continue;
					} else
						curr = curr->next;
				}
				else
					root = curr = get_entry_by_id (tbl->block_id, tbl->id, options);
				if (curr)
					curr->name = strdup (tmp->title);
			}
		}
		tmp = tmp->next;
	}

	return root;
}

static int ft_list_has_section (int section)
{
	struct st_ftlist *fl=NULL;

	fl = ftlist;
	while (fl)
		if (fl->section == section)
			return 1;
		else
			fl = fl->next;

	return 0;
}

static void load_ft_list (int section)
{
	struct st_part *part;
	struct st_ftlist *root=NULL, *curr=NULL, *last=NULL;
	struct st_wl *wl_curr=NULL, *wl_last=NULL;
	char *word, c=0;
	int found_word;
	FILE *inp=NULL;
	int count=0, multi, i;

	while ((part = get_part (st_file_type, ST_BLOCK_FTLIST, section, count++, 0)))
	{
		inp = curr_open (part->start);
		if (!inp)
			return;

		for (i=0;i<part->count;i++)
		{
			if (i)
			{
				find_next_stxt_block (inp);
				fseek (inp, 16, SEEK_CUR);
			}

			if (getc (inp) == '\"')
				ungetc ('\"', inp);
			
			c = getc (inp);
			if (c != '\"')
				c = ']';
			else
			{
				ungetc (c, inp);
				c=0;
			}

			while (c != ']')
			{

				while (getc (inp) != '\"');
				ungetc ('\"', inp);

				word = get_text_from_file (inp);

				while ((getc (inp)) != '[');

				while (c != ']')
				{
					curr = (struct st_ftlist *) malloc (sizeof (struct st_ftlist));
					curr->words = NULL;
					curr->section = section;

					if (!root)
						root = curr;

					curr->word = word;
					/* we only want the 1st one to have the word stored
					   so we can save memory */
					word = NULL;

					while ((getc (inp)) != '\"');
					ungetc ('\"', inp);

					curr->fnbase = get_text_from_file_max_length (inp, 10);

					while ((getc (inp) != ':'));
					getc (inp);
					if (isdigit (c = getc (inp)))
					{
						ungetc (c, inp);
						multi = 0;
					} else
						multi = 1;

					fscanf (inp, "%d", &found_word);
					wl_curr = (struct st_wl *) malloc (sizeof (struct st_wl));
					wl_curr->word = found_word;
					wl_curr->next = NULL;
					curr->words = wl_curr;

					if (multi)
					{
						while (getc (inp) == ',')
						{
							wl_last = wl_curr;
							wl_curr = (struct st_wl *) malloc (sizeof (struct st_wl));
							wl_curr->next = NULL;
							wl_last->next = wl_curr;
							
							fscanf (inp, "%d", &found_word);
							wl_curr->word = found_word;
						}
					}
					if (last)
						last->next = curr;
					curr->next = NULL;
					last = curr;
					
					c = getc (inp);
				}
				c = getc (inp);
			}
		}
		fclose (inp);
	}
	ftlist = root;
}

struct entry_scores
{
	char fnbase[6];
	int score;
	struct entry_scores *next;
};

static struct entry_scores *find_words (struct entry_scores *root, char *words, struct st_ftlist *fl)
{
	struct entry_scores *scores=NULL, *curr=NULL, *last=NULL;
	struct st_wl *wl;
	char *match=NULL;

	if (!words)
		return NULL;

	scores = root;

	if ((last = scores))
		while (last->next)
			last = last->next;

	while (fl)
	{
		if (fl->word)
			match = fl->word;

		if (!strcasecmp (match, words))
		{
			if (!scores)
			{
				scores = last = curr = (struct entry_scores *) malloc (sizeof (struct entry_scores));
				strcpy (curr->fnbase, fl->fnbase);

				curr->score = 5;
				wl = fl->words;
				while (wl)
				{
					curr->score++;
					wl = wl->next;
				}
				curr->next = NULL;
			} else
			{
				curr = scores;
				while (curr)
				{
					if (!strcasecmp (curr->fnbase, fl->fnbase))
					{
						curr->score += 5;

						wl = fl->words;
						while (wl)
						{
							curr->score++;
							wl = wl->next;
						}
						break;
					}
					curr = curr->next;
				}
				if (!curr)
				{
					last->next = curr = (struct entry_scores *) malloc (sizeof (struct entry_scores));
					strcpy (curr->fnbase, fl->fnbase);
					curr->score = 5;

					wl = fl->words;
					while (wl)
					{
						curr->score++;
						wl = wl->next;
					}

					curr->next = NULL;
					last = curr;
				}

			}

		}
		fl = fl->next;
	}


	return scores;
}

static struct ency_titles  *sort_scores (struct ency_titles *scores)
{
	struct ency_titles *curr=NULL, *last=NULL, *last_last=NULL, *tmp=NULL;
	int needs_sorting=1;

	if (!scores)
		return NULL;

	while (needs_sorting)
	{
		needs_sorting = 0;
		curr = scores->next;
		last = scores;
		last_last = NULL;
		while (curr)
		{
			if (last->score < curr->score)
			{
				if (last_last)
					last_last->next = curr;
				else
					scores = curr;
				last->next = curr->next;
				curr->next = last;
				tmp = curr;
				curr = last;
				last = tmp;
				needs_sorting = 1;
			}

			last_last = last;
			last = curr;
			curr = curr->next;
		}
	}
	return scores;
}

static struct ency_titles *st_find_fulltext (char *search_string, int section, int options)
{
	struct ency_titles *root=NULL, *curr=NULL;
	struct st_table *tbl=NULL, *ctbl=NULL;
	struct entry_scores *scores=NULL, *last_score;
	char *title;
	char single_word[64];
	int bad;
	float mult;

	if (!ft_list_has_section (section))
		load_ft_list (section);
	if (!ftlist)
		return NULL;

	if (!st_ptbls)
		st_ptbls = st_get_table ();
	if (!st_ptbls)
		return NULL;

	if (!load_entry_lists ())
		return NULL;

	if (options & ST_OPT_MATCH_SUBSTRING)
		options -= ST_OPT_MATCH_SUBSTRING;
	options -= ST_OPT_FT;

	while (strlen (search_string))
	{
		sscanf (search_string, "%[a-zA-Z0-9.\"\'()-]", single_word);
		search_string += strlen (single_word);
		while (isblank (*search_string))
			search_string++;
		scores = find_words (scores, single_word, ftlist);
	}

	while (scores)
	{
		if (ctbl)
		{
			if (strcasecmp (scores->fnbase, ctbl->fnbase) >= 0)
				ctbl = get_table_entry_by_fnbase (ctbl, scores->fnbase);
			else
				ctbl = NULL;
		}

		if (!ctbl)
			ctbl = get_table_entry_by_fnbase (st_ptbls, scores->fnbase);
		if (ctbl)
			title = ctbl->title;
		else
			title = NULL;

		if (title)
		{
			tbl = get_table_entry_by_title (tbl, title);
			if (!tbl)
				tbl = get_table_entry_by_title (entrylist_head, title);
			if (tbl)
			{
				bad = 0;
				if (curr)
				{
					curr->next = get_entry_by_id (tbl->block_id, tbl->id, options);
					if (curr->next)
						curr = curr->next;
					else
						bad = 1;
				}
				else
					curr = root = get_entry_by_id (tbl->block_id, tbl->id, options);

				if (curr && !bad)
				{
					curr->name = strdup (title);
					curr->score = scores->score + ((float) scores->score / ((float)curr->length / (float)1000 + 1));
					curr->next = NULL;
				}
			}
		}
		last_score = scores;
		scores = scores->next;
		free (last_score);
	}

	if (!root)
		return NULL;

	root = sort_scores (root);

	curr = root;
	mult = (float) 100 / curr->score;
	while (curr)
	{
		curr->score *= mult;
		if (curr->score > 100)
			curr->score = 100;
		curr = curr->next;
	}

	return root;
}

struct ency_titles *st_find (char *search_string, int section, int options)
{
	if (!((st_file_type >= 0) && (st_file_type < ST_FILE_TYPES)))
		return NULL;

	if ((section == ST_SECT_EPIS) && (options & ST_OPT_SORTEPIS))
		section = ST_SECT_EPIS_SORTED;

	switch (section)
	{
	case ST_SECT_ENCY:
	case ST_SECT_EPIS:
	case ST_SECT_CHRO:
	case ST_SECT_EPIS_SORTED:
		if (options & ST_OPT_FT)
			return (st_find_fulltext (search_string, section, options));

		return (st_find_in_file (st_file_type, section, search_string, options));
	default:
		return (NULL);
	}

}

static struct st_photo st_parse_captions (char *fnbasen)
{
	struct st_photo photo;
	struct st_caption *temp_pcpts = NULL;
	int used_quick = 0;

	temp_pcpts = st_pcpts;

	/* can we start later in the list? */
	/* (this assumes asciiabetical order :) */
	if (st_pcpts_quick)
		if (strcmp (fnbasen, st_pcpts_quick->fnbasen) >= 0)
		{
			temp_pcpts = st_pcpts_quick;
			used_quick = 1;
		}

	strcpy (photo.file, "");
	strcpy (photo.caption, "");
	while (temp_pcpts && (!strlen (photo.file)))
	{
		if (!strcmp (fnbasen, temp_pcpts->fnbasen))
		{
			strcpy (photo.file, fnbasen);
			strcpy (photo.caption, temp_pcpts->caption);
			break;
		}
		temp_pcpts = temp_pcpts->next;
	}

	st_pcpts_quick = temp_pcpts;

	/* just in case the list isnt asciiabetical */
	if (!strlen (photo.file) && used_quick)
		return (st_parse_captions (fnbasen));

	return (photo);
}

static struct st_photo st_parse_video_captions (char *fnbasen)
{
	struct st_photo photo;
	struct st_caption *temp_vcpts = NULL;

	temp_vcpts = st_vcpts;

	strcpy (photo.file, "");
	strcpy (photo.caption, "");

	while (temp_vcpts && (!strlen (photo.file)))
	{
		if (!strcasecmp (fnbasen, temp_vcpts->fnbasen))
		{
			strcpy (photo.file, fnbasen);
			strcat (photo.file, "1");
			strcpy (photo.caption, temp_vcpts->caption);
		}
		temp_vcpts = temp_vcpts->next;
	}

	return (photo);
}

static struct st_media *new_media (struct st_media *old_media)
{
	struct st_media *media;
	int i;

	if (old_media)
		return (old_media);

	media = malloc (sizeof (struct st_media));
	if (media)
	{
		for (i=0;i<6;i++)
		{
			strcpy (media->photos[i].file, "");
			strcpy (media->photos[i].caption, "");
		}
		strcpy (media->video.file, "");
		strcpy (media->video.caption, "");
		strcpy (media->audio.file, "");
		strcpy (media->audio.caption, "");
		strcpy (media->swf.file, "");
		strcpy (media->swf.caption, "");
	}
	return media;
}

static int in_simple_list (long filepos, int entrylen, char *match)
{
	FILE *inp;
	char entry[entrylen+1];

	inp = curr_open (filepos);
	while (getc (inp) != ']')
	{
		while (getc (inp) != '\"');
		fread (entry, entrylen, 1, inp);
		entry[entrylen] = 0;
		getc (inp);
		if (!strncasecmp (match, entry, strlen (entry)))
		{
			fclose (inp);
			return 1;
		}
	}
	fclose (inp);
	return 0;
}

static int is_flash_except (char *fnbase)
{
	struct st_part *part;
	int count=0;

	while ((part = get_part (st_file_type, ST_BLOCK_FLASHEXCEPT, 0, count++, 0)))
		if (in_simple_list (part->start, 6, fnbase))
			return 1;
	return 0;
}

struct st_media *st_get_media (char *search_string)
{
	int i = 0;
	int media_found = 0;
	struct st_media *media = NULL;
	char *temp_fnbase = NULL;
	char *ret_fnbase = NULL;
	struct st_table *temp_ptbls = NULL;
	struct st_table *temp_vtbls = NULL;
	struct st_table *ret_tbl = NULL;

	if (st_loaded_media ())
	{

		temp_ptbls = st_ptbls;
		temp_vtbls = st_vtbls;

		temp_fnbase = malloc (9);

		media = new_media (media);

		if ((ret_fnbase = get_fnbase (st_ptbls, search_string)))
		{
			for (i = 0; i < 6; i++)
			{
				sprintf (temp_fnbase, "%s%d", ret_fnbase, i + 1);
				media->photos[i] = st_parse_captions (temp_fnbase);
				if (strlen (media->photos[i].file))
					media_found = 1;
			}
			sprintf (temp_fnbase, "%sf", ret_fnbase);
			media->swf = st_parse_captions (temp_fnbase);
			if (strlen (media->swf.file))
				media_found = 1;

			if (is_flash_except (ret_fnbase))
			{
				for (i = 0; i < 6; i++)
				{
					if (!strcmp (media->photos[i].caption, media->swf.caption))
					{
						strcpy (media->photos[i].file, "");
						strcpy (media->photos[i].caption, "");
						break;
					}
				}
			}
		}

		if ((ret_tbl = get_table_entry_by_title (st_vtbls, search_string)))
		{
			if (ret_tbl->fnbase)
				media->video = st_parse_video_captions (ret_tbl->fnbase);
			if (ret_tbl->audio)
			{
				strcpy (media->audio.file, ret_tbl->audio);
				strcpy (media->audio.caption, search_string);
			}
			if (strlen (media->video.file) || strlen (media->audio.file))
				media_found = 1;
		}

		if (!media_found)
		{
			free (media);
			media = NULL;
		}

		free (temp_fnbase);
	}
	return (media);
}

static char *get_video_dir (char *fnbasen)
{
	struct st_part *part=NULL;
	char *dir;
	int count=0;

	while ((part = get_part (st_file_type, ST_SECT_VLST, 0, count++, 0)))
		if (in_simple_list (part->start, 12, fnbasen))
		{
			dir = part->dir;
			return dir;
		}

	return (char *) st_fileinfo_get_data(st_file_type,video_dir);

}

char *st_format_filename (char *fnbasen, char *base_path, media_type media)
{
	char *filename = NULL;
	char *dir=NULL;

	int dir_size = 0;

	if (fnbasen)
	{

		if (base_path)
			dir_size = strlen (base_path);

		switch (media)
		{
		case audio:
		case swf:
		case picture:
			dir_size += strlen (st_fileinfo_get_data(st_file_type,picture_dir));
			break;
		case video:
			dir = get_video_dir (fnbasen);
			dir_size += strlen (dir);
			break;
		default:
			return "";
		}

		filename = malloc (dir_size + 17);
		if (base_path)
		{
			strcpy (filename, base_path);
			strcat (filename, "/");
		}
		else
			strcat (filename, "/");

		switch (media)
		{
		case audio:
		case swf:
		case picture:
			strcat (filename, st_fileinfo_get_data(st_file_type,picture_dir));
			break;
		case video:
			strcat (filename, dir);
			break;
		}

		strcat (filename, "/");
		if (st_fileinfo_get_data(st_file_type,append_char))
		{
			filename[dir_size + 2] = fnbasen[0];
			filename[dir_size + 3] = 0;
			strcat (filename, "/");
		}
		if (media == audio)
			strncat (filename, fnbasen, 12);
		else
			strncat (filename, fnbasen, 8);

		switch (media)
		{
		case audio:
			break;
		case swf:
			strcat (filename, "f.swf");
			break;
		case picture:
			strcat (filename, "r.pic");
			break;
		case video:
			strcat (filename, "q.mov");
			break;
		}
	}
	return (filename);
}


