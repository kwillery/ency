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
extern int st_ignore_case;
extern int st_return_body;
extern int optind;		// for getopt()

int remove_fmt = 0;
int words = 0;
char filename[100] = "stdout";
int exact = 0;

int
loopies (char *txt, struct st_ency_formatting *fmt)
{
  char fmtstring[10];
  struct st_ency_formatting *fmt2;
  int i = 0, z = 0;
  int first_time = 1;
  char smeg[50];
  int print_br = 0;

  while ((txt[0] == 32) || (txt[0] == 10))
    {
      printf ("%c", txt[0]);
      txt++;
    }

  while ((fmt) && (fmt->firstword < words + 1))
    {
      fmt2 = fmt;
      fmt = fmt->next;
    }
  while (strlen (txt))
    {
      z = sscanf (txt, "%s", smeg);
      {
	words++;

	if ((*(txt + strlen (smeg)) == 0x0a) || (*(txt + strlen (smeg) + 1) == 0x0a))
	  {
	    print_br = 1;
	  }
	if (fmt != NULL)
	  if (words == fmt->firstword)
	    {
	      fmtstring[0] = 0;
	      fmtstring[1] = 0;
	      fmtstring[2] = 0;
/*
   if (fmt->bi == 0) // None
   fmtstring[0]=0;
   if (fmt->bi == 1) // Bold
   strcpy(fmtstring,"b");
   if (fmt->bi == 2) // Italic
   strcpy(fmtstring,"i");
   if (fmt->bi == 3) // Bold & Italic
   strcpy(fmtstring,"bi");
   if (fmt->bi == 4) // Underline
   strcpy(fmtstring,"u");
   if (fmt->bi == 5) // Bold & Underline
   strcpy(fmtstring,"bu");
   if (fmt->bi == 6) // Bold & Italic
   strcpy(fmtstring,"bi");
   if (fmt->bi == 7) // Bold & Underline & Italic
   strcpy(fmtstring,"bui");
 */
	      switch (fmt->bi)
		{
		case 0:	// None

		  fmtstring[0] = 0;
		  break;
		case 1:	// Bold

		  strcpy (fmtstring, "b");
		  break;
		case 2:	// Italic

		  strcpy (fmtstring, "i");
		  break;
		case 3:	// Bold & Italic

		  strcpy (fmtstring, "bi");
		  break;
		case 4:	// Underline

		  strcpy (fmtstring, "u");
		  break;
		case 5:	// Bold & Underline

		  strcpy (fmtstring, "bu");
		  break;
		case 6:	// Bold & Italic

		  strcpy (fmtstring, "bi");
		  break;
		case 7:	// Bold & Underline & Italic

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
	      for (i = 0; i < fmt->words - 1; i++)
		{
		  txt += (strlen (smeg) + 1);
		  while ((txt[0] == 32) || (txt[0] == 10))
		    txt++;
//                if (!first_time)
		  printf (" ");
//                else
		  //                  first_time = 0;
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
	if (z)
	  {
	    printf ("%s", smeg);
	  }
	if (print_br)
	  {
	    printf ("<br>\n");
	    print_br = 0;
	  }
      }
// strlen(txt)-strlen(smeg) == 0 
      txt += (strlen (smeg));
      if (strlen (txt))
	while ((txt[0] == 32) || (txt[0] == '\n'))
	  {
	    if (txt[0] == 32)
	      printf ("%c", txt[0]);
	    else
	      printf ("<br>\n");
	    txt++;
	  }
    }
  return (0);
}

int
printoff (struct ency_titles *stuff)
{
  char *tmp;
  struct st_ency_formatting *fmt1;

  printf ("<hr>\n");

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

int
main (int argc, char *argv[])
{
  char search_string[50];
  struct ency_titles *thingy, *full_body;
  struct ency_titles *kill_me;
  int i = 0;
  int search_what = 0;
  st_ignore_case = 1;
  st_return_body = 0;

  while ((i = getopt (argc, argv, "ech")) != EOF)
    {
      if (i == 'h')
	{
	  printf ("htmlenc - Searches Star Trek encyclopedia\nhttp://www.picknowl.com.au/homepages/beemer/ency.html\nUsage: htmlenc -[c|e]\n-c: searches chronology\n-e: searches episodes\ndefault: search encyclopedia\n");
	  exit (0);
	}
      search_what = i;
    }

//if (argc > optind) printf("%s\n",argv[optind]);
  if (argc > optind)
    {
      strcpy (search_string, argv[optind]);
    }
  else
    {
      printf ("Enter search string :");
      scanf ("%[a-zA-Z0-9.\"\'() -]", search_string);
    }

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
  if ((thingy != NULL) && (thingy->title != NULL))
    {
      do
	{
	  full_body = get_title_at (thingy->filepos);
// printf("**\n%s\n%s\n**",full_body->title,full_body->text);
	  printoff (full_body);
//        printoff (thingy);
	  kill_me = thingy;
	  thingy = thingy->next;
//        free (kill_me->text);
	  free (kill_me->title);
	  free (kill_me);
	  free (full_body->title);
	  free (full_body->text);
	  free (full_body);
	}
      while (thingy != NULL);
    }
  else
    printf ("No matches<br>\n");

  printf ("<hr>\nMibus' ency reader<br>\n");
  printf ("<a href=\"http://www.picknowl.com.au/homepages/beemer/ency.html\">http://www.picknowl.com.au/homepages/beemer/ency.html</a><br>\n");
  printf ("queries, comments, flames, to <a href=\"mailto:beemer@picknowl.com.au\">Robert Mibus (beemer@picknowl.com.au)</a>");

  printf ("</html>\n");
  return (0);
}
