/*****************************************************************************/
/* star trek ency reader: Reads the Star Trek Encyclopedia (1998 version)    */
/* Also reads the various Omnipedias & Episode guides                        */
/* Copyright (C) 1998 Robert Mibus                                           */
/*                                                                           */
/* This program is free software; you can redistribute it and/or             */
/* modify it under the terms of the GNU General Public License               */
/* as published by the Free Software Foundation; either version 2            */
/* of the License, or (at your option) any later version.                    */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with this program; if not, write to the Free Software               */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.*/
/*                                                                           */
/* Author:                                                                   */
/*      Email   mibus@bigpond.com                                            */
/*      Webpage http://users.bigpond.com/mibus/                              */
/*****************************************************************************/

#ifndef ESDATA_H
#define ESDATA_H

struct es_list_entry
{
	char *index;
	void *data;
	long pos;
	struct es_list_entry *next;
};

/* es_list is a single-linked list of es_list_entrys */
struct es_list
{
	struct es_list_entry *head;
	struct es_list_entry *tail;
	struct es_list_entry *curr;
};

struct es_list *es_list_new (void);
void es_list_add_entry (struct es_list *list, struct es_list_entry *entry);
void es_list_add (struct es_list *list, char *index, void *data, long pos);
struct es_list_entry *es_list_entry_new (char *index, void *data, long pos);
void es_list_free (struct es_list *list);
void es_list_free_data (struct es_list *list, int free_index, int free_data);
void es_list_dump (struct es_list *list);
struct es_list_entry *es_list_get (struct es_list *list, char *index);

/* es_slist is 27 es_lists, A-Z and 'other' (performance optimisation) */
struct es_slist
{
	struct es_list *lists[27];
};

struct es_slist *es_slist_new (void);
void es_slist_add (struct es_slist *list, char *index, void *data, long pos);
void es_slist_free (struct es_slist *list);
void es_slist_free_data (struct es_slist *list, int free_index, int free_data);
void es_slist_dump (struct es_slist *list);
struct es_list_entry *es_slist_get (struct es_slist *list, char *index);

#endif

