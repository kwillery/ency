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
#include <ctype.h>
#include <getopt.h>
#include "data.h"
#include "scan.h"

struct block
{
	char name[5];
	long size;
};

struct casttable
{
	char *name;
	int id;
	struct casttable *next;
} *casts=NULL;

struct st_part *parts=NULL;
struct st_part *pcurr=NULL;
struct st_part *plast=NULL;

int curr_id = 0;

static struct block *get_block(FILE *inp, int reverse)
{
	struct block *b=NULL;
	long mult=1;
	int i;

	if (feof (inp))
		return NULL;

	b = malloc (sizeof (struct block));

	if (reverse)
	{
		for (i=3;i>=0;i--)
			b->name[i] = getc (inp);
	} else
		fread (b->name, 4, 1, inp);

	b->name[4] = 0;

	b->size = 0;
	if (reverse)
	{
		for (i=0;i<4;i++)
		{
			b->size += mult * getc (inp);
			mult *= 256;
		}
	} else
	{
		for (i=0;i<4;i++)
			b->size = b->size * 256 + getc (inp);
	}
	return b;
}

static void identify_section (struct st_part *part)
{
	char *temp;
	int i;

	if (!part->name)
		return;

	temp = strdup (part->name);
	for (i=0;(temp[i] = tolower (temp[i])); i++);

	/* drop -URGH ones */
	if (strlen (temp) > 5)
	{
		if (strcmp (temp+(strlen(temp))-5,"-urgh") == 0)
		{
			free (temp);
			return;
		}
	}

	/* Attrib */
	if (!strncmp (temp, "attrib_", 7))
	{
		part->type = ST_BLOCK_ATTRIB;
		if (strstr (temp, "_ency"))
			part->section = 0;
		else if (strstr (temp, "_epis"))
			part->section = 1;
		else if (strstr (temp, "_chro"))
			part->section = 2;
		else
			part->type = 0;

		if (strstr (temp, "_epissub"))
			part->section = 3;
	}

	/* Look Up - ptbl etc. */
	if (strstr (temp, "_lu_") || !strncmp (temp, "lu_", 3))
		part->type = ST_SECT_PTBL;

	/* Captions */
	if (!strncmp (temp, "captxt", 6))
	{
		if (strstr (temp, "_vid"))
			part->type = ST_SECT_VCPT;
		else
			part->type = ST_SECT_PCPT;
	}

	/* FT Lists */
	if (!strncmp (temp, "ftency", 6) && !strstr (temp, "breakpoint"))
	{
		part->type = ST_BLOCK_FTLIST;
		part->section = 0;
	}
	if (!strncmp (temp, "ftepis", 6) && !strstr (temp, "breakpoint"))
	{
		part->type = ST_BLOCK_FTLIST;
		part->section = 1;
	}
	if (!strncmp (temp, "ftchro", 6) && !strstr (temp, "breakpoint"))
	{
		part->type = ST_BLOCK_FTLIST;
		part->section = 2;
	}

	if (!strncmp (temp, "flash_except", 12))
	{
		part->type = ST_BLOCK_FLASHEXCEPT;
		part->section = 0;
	}

	free (temp);
	return;
}

static void process_cast_block (FILE *inp, long size)
{
	struct st_part *tmp, *curr;
	unsigned char *block;
	unsigned char *t=NULL;
	int i;

//	printf ("\tFound CASt block");

	if (size < 45)
	{
//		printf (" - not long enough");
		fseek (inp, size, SEEK_CUR);
		return;
	}

	block = malloc (size * sizeof (char));
	fread (block, size, 1, inp);

	switch (block[3] + block[6])
	{
//	case 0: // STXTs in omni1?
//	case 6: // 'snd ' in omni1? // DONT WANT
//	case 9: // ?? in omni1?
//	case 1: // BITD
	case 3: // STXT
	case 7: // an odd STXT in ency99
		break;
		default:
//		printf (", Ignoring [%d/%d]...", block[3], block[6]);
		free (block);
		return;
	}

	tmp = new_part();

	if (!parts)
		parts = tmp;
	else
	{
		if (plast)
			plast->next = tmp;
	}

	if (!pcurr)
		pcurr = tmp;

	plast = tmp;

	if (size > 33 && block[33])
	{
		t = block + 32;
		for (i=0;i<block[33];i++)
			while (*t++ == 0)
				if (t - block >= size)
				{
//					printf (" - ends early!!");
					tmp->name = strdup ("it ended early!");
					free (block);
					return;
				}
		tmp->name = (char *) malloc (sizeof (char) * *t + 1);
	} else if (size > 45 && block[45])
	{
		t = block+44;
		for (i=0;i<block[45];i++)
			while (*t++ == 0)
				if (t - block >= size)
				{
//					printf (" - ends early!!");
					tmp->name = strdup ("it ended early!");
					free (block);
					return;
				}
		tmp->name = (char *) malloc (sizeof (char) * *t + 1);
	} else if (size > 57 && block[57])
	{
		t = block + 56;
		for (i=0;i<block[57];i++)
			while (*t++ == 0)
				if (t - block >= size)
				{
//					printf (" - ends early!!");
					tmp->name = strdup ("it ended early!");
					free (block);
					return;
				}
		tmp->name = (char *) malloc (sizeof (char) * *t + 1);
	} else if (size > 55 && block[55])
	{
		t = block + 54;
		for (i=0;i<block[55];i++)
			while (*t++ == 0)
				if (t - block >= size)
				{
//					printf (" - ends early!!");
					tmp->name = strdup ("it ended early!");
					free (block);
					return;
				}
		tmp->name = (char *) malloc (sizeof (char) * *t + 1);
	} else if (size > 58 && block[58])
	{
		t = block + 57;
		for (i=0;i<block[58];i++)
			while (*t++ == 0)
				if (t - block >= size)
				{
//					printf (" - ends early!!");
					tmp->name = strdup ("it ended early!");
					free (block);
					return;
				}
		tmp->name = (char *) malloc (sizeof (char) * *t + 1);
	}

	if (tmp->name && (t - block + *t < size))
	{
		strncpy (tmp->name, t+1, *t);
		tmp->name[(int)*t] = 0;
	} else
		tmp->name = strdup ("???"); // Damn - can't get the name, Maybe it doesn't have one.

//	printf (" (%s)", tmp->name);

//	printf (" [");
//	for (i=0;i<size;i++)
//		if (isprint (block[i]))
//			printf ("%c", block[i]);
//	printf ("]");

	identify_section (tmp);
	tmp->count = 1;
	tmp->start_id = 0;
	tmp->next = NULL;

	/* We don't want duplicate names being used */
	/* so we set later ones to Unimportant      */
	curr = parts;

	while ((curr) && (curr != tmp))
	{
		if (!strcmp (curr->name, tmp->name))
		{
			tmp->section = 0;
			tmp->type = 0;
//			printf (" [Dupe - ignored]");
			break;
		}
		curr = curr->next;
	}

	free (block);
}

static void load_cast_table (FILE *inp, int size)
{
	long start;
	struct casttable *curr=NULL, *last=NULL;
	char temp[256]="";
	char *t;

	start = ftell (inp);

	fseek (inp, 12, SEEK_CUR);

	while (getc (inp) != ']')
	{
		curr = (struct casttable *) malloc (sizeof (struct casttable));
		if (!casts)
			casts = curr;
		if (last)
			last->next = curr;

		fscanf (inp, "%d: ", &(curr->id));
		if (getc (inp) == '\"')
		{
			t = temp;
			while ((*t++ = getc (inp)) != '\"')
				;
			*--t = 0;
			curr->name = strdup (temp);
		} else
			curr->name = strdup ("NULL");

		curr->next = NULL;
		last = curr;
	}

	fseek (inp, start, SEEK_SET);
}

static void process_noncast_block (FILE *inp, long size)
{
	struct casttable *tmp_casts=NULL;
	if (pcurr)
	{
//		printf ("\tCASt says \"%s\"", pcurr->name);
		if (!strcmp (pcurr->name, "CastTable500"))
		{
			load_cast_table (inp, size);
			curr_id = 500;
		}

		tmp_casts = casts;
		curr_id++;
		if (strcmp (pcurr->name, "blank"))
			while (tmp_casts)
			{
				if (!strcasecmp (tmp_casts->name, pcurr->name))
				{
					curr_id = tmp_casts->id;
					break;
				}
				tmp_casts = tmp_casts->next;
			}

		pcurr->start = ftell (inp) + 12;
		pcurr->start_id = curr_id;
		pcurr = pcurr->next;
	} else
	{
//		printf ("\tNo prior matching CASt block");
	}
	fseek (inp, size, SEEK_CUR);
}

static int ignore_block (char *name, long size)
{
	if (strcmp (name, "STXT"))
		return 1;
	else
		return 0;
}

static void search_file (FILE *inp, int reverse)
{
	struct block *b=NULL;
	char c;

	/* Jump to the first useful block    */
	/* this seems to always be 'imap'    */
	/* ('pami' in some, due to reversal) */
	fseek (inp, 12, SEEK_SET);
	while ((b = get_block (inp, reverse)))
	{
//		printf ("%ld: %s\t%ld", ftell (inp), b->name, b->size);
		if (!strcmp (b->name, "CASt"))
			process_cast_block(inp, b->size);
		else
			if (!ignore_block (b->name, b->size))
				process_noncast_block(inp, b->size);
			else
				fseek (inp, b->size, SEEK_CUR);

//		printf ("\n");
		/* if the next byte is a NULL, get rid of it */
		if ((c = getc (inp)))
			ungetc (c, inp);
		free (b);
	}
}

struct st_part *scan_file (FILE *inp)
{
	char start[5];
	int reverse=0;

	fread (start, 4, 1, inp);
	start[4] = 0;

	if (!strcmp (start, "XFIR"))
		reverse = 1;
	else if (strcmp (start, "RIFX"))
		return NULL; /* not a valid ency (AFAWCT) */

	fseek (inp, 0, SEEK_SET);
	search_file (inp, reverse);

//	return make_useful_information (inp);
	return parts;
}













