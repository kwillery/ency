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

main ()
{
  char search_string[50];
  struct ency_titles *thingy;
  struct ency_titles *kill_me;
  struct st_ency_formatting *fmt1, *fmt2;
  printf ("Enter search string :");
//  scanf ("%s", search_string);
  scanf ("%[a-zA-Z0-9.\"\'() -]", search_string);
  thingy = ency_find_titles (search_string);
  if ((thingy != NULL) && (thingy->title != NULL))
    {
      do
	{
{
fmt1=thingy->fmt;
while (fmt1 != NULL)
 {
 fmt2=fmt1;
 fmt1=fmt2->next;
 free(fmt2);
 }
}
	  printf ("\n%s\n\n%s\n", thingy->title, thingy->text);
	  kill_me = thingy;
	  thingy = thingy->next;
          free(kill_me->text);
	  free(kill_me);
	}
      while (thingy != NULL);
    }
  else
    printf ("No matches\n");
}
