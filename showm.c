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
/*              mibus@bigpond.com                                             */
/*      webpage www.picknowl.com.au/homepages/beemer/                         */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ency.h"

extern int st_file_type;
extern char *ency_filename;
char *st_lcase (char *);	/* from encyfuncs.c */

int main (int argc, char *argv[])
{
  int silent = 0;
  int i = 0, use_ee = 0;
  char ee_string[1000] = "";
  char search_string[70], *temp_fn;
  char *lc_search, *lc_title;
  struct st_table *tbl, *oldtbl;
  struct st_media *media = NULL;
  char base_path[] = "/cdrom";

  st_init ();

  /* this enables the media funcs. in encyfuncs.c */
  st_load_media ();

  /* get a table so we know what's in the encyclopedia */
  tbl = st_get_table ();

  /* parse command-line args */
  for (i = 1; i < argc; i++) {
    if (!strcmp ("-s", argv[i]))
      silent++;
    if (!strcmp ("-ee", argv[i]))
      use_ee = 1;
    else if (argv[i][0] == '-') {
      printf ("No help yet\n");
      return (0);
    } else {
      strcpy (search_string, argv[i]);
      silent = 2;
    }
  }

  /* get the search_string */
  if (silent < 1)
    printf ("Enter search string :");
  if (silent < 2)
    scanf ("%[a-zA-Z0-9.\"\'() -]", search_string);

  while (tbl) {
    lc_search = st_lcase (search_string);	/* make search string & title lowercase */
    lc_title = st_lcase (tbl->title);	/* (ie removes case sensitivity         */

    if (strstr (lc_title, lc_search)) {
      media = st_get_media (tbl->title);
      for (i = 0; i < 5; i++)
	if (strlen (media->photos[i].file)) {	/* if there is a response */
	  temp_fn = st_format_filename (media->photos[i].file, base_path, 0);	/* makes /cdrom/media98/a/abc1q.pic etc */
	  printf ("%s : %s\n", temp_fn, media->photos[i].caption);
	  if (use_ee) {
/* NOTE: This calls a version of ee i modified 
 * to allow setting of the title bar caption
 * using the -t or --title option
 */
	    strcpy (ee_string, "ee -t \"");
	    strcat (ee_string, media->photos[i].caption);
	    strcat (ee_string, "\" ");
	    strcat (ee_string, temp_fn);
	    strcat (ee_string, " &");
	    system (ee_string);
	  }
	  free (temp_fn);
	}
    }
    free (lc_search);
    free (lc_title);
    oldtbl = tbl;		/* store tbl's memory address in oldtbl */
    tbl = tbl->next;		/* progress tbl along the list */
    free (oldtbl->title);	/* free the last entry's title */
    free (oldtbl->fnbase);	/* free the last fnbase */
    free (oldtbl);		/* free the last entry */
  }
  st_unload_media ();		/* cleans up after st_load_media() */
}
