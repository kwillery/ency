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
#include "ency.h"

extern int st_file_type;
int debug = 0;

struct st_caption *captions, *oldcaptions;

int
print_media (char *fnbase)
{
  int i = 0;
  char *temp_fnbase;

  temp_fnbase = malloc (strlen (fnbase) + 2);
  for (i = 1; i <= 5; i++)
    {
      oldcaptions = captions;
      strcpy (temp_fnbase, fnbase);
      temp_fnbase[strlen (fnbase)] = '0' + i;
      temp_fnbase[strlen (fnbase) + 1] = 0;
      if (debug > 1)
	printf ("%s\n", temp_fnbase);
      while (captions)
	{
	  if (debug)
	    printf ("%s=%s?\n", captions->fnbasen, temp_fnbase);
	  if (!strcmp (captions->fnbasen, temp_fnbase))
	    {
	      printf ("%sr.pic == %s\n", temp_fnbase, captions->caption);
	      captions = NULL;
	    }
	  if (captions)
	    captions = captions->next;
	}
      captions = oldcaptions;
    }
  return (1);
}

int
main (int argc, char *argv[])
{
  int silent = 0;
  int i = 0;
  char search_string[70];
  char title[70];
  struct st_table *tbl, *oldtbl;

  st_file_type = st_fingerprint ();

  captions = st_get_captions ();
  tbl = st_get_table ();
  for (i = 0; i < argc; i++)
    {
      if (!strcmp ("-d", argv[i]))
	debug++;
      if (!strcmp ("-s", argv[i]))
	silent++;
    }
  if (!silent)
    printf ("Enter search string :");
  scanf ("%[a-zA-Z0-9.\"\'() -]", search_string);

  while (tbl)
    {
      strcpy (title, tbl->title);
      if (strstr (title, search_string))
	{
//        printf ("/cdrom/video98/%c/%s1q.mov\n", tbl->fnbase[0], tbl->fnbase);
	  // printf("%s\n",tbl->title);
	  print_media (tbl->fnbase);
	}
      oldtbl = tbl;		/* store tbl's memory address in oldtbl */
      tbl = tbl->next;		/* progress tbl along the list */
      free (oldtbl->title);	/* free the last entry's title */
      free (oldtbl->fnbase);	/* free the last fnbase */
      free (oldtbl);		/* free the last entry */
    }
  while (captions)
    {
      oldcaptions = captions;
      captions = captions->next;
      free (oldcaptions->caption);
      free (oldcaptions->fnbasen);
      free (oldcaptions);
    }
}
