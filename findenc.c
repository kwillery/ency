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
extern int optind;

void print_usage (void)
{
  printf (" findenc - Searches the Star Trek encyclopedias\n http://users.bigpond.com/mibus/ency/\n Usage: findenc -[c|e]\n  -c: searches chronology\n  -e: searches episodes\n   (default: search encyclopedia)\n  -m: displays associated media\n");
  exit(0);
}


int main (int argc, char *argv[])
{
  int i = 0;
  char search_string[70];
  char *temp_fn = NULL;
  struct ency_titles *thingy = NULL;
  struct ency_titles *kill_me = NULL;
  struct st_ency_formatting *fmt = NULL, *kill_fmt = NULL;
  struct st_media *media = NULL;
  char base_path[] = "/cdrom"; /* where the media dirs etc. are */
  int use_media = 0;
  int section = ST_SECT_ENCY;
  strcpy (search_string, "");

  while ((i = getopt (argc, argv, "echm")) != EOF) {
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
  }

/* get the search string, one way or another */
  if (argc > optind) {
    if (!strcmp (argv[optind],"--help"))
      print_usage ();
    strcpy (search_string, argv[optind]);
  } else {
    printf ("Enter search string :");
    scanf ("%[a-zA-Z0-9.\"\'() -]", search_string);
  }

  /* run any ency init stuff */
  st_init ();

  /* tell ency to load the media lookup tables */
  if (use_media)
    st_load_media ();

  /* if you want to manually set the filename
   * st_set_filename ("/dose/trek/Reference/eg_tng.dxr");
   */

  thingy = st_find (search_string, section, ST_OPT_MATCH_SUBSTRING | ST_OPT_RETURN_BODY);

  /*
   * get from a certain point in the file...
   * thingy = get_title_at (0x149310);
   */

  if ((thingy != NULL) && (thingy->title != NULL)) {
    do {
      /* print the returned text */
      printf ("\n%s\n\n%s\n\n", thingy->title, thingy->text);
      
      media = st_get_media(thingy->title);
      if (media)
      {
	printf ("Associated media:\n");
	for (i = 0; i < 5; i++)
	  if (strlen (media->photos[i].file)) {   /* if there is photos #i */
            temp_fn = st_format_filename (media->photos[i].file, base_path, 0);
            printf ("%s: %s\n", temp_fn, media->photos[i].caption);
	    free (temp_fn);
	  }
      }
      

      /* free the returned stuff */
      kill_me = thingy;
      thingy = thingy->next;
      fmt = kill_me->fmt;

      while (fmt) {
	kill_fmt = fmt;
	fmt = fmt->next;
	if (kill_fmt)
	  free (kill_fmt);
      }

      if (kill_me->title)
	free (kill_me->title);

      if (kill_me->text)
	free (kill_me->text);

      if (kill_me)
	free (kill_me);

    }
    while (thingy != NULL);
  } else
    printf ("No matches\n");
  st_unload_media ();
  st_finish ();
  return (0);
}

