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
#include <string.h>
#include <unistd.h>
#include "ency.h"

extern int st_ignore_case;
extern int st_file_type;
extern int optind;
extern char *ency_filename;

int
main (int argc, char *argv[])
{
  int i = 0;
  char search_string[70];
  struct ency_titles *thingy = NULL;
  struct ency_titles *kill_me = NULL;
  struct st_ency_formatting *fmt = NULL, *kill_fmt = NULL;
  strcpy(search_string, "");
    
  i = getopt (argc, argv, "ech");

  if (i == 'h')
    {
      printf ("findenc - Searches Star Trek encyclopedia\nhttp://www.picknowl.com.au/homepages/beemer/ency.html\nUsage: findenc -[c|e]\n-c: searches chronology\n-e: searches episodes\ndefault: search encyclopedia\n");
      exit (0);
    }

/* get the search string, one way or another */
  if (argc > optind)
    {
      strcpy (search_string, argv[optind]);
    }
  else
    {
      printf ("Enter search string :");
      scanf ("%[a-zA-Z0-9.\"\'() -]", search_string);
    }

  /* make the search *not* case sensitive */
  st_ignore_case = 1;

  /* if you want to manually set the filename
   * ency_filename = (char *) malloc (60);
   * strcpy (ency_filename, "/dose/trek/Reference/Encyclopedia/Ency98/Data.cxt");
   */

  /* identify the file were looking at
   * (defaults to the env. var. ENCY_FILENAME
   *  or, failing that, "./Data.cxt") */
  st_file_type = st_fingerprint ();

/*
 * If you want to set it manually, use:
 * st_file_type = ST_FILE_OMNI2;
 * (ST_FILE_* is in ency.h
 */


  if (st_file_type <= ST_FILE_TYPES)
    {
      if (i == 'c')
	thingy = chro_find_list (search_string, 0);
      if (i == 'e')
	thingy = epis_find_list (search_string, 0);
      if ((i != 'c') && (i != 'e'))
	thingy = ency_find_list (search_string, 0);

      /*
       * get from a certain point in the file...
       * thingy = get_title_at (0x149310);
       */

      i = 0;

      if ((thingy != NULL) && (thingy->title != NULL))
	{
	  do
	    {
	      /* print the returned text */
	      printf ("\n%s\n\n%s\n\n", thingy->title, thingy->text);

	      /* free the returned stuff */
	      kill_me = thingy;
	      thingy = thingy->next;
	      fmt = kill_me->fmt;

	      while (fmt)
		{
		  kill_fmt = fmt;
		  fmt = fmt->next;
		  if (kill_fmt) free (kill_fmt);
		}

	    if (kill_me->title)
		  free (kill_me->title);

	    if (kill_me->text)
		  free (kill_me->text);

	    if (kill_me)
		   free (kill_me);

	    }
	  while (thingy != NULL);
	}
      else
	printf ("No matches\n");

    }
  else
    printf ("An error has occurred.\n");
  return (0);
}
