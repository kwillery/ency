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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>
#include "ency.h"

FILE *inp;
char *ency_filename;

int st_return_body = 1;
int st_ignore_case = 0;
long int file_pos_is = 0;
int upto = 0;

long int st_table_starts_at = 0x410c4;
long int st_caption_starts_at = 0x4e5064;
long int ency_starts_at[3] =
{0x7bc28, 0x576574, 0};
long int epis_starts_at[3] =
{0x3b8e20, 0x50431A, 0};
long int chro_starts_at[2] =
{0x41e32c, 0};
long int set_starts_at = 0x0;

long int ency_lastone[3] =
{7068, 68, 0};
long int epis_lastone[3] =
{402, 3, 0};
long int chro_lastone[2] =
{582, 0};

long int curr_starts_at, curr_lastone, curr;
int screwy = 0;

int
st_open ()
{
  if (curr == 0)		// Defaults to ency

    curr_starts_at = ency_starts_at[upto];

  if (curr == 1)		// Ency

    curr_starts_at = ency_starts_at[upto];

  if (curr == 2)		// Epis

    curr_starts_at = epis_starts_at[upto];

  if (curr == 3)		// Chro

    curr_starts_at = chro_starts_at[upto];

  if (curr == 4)		// table

    curr_starts_at = st_table_starts_at;

  if (curr == 5)		// Set value

    curr_starts_at = set_starts_at;

  if (curr == 6)

    curr_starts_at = st_caption_starts_at;

  return (curr_open ());
}

int
curr_open (void)
{
  char c;
  int i;

  if (ency_filename == NULL)
    {
      ency_filename = getenv ("ENCY_FILENAME");
      if (ency_filename == NULL)
	{
	  ency_filename = (char *) malloc (10);
	  strcpy (ency_filename, "Data.cxt");
	}
    }
  inp = fopen (ency_filename, "rb");

  if (inp)
    {
      i = fseek (inp, curr_starts_at, SEEK_SET);
      file_pos_is = curr_starts_at;
    }
  if (i == 0)
    return ((int) inp);
  else
    return (0);
}

int
curr_close (void)
{
  fclose (inp);
  file_pos_is = 0;
  return (0);
}

int
ency_close (void)
{
  return (curr_close ());
}

int
epis_close (void)
{
  return (curr_close ());
}

int
chro_close (void)
{
  return (curr_close ());
}

char
ency_cleantext (unsigned char c)
{
  switch (c)
    {
// FIXME: i need something not detected as a space/null by sscanf, but looks like one.
    case 13:
      return ('\n');
      break;
    case 0xD0:
      return ('-');
      break;
    case 0xD1:
      return ('_');
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

    default:
      return (c);
      break;
    }
}

char
egetc (void)
{
  file_pos_is++;
  return (getc (inp));
}

void
eungetc (char c)
{
  file_pos_is--;
  ungetc (c, inp);
}


struct st_ency_formatting *
curr_return_fmt (void)
{
  struct st_ency_formatting *root_fmt, *last_fmt, *curr_fmt;
  int first_time = 1;
  char c = 0;
  int i = 0;
  char tmp_txt[50];
  root_fmt = last_fmt = curr_fmt = NULL;

  if (st_return_body)
    {
      root_fmt = (struct st_ency_formatting *) malloc (sizeof (struct st_ency_formatting));
      if (root_fmt == NULL)
	{
	  printf ("Memory allocation failed\n");
	  exit (1);
	}
      memset (root_fmt, 0, sizeof (struct st_ency_formatting));
      curr_fmt = root_fmt;
    }

  c = egetc ();
  while (c != '@')
    {
      if (!first_time)
	{
	  if (st_return_body)
	    {
	      curr_fmt = (struct st_ency_formatting *) malloc (sizeof (struct st_ency_formatting));
	      if (curr_fmt == NULL)
		{
		  printf ("Memory allocation failed\n");
		  exit (1);
		}
	      memset (curr_fmt, 0, sizeof (struct st_ency_formatting));
	    }

	}
      first_time = 0;
      i = 0;
      while (c != ':')
	{
	  if ((c != 20) && (c != ','))
	    tmp_txt[i++] = c;
	  c = egetc ();
	}
      tmp_txt[i] = 0;
      if (st_return_body)
	curr_fmt->firstword = atoi (tmp_txt);	// starts at

      c = egetc ();
      if (c != '[')
	c = egetc ();
      i = 0;
      while ((c = egetc ()) != ',')
	{
	  tmp_txt[i++] = c;
	}
      tmp_txt[i] = 0;
      if (st_return_body)
	curr_fmt->words = atoi (tmp_txt);	// words

      c = egetc ();
      if (c != 35)
	c = egetc ();

      if (st_return_body)
	curr_fmt->bi = 0;

      while ((c = egetc ()) != ']')
	{
	  if (st_return_body)
	    switch (c)
	      {
	      case 'B':
		curr_fmt->bi += 1;
		break;
	      case 'U':
		curr_fmt->bi += 4;
		break;
	      case 'I':
		curr_fmt->bi += 2;
		break;
	      default:
		break;
	      }
	}
      c = egetc ();
      if (st_return_body)
	{
	  curr_fmt->next = NULL;
	  if (last_fmt != NULL)
	    last_fmt->next = curr_fmt;
	  last_fmt = curr_fmt;
	  curr_fmt = NULL;
	}
    }
  c = egetc ();
  return (root_fmt);
}

struct st_ency_formatting *
ency_return_fmt (void)
{
  return (curr_return_fmt ());
}
struct st_ency_formatting *
epis_return_fmt (void)
{
  return (curr_return_fmt ());
}
struct st_ency_formatting *
chro_return_fmt (void)
{
  return (curr_return_fmt ());
}


int
curr_find_start (void)
{
  int c = 0;
  int oldc = 0;
  while (c != '~')
    {
      c = egetc ();
//      if ((oldc == 0x16) && (c != 0x2E))
      if ((oldc == 0x16) && (c != 0x7E))
	{
	  eungetc (c);
	  return (0);
	}
      oldc = c;
    }

  return (0);
}

int
ency_find_start (void)
{
  return (curr_find_start ());
}
int
epis_find_start (void)
{
  return (curr_find_start ());
}
int
chro_find_start (void)
{
  return (curr_find_start ());
}


char *
curr_return_text (void)
{
  int text_size = 0;
  int bye = 0;
  char c = 0;
  char old_c = 0;
  char done_once = 0;
  char *temp_text;
  temp_text = malloc (1);
  while (!bye)
    {
      c = ency_cleantext (egetc ());
      if (c == 0)
	bye = 1;
      if ((old_c == '\n') && (c == 0x7E))
	{
	  eungetc (c);
	  bye = 1;
	}

      if (!bye)
	{
	  temp_text = realloc (temp_text, text_size + 1);
	  if (temp_text == NULL)
	    {
	      return (NULL);
	    }
	  temp_text[text_size++] = c;
	  old_c = c;
	}
    }
  temp_text[text_size - 1] = 0;
//  printf ("\n");
  return (temp_text);
}
char *
ency_return_text (void)
{
  return (curr_return_text ());
}
char *
epis_return_text (void)
{
  return (curr_return_text ());
}
char *
chro_return_text (void)
{
  return (curr_return_text ());
}


char *
curr_return_title (void)
{
  char c;
  char *titl = NULL;
  int title_size = 0;

  titl = malloc (70);		// should be 1, not 70.

// malloc & realloc calls keep crashing, no idea why.

  while ((c = ency_cleantext (egetc ())) != '@')
    {

// /* should be on! */ titl = realloc(titl,title_size+1);
      /*
         if (titl == NULL)
         {
         printf("Oh, ^$#%%!\n");
         return (NULL);
         }
       */
      titl[title_size++] = c;
// printf("%c",c);
    }
// printf("\n");
  titl[title_size] = 0;
// printf("ert2 %d %s\n",title_size,titl);
  // if (screwy){screwy=0;printf("%s\n",titl);}
  return (titl);
}

char *
ency_return_title (void)
{
  return (curr_return_title ());
}
char *
epis_return_title (void)
{
  return (curr_return_title ());
}
char *
chro_return_title (void)
{
  return (curr_return_title ());
}

char *
st_nice_error (int error_no)
{
  char *error_text;
  switch (error_no)
    {
    case 1:
      return ("The Data.cxt file was not found");
      break;
    case 2:
      return ("Memory allocation error");
      break;
    default:
      return ("An error has occurred");
      break;
    }
}
struct ency_titles *
st_title_error (int error_no)
{
/* Errors: */
/* 1 - File not found (Data.cxt) */
/* 2 - Memory Allocation Error */

  struct ency_titles *return_error;

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

struct ency_titles *
curr_find_list (char title[], int exact)
{
  long this_one_starts_at = 0;
  int found_any_yet = 0;
  int first_time = 1;
  char c;
  int i = 0;
  int found = 0;
  char ttl2[70], title2[70];
  struct ency_titles *root_title, *curr_title, *last_title;
  int no_so_far = 0;
  char *ttl, *temp_text;
  struct st_ency_formatting *text_fmt;

  root_title = curr_title = last_title = NULL;

  if (!st_open ())
    {
      return (st_title_error (1));
    }

  root_title = (struct ency_titles *) malloc (sizeof (struct ency_titles));
  if (root_title == NULL)
    {
      return (st_title_error (2));
    }

  curr_title = root_title;

  do
    {

      if (!first_time)
	ency_find_start ();
      this_one_starts_at = file_pos_is;
      first_time = 0;
      text_fmt = ency_return_fmt ();

      i = 0;
      no_so_far++;

      ttl = ency_return_title ();
/**Title & number: printf("%d:%s\n",no_so_far,ttl); **/
      c = egetc ();

// lowerise ttl > ttl2, title > title2
      while (ttl2[i++] = tolower (ttl[i]));
      ttl2[i] = 0;
      i = 0;
      while (title2[i++] = tolower (title[i]));
      title2[i] = 0;
      i = 0;

      found = 0;

      if ((!exact) && strstr (ttl, title))
	found = 1;
      if ((exact == 1) && (!strcmp (ttl, title)))
	found = 1;
      if (exact == 2)
	found = 1;
      if (st_ignore_case)
	{
	  if ((!exact) && (strstr (ttl2, title2)))
	    found = 1;
	  if ((exact == 1) && (!strcasecmp (ttl, title)))
	    found = 1;
	}

      if (found)
	{
	  found_any_yet = 1;
	  if (st_return_body)
	    temp_text = ency_return_text ();
// printf("dbg2\n");
	  // define the pointer
	  if (curr_title != root_title)
	    {
	      curr_title = (struct ency_titles *) malloc (sizeof (struct ency_titles));
	      if (curr_title == NULL)
		{
		  printf ("Memory allocation failed\n");
//                exit (1);
		}
	    }
// copy pointer stuff over
	  // printf("dbg3\n");
	  curr_title->err = 0;
	  curr_title->filepos = this_one_starts_at;
	  curr_title->title = ttl;
	  curr_title->next = NULL;
	  if (st_return_body)
	    {
	      curr_title->text = temp_text;
	      curr_title->fmt = text_fmt;
	    }
	  if (last_title != NULL)
	    last_title->next = curr_title;
	  last_title = curr_title;
	  curr_title = (struct ency_titles *) NULL;
	  ttl = temp_text = (char *) text_fmt = (struct st_ency_formatting *) NULL;
// printf("dbg4\n");
	}
      else
	free (ttl);
// printf("dbg5\n");
    }
  while (no_so_far != curr_lastone);
// printf("dbg6\n");
  curr_close ();
  if (found_any_yet)
    return (root_title);
  else
    return (NULL);
}

struct ency_titles *
ency_find_list (char title[], int exact)
{
  struct ency_titles *root, *current, *temp;
  curr = 1;
  root = current = temp = NULL;
  upto = 0;
  while (ency_lastone[upto] != 0)
    {
      if (current)
	while (current->next)
	  {
	    current = current->next;
	  }
      curr_lastone = ency_lastone[upto];
      if (upto)
	{
	  temp = (curr_find_list (title, exact));
	  if (current)
	    current->next = temp;
	  else
	    root = current = temp;
	}
      else
	{
	  root = (curr_find_list (title, exact));
	  current = root;
	}
      upto++;
    }
  return (root);
}
struct ency_titles *
epis_find_list (char title[], int exact)
{
  struct ency_titles *root, *current, *temp;
  curr = 2;
  root = current = temp = NULL;
  upto = 0;
  while (epis_lastone[upto] != 0)
    {
      if (current)
	while (current->next)
	  {
	    current = current->next;
	  }
      curr_lastone = epis_lastone[upto];
      if (upto)
	{
	  temp = (curr_find_list (title, exact));
	  if (current)
	    current->next = temp;
	  else
	    root = current = temp;
	}
      else
	{
	  root = (curr_find_list (title, exact));
	  current = root;
	}
      upto++;
    }
  return (root);
}
struct ency_titles *
chro_find_list (char title[], int exact)
{
  struct ency_titles *root, *current, *temp;
  curr = 3;
  root = current = temp = NULL;
  upto = 0;
  while (chro_lastone[upto] != 0)
    {
      if (current)
	while (current->next)
	  {
	    current = current->next;
	  }
      curr_lastone = chro_lastone[upto];
      if (upto)
	{
	  temp = (curr_find_list (title, exact));
	  if (current)
	    current->next = temp;
	  else
	    root = current = temp;
	}
      else
	{
	  root = (curr_find_list (title, exact));
	  current = root;
	}
      upto++;
    }
  return (root);
}

struct ency_titles *
curr_find_titles (char title[])
{
  long this_one_starts_at = 0;
  int found_any_yet = 0;
  int first_time = 1;
  char c;
  char *temp_text;
  int text_size = 0;
//  int title_size = 0;
  int i, found, z;
  struct ency_titles *root_title, *curr_title, *last_title;
  int no_so_far = 0;
  char *ttl, title2[70], ttl2[70];
  struct st_ency_formatting *text_fmt;

  root_title = curr_title = last_title = NULL;

  if (!st_open ())
    {
      return (st_title_error (1));
    }

  root_title = (struct ency_titles *) malloc (sizeof (struct ency_titles));
  if (root_title == NULL)
    {
      return (st_title_error (2));
    }

  curr_title = root_title;

  do
    {

      if (!first_time)
	ency_find_start ();
      this_one_starts_at = file_pos_is;
      first_time = 0;
      text_fmt = ency_return_fmt ();

      i = 0;
      no_so_far++;

// printf("dbg0\n");

      ttl = ency_return_title ();

// printf("dbg1 %s %d\n",ttl,strlen(ttl));
      c = egetc ();
// printf ("%d: %s\n", no_so_far, ttl);
      // if (screwy){screwy=0;printf("%s\n",ttl);}
      // printf("%s=%s:%d",ttl,title,strstr(ttl,title));

      for (z = 0; z < strlen (ttl); z++)
	ttl2[z] = tolower (ttl[z]);
      ttl2[z] = 0;

      for (z = 0; z < strlen (title); z++)
	title2[z] = tolower (title[z]);
      title2[z] = 0;

      found = 0;

// printf("%d,%d,%s=%s\n",strlen(ttl),strlen(ttl2),ttl,ttl2);
      if (strstr (ttl, title))
	found = 1;
      if (st_ignore_case)
	if (strstr (ttl2, title2))
	  found = 1;
// found=0;

      if (found)
	{
	  found_any_yet = 1;
// if (screwy){screwy=0;printf("%s\n",ttl);}
	  //          printf ("%d: %s\n", no_so_far, ttl);

	  temp_text = ency_return_text ();
// printf("dbg2\n");
	  // define the pointer
	  if (curr_title != root_title)
	    {
	      curr_title = (struct ency_titles *) malloc (sizeof (struct ency_titles));
	      if (curr_title == NULL)
		{
		  printf ("Memory allocation failed\n");
//                exit (1);
		}
	    }
// copy pointer stuff over
	  // printf("dbg3\n");
	  curr_title->err = 0;
	  curr_title->filepos = this_one_starts_at;
	  curr_title->title = ttl;
	  curr_title->text = temp_text;
	  curr_title->next = NULL;
	  curr_title->fmt = text_fmt;
	  if (last_title != NULL)
	    last_title->next = curr_title;
	  last_title = curr_title;
	  curr_title = NULL;
	  temp_text = NULL;
	  text_size = 0;
	  ttl = NULL;
// printf("dbg4\n");
	}
      else
	free (ttl);
// printf("dbg5\n");
    }
  while (no_so_far != curr_lastone);
// printf("dbg6\n");
  if (found_any_yet)
    return (root_title);
  else
    return (NULL);
}

struct ency_titles *
ency_find_titles (char title[])
{
/* these havent been adapted to the new 'multi-staged' system and so wont work.
   curr = 1;
   curr_lastone = ency_lastone;
   return (curr_find_titles (title));
 */
}
struct ency_titles *
epis_find_titles (char title[])
{
/*
   curr = 2;
   curr_lastone = epis_lastone;
   return (curr_find_titles (title));
 */
}
struct ency_titles *
chro_find_titles (char title[])
{
/*
   curr = 3;
   curr_lastone = chro_lastone;
   return (curr_find_titles (title));
 */
}


struct ency_titles *
curr_get_title (char title[])
{
  int first_time = 1;
  char c;
  char *temp_text;
  int i = 0;
  struct ency_titles *root_title;
  int no_so_far = 0;
  char *ttl;
  struct st_ency_formatting *text_fmt;

  root_title = NULL;

  if (!st_open ())
    {
      return (st_title_error (1));
    }

  root_title = (struct ency_titles *) malloc (sizeof (struct ency_titles));
  if (root_title == NULL)
    {
      return (st_title_error (2));
    }

  do
    {

      if (!first_time)
	ency_find_start ();
      first_time = 0;
      text_fmt = ency_return_fmt ();

      i = 0;
      no_so_far++;

      ttl = ency_return_title ();

      c = egetc ();

      if (!strcmp (ttl, title))
	{
// printf("%s, %s",ttl,title);


	  temp_text = ency_return_text ();
// copy pointer stuff over
	  root_title->title = ttl;
	  root_title->text = temp_text;
	  root_title->next = NULL;
	  root_title->fmt = text_fmt;
	  no_so_far = curr_lastone;
	}
      else
	free (ttl);
    }
  while (no_so_far != curr_lastone);
  return (root_title);
}

struct ency_titles *
ency_get_title (char title[])
{
/* these havent been adapted to the new multi-staged stuff.
   curr = 1;
   curr_lastone = ency_lastone;
   return (curr_get_title (title));
 */
}
struct ency_titles *
epis_get_title (char title[])
{
/*
   curr = 2;
   curr_lastone = epis_lastone;
   return (curr_get_title (title));
 */
}
struct ency_titles *
chro_get_title (char title[])
{
/*
   curr = 3;
   curr_lastone = chro_lastone;
   return (curr_get_title (title));
 */
}

struct st_table *
st_get_table (void)
{
  int i = 0, first_time;
  struct st_table *root_tbl, *curr_tbl, *last_tbl;
  int c = 0, text_size = 0;
  char *temp_text;

  last_tbl = NULL;
  first_time = 1;
  curr = 4;
  if (!st_open ())
    {
      return (NULL);
    }
  else
    {

      for (i = 0; i < 26; i++)
	{
	  while ((c = egetc ()) != ']')
	    {			// main loop

	      temp_text = malloc (1);
	      text_size = 0;

	      curr_tbl = (struct st_table *) malloc (sizeof (struct st_table));
	      if (curr_tbl == NULL)
		{
		  return (NULL);
		}
	      if (first_time)
		root_tbl = curr_tbl;
	      first_time = 0;
	      do
		{
		  while ((c = egetc ()) != '\"');
		  c = egetc ();
		}
	      while (!c);
	      eungetc (c);
	      while ((c = egetc ()) != '\"')
		{
		  temp_text = realloc (temp_text, text_size + 2);
		  if (temp_text == NULL)
		    {
		      return (NULL);
		    }
		  temp_text[text_size++] = tolower (ency_cleantext (c));
		}
	      temp_text[text_size] = 0;
	      curr_tbl->fnbase = temp_text;
	      temp_text = malloc (1);
	      text_size = 0;

	      while ((c = egetc ()) != '\"');
	      while ((c = egetc ()) != '\"')
		{
		  temp_text = realloc (temp_text, text_size + 2);
		  if (temp_text == NULL)
		    {
		      return (NULL);
		    }
		  temp_text[text_size++] = ency_cleantext (c);
		}
	      temp_text[text_size] = 0;
	      curr_tbl->title = temp_text;
	      if (last_tbl)
		last_tbl->next = curr_tbl;
	      last_tbl = curr_tbl;
	      curr_tbl = NULL;

	    }			// main loop

	}
    }
  curr_close ();
  return (root_tbl);
}

struct st_caption *
st_get_captions (void)
{
  int i = 0, first_time;
  struct st_caption *root_cpt, *curr_cpt, *last_cpt;
  int c = 0, text_size = 0;
  char *temp_text;

  last_cpt = NULL;
  first_time = 1;
  curr = 6;
  if (!st_open ())
    {
      return (NULL);
    }
  else
    {

      for (i = 0; i < 5; i++)
	{
	  while ((c = egetc ()) != ']')
	    {			// main loop

	      temp_text = malloc (1);
	      text_size = 0;

	      curr_cpt = (struct st_caption *) malloc (sizeof (struct
							       st_caption));
	      if (curr_cpt == NULL)
		{
		  return (NULL);
		}
	      if (first_time)
		root_cpt = curr_cpt;
	      first_time = 0;
	      do
		{
		  while ((c = egetc ()) != '\"');
		  c = egetc ();
		}
	      while (!c);
	      eungetc (c);
	      while ((c = egetc ()) != '\"')
		{
		  temp_text = realloc (temp_text, text_size + 2);
		  if (temp_text == NULL)
		    {
		      return (NULL);
		    }
		  temp_text[text_size++] = tolower (ency_cleantext (c));
		}
	      temp_text[text_size] = 0;
	      curr_cpt->fnbasen = temp_text;
	      temp_text = malloc (1);
	      text_size = 0;

	      while ((c = egetc ()) != '\"');
	      while ((c = egetc ()) != '\"')
		{
		  temp_text = realloc (temp_text, text_size + 2);
		  if (temp_text == NULL)
		    {
		      return (NULL);
		    }
		  temp_text[text_size++] = ency_cleantext (c);
		}
	      temp_text[text_size] = 0;
	      curr_cpt->caption = temp_text;
	      if (last_cpt)
		last_cpt->next = curr_cpt;
	      last_cpt = curr_cpt;
	      curr_cpt = NULL;

	    }			// main loop

	}
    }
  curr_close ();
  return (root_cpt);
}


struct ency_titles *
get_title_at (long filepos)
{
  int return_body_was;
  int first_time = 1;
  char c;
  char *temp_text;
  int i = 0;
  struct ency_titles *root_title;
  char *ttl;
  struct st_ency_formatting *text_fmt;

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
  text_fmt = ency_return_fmt ();
  i = 0;

  ttl = ency_return_title ();

  c = egetc ();


  temp_text = ency_return_text ();
// copy pointer stuff over
  root_title->filepos = filepos;
  root_title->title = ttl;
  root_title->text = temp_text;
  root_title->next = NULL;
  root_title->fmt = text_fmt;
  st_return_body = return_body_was;
  return (root_title);
}
