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
#include <string.h>
#include <unistd.h>
#include "ency.h"

extern int st_ignore_case;
extern int st_return_body;
extern int optind;		/* for getopt() */

int words = 0;

struct st_ency_formatting *loopies (char *txt, struct st_ency_formatting *fmt, FILE *output)
{
  struct st_ency_formatting *kill_fmt;
  int word_finish = 0;
  char smeg[50];

  while (strlen (txt))
    {
      sscanf (txt, "%s", smeg);
      words++;

      if (fmt)
	if (fmt->firstword == words)
	  {
	    if (fmt->bold)
	      fprintf (output, "<b>");
	    if (fmt->italic)
	      fprintf (output, "<i>");
	    if (fmt->underline)
	      fprintf (output, "<u>");

	    word_finish = words + fmt->words - 1;
	  }

      fprintf (output, "%s", smeg);

      if (words == word_finish)
	{
	  if (fmt->underline)
	    fprintf (output, "</u>");
	  if (fmt->italic)
	    fprintf (output, "</i>");
	  if (fmt->bold)
	    fprintf (output, "</b>");

	  kill_fmt = fmt;
	  fmt = fmt->next;
	  free (kill_fmt);
	}

      txt += strlen (smeg);
      while ((txt[0] == '\n') || (txt[0] == ' '))
	{
	  if (txt[0] == '\n') fprintf (output, "<br>");
	  fprintf (output, "%c", *(txt++));
	}
    }

  return (fmt);
}

int printoff (struct ency_titles *stuff, FILE *output)
{
  char *tmp;
  struct st_ency_formatting *fmt1;

  fprintf (output, "<hr>\n");
  tmp = stuff->title;
  fmt1 = stuff->fmt;
  words = 0;

  fprintf (output, "<h2>");
  fmt1 = loopies (tmp, fmt1, output);
  fprintf (output, "</h2>\n");

  tmp = stuff->text;

  loopies (tmp, fmt1, output);

  fprintf (output, "<br>\n");

  return (0);
}

void print_usage (void)
{
  printf (" htmlenc - Searches the Star Trek encyclopedias\n http://users.bigpond.com/mibus/ency/\n Usage: htmlenc -[c|e] [-m]\n   -c: searches chronology\n   -e: searches episodes\n    (default: search encyclopedia)\n   -m: displays associated media (photos etc.)\n");
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
  int section = ST_SECT_ENCY;

  st_ignore_case = 1;
  st_return_body = 0;
  
  while ((i = getopt (argc, argv, "ecmh")) != EOF) 
    switch (i)
  {
  case 'm':
    use_media = 1;
    break;
  case 'e':
    section = ST_SECT_EPIS;
    break;
  case 'c':
    section = ST_SECT_CHRO;
    break;
  default:
    print_usage ();
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

  thingy = st_find (search_string, section, ST_OPT_MATCH_SUBSTRING);
  
  i = 0;
  printf ("<html>\n");
  printf ("<head><title>Search results for: %s</title></head>", search_string);
  printf ("<h1>Star Trek %s</h1>\n", st_fileinfo_get_name (ST_FILE_CURR));
  printf ("You searched for <b>%s</b>.\n", search_string);
  if ((thingy != NULL) && (thingy->title != NULL)) {
    do {
      full_body = get_title_at (thingy->filepos);

      printoff (full_body, stdout);

      media = st_get_media(thingy->title);

      if (media)
      {
	printf("<b>Associated media</b>\n<ul>");
	for (i = 0; i < 5; i++)
	  if (strlen (media->photos[i].file)) {   /* if there is photos #i */
            temp_fn = st_format_filename (media->photos[i].file, base_path, 0);
            printf ("<li><a href=\"%s\">%s</a> (picture)\n</li>", temp_fn, media->photos[i].caption);
	    free (temp_fn);
	  }
	if (strlen(media->video.file)) {
          temp_fn = st_format_filename (media->video.file, base_path, 1);
          printf ("<li><a href=\"%s\">%s</a> (video)\n</li>\n", temp_fn, media->video.caption);
          free (temp_fn);
        }
        free (media); media = NULL;

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

  printf ("<hr>\nThe Star Trek ency reader: ");
  printf ("<a href=\"http://users.bigpond.com/mibus/ency/\">http://users.bigpond.com/mibus/ency/</a><br>\n");
  printf ("Queries, comments, and flames, to <a href=\"mailto:mibus@bigpond.com\">Robert Mibus &lt;mibus@bigpond.com&gt;</a>");

  printf ("</html>\n");

  st_unload_media ();
  st_finish ();

  return (0);
}
