/******************************************************************************/
/* Mibus's Ency 98 Reader: Reads the Star Trek Encyclopedia (1998 version)    */
/* Copyright (C) 1998 Robert Mibus                                            */
/* Also reads the various Omnipedias & Episode guides                         */
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
/*      Email   mibus@bigpond.com                                             */
/*      Webpage http://users.bigpond.com/mibus/                               */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ency.h"

extern int st_ignore_case;
extern int st_return_body;
extern int optind;		/* for getopt() */

int remove_fmt = 0;
int words = 0;
int exact = 0;

int loopies (char *txt, struct st_ency_formatting *fmt)
{
  char fmtstring[10];
  struct st_ency_formatting *fmt2;
  int i = 0, z = 0;
  char smeg[50];
  int print_br = 0;

  while ((txt[0] == 32) || (txt[0] == 10)) {
    printf ("%c", txt[0]);
    txt++;
  }

  while ((fmt) && (fmt->firstword < words + 1)) {
    fmt2 = fmt;
    fmt = fmt->next;
    if (remove_fmt)
      free (fmt2);
  }
  while (strlen (txt)) {
    z = sscanf (txt, "%s", smeg);
    {
      words++;

      if ((*(txt + strlen (smeg)) == 0x0a) || (*(txt + strlen (smeg) + 1) == 0x0a)) {
	print_br = 1;
      }
      if (fmt != NULL)
	if (words == fmt->firstword) {
	  fmtstring[0] = 0;
	  fmtstring[1] = 0;
	  fmtstring[2] = 0;

	  switch (fmt->bi) {
	  case 0:		// None

	    fmtstring[0] = 0;
	    break;
	  case 1:		// Bold

	    strcpy (fmtstring, "b");
	    break;
	  case 2:		// Italic

	    strcpy (fmtstring, "i");
	    break;
	  case 3:		// Bold & Italic

	    strcpy (fmtstring, "bi");
	    break;
	  case 4:		// Underline

	    strcpy (fmtstring, "u");
	    break;
	  case 5:		// Bold & Underline

	    strcpy (fmtstring, "bu");
	    break;
	  case 6:		// Bold & Italic

	    strcpy (fmtstring, "bi");
	    break;
	  case 7:		// Bold & Underline & Italic

	    strcpy (fmtstring, "bui");
	    break;
	  }

	  if (fmtstring[0])
	    printf ("<%c>", fmtstring[0]);
	  if (fmtstring[1])
	    printf ("<%c>", fmtstring[1]);
	  if (fmtstring[2])
	    printf ("<%c>", fmtstring[2]);

	  printf ("%s", smeg);
	  for (i = 0; i < fmt->words - 1; i++) {
	    txt += (strlen (smeg) + 1);
	    while ((txt[0] == 32) || (txt[0] == 10))
	      txt++;
	    printf (" ");
	    if (sscanf (txt, "%s", smeg) == -1)
	      i = fmt->words;
	    if (i < fmt->words)
	      printf ("%s", smeg);
	  }
	  words--;
	  words += fmt->words;
	  if (fmtstring[2])
	    printf ("</%c>", fmtstring[2]);
	  if (fmtstring[1])
	    printf ("</%c>", fmtstring[1]);
	  if (fmtstring[0])
	    printf ("</%c>", fmtstring[0]);

	  fmt2 = fmt;
	  fmt = fmt->next;
	  if (remove_fmt)
	    free (fmt2);
	  z = 0;
	}
      if (z) {
	printf ("%s", smeg);
      }
      if (print_br) {
	printf ("<br>\n");
	print_br = 0;
      }
    }
// strlen(txt)-strlen(smeg) == 0 
    txt += (strlen (smeg));
    if (strlen (txt))
      while ((txt[0] == 32) || (txt[0] == '\n')) {
	if (txt[0] == 32)
	  printf ("%c", txt[0]);
	else
	  printf ("<br>\n");
	txt++;
      }
  }
  return (0);
}

int printoff (struct ency_titles *stuff)
{
  char *tmp;
  struct st_ency_formatting *fmt1;

  printf ("<hr>\n");
  remove_fmt = 0;
  tmp = stuff->title;
  fmt1 = stuff->fmt;
  words = 0;

  loopies (tmp, fmt1);

  remove_fmt = 1;
  printf ("<br>\n");
  tmp = stuff->text;

  loopies (tmp, fmt1);

  printf ("<br>\n");
  printf ("<br>\n");

  return (0);
}

void print_usage (void)
{
  printf (" htmlenc - Searches the Star Trek encyclopedias\n http://users.bigpond.com/mibus/ency.html\n Usage: htmlenc -[c|e] [-m]\n   -c: searches chronology\n   -e: searches episodes\n    (default: search encyclopedia)\n   -m: displays associated media (photos etc.)\n");
  exit (0);
}

int main (int argc, char *argv[])
{
  char search_string[50];
  char *temp_fn = NULL;
  struct ency_titles *thingy = NULL, *full_body = NULL;
  struct ency_titles *kill_me = NULL;
  struct st_media *media = NULL;
  char base_path[] = "/cdrom"; /* where the media dirs etc. are */
  int i = 0;
  int use_media = 0;
  int search_what = 0;

  st_ignore_case = 1;
  st_return_body = 0;
  
  while ((i = getopt (argc, argv, "ecmh")) != EOF) {
    if (i == 'h')
      print_usage ();
    if (i == 'm')
      use_media = 1;
    else
      search_what = i;
  }
  
  if (argc > optind) {
    if (!strcmp(argv[optind],"--help")) print_usage();
    strcpy (search_string, argv[optind]);
  } else {
    printf ("Enter search string :");
    scanf ("%[a-zA-Z0-9.\"\'() -]", search_string);
  }

  st_init ();
  if (use_media)
    st_load_media ();

  if (search_what == 'c')
    thingy = chro_find_list (search_string, 0);
  if (search_what == 'e')
    thingy = epis_find_list (search_string, 0);
  if ((search_what != 'c') && (search_what != 'e'))
    thingy = ency_find_list (search_string, 0);

  i = 0;
  printf ("<html>\n");
  printf ("<head><title>Search results for: %s</title></head>", search_string);
  printf ("<h1>Star Trek Encyclopedia</h1>\n");
  printf ("You searched for <b>%s</b>.\n", search_string);
  if ((thingy != NULL) && (thingy->title != NULL)) {
    do {
      full_body = get_title_at (thingy->filepos);

      printoff (full_body);

      media = st_get_media(thingy->title);

      if (media)
      {
	printf("<b>Associated media</b>\n<ul>");
	for (i = 0; i < 5; i++)
	  if (strlen (media->photos[i].file)) {   /* if there is photos #i */
            temp_fn = st_format_filename (media->photos[i].file, base_path, 0);
            printf ("<li>%s: %s\n</li>", temp_fn, media->photos[i].caption);
	  }
	printf ("</ul>");
      }

      kill_me = thingy;
      thingy = thingy->next;
      free (kill_me->title);
      free (kill_me);
      free (full_body->title);
      free (full_body->text);
      free (full_body);
    }
    while (thingy != NULL);
  } else
    printf ("No matches<br>\n");

  printf ("<hr>\nMibus' ency reader<br>\n");
  printf ("<a href=\"http://users.bigpond.com/mibus/ency.html\">http://users.bigpond.com/mibus/ency.html</a><br>\n");
  printf ("queries, comments, flames, to <a href=\"mailto:mibus@bigpond.com\">Robert Mibus (mibus@bigpond.com)</a>");

  printf ("</html>\n");

  st_unload_media ();
  st_finish ();

  return (0);
}
