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
#include "ency.h"

#ifndef DONT_USE_XML
#include "data.h"
#endif

static FILE *inp;

static char *ency_filename = NULL;

int st_return_body = 1;
int st_ignore_case = 0;
static int force_unknown = 0;
static int st_file_type = 0;

#ifdef DONT_USE_XML
/* for pictures */
static const long int st_table_starts_at[] =
{0x410c4, 0, 0x388f2e, 0, 0x3CD470, 0, 0x2BBA98, 0x2BCD9B, 0, 0x322996, 0};

static const long int st_caption_starts_at[] =
{0x4e5064, 0, 0x615552, 0, 0x646D0A, 0, 0x2C5F2C, 0, 0x37CB82, 0};

/* for videos */
static const long st_video_table_starts_at[] =
{0x50e690, 0, 0x59baaa, 0, 0x5b9820, 0, 0x3967aa, 0, 0x37684a, 0};
static const long st_video_caption_starts_at[] =
{0x4FCED6, 0, 0x621174, 0, 0x65860a, 0, 0x2bba98, 0, 0x322968, 0};

/* the actual encyclopedia entries */
static const long int ency_starts_at[] =
{0x7bc28, 0x39d0d8, 0x576574, 0, 0x3A9ED8, 0x56BB62, 0, 0x3FC3BE, 0x58B51E, 0x72E89C, 0, 0x2D2A6E, 0, 0x324B34, 0x390C40, 0};

static const long int epis_starts_at[] =
{0x3b8e20, 0x397322, 0x50431A, 0, 0x5D961A, 0x622AA4, 0, 0x606630, 0x659F9E, 0, 0x1, 0, 0x1, 0};

static const long int chro_starts_at[] =
{0x41e32c, 0, 0x62764A, 0, 0x66B9C4, 0, 0x1, 0, 0x1, 0};

/* hm. articles or sections or whatever to get */
static const long int ency_lastone[] =
{6814, 230, 68, 0, 4092, 491, 0, 3904, 476, 1347, 0, 181, 0, 89, 42, 0};

static const long int epis_lastone[] =
{402, 23, 3, 0, 261, 25, 0, 262, 93, 0, 0x1, 0, 0x1, 0};

static const long int chro_lastone[] =
{582, 0, 465, 0, 582, 0, 0x1, 0, 0x1, 0};

static const long int st_table_lastone[] =
{26, 0, 26, 0, 26, 0, 2, 22, 0, 15, 0};

static const long int st_caption_lastone[] =
{5, 0, 4, 0, 4, 0, 4, 0, 4, 0};

static const long st_video_table_lastone[] =
{26, 0, 26, 0, 26, 0, 26, 0, 26, 0};

static const long st_video_caption_lastone[] =
{1, 0, 1, 0, 1, 0, 24, 0, 15, 0};

struct st_file_info
{
	char *name;
	char *filename;
	char *data_dir;
	char *pic_dir;
	char *vid_dir;
	int append_char;
	int prepend_year;
	int append_series;
	int fingerprint[16];
	long int filesize;
};

const struct st_file_info st_files[] =
{
	{"Encyclopedia", "Data.cxt", "Ency98", "media98", "video98", 1, 1, 1,
	 {0x52, 0x49, 0x46, 0x58, 0x0, 0x99, 0xD7, 0x6E, 0x4D, 0x43, 0x39, 0x35, 0x69, 0x6D, 0x61, 0x70}, 1},
	{"Omnipedia", "OMNI1.DXR", "", "media", "media", 1, 1, 1,
	 {0x58, 0x46, 0x49, 0x52, 0xBC, 0x42, 0xB7, 0x0, 0x33, 0x39, 0x56, 0x4D, 0x70, 0x61, 0x6D, 0x69}, 1},
	{"Omnipedia (updated)", "omni_v2.dxr", "startrek", "media", "media", 1, 1, 1,
	 {0x52, 0x49, 0x46, 0x58, 0x0, 0xFa, 0x1C, 0x7A, 0x4D, 0x56, 0x39, 0x33, 0x69, 0x6D, 0x61, 0x70}, 1},
	{"TNG Episode guide", "eg_tng.dxr", "source", "media", "media", 1, 0, 0,
	 {0x52, 0x49, 0x46, 0x58, 0x00, 0x51, 0x91, 0xF4, 0x4D, 0x56, 0x39, 0x33, 0x69, 0x6D, 0x61, 0x70}, 1},
	{"DS9 Episode guide", "eg_ds9.dxr", "ds9", "media", "media", 1, 0, 0,
	 {0x52, 0x49, 0x46, 0x58, 0x0, 0x4C, 0xAE, 0xC4, 0x4D, 0x56, 0x39, 0x33, 0x69, 0x6D, 0x61, 0x70}, 1}
};
#endif

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

static struct ency_titles *st_find_in_cache (int, char *, int, int);
static void st_clear_cache (void);

/* init/de-init stuff */
int st_init (void)
{
#ifndef DONT_USE_XML
	load_file_info();
#endif
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

#ifndef DONT_USE_XML
int st_count_filetypes(void)
{
	return count_files();
}
#endif

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

#ifdef DONT_USE_XML
char *st_fileinfo_get_name (int file_type)
{
	if (file_type == ST_FILE_CURR)
	{
		if (st_file_type == ST_FILE_UNKNOWN)
			return (("Unknown encyclopedia"));
		return (st_files[st_file_type].name);
	}
	else
	{
		if (file_type == ST_FILE_UNKNOWN)
			return ("Unknown encyclopedia");
		return (st_files[file_type].name);
	}
}

static const char *st_fileinfo_get_data (int file, st_filename_type type)
{
	if (file == ST_FILE_UNKNOWN)
	{
		switch (type)
		{
		case mainfilename:
			return ency_filename ? ency_filename : "";
		case data_dir:
			return "";
		case picture_dir:
			return "";
		case video_dir:
			return "";
		case append_char:
			return "yes";
#ifndef ENCY_DONT_PREPEND_APPEND_TO_UNKNOWN
		case prepend_year:
			return "yes";
		case append_series:
			return "yes";
#endif
		default:
			return NULL;
		}
	}

	if ((file < 0) || (file >= ST_FILE_TYPES))
			return NULL;
	else
		switch (type)
		{
			case mainfilename:
				return st_files[file].filename;
			case data_dir:
				return st_files[file].data_dir;
			case picture_dir:
				return st_files[file].pic_dir;
			case video_dir:
				return st_files[file].vid_dir;
			case append_char:
				return st_files[file].append_char ? "yes" : NULL;
			case prepend_year:
				return st_files[file].prepend_year ? "yes" : NULL;
			case append_series:
				return st_files[file].append_series ? "yes" : NULL;
			default:
				return NULL;
		}
}

static struct st_part *get_part (int file, int section, int number)
{
	int i, tmp = 0;
	struct st_part *ret;
	long *starts;
	long *counts;

	switch (section)
	{
	case ST_SECT_ENCY:
		starts = (long *) ency_starts_at;
		counts = (long *) ency_lastone;
		break;
	case ST_SECT_EPIS:
		starts = (long *) epis_starts_at;
		counts = (long *) epis_lastone;
		break;
	case ST_SECT_CHRO:
		starts = (long *) chro_starts_at;
		counts = (long *) chro_lastone;
		break;
	case ST_SECT_PTBL:
		starts = (long *) st_table_starts_at;
		counts = (long *) st_table_lastone;
		break;
	case ST_SECT_VTBL:
		starts = (long *) st_video_table_starts_at;
		counts = (long *) st_video_table_lastone;
		break;
	case ST_SECT_PCPT:
		starts = (long *) st_caption_starts_at;
		counts = (long *) st_caption_lastone;
		break;
	case ST_SECT_VCPT:
		starts = (long *) st_video_caption_starts_at;
		counts = (long *) st_video_caption_lastone;
		break;
	default:
		return NULL;
	}

	/* find the right file */
	for (i = 0; i < file; i++)
	{
		while (starts[tmp] != 0)
			tmp++;
		tmp++;
	}

	/* find the part theyre after */
	for (i = 0; i < number; i++)
	{
		if (starts[tmp])
			tmp++;
	}

	/* if there is one there, return the part data */
	/* a '1' means 'Reserved' ATM */
	if (starts[tmp] > 1)
	{
		ret = (struct st_part *) malloc (sizeof (struct st_part));
		if (!ret)
			return NULL;

		ret->start = starts[tmp];
		ret->count = counts[tmp];

		return ret;
	}
	return NULL;
}
#else
char *st_fileinfo_get_name (int file_type)
{
	if (file_type > ST_FILE_TYPES)
		return "Unknown";

	if (file_type == -1)
		file_type = st_file_type;

	return get_name_of_file (file_type);
}
#endif

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
	}
	return (i == 0 ? (int) inp : 0);
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

#ifdef DONT_USE_XML
int st_fingerprint (void)
{
	int i = 0, z = 0;
	unsigned char input_fp[16];

	curr = 5;

	set_starts_at = 0;

	i = st_open ();
	if (i)
	{
		fread (input_fp, 1, 16, inp);

/* compare fingerprints etc... */
		for (i = 0; i < ST_FILE_TYPES; i++)
		{
			for (z = 0; z < 16; z++)
				if (input_fp[z] != st_files[i].fingerprint[z])
					break;

			if (z == 16)
			{
				st_close_file ();
				return i;
			}
		}
		st_close_file ();
		return (254);
	}
	else
	{
		return (255);
	}
}
#endif

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
		fgetc (input);
		fgetc (input);
		if (strstr (temp_text, "\""))
		{
			fseek (input, -7, SEEK_CUR);
			while ((c = getc (input)) != '\"');
			str_text = (strstr (temp_text, "\""));
			str_text[0] = 0;
		}

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
			if (c == 0xA5)
			{
				getc (input);
			}
			else
			{
				temp_text[text_size++] = st_cleantext (c);
			}
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

	while ((part = get_part (st_file_type, ST_SECT_PTBL, count)))
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

	while ((part = get_part (st_file_type, section, count)))
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

static struct st_table *st_get_video_table (void)
{
	int i = 0;
	struct st_table *root_tbl = NULL, *curr_tbl = NULL, *last_tbl = NULL;
	struct st_part *part;
	char c = 0;
	int text_size;
	int level, commas, count = 0;
	char *temp_text = NULL;

	curr = 7;

	while ((part = get_part (st_file_type, ST_SECT_VTBL, count)))
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
						}
						while (text_size);

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
							while (level)
							{
								c = getc (inp);
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
								}
							}
							c = getc (inp);
						}

						if (curr_tbl->fnbase)
						{
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
				}	/* end main loop */

			}
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
			st_vtbls = st_get_video_table ();
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
				if ((old_old_c == 0x16) && (old_old_old_c == 0))
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
	return_error->err = error_no;

	return (return_error);
}


static struct ency_titles *curr_find_list (int section, char *search_string, int exact, int options)
{
	long this_one_starts_at = 0, temp_pos;
	int found_any_yet = 0;
	int first_time = 1;
	char c;
	int i = 0, z = 1;
	struct ency_titles *root_title = NULL, *curr_title = NULL, *last_title = NULL;
	int no_so_far = 0;
	char *title = NULL, *temp_text = NULL, *new_title = NULL;
	struct st_ency_formatting *text_fmt = NULL;
	char last_year[5] = "";
	int prepend_year=0, append_series=0;

	if (!st_open ())
	{
		return (st_title_error (1));
	};

	prepend_year = (st_fileinfo_get_data (st_file_type, prepend_year) ? 1 : 0);
	append_series = (st_fileinfo_get_data (st_file_type, append_series) ? 1 : 0);
	do
	{
		no_so_far++;
		if (!first_time)
			z = st_find_start (inp);
		if (z)
		{
			this_one_starts_at = ftell (inp);

			first_time = 0;
			while ((getc(inp) != '@'));
			getc(inp);

			i = 0;

			title = st_return_title ();

			/* some chronology entries need years prepended */
			if ((curr == 3) && prepend_year)
			{
				/* if it's a year, save it */
				if (strlen (title) == 4)
				{
					strcpy (last_year, title);
				}
				/* if it's an episode or a movie, add the year */
				if ((*title == '\"') || (!strncmp (title, "Star Trek", 9)))
				{
					new_title = (char *) malloc (strlen (title) + 6);
					sprintf (new_title, "%s %s", last_year, title);
					free (title);
					title = new_title;
				}
			}

			/* and episode entries need (TOS) etc. appended */
			if ((curr == 2) && append_series)
			{
				getc (inp);
				c = getc (inp);
				if (c == 0x0D)
				{
					c = getc (inp);
					fseek (inp, -1, SEEK_CUR);
				}
				fseek (inp, -2, SEEK_CUR);
				new_title = (char *) malloc (strlen (title) + 7);
				switch (c)
				{
				case 'O':
					sprintf (new_title, "%s (TOS)", title);
					break;
				case 'N':
					sprintf (new_title, "%s (TNG)", title);
					break;
				case 'D':
					sprintf (new_title, "%s (DS9)", title);
					break;
				case 'V':
					sprintf (new_title, "%s (VGR)", title);
					break;
				default:
					sprintf (new_title, "%s", title);
					break;
				}
				free (title);
				title = new_title;
			}

#ifdef ENCY_DEBUG_PRINT_TITLES
			fprintf (stderr, "%d:%s\n", no_so_far, title);
#endif
			/* build the cached version of this entry */
			if (!(options & ST_OPT_NO_CACHE))
				st_add_to_cache (section, title, this_one_starts_at);

			c = getc (inp);

			if (check_match (search_string, title, exact))
			{	/* If its a match... */
				found_any_yet = 1;
				if (st_return_body)
				{
					temp_text = st_return_text (0);
					/* this is a cheat. step back in the
					   file to get the formatting. doing
					   it this way saves time in the
					   likely chance that the entry is
					   the wrong one */
					if (!(options & ST_OPT_NO_FMT))
					{
						temp_pos = ftell (inp);
						fseek (inp, this_one_starts_at, SEEK_SET);
						text_fmt = st_return_fmt();
						fseek (inp, temp_pos, SEEK_SET);
					}
				}
					

				/* define the pointer */
				{
					curr_title = (struct ency_titles *) malloc (sizeof (struct ency_titles));

					if (curr_title == NULL)
						printf ("Memory allocation failed\n");
				}
				if ((root_title) == NULL)
					root_title = curr_title;

/* copy pointer stuff over */
				curr_title->err = 0;
				curr_title->filepos = this_one_starts_at;
				curr_title->title = title;
				curr_title->next = NULL;
				curr_title->text = temp_text;
				curr_title->fmt = text_fmt;
				if (last_title != NULL)
					last_title->next = curr_title;
				last_title = curr_title;
				curr_title = NULL;
				title = NULL;
				temp_text = NULL;
				text_fmt = NULL;
/* */
			}
			else
				/* It's not the one we want */
			{
				free (title);
			}
		}
	}
	while (no_so_far != curr_lastone);
	st_close_file ();

	if (found_any_yet)
		return (root_title);
	else
		return (NULL);
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
	struct ency_titles *root = NULL, *current = NULL, *temp = NULL;
	struct st_part *part = NULL;
	int first_time = 1;
	int count = 0;

	curr = section + 1;

	while ((part = get_part (st_file_type, section, count)))
	{
		if (current)
			while (current->next)
				current = current->next;
		curr_lastone = part->count;
		curr_starts_at = part->start;
		if (!first_time)
		{
			temp = (curr_find_list (section, search_string, exact, options));
			if (current)
				current->next = temp;
			else
				root = current = temp;
		}
		else
		{
			root = (curr_find_list (section, search_string, exact, options));
			current = root;
			first_time = 0;
		}
		count++;
		free (part);
	}
	return (root);
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
		if ((cache_has_section(section)) && (!st_return_body))
			return (st_find_in_cache (section, search_string, exact, 0));
		else
			if (st_file_type == ST_FILE_UNKNOWN)
				return (st_find_unknown (section, search_string, exact, options));
			else
				return (st_find_in_file (st_file_type, section, search_string, exact, options));
	default:
		return (NULL);
	}
}

struct ency_titles *st_read_title_at (long filepos, int options)
{
	int return_body_was;
	char c;
	char *temp_text = NULL;
	int i = 0;
	struct ency_titles *root_title = NULL, *curr_title = NULL;
	char *ttl = NULL;
	struct st_ency_formatting *text_fmt = NULL;

	root_title = NULL;
	return_body_was = st_return_body;
	st_return_body = 1;
	set_starts_at = filepos;
	curr = 5;

	if (!st_open ())
	{
		return (st_title_error (1));
	}
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
	root_title->fmt = text_fmt;
	root_title->err = 0;
	st_return_body = return_body_was;
	st_close_file ();

	/* but of course, the title is often mangled :( */
	/* thus, we check the cache for an entry w/ the same */
	/* filepos, and use its title */
	curr_title = cache;
	while (curr_title)
	{
		if (curr_title->filepos == filepos)
		{
			free (root_title->title);
			root_title->title = strdup (curr_title->title);
			/* make sure we break out of the loop */
			curr_title = NULL;
			i = 3;
		}
		if (curr_title)
			curr_title = curr_title->next;
	}

	return (root_title);
}

struct ency_titles *st_get_title_at (long filepos)
{
	return (st_read_title_at (filepos, ST_OPT_RETURN_BODY));
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

	if (old_media)
		return (old_media);

	media = malloc (sizeof (struct st_media));
	if (media)
	{
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
	char *title_with_dot = NULL;
	struct st_table *temp_ptbls = NULL;
	struct st_table *temp_vtbls = NULL;

	if (st_loaded_media ())
	{

		temp_ptbls = st_ptbls;
		temp_vtbls = st_vtbls;

		temp_fnbase = malloc (9);

		title_with_dot = malloc (strlen (search_string) + 2);
		sprintf (title_with_dot, "%s.", search_string);

		media = new_media (media);

		while (temp_ptbls)
		{
			if ((!strcmp (temp_ptbls->title, search_string)) || (!strcmp (temp_ptbls->title, title_with_dot)))
			{
				for (i = 0; i < 5; i++)
				{
					sprintf (temp_fnbase, "%s%d", temp_ptbls->fnbase, i + 1);
					media->photos[i] = st_parse_captions (temp_fnbase);
					if (strlen (media->photos[i].file))
						media_found = 1;
				}
				sprintf (temp_fnbase, "%sf", temp_ptbls->fnbase);
				media->swf = st_parse_captions (temp_fnbase);
				if (strlen (media->swf.file))
					media_found = 1;
				
				goto end_photo_search;
			}
			temp_ptbls = temp_ptbls->next;
		}

	      end_photo_search:

		while (temp_vtbls)
		{
			if ((!strcmp (temp_vtbls->title, search_string)) || (!strcmp (temp_vtbls->title, title_with_dot)))
			{
				media->video = st_parse_video_captions (temp_vtbls->fnbase);
				media_found = 1;
				goto end_video_search;
			}
			temp_vtbls = temp_vtbls->next;
		}

	      end_video_search:

		if (!media_found)
		{
			free (media);
			media = NULL;
		}

		free (temp_fnbase);
		free (title_with_dot);
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
