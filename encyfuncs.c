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

FILE *inp;
char ency_filename[] = "Data.cxt";
long int ency_starts_at = 506920; // Bytes into file info starts at - 1
long int ency_lastone = 6937; // Zytchin III

int ency_open (void)
{
  char c;
  int i;

  if ((inp = fopen (ency_filename, "r")) == NULL)
    {
//      printf ("I can't find/open Data.cxt. Read the INSTALL file.\n");
      return (1);
    }
  for (i = 0; i < ency_starts_at; i++)
   c = getc (inp);

return(0);
}

int ency_close (void)
{
  fclose (inp);
}

char ency_cleantext (unsigned char c)
{
  switch (c)
    {
    case 0xD2:
      return (34);
      break;
    case 0xD3:
      return (34);
      break;
    case 0xD4:
      return (39);
      break;
    case 0xD5:
      return (39);
      break;
    case 0x88:
      return ('a');
      break;
    case 0x8E:
      return ('e');
      break;
    case 0x8F:
      return ('e');
      break;
    default:
      return (c);
      break;
    }
}

struct st_ency_formatting *ency_return_fmt (void)
{
struct st_ency_formatting *root_fmt, *last_fmt, *curr_fmt;
int first_time = 1;
char c = 0;
int i = 0;
char tmp_txt[50];
root_fmt = last_fmt = curr_fmt = NULL;

  root_fmt = (struct st_ency_formatting *) malloc (sizeof (struct st_ency_formatting));
  if (root_fmt == NULL)
    {
      printf ("Memory allocation failed\n");
exit(1);  
  }

curr_fmt=root_fmt;
c=getc(inp);
while (c != '@')
{
if (!first_time)
{
  curr_fmt = (struct st_ency_formatting *) malloc (sizeof (struct st_ency_formatting));
  if (curr_fmt == NULL)
    {
      printf ("Memory allocation failed\n");
      exit(1); 
    }

}
first_time=0;
i=0;
while (c != ':')
 {
   if ((c != 20) && (c != ',')) tmp_txt[i++]=c;
c=getc(inp);
}
tmp_txt[i] = 0;
curr_fmt->firstword = atoi(tmp_txt); // starts at
c=getc(inp);
if (c != '[') c=getc(inp);
i=0;
while ((c = getc(inp)) != ',')
 {
  tmp_txt[i++] = c;
 }
tmp_txt[i] = 0;
curr_fmt->words = atoi(tmp_txt); // words
c=getc(inp);
if (c != 35) c=getc(inp);
curr_fmt->bi = 0;

while ((c=getc(inp)) != ']')
{
switch(c)
    {
    case 'B':
      curr_fmt->bi += 1;
      break;
    case 'U':
      curr_fmt->bi += 4;
      break;
    case 'I':
      curr_fmt->bi += 2;
      break;
    default:
      break;
    }
}
c=getc(inp);

if (last_fmt != NULL) last_fmt->next = curr_fmt;
curr_fmt->next=NULL;
last_fmt=curr_fmt;
curr_fmt=NULL;
}
c=getc(inp);

 return(root_fmt);
}

int ency_find_start (void)
{
int c = 0;
 while ((c = getc (inp)) != '~')
      ;
}

char *ency_return_text (void)
{
int text_size = 0;
int bye = 0;
char c;
char old_c = 0;
char *temp_text;
temp_text = malloc(1);
          while (!bye)
            {
c=ency_cleantext(getc(inp));
if ((old_c == 13) && (c == 13)) bye = 1;
if (!bye)
{
              if (c == 0)
                exit;
              temp_text = realloc(temp_text,text_size+1);
              if (temp_text == NULL)
                {
                  return (NULL);
                }
              temp_text[text_size++] = c;
old_c=c;
}
            }
temp_text[text_size] = 0;
// printf("%s",temp_text);
return(temp_text);
}

char *ency_return_title (void)
{
char c;
char *ttl = NULL;
int title_size = 0;

ttl = malloc(50); // should be 1, not 50.

// malloc & realloc calls keep crashing, no idea why.

      while ((c = ency_cleantext (getc (inp))) != '@')
{

// /* should be on! */ ttl = realloc(ttl,title_size+1);
/*
              if (ttl == NULL)
                {
                  printf("Oh, ^$#%%!\n");
                  return (NULL);
                }
*/
              ttl[title_size++] = c;
}
ttl[title_size] = 0;
// printf("ert2 %d %s\n",title_size,ttl);
return (ttl);
}

struct ency_titles *ency_find_titles (char title[])
{
  int first_time = 1;
  char c;
  char *temp_text;
  int text_size = 0;
  int title_size = 0;
  int i = 0;
  struct ency_titles *root_title, *curr_title, *last_title;
  int no_so_far = 0;
  char *ttl;
  struct st_ency_formatting *text_fmt;

  root_title = curr_title = last_title = NULL;

  if (ency_open () == 1)
{
printf("Error opening file.\n");
}

  root_title = (struct ency_titles *) malloc (sizeof (struct ency_titles));
  if (root_title == NULL)
    {
      printf ("Memory allocation failed\n");
    }

  curr_title = root_title;

  do
    {

if (!first_time)
  ency_find_start();
first_time = 0;
 text_fmt = ency_return_fmt();

i=0;
no_so_far++;

// printf("dbg0\n");

 ttl = ency_return_title();

 // printf("dbg1 %s %d\n",ttl,strlen(ttl));
      c = getc (inp);
      if (strstr (ttl, title))
	{

//          printf ("%d: %s\n", no_so_far, ttl);

temp_text = ency_return_text();
// printf("dbg2\n");
// define the pointer
	  if (curr_title != root_title)
	    {
	      curr_title = (struct ency_titles *) malloc (sizeof (struct ency_titles));
	      if (curr_title == NULL)
		{
		  printf ("Memory allocation failed\n");
//		  exit (1);
		}
	    }
// copy pointer stuff over
// printf("dbg3\n");
	  curr_title->title = ttl;
	  curr_title->text = temp_text;
	  curr_title->next = NULL;
          curr_title->fmt = text_fmt;
	  if (last_title != NULL)
	  last_title->next = curr_title;
	  last_title = curr_title;
	  curr_title = NULL;
	  temp_text = NULL;
	  text_size = 0;
	  ttl = NULL;
// printf("dbg4\n");
	} else free(ttl);
// printf("dbg5\n");
    }
  while (no_so_far != ency_lastone);
// printf("dbg6\n");
return(root_title);
}

struct ency_titles *ency_get_title (char title[])
{
  int first_time = 1;
  char c;
  char *temp_text;
  int text_size = 0;
  int title_size = 0;
  int i = 0;
  struct ency_titles *root_title;
  int no_so_far = 0;
  char *ttl;
  struct st_ency_formatting *text_fmt;

  root_title = NULL;

  if (ency_open () == 1)
{
printf("Error opening file.\n");
}

  root_title = (struct ency_titles *) malloc (sizeof (struct ency_titles));
  if (root_title == NULL)
    {
      printf ("Memory allocation failed\n");
    }

  do
    {

if (!first_time)
  ency_find_start();
first_time = 0;
 text_fmt = ency_return_fmt();

i=0;
no_so_far++;

 ttl = ency_return_title();

      c = getc (inp);
      if (!strcmp (ttl, title))
        {
// printf("%s, %s",ttl,title);


temp_text = ency_return_text();
// copy pointer stuff over
          root_title->title = ttl;
          root_title->text = temp_text;
          root_title->next = NULL;
          root_title->fmt = text_fmt;
no_so_far = ency_lastone;
        } else free(ttl);
    }
  while (no_so_far != ency_lastone);
return(root_title);
}

