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

static FILE *inp;

static char *ency_filename = NULL;

int st_return_body = 1;
int st_ignore_case = 0;
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

/* for use w/ st_get_title_at() */
static long int set_starts_at = 0x0;

static long int curr_starts_at, curr;

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

/* init/de-init stuff */
int st_init (void)
{
	load_xmlfile_info(NULL);
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

/* cache stuff */
static void st_clear_cache ()
{
	st_free_entry_tree (cache);
	cache = NULL;
	cache_last = NULL;
	cache_quick = NULL;
}

/* file stuff */
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

int st_load_xml_file (char *filename)
{
	st_data_clear ();
	return (load_xmlfile_info (filename));
}

char *st_fileinfo_get_name (int file_type)
{
	if (file_type > ST_FILE_TYPES)
		return "Unknown";

	if (file_type == -1)
		file_type = st_file_type;

	return get_name_of_file (file_type);
}

int curr_open (long start)
{
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

	return ((int) inp);
}

static int st_open ()
{
	switch (curr)
	{
	case 0:		/* Defaults to ency */
	case 1:		/* Ency */
	case 2:		/* Epis */
	case 3:		/* Chro */
	case 4:		/* table */
		break;
	case 5:		/* Set value */
		curr_starts_at = set_starts_at;
		break;
	case 6:		/* Captions */
	case 7:		/* video table */
	case 8:		/* video captions */
		break;
	default:
		return (0);
		break;
	}
	return (curr_open (curr_starts_at));
}

static int st_close_file (void)
{
	fclose (inp);
	return (0);
}

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

		if (strcmp (base_dir, "."))
		{
			sprintf (test_filename, "%s", base_dir);
			if (st_fingerprint () == st_file_version)
			{
				free (lc_data_dir);
				free (lc_filename);
				ency_filename = ency_fn_backup;
				return (test_filename);
			}
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

/* media stuff */
static void find_next_stxt_block (FILE *input)
{
	char d[5]="    ";
	
	while (!feof (inp))
	{
		d[3] = getc (input);

		if ((!strcmp (d, "STXT")) || (!strcmp (d, "TXTS")))
			return;
		d[0] = d[1];
		d[1] = d[2];
		d[2] = d[3];
	}
}

static struct st_table *read_table (FILE *input, struct st_table *root)
{
	struct st_table *root_tbl = NULL;
	struct st_table *curr_tbl = NULL;
	struct st_table *last_tbl = NULL;

	int text_size = 0;
	unsigned char c = 0;
	unsigned char d = 0;
	char *temp_text = NULL, *str_text = NULL;
	int z;

	if (root)
	{
		root_tbl = root;
		last_tbl = root;
		while (last_tbl->next)
			last_tbl = last_tbl->next;
	}

	c = getc (inp);

	if (c != '\"')
	{
		d = ungetc (getc (inp), inp);
		if ((d != '\"') && (!isdigit (d)))
			return (root);
	}

	ungetc (c, inp);

	if (c == '\"')
		ungetc ('[', inp);

	while ((c = getc (input)) != ']')
	{	/* main loop */

		curr_tbl = (struct st_table *) malloc (sizeof (struct st_table));

		if (curr_tbl == NULL)
		{
			return NULL;
		}

		if (!root_tbl)
			root_tbl = curr_tbl;

		do
		{
			while ((c = getc (input)) != '\"');
			c = getc (input);
		}
		while (!c);
		ungetc (c, input);

		temp_text = malloc (8);
		text_size = 0;
			
		fread (temp_text, 1, 6, input);
		temp_text[6] = 0;

		if (strstr (temp_text, "\""))
		{
			fseek (input, -6, SEEK_CUR);
			while ((c = getc (input)) != '\"');
			str_text = (strstr (temp_text, "\""));
			str_text[0] = 0;
		} else
			while ((getc (input) != '\"'))
				;

		curr_tbl->fnbase = temp_text;
		z = 0;
		while ((temp_text[z] = tolower (temp_text[z])))
			z++;

/* TODO: make this 70 autodetected ala st_return text */
		temp_text = malloc (70);
		text_size = 0;

		while ((c = getc (input)) != '\"');
		while ((c = getc (input)) != '\"')
		{
			temp_text[text_size++] = st_cleantext (c);
		}
		temp_text[text_size] = 0;

		curr_tbl->title = temp_text;

		curr_tbl->next = NULL;
		if (last_tbl)
			last_tbl->next = curr_tbl;
		last_tbl = curr_tbl;
		curr_tbl = NULL;
	}	/* end main loop */

	return (root_tbl);
}

static struct st_table *st_get_table ()
{
	int i;
	int count = 0;
	struct st_table *root_tbl = NULL;
	struct st_part *part;

	curr = 4;

	while ((part = get_part (st_file_type, ST_SECT_PTBL, 0, count, 0)))
	{
		curr_starts_at = part->start;
		if (!st_open ())
		{
			return (NULL);
		}
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
		st_close_file ();
		count++;
	}
	return (root_tbl);
}

static struct st_caption *read_captions (FILE *input, struct st_caption *root, int section)
{
	int z;
	struct st_caption *root_cpt = NULL, *curr_cpt = NULL, *last_cpt = NULL;
	char c = 0;
	int text_size = 0;
	char *temp_text = NULL;

	if (root)
	{
		root_cpt = root;
		last_cpt = root;
		while (last_cpt->next)
			last_cpt = last_cpt->next;
	}

	c = getc (input);
	if (c == '[')
		c = ungetc (c, input);

	while (c != ']')
	{	/* main loop */
		curr_cpt = (struct st_caption *) malloc (sizeof (struct st_caption));

		if (curr_cpt == NULL)
		{
			return (NULL);
		}
		if (!root_cpt)
			root_cpt = curr_cpt;

		do
		{
			while ((c != '\"') && (c != '[') && ((c = getc (inp)) != ' '));
			c = getc (inp);
		}
		while (!c);
		ungetc (c, inp);

		if ((c = getc (inp)) != '[')
			c = ungetc (c, inp);
		if ((c = getc (inp)) != ' ')
			c = ungetc (c, inp);
		if ((c = getc (inp)) != ' ')
			c = ungetc (c, inp);
		if ((c = getc (inp)) != ' ')
			c = ungetc (c, inp);

		if ((c = getc (inp)) != '\"')
			c = ungetc (c, inp);
		if ((c = getc (inp)) != ':')
		{
			ungetc (c, inp);
			c = 0;

			temp_text = malloc (8);

			text_size = (section == ST_SECT_PCPT) ? 8 : 7;

			fread (temp_text, 1, text_size, inp);
			if ((temp_text[text_size-1] != '\"') && (temp_text[text_size-1] != ':'))
				while ((getc (inp)) != '\"');

			temp_text[text_size-1] = 0;

			z = 0;
			while ((temp_text[z] = tolower (temp_text[z])))
				z++;

			c = getc (inp);
			if (c == ' ') c = getc (inp);
			if (c == '\"') c = ungetc (c, inp);

			curr_cpt->fnbasen = temp_text;

			temp_text = malloc (70);
			text_size = 0;

			while ((c = getc (inp)) != '\"');
			while ((c = getc (inp)) != '\"')
				temp_text[text_size++] = st_cleantext (c);
			temp_text[text_size] = 0;

			curr_cpt->caption = temp_text;
			c = getc (inp);

			curr_cpt->next = NULL;
			if (last_cpt)
				last_cpt->next = curr_cpt;
			last_cpt = curr_cpt;
			curr_cpt = NULL;
		}
	}	/* end main loop */
	return (root_cpt);
}

static struct st_caption *st_get_captions (int section)
{
	int i;
	int count = 0;
	struct st_caption *root_cpt = NULL;
	struct st_part *part;

	curr = 6;

	while ((part = get_part (st_file_type, section, 0, count, 0)))
	{
		curr_starts_at = part->start;
		if (!st_open ())
		{
			return (NULL);
		}
		else
		{
			for (i = 0; i < part->count; i++)
			{
				root_cpt = read_captions (inp, root_cpt, section);

			}
		}
		count++;
		st_close_file ();
	}
	return (root_cpt);
}

static struct st_table *read_attribs_table (FILE *inp, int section, int count)
{
	struct st_table *root_tbl = NULL, *curr_tbl = NULL, *last_tbl = NULL;
	char c = 0;
	int i;
	int in_quote;
	int text_size;
	int level, commas;
	char *temp_text = NULL;

	c = getc (inp);
	if (ungetc (getc (inp), inp) == ']')
		return NULL;
	ungetc (c, inp);

	for (i = 0; i < count; i++)
	{
		if (i)
		{
			find_next_stxt_block (inp);
			fseek (inp, 16, SEEK_CUR);
		}

		c = 0;
		while (c != ']')
		{	/* main loop */

			while ((c = getc (inp)) != '[');
			while (c != ']')
			{

				curr_tbl = (struct st_table *) malloc (sizeof (struct st_table));

				if (curr_tbl == NULL)
				{
					return (NULL);
				}

				curr_tbl->block_id = 0;
				curr_tbl->id = 0;
				curr_tbl->fnbase = NULL;
				curr_tbl->audio = NULL;
				curr_tbl->section = section;

				/* TODO: Make this '70' an autodetected size */
				temp_text = malloc (sizeof (char) * 70);
				text_size = 0;
				while ((c = getc (inp)) != '\"');
				while ((temp_text[text_size++] = getc (inp)) != '\"');
				temp_text[--text_size] = 0;

				do
				{
					text_size--;
					temp_text[text_size] = st_cleantext (temp_text[text_size]);
				} while (text_size);

				curr_tbl->title = temp_text;
				temp_text = NULL;
				curr_tbl->fnbase = NULL;

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
								/* TODO: Make this '70' an autodetected size */
								temp_text = malloc (sizeof (char) * 70);
								text_size = 0;

								while ((temp_text[text_size++] = tolower (getc (inp))) != '\"');
								temp_text[text_size - 1] = 0;
								if (commas == 4)
									curr_tbl->fnbase = temp_text;
								else
									curr_tbl->audio = temp_text;
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
					c = getc (inp);
				}

				if (curr_tbl->fnbase)
					st_cleanstring (curr_tbl->fnbase);
				st_cleanstring (curr_tbl->title);
				if (!root_tbl)
					root_tbl = curr_tbl;
				curr_tbl->next = NULL;
				if (last_tbl)
					last_tbl->next = curr_tbl;
				last_tbl = curr_tbl;
				curr_tbl = NULL;
			}
		}
	}
	return root_tbl;
}

static struct st_table *st_get_video_table (int section)
{
	struct st_table *root_tbl = NULL, *curr_tbl=NULL;
	struct st_part *part;
	int count=0;

	curr = 7;

	while ((part = get_part (st_file_type, ST_BLOCK_ATTRIB, section == ST_SECT_VTBL ? 0 : section, count, 0)))
	{
		curr_starts_at = part->start;
		if (!st_open ())
		{
			return (NULL);
		}
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

			st_close_file ();
			count++;
		}
	}
	return (root_tbl);
}

int st_load_media (void)
{
	/*
	 * Get the table & captions (if we don't already have them)
	 */
	if (!st_ptbls)
		st_ptbls = st_get_table ();
	if (!st_pcpts)
		st_pcpts = st_get_captions (ST_SECT_PCPT);
	if (!st_vtbls)
		st_vtbls = entrylist_head;
	if (!st_vcpts)
		st_vcpts = st_get_captions (ST_SECT_VCPT);

	return (1);
}

int st_loaded_media (void)
{
	if (((st_pcpts) && (st_ptbls)) || ((st_vtbls) && (st_vcpts)))
		return (1);
	else
		return (0);
}

void st_unload_media (void)
{
	static struct st_table *st_oldptbls = NULL;
	static struct st_caption *st_oldpcpts = NULL;
	static struct st_table *st_oldvtbls = NULL;
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

	while (st_vtbls)
	{
		st_oldvtbls = st_vtbls;
		st_vtbls = st_vtbls->next;
		if (st_oldvtbls)
		{
			if (st_oldvtbls->fnbase)
				free (st_oldvtbls->fnbase);
			if (st_oldvtbls->title)
				free (st_oldvtbls->title);
			free (st_oldvtbls);
		}
	}

	st_ptbls = NULL;
	st_pcpts = NULL;
	st_vtbls = NULL;
	st_vcpts = NULL;
}


/* unsorted, mostly file/data stuff */
static int check_match (char *search_string, char *title, int exact)
{
	int found = 0;
	char *lc_title = NULL;
	char *lc_search_string = NULL;

	/* Is this the one we want?? */
	if ((!exact) && strstr (title, search_string))
		found = 1;
	if ((exact == 1) && (!strcmp (title, search_string)))
		found = 1;
	if (exact == 2)
		found = 1;
	if (st_ignore_case)
	{
		lc_title = st_lcase (title);
		lc_search_string = st_lcase (search_string);
		if ((!exact) && (strstr (lc_title, lc_search_string)))
			found = 1;
		if ((exact == 1) && (!strcasecmp (title, search_string)))
			found = 1;
		free (lc_title);
		free (lc_search_string);
	}
	return found;
}

static struct st_ency_formatting *st_return_fmt (void)
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

static char *st_return_text (int options)
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
		if ((options & ST_OPT_TEXTLEN_MAX32) && (text_size >= 32))
			bye = 1;
	}

	temp_text = malloc (text_size + 1);
	fseek (inp, text_starts_at, SEEK_SET);
	fread (temp_text, 1, text_size, inp);

	for (i = 0; i < text_size; i++)
		temp_text[i] = st_cleantext (temp_text[i]);
	temp_text[i] = 0;

	return (temp_text);
}

static char *st_return_title ()
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

/* sorted episode list stuff */
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

static int load_entry_lists (void)
{
	if (!entrylist_head)
		entrylist_head = st_get_video_table (-1);
	
	st_vtbls = entrylist_head;

	return 1;
}

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

static struct st_table *get_table_entry_by_title (struct st_table *tbl, char *title)
{
	char *exception;
	struct st_table *root=tbl;
	char *temp;

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
	if (strlen (title) > 14)
		if (!strncmp (title + 5, "Star Trek", 9))
			return (get_table_entry_by_title (root, title + 4));

	if (*title != '*')
	{
		temp = (char *) malloc (strlen (title) + 3);
		sprintf (temp, "* %s", title);
		root = get_table_entry_by_title (root, temp);
		free (temp);
		return root;
	}

	return NULL;
}

static struct ency_titles *read_entry (FILE *inp, int options)
{
	int return_body_was;
	char c;
	char *temp_text = NULL;
	struct ency_titles *root_title = NULL;
	char *ttl = NULL;
	struct st_ency_formatting *text_fmt = NULL;
	long filepos;

	root_title = NULL;
	return_body_was = st_return_body;
	st_return_body = 1;

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
		text_fmt = st_return_fmt ();

	ttl = st_return_title ();

	c = getc (inp);

	if (options & ST_OPT_RETURN_BODY)
		temp_text = st_return_text (options);

/* copy pointer stuff over */
	root_title->filepos = filepos;
	root_title->title = ttl;
	root_title->text = temp_text;
	root_title->next = NULL;
	root_title->name = NULL;
	root_title->fmt = text_fmt;
	root_title->err = 0;
	st_return_body = return_body_was;

	return (root_title);
}

struct ency_titles *st_read_title_at (long filepos, int options)
{
	FILE *input;
	struct ency_titles *ret=NULL;

	input = (FILE *) curr_open (filepos);
	if (!input)
	{
		return (st_title_error (1));
	}

	ret = read_entry (input, options);

	fclose (inp);

	return (ret);
}

struct ency_titles *st_get_title_at (long filepos)
{
	return (st_read_title_at (filepos, ST_OPT_RETURN_BODY));
}

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

static void load_block_cache (int block_id)
{
	char c;
	int i;
	int id;
	struct st_part *part;

	part = get_part_by_id (st_file_type, block_id);

	inp = (FILE *) curr_open (part->start);

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

static struct ency_titles *get_entry_by_id (int block_id, int id, int options)
{
	static struct ency_titles *ret;
	FILE *input;
	long filepos;

	if (!block_id || !id)
		return NULL;

	if ((st_return_body == 0) && (options & ST_OPT_NO_FILEPOS))
		filepos = 1; /* can't be 0 'cos of the if() below */
	else
		filepos = get_block_pos_from_cache (block_id, id, 1);

	if (filepos >= 0)
	{
		if (st_return_body)
		{
			input = (FILE *) curr_open (filepos);
			if (!input)
				fprintf (stderr, "Oh damn! curr_open() failed for %ld (entry %d:%d)\n(%s)\n", filepos, block_id, id, strerror (errno));
			if (!input)
				return NULL;

			ret = read_entry (input, options);
			fclose (input);
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

static struct ency_titles *st_find_in_file (int file, int section, char *search_string, int exact, int options)
{
	struct ency_titles *root = NULL, *curr = NULL;
	struct st_table *tmp = NULL;
	struct st_table *tbl=NULL;

	if (!entry_list_has_section (section))
		if (!load_entry_lists ())
			return NULL;

	tmp = entrylist_head;
	if (!tmp)
		return NULL;

	while (tmp)
	{
		if (tmp->section == section)
		{
			if (check_match (search_string, tmp->title, exact))
			{
				if (tbl)
					if (strcmp (tmp->title, tbl->title) >= 0)
					{
						tbl = get_table_entry_by_title (tbl, tmp->title);
					} else
						tbl = NULL;
				if (!tbl)
					tbl = get_table_entry_by_title (entrylist_head, tmp->title);
				if (curr)
				{
					curr->next = get_entry_by_id (tbl->block_id, tbl->id, options);
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
	char word[70]="", c=0;
	int first=0;
	char fnbase[8], *t;
	int found_word;
	FILE *inp=NULL;
	int count=0, multi, i;

	while ((part = get_part (st_file_type, ST_BLOCK_FTLIST, section, count++, 0)))
	{
		inp = (FILE *) curr_open (part->start);
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

				t = word;
				while ((*t++ = getc (inp)) != '\"')
					;
				*--t = 0;

				while ((getc (inp)) != '[');

				first = 1;

				while (c != ']')
				{
					curr = (struct st_ftlist *) malloc (sizeof (struct st_ftlist));
					curr->words = NULL;
					curr->section = section;

					if (!root)
						root = curr;
					if (first)
					{
						curr->word = strdup (word);
						first = 0;
					} else
						curr->word = NULL;

					while ((getc (inp)) != '\"');
					t = fnbase;
					while ((*t++ = getc (inp)) != '\"')
						;
					*--t = 0;
					curr->fnbase = strdup (fnbase);

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

	if (options & ST_OPT_NO_CACHE)
		options -= ST_OPT_NO_CACHE;
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
	int exact = 0;

	if (!((st_file_type >= 0) && (st_file_type < ST_FILE_TYPES)))
		return NULL;

	if (options & ST_OPT_CASE_SENSITIVE)
		st_ignore_case = 0;
	else
		st_ignore_case = 1;

	if (options & ST_OPT_RETURN_BODY)
		st_return_body = 1;
	else
		st_return_body = 0;

	if (options & ST_OPT_MATCH_SUBSTRING)
		exact = 0;
	else
		exact = 1;

	if ((section == ST_SECT_EPIS) && (options & ST_OPT_SORTEPIS))
		section = 3;

	switch (section)
	{
	case ST_SECT_ENCY:
	case ST_SECT_EPIS:
	case ST_SECT_CHRO:
	case ST_SECT_EPIS_SORTED:
		if (options & ST_OPT_FT)
			return (st_find_fulltext (search_string, section, options));

		return (st_find_in_file (st_file_type, section, search_string, exact, options));
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
		if (!strcasecmp (fnbasen, temp_pcpts->fnbasen))
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
		strcpy (media->audio.file, "");
		strcpy (media->swf.file, "");
	}
	return media;
}

static int in_simple_list (long filepos, int entrylen, char *match)
{
	FILE *inp;
	char entry[entrylen+1];

	inp = (FILE *) curr_open (filepos);
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


