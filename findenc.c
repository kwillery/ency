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
/*      webpage www.picknowl.com.au/homepages/beemer/robonly.html             */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ency.h"
extern int st_ignore_case;

main ()
{
  int i = 0;
  char search_string[70];
  char titlle[70];
  struct st_table *tbl, *oldtbl;
  struct ency_titles *thingyz;
  struct ency_titles *thingy;
  struct ency_titles *kill_me;
  struct st_ency_formatting *fmt1, *fmt2;
  printf ("Enter search string :");
  scanf ("%[a-zA-Z0-9.\"\'() -]", search_string);

  st_ignore_case = 1;


  thingy = ency_find_list (search_string, 0);
// thingy = chro_find_list (search_string,0);
// thingy = get_title_at (0x149310);
// thingy = ency_find_titles (search_string);

  if ((thingy != NULL) && (thingy->title != NULL))
    {
      do
	{
	  printf ("\n%s\n\n", thingy->title);
	  thingyz = ency_get_title (thingy->title);

	  printf ("%s\n\n", thingyz->text);
	  free (thingyz->fmt);
	  free (thingyz->text);
	  free (thingyz->title);
	  kill_me = thingy;
	  thingy = thingy->next;
	  free (kill_me);
	}
      while (thingy != NULL);
    }
  else
    printf ("No matches\n");



/* tbl=st_get_table();
   i=0;
   while(tbl)
   {
   strcpy(titlle,tbl->title);
   if(strstr(titlle,search_string))
   {
   printf("/cdrom/video98/%c/",tolower(tbl->fnbase[0]));
   i=0;
   while(tbl->fnbase[i])
   printf("%c",tolower(tbl->fnbase[i++]));
   printf("1q.mov\n");
   }
   oldtbl=tbl;
   tbl=tbl->next;
   free(oldtbl->title);
   free(oldtbl->fnbase);
   free(oldtbl);
   }
 */
  return (0);
}
