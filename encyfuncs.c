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
#include <ctype.h>
#include <string.h>
#include "ency.h"

static FILE *inp;

static char *ency_filename = NULL;

int st_return_body = 1;
int st_ignore_case = 0;
static int upto = 0;
static int st_file_type = 0;

static const long int st_table_starts_at[] =
{0x410c4, 0, 0x388f2e, 0, 0x3CD470, 0, 0x2BBA98, 0x2BCD9B, 0, 0x322996, 0};

static const long int st_caption_starts_at[] =
{0x4e5064, 0, 0x615552, 0, 0x646D0A, 0, 0x2C5F2C, 0, 0x37CB82, 0};

static const long int ency_starts_at[] =
{0x7bc28, 0x576574, 0, 0x3A9ED8, 0x56BB62, 0, 0x3FC3BE, 0x58B51E, 0x72E89C, 0, 0x1, 0, 0x1, 0};

static const long int epis_starts_at[] =
{0x3b8e20, 0x50431A, 0, 0x5D961A, 0x622AA4, 0, 0x606630, 0x659F9E, 0, 0x2D2A6E, 0, 0x324B34, 0x390C40, 0};

static const long int chro_starts_at[] =
{0x41e32c, 0, 0x62764A, 0, 0x66B9C4, 0, 0x1, 0, 0x1, 0};

static long int set_starts_at = 0x0;

static const long int ency_lastone[] =
{7068, 68, 0, 4092, 491, 0, 3905, 476, 1353, 0, 0x1, 0, 0x1, 0};

static const long int epis_lastone[] =
{402, 3, 0, 261, 25, 0, 262, 93, 0, 181, 0, 89, 42, 0};

static const long int chro_lastone[] =
{582, 0, 465, 0, 582, 0, 0x1, 0, 0x1, 0};

static const long int st_table_lastone[] =
{26, 0, 26, 0, 26, 0, 2, 22, 0, 15, 0};

static const long int st_caption_lastone[] =
{5, 0, 4, 0, 4, 0, 5, 0, 4, 0};

static long int curr_starts_at, curr_lastone, curr;

const struct st_file_info st_files[] =
{
  {"Encyclopedia", "Data.cxt", "Ency98", "media98", "video98", 1,
   {0x52, 0x49, 0x46, 0x58, 0x0, 0x99, 0xD7, 0x6E, 0x4D, 0x43, 0x39, 0x35, 0x69, 0x6D, 0x61, 0x70}, 1},
  {"Omnipedia", "omni1.dxr", "", "", "", 1,
   {0x58, 0x46, 0x49, 0x52, 0xBC, 0x42, 0xB7, 0x0, 0x33, 0x39, 0x56, 0x4D, 0x70, 0x61, 0x6D, 0x69}, 1},
  {"Omnipedia (updated)", "omni_v2.dxr", "startrek", "media", "media", 1,
   {0x52, 0x49, 0x46, 0x58, 0x0, 0xFa, 0x1C, 0x7A, 0x4D, 0x56, 0x39, 0x33, 0x69, 0x6D, 0x61, 0x70}, 1},
  {"TNG Episode guide", "eg_tng.dxr", "source", "media", "media", 1,
   {0x52, 0x49, 0x46, 0x58, 0x00, 0x51, 0x91, 0xF4, 0x4D, 0x56, 0x39, 0x33, 0x69, 0x6D, 0x61, 0x70}, 1},
  {"DS9 Episode guide", "eg_ds9.dxr", "ds9", "media", "media", 1,
   {0x52, 0x49, 0x46, 0x58, 0x0, 0x4C, 0xAE, 0xC4, 0x4D, 0x56, 0x39, 0x33, 0x69, 0x6D, 0x61, 0x70}, 1}
};

static struct st_caption *st_cpts = NULL, *st_oldcpts = NULL;
static struct st_table *st_tbls = NULL, *st_oldtbls = NULL;

int st_init (void)
{
  st_fingerprint ();
  return (1);
}

int st_finish (void)
{
  if (st_loaded_media ())
    st_unload_media ();
  if (ency_filename)
    free (ency_filename);
  return (1);
}

int st_set_filename (char *filename)
{
  int type = 255;
  if (ency_filename)
    free (ency_filename);
  ency_filename = strdup (filename);
  if (ency_filename) {
    type = st_fingerprint ();
    if (0 <= type < ST_FILE_TYPES) {
      st_file_type = type;
      return (1);
    } else {
      free (ency_filename);
      return (0);
    }
  }
  return (0);
}

char *st_get_filename (void)
{
  return (ency_filename);
}

char *st_fileinfo_get_name (int file_type)
{
  if (file_type == ST_FILE_CURR)
    return (st_files[st_file_type].name);
  else
    return (st_files[file_type].name);
}

int st_load_media (void)
{
  /*
   * Get the table & captions (if we don't already have them)
   */
  if (!st_tbls)
    st_tbls = st_get_table ();
  if (!st_cpts)
    st_cpts = st_get_captions ();
  return (1);
}

int st_loaded_media (void)
{
  if ((st_cpts) && (st_tbls))
    return (1);
  else
    return (0);
}

int st_unload_media (void)
{
/* Free the caption & table info */
  while (st_cpts) {
    st_oldcpts = st_cpts;
    st_cpts = st_cpts->next;
    if (st_oldcpts) {
      if (st_oldcpts->fnbasen)
	free (st_oldcpts->fnbasen);
      if (st_oldcpts->caption)
	free (st_oldcpts->caption);
      free (st_oldcpts);
    }
  }

  while (st_tbls) {
    st_oldtbls = st_tbls;
    st_tbls = st_tbls->next;
    if (st_oldtbls) {
      if (st_oldtbls->fnbase)
	free (st_oldtbls->fnbase);
      if (st_oldtbls->title)
	free (st_oldtbls->title);
      free (st_oldtbls);
    }
  }
  return (1);
}

static int curr_open (void)
{
  int i = 0;
  char *temp_fn = NULL;

  if (ency_filename == NULL) {
    temp_fn = getenv ("ENCY_FILENAME");
    if (temp_fn == NULL) {
      st_set_filename ("Data.cxt");
    } else {
      st_set_filename (temp_fn);
    }
  }
  inp = fopen (ency_filename, "rb");

  i = 0;
  if (inp) {
    i = fseek (inp, curr_starts_at, SEEK_SET);
  }
  if (i == 0)
    return ((int) inp);
  else
    return (0);
}

static int st_open ()
{
  if (curr == 0)		/* Defaults to ency */
    curr_starts_at = ency_starts_at[upto];

  if (curr == 1)		/* Ency */
    curr_starts_at = ency_starts_at[upto];

  if (curr == 2)		/* Epis */
    curr_starts_at = epis_starts_at[upto];

  if (curr == 3)		/* Chro */
    curr_starts_at = chro_starts_at[upto];

  if (curr == 4)		/* table */
    curr_starts_at = st_table_starts_at[upto];

  if (curr == 5)		/* Set value */
    curr_starts_at = set_starts_at;

  if (curr == 6)		/* Captions */
    curr_starts_at = st_caption_starts_at[upto];

  return (curr_open ());
}


static int st_close_file (void)
{
  fclose (inp);
  return (0);
}

static char st_cleantext (unsigned char c)
{
  switch (c) {
  case 13:
    return ('\n');
    break;
  case 0xD0:
    return ('-');
    break;
  case 0xD1:
    return ('-');
    break;
  case 0xD2:
    return (34);
    break;
  case 0xD3:
    return (34);
    break;
  case 0xD4:
    return (39);
    break;
  case 0xD5:
    return (39);
    break;
  case 0x88:
    return ('a');
    break;
  case 0x8E:
    return ('e');
    break;
  case 0x8F:
    return ('e');
    break;
  case 0xA5:
    return ('*');
    break;
  case 0x93:
    return ('\"');
    break;
  case 0x94:
    return ('\"');
    break;
  case 0x92:
    return ('\'');
    break;
  default:
    return (c);
    break;
  }
}


static char *st_lcase (char *mcase)
{
  char *lcase = NULL;
  int i = 0;
  int length = 0;

  length = strlen (mcase) + 1;
  lcase = (char *) malloc (length);

  for (i = 0; i < length; i++)
    lcase[i] = tolower (mcase[i]);

  return (lcase);
}

int st_fingerprint (void)
{
  int good = 0;
  int i = 0, z = 0;
  int input_fp[16];

  curr = 5;

  set_starts_at = 0;

  i = st_open ();
  if (i) {
    for (i = 0; i < 16; i++)
      input_fp[i] = getc (inp);

/* compare fingerprints etc... */
    for (i = 0; i < ST_FILE_TYPES; i++) {
      good = 0;
      for (z = 0; z < 16; z++) {
	if (input_fp[z] == st_files[i].fingerprint[z])
	  good++;
      }
      if (good == 16) {
	st_close_file ();
	return i;
      }
    }
    st_close_file ();
    return (254);
  } else {
    return (255);
  }
}

char *st_autofind (int st_file_version, char *base_dir)
{
  char *test_filename = NULL;
  char *ency_fn_backup = NULL;
  char *data_dir = NULL, *filename = NULL;
  char *lc_data_dir = NULL, *lc_filename = NULL;

  if ((st_file_version < ST_FILE_TYPES) && (st_file_version >= 0)) {

    data_dir = st_files[st_file_version].data_dir;
    filename = st_files[st_file_version].filename;
    lc_data_dir = st_lcase (data_dir);
    lc_filename = st_lcase (filename);

    ency_fn_backup = ency_filename;
    test_filename = malloc (strlen (base_dir) + strlen (data_dir) + strlen (filename) + 3);
    ency_filename = test_filename;

    sprintf (test_filename, "%s", base_dir);
    if (st_fingerprint () == st_file_version) {
      ency_filename = ency_fn_backup;
      return (test_filename);
    }

    sprintf (test_filename, "%s/%s",base_dir, filename);
    if (st_fingerprint () == st_file_version) {
      ency_filename = ency_fn_backup;
      return (test_filename);
    }

    sprintf (test_filename, "%s/%s/%s", base_dir, data_dir, filename);
    if (st_fingerprint () == st_file_version) {
      ency_filename = ency_fn_backup;
      return (test_filename);
    }

    sprintf (test_filename, "%s/%s", base_dir, lc_filename);
    if (st_fingerprint () == st_file_version) {
      ency_filename = ency_fn_backup;
      return (test_filename);
    }

    sprintf (test_filename, "%s/%s/%s", base_dir, lc_data_dir, lc_filename);
    if (st_fingerprint () == st_file_version) {
      ency_filename = ency_fn_backup;
      return (test_filename);
    }

    sprintf (test_filename, "%s/%s/%s", base_dir, data_dir, lc_filename);
    if (st_fingerprint () == st_file_version) {
      ency_filename = ency_fn_backup;
      return (test_filename);
    }

    sprintf(test_filename, "%s/%s/%s", base_dir, lc_data_dir, lc_filename);
    if (st_fingerprint () == st_file_version) {
      ency_filename = ency_fn_backup;
      return (test_filename);
    }

    free (lc_filename);
    free (lc_data_dir);
  }
  return (NULL);
}

static int st_find_start (void)
{
  unsigned char c=0,old_c=0;
  int keep_going = 1;
  while (keep_going)
    {
      c=getc (inp);
      switch (c)
	{
	case '~':
	  keep_going = 0;
	  break;
	case 0x16:
	  if (old_c == 0) keep_going = 0;
	  break;
	}
      old_c = c;
    }
}

static struct st_ency_formatting *st_return_fmt (void)
{
  struct st_ency_formatting *root_fmt = NULL, *last_fmt = NULL, *curr_fmt = NULL;
  int first_time = 1;
  char c = 0;
  int i = 0;

  c = getc (inp);

  while (c != '@') {
    if (st_return_body) {
      curr_fmt = (struct st_ency_formatting *) malloc (sizeof (struct st_ency_formatting));

      if (curr_fmt == NULL) {
	printf ("Memory allocation failed\n");
	exit (1);
      }
    }
    if (first_time)
      root_fmt = curr_fmt;

    first_time = 0;
    i = 0;

    while (c != ':') {
      if ((c != 20) && (c != ',') && (st_return_body))
	curr_fmt->firstword = curr_fmt->firstword * 10 + (c - '0');
      c = getc (inp);
    }

    c = getc (inp);

    if (c != '[')
      c = getc (inp);

    i = 0;

    while ((c = getc (inp)) != ',')
      if (st_return_body)
	curr_fmt->words = curr_fmt->words * 10 + (c-'0');

    c = getc (inp);

    if (c != 35)
      c = getc (inp);

    while ((c = getc (inp)) != ']') {
      if (st_return_body)
	switch (c) {
	case 'B':
	  curr_fmt->bold = 1;
	  break;
	case 'U':
	  curr_fmt->underline = 1;
	  break;
	case 'I':
	  curr_fmt->italic = 1;
	  break;
	default:
	  break;
	}
    }

    c = getc (inp);

    if (st_return_body) {
      curr_fmt->next = NULL;
      if (last_fmt != NULL)
	last_fmt->next = curr_fmt;
      last_fmt = curr_fmt;
      curr_fmt = NULL;
    }
  }

  c = getc (inp);

  return (root_fmt);
}

static char *st_return_text (void)
{
  long text_starts_at = 0;
  int text_size = 1;
  int bye = 0, i;
  char c = 0;
  char old_c = 0;
  char *temp_text = NULL;

  text_starts_at = ftell (inp);

  while (!bye) {
    c=getc(inp);
    if (c == 0) bye=1;
    else
      if ((old_c == 0x0D) && (c == 0x7E)) {
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
 
  for (i=0;i<text_size;i++)
    temp_text[i] = st_cleantext(temp_text[i]);
  temp_text[i] = 0;

  return (temp_text);
}

static char *st_return_title (void)
{
  char c;
  char *title = NULL;
  int title_size = 0;

/* TODO: make this 70 autodetected ala st_return_text */
  title = malloc (70);

  while ((c = st_cleantext (getc (inp))) != '@') {
    if (title == NULL) {
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
  switch (error_no) {
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

  if (return_error == NULL) {
    return (NULL);
  }
  return_error->title = NULL;
  return_error->text = NULL;
  return_error->fmt = NULL;
  return_error->next = NULL;
  return_error->err = error_no;

  return (return_error);
}


static struct ency_titles *curr_find_list (char *search_string, int exact)
{
  long this_one_starts_at = 0;
  int found_any_yet = 0;
  int first_time = 1;
  char c;
  int i = 0;
  int found = 0;
  char *lc_title = NULL, *lc_search_string = NULL;
  struct ency_titles *root_title = NULL, *curr_title = NULL, *last_title = NULL;
  int no_so_far = 0;
  char *title = NULL, *temp_text = NULL;
  struct st_ency_formatting *text_fmt = NULL, *kill_fmt = NULL;

  lc_search_string = st_lcase (search_string);

  if (!st_open ()) {
    return (st_title_error (1));
  };

  do {
    if (!first_time)
      st_find_start ();
    this_one_starts_at = ftell (inp);

    first_time = 0;
    text_fmt = st_return_fmt ();
    i = 0;
    no_so_far++;

    title = st_return_title ();
    /* Title & number:  printf ("%d:%s\n", no_so_far, title); */

    c = getc (inp);

    lc_title = st_lcase (title);
    /* */
    found = 0;

    /* Is this the one we want?? */
    if ((!exact) && strstr (title, search_string))
      found = 1;
    if ((exact == 1) && (!strcmp (title, search_string)))
      found = 1;
    if (exact == 2)
      found = 1;
    if (st_ignore_case) {
      if ((!exact) && (strstr (lc_title, lc_search_string)))
	found = 1;
      if ((exact == 1) && (!strcasecmp (title, search_string)))
	found = 1;
    }
    /* */

    free (lc_title);

    if (found) {		/* If yes... */
      found_any_yet = 1;
      if (st_return_body)
	temp_text = st_return_text ();

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
      if (st_return_body) {
	curr_title->text = temp_text;
	curr_title->fmt = text_fmt;
      }
      if (last_title != NULL)
	last_title->next = curr_title;
      last_title = curr_title;
      curr_title = (struct ency_titles *) NULL;
      title = temp_text = (char *) text_fmt = (struct st_ency_formatting *) NULL;
/* */
    } else
      /* It's not the one we want */
      {
	free (title);
	while (text_fmt) {
	  kill_fmt = text_fmt;
	  text_fmt = text_fmt->next;
	  free (kill_fmt);
	}
      }

  }
  while (no_so_far != curr_lastone);
  st_close_file ();
  free (lc_search_string);
  if (found_any_yet)
    return (root_title);
  else
    return (NULL);
}

struct ency_titles *ency_find_list (char *title, int exact)
{
  struct ency_titles *root = NULL, *current = NULL, *temp = NULL;
  int i, first_time = 1;

  upto = 0;

  for (i = 0; i < st_file_type; i++) {
    while (ency_starts_at[upto] != 0)
      upto++;
    upto++;
  }
  if (ency_starts_at[upto] != 0x1) {	/* if its 0x1, its reserved. */

    curr = 1;
    while (ency_starts_at[upto] != 0) {
      if (current)
	while (current->next) {
	  current = current->next;
	}
      curr_lastone = ency_lastone[upto];
      if (!first_time) {
	temp = (curr_find_list (title, exact));
	if (current)
	  current->next = temp;
	else
	  root = current = temp;
      } else {
	root = (curr_find_list (title, exact));
	current = root;
	first_time = 0;
      }
      upto++;
    }
  }
  return (root);
}

struct ency_titles *epis_find_list (char *title, int exact)
{
  struct ency_titles *root = NULL, *current = NULL, *temp = NULL;
  int i, first_time = 1;

  upto = 0;

  for (i = 0; i < st_file_type; i++) {
    while (epis_starts_at[upto] != 0)
      upto++;
    upto++;
  }
  if (epis_starts_at[upto] != 0x1) {	/* if its 0x1, its reserved. */

    curr = 2;
    while (epis_starts_at[upto] != 0) {
      if (current)
	while (current->next) {
	  current = current->next;
	}
      curr_lastone = epis_lastone[upto];
      if (!first_time) {
	temp = (curr_find_list (title, exact));
	if (current)
	  current->next = temp;
	else
	  root = current = temp;
      } else {
	root = (curr_find_list (title, exact));
	current = root;
	first_time = 0;
      }
      upto++;
    }
  }
  return (root);
}

struct ency_titles *chro_find_list (char *title, int exact)
{
  struct ency_titles *root = NULL, *current = NULL, *temp = NULL;
  int i, first_time = 1;

  upto = 0;

  for (i = 0; i < st_file_type; i++) {
    while (chro_starts_at[upto] != 0)
      upto++;
    upto++;
  }
  if (chro_starts_at[upto] != 0x1) {	/* if its 0x1, its reserved. */

    curr = 3;
    while (chro_starts_at[upto] != 0) {
      if (current)
	while (current->next) {
	  current = current->next;
	}
      curr_lastone = chro_lastone[upto];
      if (!first_time) {
	temp = (curr_find_list (title, exact));
	if (current)
	  current->next = temp;
	else
	  root = current = temp;
      } else {
	root = (curr_find_list (title, exact));
	current = root;
	first_time = 0;
      }
      upto++;
    }
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
      return (ency_find_list (search_string, exact));
      break;
    case ST_SECT_EPIS:
      return (epis_find_list (search_string, exact));
      break;
    case ST_SECT_CHRO:
      return (chro_find_list (search_string, exact));
      break;
    default:
      break;
      return (NULL);
    }
  return (0);
}

struct st_table *st_get_table (void)
{
  int i = 0;
  struct st_table *root_tbl = NULL, *curr_tbl = NULL, *last_tbl = NULL;
  int c = 0, text_size = 0;
  char *temp_text = NULL, *str_text = NULL;

  upto = 0;

  curr = 4;
  for (i = 0; i < st_file_type; i++) {
    while (st_table_starts_at[upto] != 0)
      upto++;
    upto++;
  }
  if (st_table_starts_at[upto] != 0x1) {
    while (st_table_starts_at[upto] != 0) {
      if (!st_open ()) {
	return (NULL);
      } else {

	for (i = 0; i < st_table_lastone[upto]; i++) {
	  while ((c = getc (inp)) != ']') {	/* main loop */

	    curr_tbl = (struct st_table *) malloc (sizeof (struct st_table));

	    if (curr_tbl == NULL) {
	      return (NULL);
	    }

	    if (!root_tbl)
	      root_tbl = curr_tbl;

	    do {
	      while ((c = getc (inp)) != '\"');
	      c = getc (inp);
	    }
	    while (!c);
	    ungetc (c, inp);

	    temp_text = malloc (8);
	    text_size = 0;

	    fread (temp_text, 1, 6, inp);
	    temp_text[6] = 0;
	    fgetc(inp);fgetc(inp);
	    if (strstr (temp_text, "\""))
	      {
		fseek (inp, -7, SEEK_CUR);
		while ((c = getc (inp)) != '\"');
		str_text = (strstr (temp_text, "\""));
		str_text[0] = 0;
	      }

	    curr_tbl->fnbase = temp_text;

/* TODO: make this 70 autodetected ala st_return text */
	    temp_text = malloc (70);
	    text_size = 0;

	    while ((c = getc (inp)) != '\"');
	    while ((c = getc (inp)) != '\"') {
	      if (c == 0xA5) {
		getc (inp);
	      } else {
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
	  }			/* end main loop */

	}
      }
      st_close_file ();
      upto++;
    }

/* */

  }
  return (root_tbl);
}

struct st_caption *st_get_captions (void)
{
  int i = 0;
  struct st_caption *root_cpt = NULL, *curr_cpt = NULL, *last_cpt = NULL;
  int c = 0, text_size = 0;
  char *temp_text = NULL;

  curr = 6;

  upto = 0;

  for (i = 0; i < st_file_type; i++) {
    while (st_caption_starts_at[upto] != 0)
      upto++;
    upto++;
  }
  if (st_caption_starts_at[upto] != 0x1) {
    while (st_caption_starts_at[upto] != 0) {

      if (!st_open ()) {
	return (NULL);
      } else {
	for (i = 0; i < st_caption_lastone[upto]; i++) {
	  c = getc(inp);
	  while (c != ']') {	/* main loop */
	    curr_cpt = (struct st_caption *) malloc (sizeof (struct
							     st_caption));

	    if (curr_cpt == NULL) {
	      return (NULL);
	    }
	    if (!root_cpt)
	      root_cpt = curr_cpt;

	    do {
	      while ((c != '\"') && (c != '[') && (c = getc (inp)) != (' '));

	      c = getc (inp);
	    }
	    while (!c);
	    ungetc (c, inp);
	    if ((c = getc (inp)) != ' ')
	      c = ungetc (c, inp);
	    if ((c = getc (inp)) != '\"')
	      c = ungetc (c, inp);
	    c = 0;

	    temp_text = malloc (8);
	    text_size = 0;

	    fread (temp_text, 1, 8, inp);
	    temp_text[7] = 0;

	    c = getc(inp);

	    curr_cpt->fnbasen = temp_text;

	    temp_text = malloc (70);
	    text_size = 0;

	    while ((c = getc (inp)) != '\"');
	    while ((c = getc (inp)) != '\"') {
	      temp_text[text_size++] = st_cleantext (c);
	    }
	    temp_text[text_size] = 0;

	    curr_cpt->caption = temp_text;
	    c=getc(inp);

	    curr_cpt->next = NULL;
	    if (last_cpt)
	      last_cpt->next = curr_cpt;
	    last_cpt = curr_cpt;
	    curr_cpt = NULL;
	  }			/* end main loop */

	}
      }
      upto++;
      st_close_file ();
    }
  }
  return (root_cpt);
}


struct ency_titles *get_title_at (long filepos)
{
  int return_body_was;
  char c;
  char *temp_text = NULL;
  int i = 0;
  struct ency_titles *root_title = NULL;
  char *ttl = NULL;
  struct st_ency_formatting *text_fmt = NULL;

  root_title = NULL;
  return_body_was = st_return_body;
  st_return_body = 1;
  set_starts_at = filepos;
  curr = 5;

  if (!st_open ()) {
    return (st_title_error (1));
  }
  root_title = (struct ency_titles *) malloc (sizeof (struct ency_titles));

  if (root_title == NULL) {
    return (st_title_error (2));
  }
  text_fmt = st_return_fmt ();
  i = 0;

  ttl = st_return_title ();

  c = getc (inp);

  temp_text = st_return_text ();
/* copy pointer stuff over */
  root_title->filepos = filepos;
  root_title->title = ttl;
  root_title->text = temp_text;
  root_title->next = NULL;
  root_title->fmt = text_fmt;
  st_return_body = return_body_was;
  st_close_file ();
  return (root_title);
}

static struct st_photo st_parse_captions (char *fnbasen)
{
  struct st_photo photo;

  st_oldcpts = st_cpts;

  strcpy (photo.file, "");
  strcpy (photo.caption, "");
  while (st_cpts) {
    if (!strcmp (fnbasen, st_cpts->fnbasen)) {
      strcpy (photo.file, fnbasen);
      strcpy (photo.caption, st_cpts->caption);
    }
    st_cpts = st_cpts->next;
  }

  st_cpts = st_oldcpts;
  return (photo);
}

struct st_media *st_get_media (char *search_string)
{
  int i = 0;
  int media_found = 0;
  struct st_media *media = NULL;
  char *temp_fnbase = NULL;
  char *title_with_dot = NULL;
  struct st_table *root_tbl = NULL;
  struct st_caption *root_cpt = NULL;

  if (st_loaded_media ())
  {

    root_tbl = st_tbls;
    root_cpt = st_cpts;
    temp_fnbase = malloc (9);

    title_with_dot = malloc (strlen (search_string) + 2);
    snprintf (title_with_dot, strlen (search_string) + 2, "%s.", search_string);


    while (st_tbls) {
      if ((!strcmp (st_tbls->title, search_string)) || (!strcmp (st_tbls->title, title_with_dot))) {
	for (i = 0; i < 5; i++) {
	  if (!media)
	    media = malloc (sizeof (struct st_media));
	  snprintf (temp_fnbase, 9, "%s%d", st_tbls->fnbase, i + 1);
	  media->photos[i] = st_parse_captions (temp_fnbase);
	  if (strlen(media->photos[i].file)) media_found = 1;
	}
	goto end_media_search;
      }
      st_tbls = st_tbls->next;
    }
  
  end_media_search:

    if (!media_found)
    {
      free (media);
      media = NULL;
    }

    st_tbls = root_tbl;
    st_cpts = root_cpt;
    free (temp_fnbase);
    free (title_with_dot);
  }
  return (media);
}

char *st_format_filename (char *fnbasen, char *base_path, int media)
{
/* media: 0 == pic, 1 == vid */
  char *filename = NULL;
  int dir_size = 0;

  if (fnbasen && (media < 2)) {

    if (base_path)
      dir_size = strlen (base_path);

    if (media == 0)
      dir_size += strlen (st_files[st_file_type].pic_dir);
    if (media == 1)
      dir_size += strlen (st_files[st_file_type].vid_dir);

    filename = malloc (dir_size + 17);
    if (base_path) {
      strcpy (filename, base_path);
      strcat (filename, "/");
    } else
      strcat (filename, "/");

    if (media == 0)
      strcat (filename, st_files[st_file_type].pic_dir);
    if (media == 1)
      strcat (filename, st_files[st_file_type].vid_dir);

    strcat (filename, "/");
    if (st_files[st_file_type].append_char) {
      filename[dir_size + 2] = fnbasen[0];
      filename[dir_size + 3] = 0;
      strcat (filename, "/");
    }
    strncat (filename, fnbasen, 8);

    if (media == 0)
      strcat (filename, "r.pic");
    if (media == 1)
      strcat (filename, "q.mov");

  }
  return (filename);
}
