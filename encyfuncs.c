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
#define free(A) {if (A) free (A); A=NULL;}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "ency.h"
#include "encyfuncs.h"
#include "data.h"

static FILE *inp;

static char *ency_filename = NULL;

int st_return_body = 1;
int st_ignore_case = 0;
static int force_unknown = 0;
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

static long int curr_starts_at, curr_lastone, curr;

/* internal */
#define ST_SECT_PTBL 10
#define ST_SECT_VTBL 11
#define ST_SECT_PCPT 12
#define ST_SECT_VCPT 13

/* for pictures */
static struct st_table *st_ptbls = NULL, *st_ptbls_tail = NULL;
static struct st_caption *st_pcpts = NULL, *st_pcpts_tail = NULL;

/* for videos */
static struct st_table *st_vtbls = NULL, *st_vtbls_tail = NULL;
static struct st_caption *st_vcpts = NULL, *st_vcpts_tail = NULL;

/* cache */
static struct ency_titles *cache = NULL;
static struct ency_titles *cache_last = NULL;
static struct ency_titles *cache_quick = NULL;

static struct ency_titles *st_find_in_cache (int, char *, int, int);
static void st_clear_cache (void);

/* init/de-init stuff */
int st_init (void)
{
	load_file_info(NULL);
	st_fingerprint ();
	return (st_file_type >= 254 ? force_unknown : 1);
}

int st_finish (void)
{
	if (st_loaded_media ())
		st_unload_media ();
	if (ency_filename)
		free (ency_filename);
	st_clear_cache ();
	free_xml_doc();

	return (1);
}

/* useful bits */
static int is_all_ascii (char *string)
{
	while (*string)
		if (isascii (*string++) == 0) return 0;
	return 1;
}

static int ends_in_number (char *string, int expected_length)
{
	return (isdigit(string[expected_length]) || (string[expected_length] == 'F'));
}

static int ends_in_quote (char *string, int expected_length)
{
	return ((string[1] == '\"') || (string[expected_length] == '\"'));
}

static int not_embedding_bracket (char *string)
{
	return ((string[0] != '[') && (string[1] != '['));
}

static int doesnt_have_junk (char *string)
{
	return (isalpha (string[0]) || isalpha (string[1]) || ispunct (string[0]) || ispunct (string[1]));
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
}

static int cache_has_section (int section)
{
	struct ency_titles *curr = cache;

	while (curr)
	{
		if (curr->section == section)
			return 1;
		curr = curr->next;
	}
	return 0;
}

static void st_add_to_cache (int section, char *title, long filepos)
{
	struct ency_titles *temp_cache=NULL, *curr=NULL;

	temp_cache = malloc (sizeof (struct ency_titles));
	if (!temp_cache) return;

	temp_cache->title = strdup (title);
	temp_cache->text = NULL;
	temp_cache->fmt = NULL;
	temp_cache->next = NULL;
	temp_cache->section = section;
	temp_cache->filepos = filepos;

	curr = cache_last;

	if (!curr)
	{
		cache_last = cache = temp_cache;
		return;
	}

	cache_last = curr->next = temp_cache;

	return;
}

static void add_to_table (struct st_table *list, int section)
{
	struct st_table *curr=NULL;
	struct st_table **head=NULL;
	struct st_table **tail=NULL;

	if (!list)
		return;

	switch (section)
	{
	case ST_SECT_PTBL:
		head = &st_ptbls;
		tail = &st_ptbls_tail;
		break;
	case ST_SECT_VTBL:
		break;
	default:
		return;
	}
	curr = *tail;

	if (!curr)
	{
		*head = list;
		while (list->next)
			list = list->next;
		*tail = list;
		return;
	} else
	{
		curr->next = list;
		while (list->next)
			list = list->next;
		*tail = list;
	}

	return;
}

static void add_to_captions (struct st_caption *list, int section)
{
	struct st_caption *curr=NULL;
	struct st_caption **head=NULL;
	struct st_caption **tail=NULL;

	if (!list)
		return;

	switch (section)
	{
	case ST_SECT_PCPT:
		head = &st_pcpts;
		tail = &st_pcpts_tail;
		break;
	case ST_SECT_VCPT:
		break;
	default:
		return;
	}
	curr = *tail;

	if (!curr)
	{
		*head = list;
		while (list->next)
			list = list->next;
		*tail = list;
		return;
	} else
	{
		curr->next = list;
		while (list->next)
			list = list->next;
		*tail = list;
	}

	return;
}

/* file stuff */
void st_force_unknown_file (int true)
{
	force_unknown = true;
}

int st_set_filename (char *filename)
{
	int type;
	if (ency_filename)
		free (ency_filename);
	ency_filename = strdup (filename);
	if (ency_filename)
	{
		type = st_fingerprint();
		if ((force_unknown && (type = ST_FILE_UNKNOWN)) || ((type >= 0) && (type < ST_FILE_TYPES)))
		{
			st_file_type = type;
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
	free_xml_doc();
	return (load_file_info (filename));
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
		{
			if (st_set_filename (temp_fn) == 0)
			{
				st_force_unknown_file (1);
				st_set_filename (temp_fn);
			}
		}
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

	if (((st_file_version < ST_FILE_TYPES) && (st_file_version >= 0)) ||
			(st_file_version == ST_FILE_UNKNOWN))
	{

		datadir = st_fileinfo_get_data (st_file_version,data_dir);
		filename = st_fileinfo_get_data (st_file_version,mainfilename);

		lc_data_dir = st_lcase ((char *)datadir);
		lc_filename = st_lcase ((char *)filename);

		ency_fn_backup = ency_filename;
		test_filename = malloc (strlen (base_dir) + strlen (datadir) + strlen (filename) + 3);
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
	if ((c != '[') && (c != '\"'))
		if (ungetc (getc (inp), inp) != '\"')
			return (root);
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

	while ((part = get_part (st_file_type, ST_SECT_PTBL, count, 0)))
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
		free (part);
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

	while ((part = get_part (st_file_type, section, count, 0)))
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
		free (part);
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
							if ((c == '\"') && (commas == 4))
							{
								/* TODO: Make this '70' an autodetected size */
								temp_text = malloc (sizeof (char) * 70);
								text_size = 0;

								while ((temp_text[text_size++] = tolower (getc (inp))) != '\"');
								temp_text[text_size - 1] = 0;
								curr_tbl->fnbase = temp_text;
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

				if (1 /*curr_tbl->fnbase*/)
				{
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
				else
				{
					free (curr_tbl->title);
					free (curr_tbl);
				}
			}
		}
	}
	return root_tbl;
}

static struct st_table *st_get_video_table (int section, int reverse)
{
	struct st_table *root_tbl = NULL, *curr_tbl=NULL;
	struct st_part *part;
	int count=0;

	curr = 7;

	while ((part = get_part (st_file_type, section, count, (reverse) ? ST_PART_OPT_EPISLIST : 0)))
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

				curr_tbl->next = read_attribs_table (inp, section, part->count);
			}
			else
				curr_tbl = root_tbl = read_attribs_table (inp, section, part->count);

			st_close_file ();
			count++;
			free (part);
		}
	}
	return (root_tbl);
}

static void check_for_captions (FILE *input)
{
	char c=0;
	char fnbase[9]="        ";
	char temp[9] = "        ";
	long found_at;

	found_at = ftell (input);

	fread (fnbase, 8, 1, input);
	st_cleanstring (fnbase);

	if (is_all_ascii (fnbase) && ends_in_quote (fnbase, 7) && ends_in_number (fnbase, 6))
	{
		if (fnbase[1] == '\"')
			fseek (input, found_at + 2, SEEK_SET);

		c = getc (input);

		if ((c == ':') || ((c == ' ') && (getc(input) == ':')))
		{
			fread (temp, 8, 1, input);
			if (not_embedding_bracket(temp))
			{
				if (doesnt_have_junk(temp))
				{
					*strchr (fnbase, '\"') = 0;

					fseek (input, found_at, SEEK_SET);
					c=0;
					while ((c != ']') && (c != '\"'))
						c = getc (input);
					if (c == ']')
						return;
					while ((c = getc (input)) != '\"')
						;
					while ((c = getc (input)) != '\"')
						;
					while ((c = getc (input)) != '\"')
						;

					fread (temp, 8, 1, input);
					if (!is_all_ascii (temp))
						return;
					if (!ends_in_quote (temp, 7))
						return;
					if (!ends_in_number (temp, 6))
						return;

					fseek (input, (found_at > 2) ? (found_at - 2) : found_at, SEEK_SET);
					add_to_captions (read_captions (input, NULL, ST_SECT_PCPT), ST_SECT_PCPT);
					return;
				}
			}
		}
	}
	fseek (input, found_at, SEEK_SET);
}

static int check_for_table (FILE *input)
{
	char c=0;
	char fnbase[8]="       ";
	char temp[8] = "       ";
	long found_at;

	found_at = ftell (input);

	fread (fnbase, 7, 1, input);
	st_cleanstring (fnbase);

	if (is_all_ascii (fnbase) && ends_in_quote (fnbase, 6))
	{
		if (fnbase[1] == '\"')
			fseek (input, found_at + 2, SEEK_SET);

		if (getc (input) == ':')
		{
			fread (temp, 7, 1, input);
			if (not_embedding_bracket(temp))
			{
				if (doesnt_have_junk(temp))
				{
					fseek (input, found_at, SEEK_SET);
					while ((c != ']') && (c != '\"'))
						c = getc (input);
					if (c == ']')
						return 0;
					while ((c = getc (input)) != '\"')
						;
					while ((c = getc (input)) != '\"')
						;
					while ((c = getc (input)) != '\"')
						;

					fread (temp, 7, 1, input);
					if (!is_all_ascii (temp))
						return 0;
					if (!ends_in_quote (temp, 6))
						return 0;

					*strchr (fnbase, '\"') = 0;

					fseek (input, (found_at > 2) ? (found_at - 2) : found_at, SEEK_SET);
					add_to_table (read_table (input, NULL), ST_SECT_PTBL);

					return 1;
				}
			}
		}
	}
	fseek (input, found_at, SEEK_SET);
	return 0;
}

static void get_unknown_tables (FILE *input)
{
	unsigned char c=0, old_c=0, old_old_c=0;

	while (!feof(input))
	{
		c = getc(input);
		if ((old_c == '[') && (old_old_c != 0x20))
		{
			if (c == '\"')
			{
				if (!check_for_table(input))
					check_for_captions(input);
			} else if (c == '3')
			{
				if ((getc (input) == '9') && (getc (input) == '5') && (getc (input) == ':'))
				{
					fseek (input, 0x21, SEEK_CUR);
					if (getc (input) == '\"')
						check_for_table(input);
				}
			}
		}
		old_old_c = old_c;
		old_c = c;
	}
}

int st_load_media (void)
{
	/*
	 * Get the table & captions (if we don't already have them)
	 */
	if (st_file_type != ST_FILE_UNKNOWN) {
		if (!st_ptbls)
			st_ptbls = st_get_table ();
		if (!st_pcpts)
			st_pcpts = st_get_captions (ST_SECT_PCPT);
		if (!st_vtbls)
			st_vtbls = st_get_video_table (ST_SECT_VTBL, 0);
		if (!st_vcpts)
			st_vcpts = st_get_captions (ST_SECT_VCPT);
	} else
	{
		curr = 5;
		set_starts_at = 0;
		
		if (!st_open())
			return 0;
		get_unknown_tables (inp);
		st_close_file ();
	}

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

inline int st_find_start (FILE * input)
{
	unsigned char c = 0, old_c = 0, old_old_c = 0, old_old_old_c = 0;
	unsigned char temp;

	int keep_going = 1;
	while (keep_going && !(feof (input)))
	{
		if ((c = getc (input)) == '1')
		{
			switch (old_c)
			{
			case '~':
				if ((old_old_c == 0xd) || ((!old_old_c) && (old_old_old_c != 0x3a)))
				{
					keep_going = 0;
					fseek (input, -1, SEEK_CUR);
				}
				break;
			case 0x16:
				if ((old_old_c == 0) && (old_old_old_c != 0xFC))
				{
					temp = getc (input);
					if ((temp != 0xff) && (temp != 0) && (temp != '.'))
						ungetc (temp, input);
					else
						break;
					keep_going = 0;
					fseek (input, -1, SEEK_CUR);
				}
				break;
			case '@':
				if (((old_old_c == 0x16) && (old_old_old_c == 0)) || ((old_old_c == '~') && (old_old_old_c == 0x0d)))
				{
					keep_going = 0;
					fseek (input, -2, SEEK_CUR);
				}
				break;
			case 0xE2:
				if (old_old_c)
				{
					keep_going = 0;
					fseek (input, -1, SEEK_CUR);
				}
			}
		} else if (c == 'D')
		{
			if ((old_old_old_c == 'B') && (old_old_c == 'I') && (old_c == 'T'))
				fseek (input, 8, SEEK_CUR);
		} else if (c == 0x0c)
		{
			if ((!old_old_old_c) && (!old_old_c) && (!old_c))
				fseek (input, 6, SEEK_CUR);
		}

		if (((c == 'S') && (old_c == 'T') && (old_old_c == 'X') && (old_old_old_c == 'T')) || ((c == 'T') && (old_c == 'X') && (old_old_c == 'T') && (old_old_old_c == 'S')))
		{
			fseek (input, 16, SEEK_CUR);
			return 1;
		}
		if ((c != '1') && isdigit (c) && (old_c == '~'))
		{
			ungetc (c, inp);
			return 1;
		}

		old_old_old_c = old_old_c;
		old_old_c = old_c;
		old_c = c;
	}

	return (feof (input) ? 0 : 1);
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
	return_error = (struct ency_titles *) malloc (sizeof (struct ency_titles));

	if (return_error == NULL)
	{
		return (NULL);
	}
	return_error->title = NULL;
	return_error->text = NULL;
	return_error->fmt = NULL;
	return_error->next = NULL;
	return_error->name = NULL;
	return_error->err = error_no;

	return (return_error);
}

/* sorted episode list stuff */
static struct st_table *entrylist_head=NULL;

int entry_list_has_section (section)
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

int load_entry_list (int section)
{
	struct st_table *curr;

	curr = entrylist_head;
	if (!curr)
	{
		entrylist_head = st_get_video_table (section, 1);
		if (entry_list_has_section (section))
			return 1;
		else
			return 0;
	}

	while (curr->next)
		curr = curr->next;

	if (!entry_list_has_section (section))
	{
		curr->next = st_get_video_table (section, 1);
		if (entry_list_has_section (section))
			return 1;
	} else
		return 1;
	return 0;
}

int load_entry_lists (void)
{
	int i;

	for (i=0;i<3;i++)
	{
		load_entry_list (i);
	}

	return (entrylist_head ? 1 : 0);
}

char *get_fnbase (struct st_table *tbl, char *title)
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

char *get_title (struct st_table *tbl, char *fnbase)
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

struct st_table *get_table_entry_by_fnbase (struct st_table *tbl, char *fnbase)
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

struct st_table *get_table_entry_by_title (struct st_table *tbl, char *title)
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

int get_block_id_by_fnbase (struct st_table *tbl, char *fnbase)
{
	char *exception;
	if (!fnbase)
		return 0;
	if ((exception = get_exception (st_file_type, "LU fnbase", fnbase)))
		fnbase = exception;
	while (tbl)
		if (!strcasecmp (tbl->fnbase, fnbase))
			return tbl->block_id;
		else
			tbl = tbl->next;		
	return 0;
}

int get_block_id_by_title (struct st_table *tbl, char *title)
{
	char *exception;
	if (!title)
		return 0;
	if ((exception = get_exception (st_file_type, "LU title", title)))
		title = exception;
	while (tbl)
	{
		if (!strcasecmp (tbl->title, title))
			return tbl->block_id;
		else if (strlen (title) > 14)
			if (!strncmp (title + 5, "Star Trek", 9))
				if (!strcasecmp (tbl->title, title + 4))
					return tbl->block_id;
		tbl = tbl->next;		
	}
	return 0;
}

int get_entry_id_by_fnbase (struct st_table *tbl, char *fnbase)
{
	char *exception;
	if (!fnbase)
		return 0;
	if ((exception = get_exception (st_file_type, "LU fnbase", fnbase)))
		fnbase = exception;
	while (tbl)
		if (!strcasecmp (tbl->fnbase, fnbase))
			return tbl->id;
		else
			tbl = tbl->next;		
	return 0;
}

int get_entry_id_by_title (struct st_table *tbl, char *title)
{
	char *exception;
	if (!title)
		return 0;
	if ((exception = get_exception (st_file_type, "LU title", title)))
		title = exception;
	while (tbl)
	{
		if (!strcmp (tbl->title, title))
			return tbl->id;
		else if (strlen (title) > 14)
			if (!strncmp (title + 5, "Star Trek", 9))
				if (!strcasecmp (tbl->title, title + 4))
					return tbl->id;
		tbl = tbl->next;		
	}
	return 0;
}

static struct ency_titles *sort_by_epis (struct ency_titles *root, int section)
{
	struct ency_titles *curr, *last;
	struct ency_titles *new_root = NULL, *new_curr = NULL;
	struct st_table *tbl, *atbl;
	char fnbase[8];
	char *title;

	load_entry_list (section);

	tbl = entrylist_head;
	while ((tbl) && (isdigit (tbl->title[0]) || (isblank (tbl->title[0]))))
	{
		curr = root;
		last = NULL;

		if (tbl->fnbase)
		{
			atbl = tbl->next;
			title = get_title (atbl, tbl->fnbase);
			/* Special case */
			if (!strcmp (tbl->fnbase, "oneong"))
				title = "\"11001001\" (TNG)";
			/* Strange cases */
			if (!title)
			{
				sprintf (fnbase, "e%c%c%c%c%c", tbl->fnbase[0], tbl->fnbase[1], tbl->fnbase[2], tbl->fnbase[4], tbl->fnbase[5]);
				title = get_title (atbl, fnbase);
			}
			if (!title)
			{
				sprintf (fnbase, "e%c%c%c%c%c", tbl->fnbase[0], tbl->fnbase[2], tbl->fnbase[3], tbl->fnbase[4], tbl->fnbase[5]);
				title = get_title (atbl, fnbase);
			}

			if (title)
				while (curr)
				{
					if (!strcasecmp (curr->title, title)
					 || !strcasecmp (curr->title, tbl->title+3)
					 || !strcasecmp (curr->title, tbl->title+4))
					{
						if (new_curr)
						{
							new_curr->next = curr;
							new_curr = curr;
						}
						else
							new_root = new_curr = curr;
						if (last)
							last->next = curr->next;
						if (curr == root)
							root = root->next;
						curr->next = NULL;

						free (curr->title);
						curr->title = strdup (tbl->title);
					
						curr = NULL;
					} else
					{
						last = curr;
						curr = curr->next;
					}
				}
		}
		tbl = tbl->next;
	}

	/* Add whats left on to the end, just in case */
	curr = new_root;
	if (curr)
	{
		while (curr->next)
			curr = curr->next;
		curr->next = root;
	}
	return (new_root);
}

struct ency_titles *sort_alphabetical (struct ency_titles *root)
{
        struct ency_titles *lowest, *last_lowest;
        struct ency_titles *curr, *last;
        struct ency_titles *new_root=NULL;

        while (root)
        {
                curr = root;
                last = NULL;
                lowest = root;
                last_lowest = NULL;

                while (curr)
                {
                        if (strcasecmp (curr->title, lowest->title) < 0)
                        {
                                lowest = curr;
                                last_lowest = last;
                        }
                        last = curr;
                        curr = curr->next;
                }

                curr = new_root;
                if (curr)
                {
                        while (curr->next)
                        {
                                curr = curr->next;
                        }
                        curr->next = lowest;
                } else
                        new_root = lowest;

                if (last_lowest)
                        last_lowest->next = lowest->next;
                if (lowest == root)
                        root = root->next;

                lowest->next = NULL;
        }
        return (new_root);
}

static struct ency_titles *sort_entries (struct ency_titles *root, int section, int options)
{
	if (options & ST_OPT_SORTEPIS)
		return (sort_by_epis (root, section));
	else if (options & ST_OPT_SORTALPHA)
		return (sort_alphabetical (root));
	else
		return (root);
}

struct ency_titles *read_entry (FILE *inp, int options)
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

	root_title = (struct ency_titles *) malloc (sizeof (struct ency_titles));

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

void add_to_block_cache (int block_id, int id, long filepos)
{
	if (cache)
	{
		cache_last->next = (struct ency_titles *) malloc (sizeof (struct ency_titles));
		cache_last = cache_last->next;
	}
	else
		cache_last = cache = (struct ency_titles *) malloc (sizeof (struct ency_titles));

	cache_last->filepos = filepos;
	cache_last->block_id = block_id;
	cache_last->id = id;
	cache_last->name = NULL;
	cache_last->title = NULL;
	cache_last->fmt = NULL;
	cache_last->text = NULL;
	cache_last->next = NULL;
}

void load_block_cache (void)
{
	char c;
	int section, n, i=0;
	int id;
	struct st_part *part;

	/* this next for is *NOT* the right way to do this... */
	for (section = 0; section < 3; section++)
	{
		n=0;
		while ((part = get_part(st_file_type, section, n++, 0)))
		{
			if ((part->start_id) && (part->bcount))
			{
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
								c = getc (inp);
								if (isdigit (c) || (c == '@'))
								{
									id++;
									add_to_block_cache (i, id, ftell (inp)-1);
								}
							}
						}
					}
					fclose (inp);
				}
			}
			free (part);
		}
	}
}

long get_block_pos_from_cache (int block_id, int id)
{
	struct ency_titles *curr=NULL;
	int ok_to_go_again=0;


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
		if (curr->block_id > block_id)
			ok_to_go_again = 1;
		if ((curr->block_id == block_id) && (curr->id == id))
			return (curr->filepos);
		cache_quick = curr = curr->next;
	}

	if (ok_to_go_again)
		return -2;
	else
		return -1;
}

static struct ency_titles *get_entry_by_id (int block_id, int id, int options)
{
	static struct ency_titles *ret;
	FILE *input;
	long filepos;

	if (!block_id || !id)
		return NULL;

	if (!cache)
		load_block_cache ();

	filepos = get_block_pos_from_cache (block_id, id);

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
			ret = (struct ency_titles *) malloc (sizeof (struct ency_titles));
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
		return (ret);
	}

	fprintf (stderr, "Uhoh - entry not found @ %d:%d%s\n", block_id, id, (filepos == -2) ? ", retrying" : "");

	if (filepos == -2)
		return (get_entry_by_id (block_id+1, id, options));

	return NULL;
}

static struct ency_titles *st_find_in_cache (int section, char *search_string, int exact, int get_body)
{
	struct ency_titles *mine, *r_r = NULL, *r_c = NULL, *r_l = NULL;

	mine = cache;

	while (mine)
	{
		if (mine->section == section)
			if (check_match (search_string, mine->title, exact))
			{
				r_l = r_c;
				if (get_body)
					r_c = st_get_title_at (mine->filepos);
				else
					st_copy_part_entry (&r_c, mine);
				if (r_l)
					r_l->next = r_c;
				if (!r_r)
					r_r = r_c;
			}
		mine = mine->next;
	}
	return (r_r);
}

static int st_guess_section (char *title, char *text, int last_section)
{
	char *episode_starts[7] =
	{
		"Original Series",
		"Next Generation",
		"Deep Space Nine",
		"Voyager episode",
		"No episodes",
		"There are no episodes",
		"There were no episodes"
	};
	int i;

	/* Episodes */
	if ((*title == '\"') || (strlen (title) == 1))
		for (i = 0; i < 5; i++)
		{
			if (!strncmp (text, episode_starts[i], strlen (episode_starts[i])))
				return 1;
			if (!strncmp (text + 1, episode_starts[i], strlen (episode_starts[i])))
				return 1;
		}
	if ((!strncmp (title, "Star Trek", 9)) && (last_section == 1))
	return 1;

	/* Chronology */
	if (*title == '\"')
		return 2;
	if (!strncmp (text, "\n\n", 2))
		return 2;
	if ((!strncmp (title, "Star Trek", 9)) && (last_section == 2))
		return 2;

	/* Encyclopedia */
	return 0;
}


static struct ency_titles *st_find_unknown (int section, char *search_string, int exact, int options)
{
	long this_one_starts_at = 0;
	struct ency_titles *curr_title = NULL;
	char last_start=0;
	char *new_title=NULL;
	char last_year[5];
	int last_section=-1;
	int prepend_year=0, append_series=0, c=0;

	FILE *input_temp;

	prepend_year = (st_fileinfo_get_data (st_file_type, prepend_year) ? 1 : 0);
	append_series = (st_fileinfo_get_data (st_file_type, append_series) ? 1 : 0);

	if ((cache == NULL) && st_open ())
	{
		while (st_find_start (inp))
		{
			/* st_get_title_at opens the file again, so... */
			input_temp = inp;
			curr_title = st_read_title_at (this_one_starts_at = ftell (input_temp), ST_OPT_NO_FMT | ST_OPT_RETURN_BODY | ST_OPT_TEXTLEN_MAX32);
			inp = input_temp;
			getc (inp);	/* make sure we dont get the same entry again */

			/* determine what section its in */
			if ((!last_start) || (last_start > tolower (*curr_title->title)) || ((last_start == '\"') && (*curr_title->title != '\"')))
				last_section = st_guess_section (curr_title->title, curr_title->text, last_section);
			last_start = *curr_title->title;

			/* some chronology entries need years prepended */
			if ((last_section == 2) && prepend_year)
			{
				/* if it's a year, save it */
				if (strlen (curr_title->title) == 4)
				{
					strcpy (last_year, curr_title->title);
				}
				/* if it's an episode or a movie, add the year */
				if ((*curr_title->title == '\"') || (!strncmp (curr_title->title, "Star Trek", 9)))
				{
					new_title = (char *) malloc (strlen (curr_title->title) + 6);
					sprintf (new_title, "%s %s", last_year, curr_title->title);
					free (curr_title->title);
					curr_title->title = new_title;
				}
			}

			/* and episode entries need (TOS) etc. appended */
			if ((curr == 1) && append_series)
			{
				getc (inp);
				c = getc (inp);
				if (c == 0x0D)
				{
					c = getc (inp);
					fseek (inp, -1, SEEK_CUR);
				}
				fseek (inp, -2, SEEK_CUR);
				new_title = (char *) malloc (strlen (curr_title->title) + 7);
				switch (c)
				{
				case 'O':
					sprintf (new_title, "%s (TOS)", curr_title->title);
					break;
				case 'N':
					sprintf (new_title, "%s (TNG)", curr_title->title);
					break;
				case 'D':
					sprintf (new_title, "%s (DS9)", curr_title->title);
					break;
				case 'V':
					sprintf (new_title, "%s (VGR)", curr_title->title);
					break;
				default:
					sprintf (new_title, "%s", curr_title->title);
					break;
				}
				free (curr_title->title);
				curr_title->title = new_title;
			}

			/* build the cached version of this entry */
			st_add_to_cache (last_section,curr_title->title,this_one_starts_at);
		}
	}
	return (st_find_in_cache (section, search_string, exact, 1));
}

static struct ency_titles *st_find_in_file (int file, int section, char *search_string, int exact, int options)
{
	struct ency_titles *root = NULL, *curr = NULL;
	struct st_table *tmp = NULL;
	struct st_table *tbl=NULL;
	int skip;

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
			skip = 0;
			if (section == 1) /* Episodes */
			{
				if (isdigit(tmp->title[0]) || isblank (tmp->title[0]))
				{
					if (!(options & ST_OPT_SORTEPIS))
						skip = 1;
				} else
				{
					if (options & ST_OPT_SORTEPIS)
						skip = 1;
				}
			}

			if (!skip && check_match (search_string, tmp->title, exact))
			{
				if (tbl)
					tbl = get_table_entry_by_title (tbl, tmp->title);
				if (!tbl)
					tbl = get_table_entry_by_title (entrylist_head, tmp->title);
				if (curr)
				{
					curr->next = get_entry_by_id (tbl->block_id, tbl->id, options);
					curr = curr->next;
				}
				else
					root = curr = get_entry_by_id (tbl->block_id, tbl->id, options);
				curr->name = strdup (tmp->title);
			}
		}
		tmp = tmp->next;
	}

	return root;
}

int ft_list_has_section (int section)
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

void load_ft_list (int section)
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

	while ((part = get_part (st_file_type, section, count++, ST_PART_OPT_FTLIST)))
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
		free (part);
	}
	ftlist = root;
}

struct ency_titles *st_find_fulltext (char *search_string, int section, int options)
{
	struct st_ftlist *fl;
	struct st_wl *wl;
	struct ency_titles *root=NULL, *curr=NULL;
	struct st_table *tbl=NULL;
	char *last=NULL, *title;
	int bad;

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

	fl = ftlist;

	if (options & ST_OPT_NO_CACHE)
		options -= ST_OPT_NO_CACHE;
	if (options & ST_OPT_MATCH_SUBSTRING)
		options -= ST_OPT_MATCH_SUBSTRING;
	options -= ST_OPT_FT;

	while (fl)
	{
		if (fl->word)
			last = fl->word;

		if (!strcasecmp (last, search_string))
		{
			title = get_title (st_ptbls, fl->fnbase);
			if (title)
			{
				tbl = get_table_entry_by_title (entrylist_head, title);
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
					curr->next = NULL;
				}
			}
		}
		fl = fl->next;
	}

	return root;
}

struct ency_titles *st_find (char *search_string, int section, int options)
{
	int exact = 0;

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

	switch (section)
	{
	case ST_SECT_ENCY:
	case ST_SECT_EPIS:
	case ST_SECT_CHRO:
		if (options & ST_OPT_FT)
			return (st_find_fulltext (search_string, section, options));
		if (cache_has_section(section))
			return (st_find_in_cache (section, search_string, exact, st_return_body ? 1 : 0));
		else
			if (st_file_type == ST_FILE_UNKNOWN)
				return (st_find_unknown (section, search_string, exact, options));
			else
				return (st_find_in_file (st_file_type, section, search_string, exact, options));
	default:
		return (NULL);
	}

}

static struct st_photo st_parse_captions (char *fnbasen)
{
	struct st_photo photo;
	struct st_caption *temp_pcpts = NULL;

	temp_pcpts = st_pcpts;

	strcpy (photo.file, "");
	strcpy (photo.caption, "");
	while (temp_pcpts && (!strlen (photo.file)))
	{
		if (!strcmp (fnbasen, temp_pcpts->fnbasen))
		{
			strcpy (photo.file, fnbasen);
			strcpy (photo.caption, temp_pcpts->caption);
		}
		temp_pcpts = temp_pcpts->next;
	}

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
		if (!strcmp (fnbasen, temp_vcpts->fnbasen))
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
		for (i=0;i<5;i++)
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

struct st_media *st_get_media (char *search_string)
{
	int i = 0;
	int media_found = 0;
	struct st_media *media = NULL;
	char *temp_fnbase = NULL;
	char *ret_fnbase = NULL;
	struct st_table *temp_ptbls = NULL;
	struct st_table *temp_vtbls = NULL;

	if (st_loaded_media ())
	{

		temp_ptbls = st_ptbls;
		temp_vtbls = st_vtbls;

		temp_fnbase = malloc (9);

		media = new_media (media);

		if ((ret_fnbase = get_fnbase (st_ptbls, search_string)))
		{
			for (i = 0; i < 5; i++)
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
		}

		if ((ret_fnbase = get_fnbase (st_vtbls, search_string)))
		{
			media->video = st_parse_video_captions (ret_fnbase);
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

char *st_format_filename (char *fnbasen, char *base_path, media_type media)
{
	char *filename = NULL;
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
			dir_size += strlen (st_fileinfo_get_data(st_file_type,video_dir));
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
			strcat (filename, st_fileinfo_get_data(st_file_type,video_dir));
			break;
		}

		strcat (filename, "/");
		if (st_fileinfo_get_data(st_file_type,append_char))
		{
			filename[dir_size + 2] = fnbasen[0];
			filename[dir_size + 3] = 0;
			strcat (filename, "/");
		}
		strncat (filename, fnbasen, 8);

		switch (media)
		{
		case audio:
			strcat (filename, "a.aif");
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
