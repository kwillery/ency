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

int words = 0;
char filename[100] = "stdout";
int exact = 0;

int loopies(char *txt,struct st_ency_formatting *fmt,int size)
{
 char fmtstring[10];
 struct st_ency_formatting *fmt2;
 int i = 0, z = 0;
 int first_time = 1;
 char smeg[50];
 int print_br = 0;

while (fmt->firstword < words)
 {
fmt2=fmt;
fmt=fmt->next;
free(fmt2);
 }

while (strlen(txt))
{
z=sscanf(txt,"%s",smeg);
// if (z == 1)
{
words++;
if (!first_time) printf(" "); else first_time = 0;

if ((*(txt+strlen(smeg)) == 0x0d) || (*(txt+strlen(smeg)+1) == 0x0d))
{
print_br = 1;
}
if (fmt != NULL)
if (words==fmt->firstword)
{
fmtstring[0]=0;
fmtstring[1]=0;
fmtstring[2]=0;
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
 case 0: // None
  fmtstring[0] = 0;
  break;
 case 1: // Bold
  strcpy(fmtstring,"b");
  break;
 case 2: // Italic
  strcpy(fmtstring,"i");
  break;
 case 3: // Bold & Italic
  strcpy(fmtstring,"bi");
  break;
 case 4: // Underline
  strcpy(fmtstring,"u");
  break;
 case 5: // Bold & Underline
  strcpy(fmtstring,"bu");
  break;
 case 6: // Bold & Italic
  strcpy(fmtstring,"bi");
  break;
 case 7: // Bold & Underline & Italic
  strcpy(fmtstring,"bui");
  break;
}

if (fmtstring[0])
 printf("<%c>",fmtstring[0]);
if (fmtstring[1])
 printf("<%c>",fmtstring[1]);
if (fmtstring[2])
 printf("<%c>",fmtstring[2]);

printf("%s",smeg);
for(i=0; i < fmt->words-1; i++)
{
txt += (strlen(smeg)+1);
while (txt[0] == 32)
 txt++;
if (!first_time) printf(" "); else first_time = 0;
if (sscanf(txt,"%s",smeg) == -1) i=fmt->words;
if (i < fmt->words) printf("%s",smeg);
}
words--;
words += fmt->words;
if (fmtstring[2])
 printf("</%c>",fmtstring[2]);
if (fmtstring[1])
 printf("</%c>",fmtstring[1]);
if (fmtstring[0])
 printf("</%c>",fmtstring[0]);

fmt2=fmt;
fmt=fmt->next;
free(fmt2);
z=0;
}
if (z)
{
 printf("%s",smeg);
}
if (print_br)
{
printf("<br>\n");
print_br = 0;
first_time = 1;
}
}
txt += (strlen(smeg)+1);
while ((txt[0] == 32) || (txt[0] == 13))
 {
txt++;
}
}
return(0);
}

int printoff(struct ency_titles *stuff)
{
 int size;
 char *tmp;
 struct st_ency_formatting *fmt1;
printf("<hr>\n");
tmp=stuff->title;
size=strlen(stuff->title);
fmt1=stuff->fmt;
words=0;
loopies(tmp,fmt1,size);

printf("<br>\n");

tmp=stuff->text;
size=strlen(stuff->text) - 10;

loopies(tmp,fmt1,size);

printf("<br>\n");
printf("<br>\n");

return(0);
}
 
int usage()
{
printf("usage: htmlenc [-e] \"search string\"\n");
printf("-e: exact matches only");
exit(0);
}

int main(int argc, char *argv[])
{
  int args_read = 0;
  char search_string[50];
  struct ency_titles *thingy;
  struct ency_titles *kill_me;
  int i, fin_arg;
// if (argc > 1)
{
for (i = 0; i < argc; i++)
{
// printf("%s",argv[i]);
fin_arg = 0;
// if (!fin_arg)
//  if (argv[i][1] ==  "f")
//   if(strcpy(filename,argv[++i])) fin_arg = 1; else usage;
// printf("%s, %c",argv[i],argv[i][1]);
if (!fin_arg)
 if (argv[i][1] ==  'e')
//  { /* printf("exact\n"); */ if (exact == 1) fin_arg = 1; else usage;}
  ;
if (argv[i][1] == '-')
// {fin_arg = 1; usage;}
  ;
if (fin_arg) args_read++;
}
}
if (argc == 1) // no args
{
// usage();
}
if (argc-args_read > 0) strcpy(search_string,argv[argc-1]);
if (argc-args_read == 0) 
{
  printf ("Enter search string :");
  scanf ("%[a-zA-Z0-9.\"\'() -]", search_string);
}
if (exact)
 thingy = ency_get_title (search_string);
else
 thingy = ency_find_titles (search_string);

printf("<html>\n");
printf("<head><title>Search results for: %s</title></head>",search_string);
printf("<h1>Star Trek Encyclopedia</h1><br>\n");
printf("You searched for <b>%s</b>.\n",search_string);
  if ((thingy != NULL) && (thingy->title != NULL))
    {
      do
	{
words=0;
 printoff(thingy);
//	  printf ("\n%s\n\n%s\n", thingy->title, thingy->text);
	  kill_me = thingy;
	  thingy = thingy->next;
          free(kill_me->text);
	  free(kill_me);
	}
      while (thingy != NULL);
printf("<hr>\nMibus' ency reader<br>\n");
printf("queries, comments, flames, to <a href=\"mailto:beemer@picknowl.com.au\">Robert Mibus (beemer@picknowl.com.au)</a>");
    }
  else
    printf ("No matches\n");
printf("</html>\n");
return(0);
}
