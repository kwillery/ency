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
static int force_unknown = 0;
static int upto = 0;
static int st_file_type = 0;

/* for pictures */
static const long int st_table_starts_at[] =
{0x410c4, 0, 0x388f2e, 0, 0x3CD470, 0, 0x2BBA98, 0x2BCD9B, 0, 0x322996, 0};

static const long int st_caption_starts_at[] =
{0x4e5064, 0, 0x615552, 0, 0x646D0A, 0, 0x2C5F2C, 0, 0x37CB82, 0};

/* for videos */
static const long st_video_table_starts_at[] =
{0x50e690, 0,0x59baaa,0,0x5b9820,0,0x3967aa,0,0x37684a,0};
static const long st_video_caption_starts_at[] =
{0x4FCED6, 0,0x621174,0,0x65860a,0,0x2bba98,0,0x322968,0};

/* the actual encyclopedia entries */
static const long int ency_starts_at[] =
{0x7bc28, 0x576574, 0, 0x3A9ED8, 0x56BB62, 0, 0x3FC3BE, 0x58B51E, 0x72E89C, 0, 0x2D2A6E, 0, 0x324B34, 0x390C40, 0};

static const long int epis_starts_at[] =
{0x3b8e20, 0x50431A, 0, 0x5D961A, 0x622AA4, 0, 0x606630, 0x659F9E, 0, 0x1, 0, 0x1, 0};

static const long int chro_starts_at[] =
{0x41e32c, 0, 0x62764A, 0, 0x66B9C4, 0, 0x1, 0, 0x1, 0};

/* for use w/ get_title_at() */
static long int set_starts_at = 0x0;

/* hm. articles or sections or whatever to get */
static const long int ency_lastone[] =
{7068, 68, 0, 4092, 491, 0, 3905, 476, 1353, 0, 181, 0, 89, 42, 0};

static const long int epis_lastone[] =
{402, 3, 0, 261, 25, 0, 262, 93, 0, 0x1, 0, 0x1, 0};

static const long int chro_lastone[] =
{582, 0, 465, 0, 582, 0, 0x1, 0, 0x1, 0};

static const long int st_table_lastone[] =
{26, 0, 26, 0, 26, 0, 2, 22, 0, 15, 0};

static const long int st_caption_lastone[] =
{5, 0, 4, 0, 4, 0, 5, 0, 4, 0};

static const long st_video_table_lastone[] = {26,0,26,0,26,0,26,0,26,0};
static const long st_video_caption_lastone[] = {1,0,1,0,1,0,24,0,15,0};

static long int curr_starts_at, curr_lastone, curr;

const struct st_file_info st_files[] =
{
  {"Encyclopedia", "Data.cxt", "Ency98", "media98", "video98", 1,
   {0x52, 0x49, 0x46, 0x58, 0x0, 0x99, 0xD7, 0x6E, 0x4D, 0x43, 0x39, 0x35, 0x69, 0x6D, 0x61, 0x70}, 1},
  {"Omnipedia", "OMNI1.DXR", "", "media", "media", 1,
   {0x58, 0x46, 0x49, 0x52, 0xBC, 0x42, 0xB7, 0x0, 0x33, 0x39, 0x56, 0x4D, 0x70, 0x61, 0x6D, 0x69}, 1},
  {"Omnipedia (updated)", "omni_v2.dxr", "startrek", "media", "media", 1,
   {0x52, 0x49, 0x46, 0x58, 0x0, 0xFa, 0x1C, 0x7A, 0x4D, 0x56, 0x39, 0x33, 0x69, 0x6D, 0x61, 0x70}, 1},
  {"TNG Episode guide", "eg_tng.dxr", "source", "media", "media", 1,
   {0x52, 0x49, 0x46, 0x58, 0x00, 0x51, 0x91, 0xF4, 0x4D, 0x56, 0x39, 0x33, 0x69, 0x6D, 0x61, 0x70}, 1},
  {"DS9 Episode guide", "eg_ds9.dxr", "ds9", "media", "media", 1,
   {0x52, 0x49, 0x46, 0x58, 0x0, 0x4C, 0xAE, 0xC4, 0x4D, 0x56, 0x39, 0x33, 0x69, 0x6D, 0x61, 0x70}, 1}
};

/* for pictures */
static struct st_table *st_ptbls = NULL, *st_oldptbls = NULL;
static struct st_caption *st_pcpts = NULL, *st_oldpcpts = NULL;

/* for videos */
static struct st_table *st_vtbls = NULL, *st_oldvtbls = NULL;
static struct st_caption *st_vcpts = NULL, *st_oldvcpts = NULL;

/* cache */
static struct ency_titles *cache[3]={0,0,0};
static struct ency_titles *st_find_in_cache (int, char *, int);
static void st_clear_cache(void);

/* init/de-init stuff */
int st_init (void)
{
  st_fingerprint ();
  return (st_file_type >= 254 ? force_unknown : 1);
}

int st_finish (void)
{
  if (st_loaded_media ())
    st_unload_media ();
  if (ency_filename)
    free (ency_filename);
  st_clear_cache();
  return (1);
}

/* useful bits */
static char st_cleantext (unsigned char c)
{
  switch (c) {
  case 13:
    return ('\n');
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
  case 0x92:
    return ('\'');
    break;
  case 0x93:
    return ('\"');
    break;
  case 0x94:
    return ('\"');
    break;
  case 0x97:
    return (':');
    break;
  case 0xA5:
    return ('*');
    break;
  case 0xD0:
    return ('-');
    break;
  case 0xD1:
    return ('-');
    break;
  case 0xD2:
    return ('\"');
    break;
  case 0xD3:
    return ('\"');
    break;
  case 0xD4:
    return ('\'');
    break;
  case 0xD5:
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

void st_free_fmt (struct st_ency_formatting *fmt)
{
  if (fmt)
    free (fmt);
}

void st_free_fmt_tree (struct st_ency_formatting *fmt)
{
  struct st_ency_formatting *last=NULL;
  while (fmt) {
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
      last=*fmt;
      *fmt=(*fmt)->next;
      free (last);
    }
}

void st_free_entry (struct ency_titles *entry)
{
  if (entry) {

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

  while (entry) {
    last=entry;
    entry = entry->next;
    st_free_entry (last);
  }
}

void st_free_entry_and_advance (struct ency_titles **entry)
{
  struct ency_titles *last;

  last=*entry;
  *entry = (*entry)->next;
  st_free_entry (last);
}

void st_copy_part_entry (struct ency_titles **to, struct ency_titles *from)
{
  if (from)
  {
    *to = malloc (sizeof (struct ency_titles));
    if (*to) {
      (*to)->title = strdup (from->title);
      (*to)->filepos = from->filepos;
      (*to)->fmt = NULL;
      (*to)->text = NULL;
      (*to)->next = NULL;
      (*to)->err = 0;
    }
  }
}

/* file stuff */
void st_force_unknown_file (int true)
{
  force_unknown = true;
}

int st_set_filename (char *filename)
{
  int type = 255;
  if (ency_filename)
    free (ency_filename);
  ency_filename = strdup (filename);
  if (ency_filename) {
    type = st_fingerprint ();
    if ((force_unknown) || ((type >= 0) && ( type < ST_FILE_TYPES))) {
      st_file_type = type;
      st_clear_cache();
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
  if (file_type == ST_FILE_CURR) {
    if (st_file_type == ST_FILE_UNKNOWN)
      return (("Unknown encyclopedia"));
    return (st_files[st_file_type].name);
  } else
    return (st_files[file_type].name);
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
      free (temp_fn);
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
  switch (curr) {
    case 0:                /* Defaults to ency */
        curr_starts_at = ency_starts_at[upto];
        break;
    case 1:                /* Ency */
        curr_starts_at = ency_starts_at[upto];
        break;
    case 2:                /* Epis */
        curr_starts_at = epis_starts_at[upto];
        break;
    case 3:                /* Chro */
        curr_starts_at = chro_starts_at[upto];
        break;
    case 4:                /* table */
        curr_starts_at = st_table_starts_at[upto];
        break;
    case 5:                /* Set value */
        curr_starts_at = set_starts_at;
        break;
    case 6:                /* Captions */
        curr_starts_at = st_caption_starts_at[upto];
        break;
    case 7:                /* video table */
        curr_starts_at = st_video_table_starts_at[upto];
        break;
    case 8:                /* video captions */
        curr_starts_at = st_video_caption_starts_at[upto];
        break;
    default:
        return (0);
        break;
  }
  return (curr_open ());
}

static int st_close_file (void)
{
  fclose (inp);
  return (0);
}

int st_fingerprint (void)
{
  int i = 0, z = 0;
  unsigned char input_fp[16];

  curr = 5;

  set_starts_at = 0;

  i = st_open ();
  if (i) {
    fread (input_fp,1,16,inp);

/* compare fingerprints etc... */
    for (i = 0; i < ST_FILE_TYPES; i++) {
      for (z = 0; z < 16; z++)
        if (input_fp[z] != st_files[i].fingerprint[z]) break;

      if (z == 16) {
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

/* media stuff */

static struct st_table *st_get_table (void)
{
  int i,z;
  struct st_table *root_tbl = NULL, *curr_tbl = NULL, *last_tbl = NULL;
  int text_size = 0;
  unsigned char c=0;
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
          while ((c = getc (inp)) != ']') {     /* main loop */

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
            z=0; while ((temp_text[z] = tolower(temp_text[z]))) z++;

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
          }                     /* end main loop */

        }
      }
      st_close_file ();
      upto++;
    }
  }
  return (root_tbl);
}

static struct st_caption *st_get_captions (void)
{
  int i, z;
  struct st_caption *root_cpt = NULL, *curr_cpt = NULL, *last_cpt = NULL;
  char c = 0;
  int text_size = 0;
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
          while (c != ']') {    /* main loop */
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

            z=0;while ((temp_text[z] = tolower(temp_text[z]))) z++;

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
          }                     /* end main loop */

        }
      }
      upto++;
      st_close_file ();
    }
  }
  return (root_cpt);
}

static struct st_table *st_get_video_table (void)
{
  int i = 0;
  struct st_table *root_tbl = NULL, *curr_tbl = NULL, *last_tbl = NULL;
  char c=0;
  int text_size = 0;
  int level, commas;
  char *temp_text = NULL;
  
  upto = 0;

  curr = 7;

  for (i = 0; i < st_file_type; i++) {
    while (st_video_table_starts_at[upto] != 0)
      upto++;
    upto++;
  }

  if (st_video_table_starts_at[upto] != 0x1) {
    while (st_video_table_starts_at[upto] != 0) {
      if (!st_open ()) {
        return (NULL);
      } else {
        
        for (i = 0; i < st_video_table_lastone[upto]; i++) {
          c=0;
          while (c != ']') {     /* main loop */
            
            while ((c = getc (inp)) != '[');
            while (c != ']') {
              
              curr_tbl = (struct st_table *) malloc (sizeof (struct st_table));
              
              if (curr_tbl == NULL) {
                return (NULL);
              }
              
              if (!root_tbl)
                root_tbl = curr_tbl;
              
              /* TODO: Make this '70' an autodetected size */
              temp_text = malloc (sizeof (char) * 70);
              text_size = 0;
              while ((c = getc (inp)) != '\"');
              while ((temp_text[text_size++] = getc (inp)) != '\"');
              temp_text[--text_size] = 0;
              
              do {
                text_size--;
                temp_text[text_size] = st_cleantext (temp_text[text_size]);
              } while (text_size);
              
              curr_tbl->title = temp_text;
              temp_text = NULL;

              c=getc(inp);
              if (c == ':') 
                {
                  c = getc (inp);
                  c = getc (inp);
                  level = 1;
                  commas = 0;
                  while (level) {
                    c = getc (inp);
                    if (c == '[')
                      level++;
                    if (c == ']')
                      level--;
                    if ((level == 1) && (c == ','))
                      commas++;
                    if ((c == '\"') && (commas == 4)) {
                      
                      /* TODO: Make this '70' an autodetected size */
                      temp_text = malloc (sizeof (char) * 70);
                      text_size = 0;
                      
                      while ((temp_text[text_size++] = tolower(getc (inp))) != '\"');
                      temp_text[text_size - 1] = 0;
                      curr_tbl->fnbase = temp_text;
                    }
                  }
                  c = getc (inp);
                }

              if (curr_tbl->fnbase) {
                curr_tbl->next = NULL;
                if (last_tbl)
                  last_tbl->next = curr_tbl;
                last_tbl = curr_tbl;
                curr_tbl = NULL;
              } else {
                free (curr_tbl->title);
                free (curr_tbl);
              }
              
            }
          }                     /* end main loop */
          
        }
      
        st_close_file ();
        upto++;
      }
    }
  }
  return (root_tbl);
}

static struct st_caption *st_get_video_captions (void)
{
  int i, z;
  struct st_caption *root_cpt = NULL, *curr_cpt = NULL, *last_cpt = NULL;
  int text_size = 0; 
  char c=0;
  char *temp_text = NULL;

  curr = 8;

  upto = 0;

  for (i = 0; i < st_file_type; i++) {
    while (st_video_caption_starts_at[upto] != 0)
      upto++;
    upto++;
  }
  if (st_video_caption_starts_at[upto] != 0x1) {
    while (st_video_caption_starts_at[upto] != 0) {

      if (!st_open ()) {
        return (NULL);
      } else {
        for (i = 0; i < st_video_caption_lastone[upto]; i++) {
          c = getc(inp);
          while (c != ']') {    /* main loop */
            curr_cpt = (struct st_caption *) malloc (sizeof (struct
                                                             st_caption));

            if (curr_cpt == NULL) {
              return (NULL);
            }
            if (!root_cpt)
              root_cpt = curr_cpt;

            do {
              while ((c != '\"') && (c != '[') && (c = getc (inp)) != (' '))
                ;

              c = getc (inp);
            }
            while (!c);
            c = ungetc (c, inp);
            if ((c = getc (inp)) != ' ')
              c = ungetc (c, inp);
            if ((c = getc (inp)) != '\"')
              c = ungetc (c, inp);
            if ((c = getc (inp)) != ':')
              { c = ungetc (c, inp);
            c = 0;

            temp_text = malloc (8);
            text_size = 0;

            fread (temp_text, 1, 7, inp); // was 8, inp);
            temp_text[6] = 0; // was ...[7] = 0;

            z=0;while ((temp_text[z] = tolower(temp_text[z]))) z++;

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
            curr_cpt = NULL;} else {free (root_cpt); root_cpt = NULL;}
          }                     /* end main loop */

        }
      }
      upto++;
      st_close_file ();
    }
  }
  return (root_cpt);
}


int st_load_media (void)
{
  /*
   * Get the table & captions (if we don't already have them)
   */
  if (!st_ptbls)
    st_ptbls = st_get_table ();
  if (!st_pcpts)
    st_pcpts = st_get_captions ();
  if (!st_vtbls)
    st_vtbls = st_get_video_table ();
  if (!st_vcpts)
    st_vcpts = st_get_video_captions ();

  return (1);
}

int st_loaded_media (void)
{
  if (((st_pcpts) && (st_ptbls)) || ((st_vtbls) && (st_vcpts)))
    return (1);
  else
    return (0);
}

int st_unload_media (void)
{
/* Free the caption & table info for the pictures*/
  while (st_pcpts) {
    st_oldpcpts = st_pcpts;
    st_pcpts = st_pcpts->next;
    if (st_oldpcpts) {
      if (st_oldpcpts->fnbasen)
	free (st_oldpcpts->fnbasen);
      if (st_oldpcpts->caption)
	free (st_oldpcpts->caption);
      free (st_oldpcpts);
    }
  }

  while (st_ptbls) {
    st_oldptbls = st_ptbls;
    st_ptbls = st_ptbls->next;
    if (st_oldptbls) {
      if (st_oldptbls->fnbase)
	free (st_oldptbls->fnbase);
      if (st_oldptbls->title)
	free (st_oldptbls->title);
      free (st_oldptbls);
    }
  }

/* & for the videos */
  while (st_vcpts) {
    st_oldvcpts = st_vcpts;
    st_vcpts = st_vcpts->next;
    if (st_oldvcpts) {
      if (st_oldvcpts->fnbasen)
        free (st_oldvcpts->fnbasen);
      if (st_oldvcpts->caption)
        free (st_oldvcpts->caption);
      free (st_oldvcpts);
    }
  }

  while (st_vtbls) {
    st_oldvtbls = st_vtbls;
    st_vtbls = st_vtbls->next;
    if (st_oldvtbls) {
      if (st_oldvtbls->fnbase)
        free (st_oldvtbls->fnbase);
      if (st_oldvtbls->title)
        free (st_oldvtbls->title);
      free (st_oldvtbls);
    }
  }

  return (1);
}


/* unsorted, mostly file/data stuff */
static int check_match (char *search_string, char *title, int exact)
{
    int found = 0;
    char *lc_title=NULL;
    char *lc_search_string=NULL;

    /* Is this the one we want?? */
    if ((!exact) && strstr (title, search_string))
      found = 1;
    if ((exact == 1) && (!strcmp (title, search_string)))
      found = 1;
    if (exact == 2)
      found = 1;
    if (st_ignore_case) {
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

static void st_clear_cache()
{
  int i;
  for (i=0;i<3;i++) {
    st_free_entry_tree (cache[i]);
    cache[i] = NULL;
  }
}

/* old one
static void st_find_start (void)
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
*/
/* brand new one :) */
static int
st_find_start (void)
{
  unsigned char c = 0, old_c = 0, old_old_c = 0, old_old_old_c = 0;
  int keep_going = 1;
  while (keep_going)
    {
      if (feof (inp)) return 0;
      c = getc (inp);
      if (c == '1')
        {
          switch (old_c)
            {
            case '~':
              keep_going = 0;
              fseek (inp, -2, SEEK_CUR);
              break;
            case 0x16:
              if (old_old_c == 0)
                {
                  keep_going = 0;
                  fseek (inp, -1, SEEK_CUR);
                }
              break;
            case '@':
              if ((old_old_c == 0x16) && (old_old_old_c == 0))
                {
                  keep_going = 0;
                  fseek (inp, -1, SEEK_CUR);
                }
              break;
            }
        }
      old_old_old_c = old_old_c;
      old_old_c = old_c;
      old_c = c;
    }
  return (feof(inp) ? 0 : 1);
}


static struct st_ency_formatting *st_return_fmt (void)
{
  struct st_ency_formatting *root_fmt = NULL, *last_fmt = NULL, *curr_fmt = NULL;
  char c = 0;
  int i = 0;

  c = getc (inp);

  while (c != '@') {
    if (st_return_body) {
      curr_fmt = (struct st_ency_formatting *) malloc (sizeof (struct st_ency_formatting));
      curr_fmt->firstword = curr_fmt->words = curr_fmt->bold = 0;
      curr_fmt->italic = curr_fmt->underline = 0;
      curr_fmt->next = NULL;

      if (curr_fmt == NULL) {
	printf ("Memory allocation failed\n");
	exit (1);
      }
    }

    i = 0;
    
    if (c == '~') c = getc(inp);

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
	default:
	  break;
	}
    }

    c = getc (inp);

    if (st_return_body) {
      if (!root_fmt)
	root_fmt = curr_fmt;
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

  while (!bye && !feof(inp)) {
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
  struct ency_titles *root_title = NULL, *curr_title = NULL, *last_title = NULL;
  struct ency_titles *root_cache=NULL, *curr_cache=NULL, *temp_cache=NULL, *old_cache=NULL;
  int no_so_far = 0;
  char *title = NULL, *temp_text = NULL, *new_title=NULL;
  struct st_ency_formatting *text_fmt = NULL, *kill_fmt = NULL;
  char last_year[5]="";
  root_cache = cache[curr-1];

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

    /* some chronology entries need years prepended */
    if (curr == 3) {
      /* if it's a year, save it */
      if (strlen (title) == 4) {
        strcpy (last_year, title);
      }
      /* if it's an episode or a movie, add the year */
      if ((*title == '\"') || (!strncmp (title, "Star Trek", 9))) {
        new_title = (char *) malloc (strlen (title) + 6);
        sprintf (new_title, "%s %s", last_year, title);
        free (title);
        title = new_title;
      }
    }
    
    /* and episode entries need (TOS) etc. appended */
    if (curr == 2) {
        getc(inp);
        c = getc(inp);
        if (c == 0x0D) {
            c = getc(inp);
            fseek (inp, -1, SEEK_CUR);
            }
        fseek (inp, -2, SEEK_CUR);
        new_title = (char *) malloc (strlen (title) + 7);
        switch (c) {
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
    
    /* Title & number:  printf ("%d:%s\n", no_so_far, title); */

    /* build the cached version of this entry */
    temp_cache = malloc (sizeof(struct ency_titles));
    temp_cache->title = strdup (title);
    temp_cache->text = NULL;
    temp_cache->fmt = NULL;
    temp_cache->filepos = this_one_starts_at;
    st_copy_part_entry (&curr_cache, temp_cache);
    st_free_entry (temp_cache);
    if (root_cache == NULL) root_cache = curr_cache;
    else {
    if (old_cache == NULL) old_cache = root_cache;
    while (old_cache->next) old_cache=old_cache->next;
    old_cache->next = curr_cache; }
    curr_cache = curr_cache->next;

    c = getc (inp);

    if (check_match(search_string, title, exact)) {		/* If its a match... */
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
      curr_title->text = temp_text;
      curr_title->fmt = text_fmt;
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

  cache[curr-1] = root_cache;

  if (found_any_yet)
    return (root_title);
  else
    return (NULL);
}

struct ency_titles *ency_find_list (char *title, int exact)
{
  struct ency_titles *root = NULL, *current = NULL, *temp = NULL;
  int i, first_time = 1;
  if ((cache[0]) && (!st_return_body))
    return (st_find_in_cache (0, title, exact));

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
  if ((cache[1]) && (!st_return_body))
    return (st_find_in_cache (1, title, exact));

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
  if ((cache[2]) && (!st_return_body))
    return (st_find_in_cache (2, title, exact));

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

static struct ency_titles *st_find_in_cache (int section, char *search_string, int exact)
{
  struct ency_titles *mine, *r_r=NULL, *r_c=NULL, *r_l=NULL;

  mine = cache[section];

  while (mine) {
    if (check_match (search_string, mine->title, exact)) {
     r_l = r_c;
     st_copy_part_entry (&r_c, mine); 
     if (r_l) r_l->next = r_c;
     if (!r_r) r_r = r_c;
    }
   mine=mine->next;
  }
return (r_r);
}

static struct ency_titles *st_find_unknown (char *search_string, int exact)
{
    long this_one_starts_at=0;
    struct ency_titles *root_title = NULL, *curr_title=NULL, *last_title=NULL;
    struct ency_titles *root_cache = NULL, *curr_cache=NULL;
    struct ency_titles *old_cache=NULL, *temp_cache=NULL;

    FILE *input_temp;
    
    if (cache[0])
        return (st_find_in_cache (0, search_string, exact));
    if (st_open())
    {
        while (st_find_start())
        {
            /* get_title_at opens the file again, so... */
            input_temp = inp;
            curr_title = get_title_at (this_one_starts_at = ftell(input_temp));
            inp = input_temp;
            getc(inp); /* make sure we dont get the same entry again */

            /* build the cached version of this entry */
            temp_cache = malloc (sizeof(struct ency_titles));
            temp_cache->title = strdup (curr_title->title);
            temp_cache->text = NULL;
            temp_cache->fmt = NULL;
            temp_cache->filepos = this_one_starts_at;
            st_copy_part_entry (&curr_cache, temp_cache);
            st_free_entry (temp_cache);
            if (root_cache == NULL) root_cache = curr_cache;
            else {
            if (old_cache == NULL) old_cache = root_cache;
            while (old_cache->next) old_cache=old_cache->next;
            old_cache->next = curr_cache; }
            curr_cache = curr_cache->next;
       
            if (check_match (search_string, curr_title->title, exact))
            {
                if (!root_title) root_title = curr_title;
                if (last_title)
                    last_title->next = curr_title;
                last_title = curr_title;
                if (!st_return_body)
                {
                    st_free_fmt_tree (curr_title->fmt);
                    curr_title->fmt = NULL;
                    free (curr_title->text);
                    curr_title->text = NULL;
                }
            } else {
                st_free_entry (curr_title);
            }
        }
    }
    cache[0] = root_cache;
    return (root_title);
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

  if (st_file_type != ST_FILE_UNKNOWN) {
    switch (section)
      {
      case ST_SECT_ENCY:
       if ((cache[0]) && (!st_return_body))
  	  return (st_find_in_cache (0, search_string, exact));
        return (ency_find_list (search_string, exact));
        break;
      case ST_SECT_EPIS:
        if ((cache[1]) && (!st_return_body))
 	  return (st_find_in_cache (1, search_string, exact));
        return (epis_find_list (search_string, exact));
        break;
      case ST_SECT_CHRO:
        if ((cache[2]) && (!st_return_body))
 	  return (st_find_in_cache (2, search_string, exact));
        return (chro_find_list (search_string, exact));
        break;
      default:
        break;
        return (NULL);
      }
    return (0);
  } else { /* unknown file type */
        return (st_find_unknown(search_string,exact));
  }
}

struct ency_titles *get_title_at (long filepos)
{
  int return_body_was;
  char c;
  char *temp_text = NULL;
  int i = 0;
  struct ency_titles *root_title = NULL, *curr_title=NULL;
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

  /* but of course, the title is often mangled :( */
  /* thus, we check the cache for an entry w/ the same */
  /* filepos, and use its title*/
  for (i=0;i<3;i++) {
    curr_title = cache[i];
    while (curr_title) {
      if (curr_title->filepos == filepos) {
        free (root_title->title);
        root_title->title = strdup (curr_title->title);
        /* make sure we break out of the loop */
        curr_title=NULL;
        i=3;
      }
      if (curr_title) curr_title = curr_title->next;
    }
  }

  return (root_title);
}

static struct st_photo st_parse_captions (char *fnbasen)
{
  struct st_photo photo;

  st_oldpcpts = st_pcpts;

  strcpy (photo.file, "");
  strcpy (photo.caption, "");
  while (st_pcpts && (!strlen(photo.file))) {
    if (!strcmp (fnbasen, st_pcpts->fnbasen)) {
      strcpy (photo.file, fnbasen);
      strcpy (photo.caption, st_pcpts->caption);
    }
    st_pcpts = st_pcpts->next;
  }

  st_pcpts = st_oldpcpts;
  return (photo);
}

static struct st_photo st_parse_video_captions (char *fnbasen)
{
  struct st_photo photo;

  st_oldvcpts = st_vcpts;

  strcpy (photo.file, "");
  strcpy (photo.caption, "");

  while (st_vcpts && (!strlen(photo.file))) {
    if (!strcmp (fnbasen, st_vcpts->fnbasen)) {
      strcpy (photo.file, fnbasen); strcat (photo.file, "1");
      strcpy (photo.caption, st_vcpts->caption);
    }
    st_vcpts = st_vcpts->next;
  }

  st_vcpts = st_oldvcpts;
  return (photo);
}

struct st_media *st_get_media (char *search_string)
{
  int i = 0;
  int media_found = 0;
  struct st_media *media = NULL;
  char *temp_fnbase = NULL;
  char *title_with_dot = NULL;
  struct st_table *root_ptbl = NULL;
  struct st_table *root_vtbl = NULL;
  struct st_caption *root_pcpt = NULL;
  struct st_caption *root_vcpt = NULL;

  if (st_loaded_media ())
  {

    root_ptbl = st_ptbls;
    root_pcpt = st_pcpts;
    root_vtbl = st_vtbls;
    root_vcpt = st_vcpts;

    temp_fnbase = malloc (9);

    title_with_dot = malloc (strlen (search_string) + 2);
    sprintf (title_with_dot, "%s.", search_string);


    while (st_ptbls) {
      if ((!strcmp (st_ptbls->title, search_string)) || (!strcmp (st_ptbls->title, title_with_dot))) {
	for (i = 0; i < 5; i++) {
	  if (!media) {
	    media = malloc (sizeof (struct st_media));
            strcpy (media->video.file,""); }
	  sprintf (temp_fnbase, "%s%d", st_ptbls->fnbase, i + 1);
	  media->photos[i] = st_parse_captions (temp_fnbase);
	  if (strlen(media->photos[i].file)) media_found = 1;
	}
	goto end_photo_search;
      }
      st_ptbls = st_ptbls->next;
    }
  
  end_photo_search:

    while (st_vtbls) {
      if ((!strcmp (st_vtbls->title, search_string)) || (!strcmp (st_vtbls->title, title_with_dot))) {
        if (!media) {
          media = malloc (sizeof (struct st_media));
          strcpy (media->video.file,""); }
        media->video = st_parse_video_captions (st_vtbls->fnbase);
        media_found = 1;
        goto end_video_search;
      }
      st_vtbls = st_vtbls->next;
    }

  end_video_search:

    if (!media_found)
    {
      free (media);
/* FIXME: should i be free'ing media->photos[*]->* ???? find out. */
      media = NULL;
    }

    st_ptbls = root_ptbl;
    st_pcpts = root_pcpt;
    st_vtbls = root_vtbl;
    st_vcpts = root_vcpt;

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
