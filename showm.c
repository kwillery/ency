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
extern char *ency_filename;

int 
main (int argc, char *argv[])
{
  int silent = 0;
  int i = 0;
  char search_string[70], *temp_fn;
  struct st_table *tbl, *oldtbl;
  struct st_media *media = NULL;
  char base_path[] = "/cdrom";

  /* identify whatever file were looking at */
  st_file_type = st_fingerprint ();

  /* this enables the media funcs. in encyfuncs.c */
  ency_init ();

  /* get a table so we know what's in the encyclopedia */
  tbl = st_get_table ();

  /* parse command-line args */
  for (i = 1; i < argc; i++)
    {
      if (!strcmp ("-s", argv[i]))
	silent++;
      else if (argv[i][0] == '-')
	{
	printf("No help yet\n");
	return(0);
	}
      else
	{
	strcpy(search_string, argv[i]);
	silent=2;
	}
    }

  /* get the search_string */
  if (silent < 1)
    printf ("Enter search string :");
  if (silent < 2) scanf ("%[a-zA-Z0-9.\"\'() -]", search_string);

  while (tbl)
    {
      if (strstr (tbl->title, search_string))
	{
	  media = st_get_media (tbl->title);
	  for (i = 0; i < 5; i++)
	    if (strlen (media->photos[i].file))		/* if there is a response */
	      {
		temp_fn = st_format_filename (media->photos[i].file, base_path, 0);	/* makes /a/abc1q.pic etc */
		printf ("%s : %s\n", temp_fn, media->photos[i].caption);
		free (temp_fn);
	      }
	}
      oldtbl = tbl;		/* store tbl's memory address in oldtbl */
      tbl = tbl->next;		/* progress tbl along the list */
      free (oldtbl->title);	/* free the last entry's title */
      free (oldtbl->fnbase);	/* free the last fnbase */
      free (oldtbl);		/* free the last entry */
    }
  ency_finish ();		/* cleans up after ency_init() */
  if (ency_filename)
    free (ency_filename);
}
