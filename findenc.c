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
extern char *ency_filename;

main (int argc, char *argv[])
{
  int i = 0;
  char search_string[70];
  char titlle[70];
  struct st_table *tbl, *oldtbl;
  struct ency_titles *thingyz;
  struct ency_titles *thingy;
  struct ency_titles *kill_me;
  struct st_ency_formatting *fmt1, *fmt2;

  i = getopt (argc, argv, "ech");

  if (i == 'h')
    {
      printf ("findenc - Searches Star Trek encyclopedia\nhttp://www.picknowl.com.au/homepages/beemer/ency.html\nUsage: findenc -[c|e]\n-c: searches chronology\n-e: searches episodes\ndefault: search encyclopedia\n");
      exit (0);
    }

  printf ("Enter search string :");
  scanf ("%[a-zA-Z0-9.\"\'() -]", search_string);

  st_ignore_case = 1;
  st_file_type = st_fingerprint ();
//  st_file_type = ST_FILE_OMNI2;
  if (st_file_type <= ST_FILE_TYPES)
    {
//  ency_filename = (char *) malloc (60);

//  strcpy (ency_filename, "/dose/trek/Reference/Encyclopedia/Ency98/Data.cxt");
      //  strcpy (ency_filename, "/dose/trek/Reference/omni1.dxr");
      //  strcpy (ency_filename, "/dose/trek/Reference/omni_v2.dxr");
      //  strcpy (ency_filename, "/dose/trek/Reference/ds9/ds9/eg_ds9.dxr");
      //  strcpy (ency_filename, "/dose/trek/Reference/tng/source/eg_tng.dxr");

      if (i == 'c')
	thingy = chro_find_list (search_string, 0);
      if (i == 'e')
	thingy = epis_find_list (search_string, 0);
      if ((i != 'c') && (i != 'e'))
	thingy = ency_find_list (search_string, 0);
      // thingy = get_title_at (0x149310);

      i = 0;

      if ((thingy != NULL) && (thingy->title != NULL))
	{
	  do
	    {
	      printf ("\n%s\n\n%s\n\n", thingy->title, thingy->text);
	      kill_me = thingy;
	      thingy = thingy->next;
	      free (kill_me);
	    }
	  while (thingy != NULL);
	}
      else
	printf ("No matches\n");

/*  tbl = st_get_table ();
   i = 0;
   while (tbl)
   {
   //   strcpy(titlle,tbl->title);
   //   if(strstr(titlle,search_string))
   {
   printf ("%s:%s\n", tbl->title, tbl->fnbase);
   }
   oldtbl = tbl;
   tbl = tbl->next;
   free (oldtbl->title);
   free (oldtbl->fnbase);
   free (oldtbl);
   } 
 */
    }
  else
    printf ("An error has occurred.\n");
  return (0);
}
