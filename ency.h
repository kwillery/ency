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

#include <gtk/gtk.h>

GtkWidget *window, *box0, *box1, *entry1, *label1, *button1, *results, *vscroll;
GtkWidget *table;
char *label1text;
char *rtnresults;

char ency_cleantext (unsigned char);
int ency_open (void);
int ency_close (void);
int getnext (char[]);
struct ency_titles *ency_find_titles (char[]);
struct ency_titles *ency_get_title (char[]);
struct st_ency_formatting
{
int bi;
int words;
int firstword;
struct st_ency_formatting *next;
};

struct ency_titles
  {
    char *title;
    char *text;
    struct st_ency_formatting *fmt;
    struct ency_titles *next;
  };
