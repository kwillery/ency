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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esdata.h"

struct es_list *es_list_new ()
{
	struct es_list *list=NULL;
	list = (struct es_list *) malloc (sizeof (struct es_list));

	list->head = NULL;
	list->tail = NULL;
	list->curr = NULL;

	return list;
}

void es_list_add (struct es_list *list, char *index, void *data, long pos)
{
	struct es_list_entry *entry=NULL;

	entry = es_list_entry_new (index, data, pos);

	es_list_add_entry (list, entry);
}

void es_list_add_entry (struct es_list *list, struct es_list_entry *entry)
{
	int curr_diff, next_diff;

	if (list->curr && list->curr->next)
		if (strcmp (entry->index, list->curr->index) >= 0 && strcmp (entry->index, list->curr->next->index) < 0)
		{
			/* Immediately after list->curr */
			entry->next = list->curr->next;
			list->curr->next = entry;
			list->curr = entry;
			return;
		}

	if (!list->head)
	{
		/* There is nothing else in the list */
		list->head = list->tail = list->curr = entry;
		list->curr = entry;
		return;
	}
	if (strcmp (entry->index, list->head->index) < 0)
	{
		/* Before the first item */
		entry->next = list->head;
		list->head = entry;
		list->curr = entry;
		return;
	}
	if (strcmp (entry->index, list->tail->index) >= 0)
	{
		/* After the last item */
		list->tail->next = entry;
		list->tail = entry;
		list->curr = entry;
		return;
	}

	/*  Find it the long way. */
	if (!list->head || strcmp (entry->index, list->curr->index) < 0)
		list->curr = list->head;

	curr_diff = strcmp (entry->index, list->curr->index);
	while (list->curr->next)
	{
		next_diff = strcmp (entry->index, list->curr->next->index);
		if (list->curr->next && curr_diff > 0 && next_diff <= 0)
		{
			entry->next = list->curr->next;
			list->curr->next = entry;
			break;
		}
		list->curr = list->curr->next;
		curr_diff = next_diff;
	}
	if (!list->curr)
		fprintf (stderr, "es_list_add_entry: can't find place to add '%s' (data ='%p'; pos='%ld) to list at %p'\n", entry->index, entry->data, entry->pos, list);
}

struct es_list_entry *es_list_entry_new (char *index, void *data, long pos)
{
	struct es_list_entry *entry=NULL;

	entry = (struct es_list_entry *) malloc (sizeof (struct es_list_entry));

	entry->index = index;
	entry->data = data;
	entry->pos = pos;
	entry->next = NULL;

	return entry;
}

void es_list_free (struct es_list *list)
{
	struct es_list_entry *e,*f;

	if (!list)
		return;

	e = list->head;
	while (e)
	{
		f = e;
		e = e->next;
		free (f);
	}
	free (list);
}

void es_list_free_data (struct es_list *list, int free_index, int free_data)
{
	struct es_list_entry *e,*f;

	if (!list)
		return;

	e = list->head;
	while (e)
	{
		f = e;
		e = e->next;
		if (free_index)
			free (f->index);
		if (free_data && f->data)
			free (f->data);
		free (f);
	}
	free (list);
}

void es_list_dump (struct es_list *list)
{
	struct es_list_entry *e=NULL;

	printf ("Beginning dump of list at %p\n", list);
	e = list->head;
	while (e)
	{
		printf ("Entry '%s'; data='%p'; pos='%ld'\n", e->index, e->data, e->pos);
		e = e->next;
	}
}

struct es_list_entry *es_list_get (struct es_list *list, char *index)
{
	/* Probably over-pedantic... */

	/* No list */
	if (!list)
		return NULL;

	/* Empty list */
	if (!list->head)
		return NULL;

	/* Before first entry */
	if (strcmp (index, list->head->index) < 0)
		return NULL;

	/* After last entry */
	if (strcmp (index, list->tail->index) > 0)
		return NULL;

	if (!list->curr || strcmp (index, list->curr->index) < 0)
		list->curr = list->head;

	while (list->curr)
	{
		int diff;
		diff = strcmp (index, list->curr->index);
		if (diff == 0)
			return list->curr;
		else if (diff < 0)
			return NULL;
		list->curr = list->curr->next;
	}
	return NULL;
}
