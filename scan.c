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
#include "data.h"
#include "scan.h"

struct block
{
	char name[5];
	long size;
};

struct st_block *blocks=NULL;
struct st_block *blast=NULL;

static void get_4b_string (FILE *inp, char *string, int reverse)
{
	int i;

	if (!inp)
		return;
	if (!string)
		return;

	if (reverse)
	{
		for (i=3;i>=0;i--)
			string[i] = getc (inp);
	} else
		fread (string, 4, 1, inp);
}

static long get_4b_int (FILE *inp, int reverse)
{
	int i;
	long num=0;
	long mult=1;

	if (!inp)
		return -1;

	if (reverse)
	{
		for (i=0;i<4;i++)
		{
			num += mult * getc (inp);
			mult *= 256;
		}
	} else
	{
		for (i=0;i<4;i++)
			num = num * 256 + getc (inp);
	}
	return num;
}

static struct block *read_block(FILE *inp, int reverse)
{
	struct block *b=NULL;

	if (feof (inp))
		return NULL;

	b = malloc (sizeof (struct block));

	get_4b_string (inp, b->name, reverse);

	b->name[4] = 0;

	b->size = get_4b_int (inp, reverse);

	return b;
}

static void identify_section (struct st_block *block)
{
	char *temp;
	int i;

	if (!block->name)
		return;

	/* We only want to identify STXT blocks */
	if (strcmp (block->btype, "STXT"))
		return;

	temp = strdup (block->name);
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
		block->type = ST_BLOCK_ATTRIB;
		if (strstr (temp, "_ency"))
			block->section = 0;
		else if (strstr (temp, "_epis"))
			block->section = 1;
		else if (strstr (temp, "_chro"))
			block->section = 2;
		else
			block->type = 0;

		if (strstr (temp, "_epissub"))
			block->section = 3;
	}

	/* Look Up - ptbl etc. */
	if (strstr (temp, "_lu_") || !strncmp (temp, "lu_", 3))
		block->type = ST_SECT_PTBL;

	/* Captions */
	if (!strncmp (temp, "captxt", 6))
	{
		if (strstr (temp, "_vid"))
			block->type = ST_SECT_VCPT;
		else
			block->type = ST_SECT_PCPT;
	}

	/* FT Lists */
	if (!strncmp (temp, "ftency", 6) && !strstr (temp, "breakpoint"))
	{
		block->type = ST_BLOCK_FTLIST;
		block->section = 0;
	}
	if (!strncmp (temp, "ftepis", 6) && !strstr (temp, "breakpoint"))
	{
		block->type = ST_BLOCK_FTLIST;
		block->section = 1;
	}
	if (!strncmp (temp, "ftchro", 6) && !strstr (temp, "breakpoint"))
	{
		block->type = ST_BLOCK_FTLIST;
		block->section = 2;
	}

	if (!strncmp (temp, "flash_except", 12))
	{
		block->type = ST_BLOCK_FLASHEXCEPT;
		block->section = 0;
	}

	free (temp);
	return;
}

static void process_cast_block (FILE *inp, int reverse, char *btype, long pblock_pos, long pblock_size)
{
	struct st_block *tmp, *curr;
	unsigned char *block;
	unsigned char *t=NULL;
	int i,j;
	int size;
	char name[5]="1234";

	get_4b_string (inp, name, reverse);
	size = get_4b_int (inp, reverse);

	if (size < 45)
		return;

	block = malloc (size * sizeof (char));
	fread (block, size, 1, inp);

	tmp = new_block();

	if (!blocks)
		blocks = tmp;
	else
	{
		if (blast)
			blast->next = tmp;
	}

	blast = tmp;

	tmp->start = pblock_pos;
	tmp->size = pblock_size;

	if (block[39])
		t = block + 37 + block[39];
	else
		t = block + 13 + block[15];
	
	while (*t == 0)
	{
		t++;
		if (t - block >= size)
		{
			tmp->name = strdup ("???");
			free (block);
			return;
		}
	}

	j = *t;
	for (i=0;i<j;i++)
		while (*t++ == 0)
			if (t - block >= size)
			{
				tmp->name = strdup ("???");
				free (block);
				return;
			}

	tmp->name = (char *) malloc (sizeof (char) * *t + 1);

	if (tmp->name && (t - block + *t < size))
	{
		strncpy (tmp->name, t+1, *t);
		tmp->name[(int)*t] = 0;
	} else
		tmp->name = strdup ("???"); // Damn - can't get the name, Maybe it doesn't have one.

	strcpy (tmp->btype, btype);
	identify_section (tmp);
	tmp->start_id = 0;
	tmp->next = NULL;

//	printf ("found '%s' at %ld\n", tmp->name, tmp->start);

	/* We don't want duplicate names being used */
	/* so we set later ones to Unimportant      */
	curr = blocks;

	while ((curr) && (curr != tmp))
	{
		if (!strcmp (curr->name, tmp->name) && curr->type)
		{
			tmp->section = 0;
			tmp->type = 0;
			break;
		}
		curr = curr->next;
	}

	free (block);
}

static void load_cast_table (FILE *inp)
{
	struct st_block *b=NULL;
	char name[256]="";
	char *t;
	int id=0;

	while (getc (inp) != ']')
	{
		/* Read the block name & ID */
		fscanf (inp, "%d: ", &id);
		if (getc (inp) == '\"')
		{
			t = name;
			while ((*t++ = getc (inp)) != '\"')
				;
			*--t = 0;
		} else
			continue;

		/* Find the block w/ that name, set its ID */
		/* (continue from where we were...) */
		while (b)
		{
			if (!strcmp (b->name, name))
			{
//				printf ("Given id %d to '%s' (%ld).\n", id,p->name,p->start);
				b->start_id = id;
				b=b->next;
				break;
			}
			b = b->next;
		}

		/* Skip this if we found it. */
		if (b)
			continue;

		/* ... but try again if we didn't */
		b = blocks;
		while (b)
		{
			if (!strcmp (b->name, name))
			{
				//printf ("Given id %d to '%s' (%ld) on a restarted search.\n", id,p->name,p->start);
				b->start_id = id;
				b=b->next;
				break;
			}
			b = b->next;
		}
	}
}

static void sort_blocks ()
{
	struct st_block *p=NULL, *l=NULL, *t=NULL;
	int need_sort=1;

	while (need_sort)
	{
		need_sort = 0;
		p = blocks;
		l = NULL;
		while (p)
		{
			t=p->next;
			if (t)
			{
				if (p->start > t->start)
				{
					if (l)
						l->next = t;
					else
						blocks = t;
					p->next = t->next;
					t->next = p;
					need_sort = 1;
				}
			}
			l=p;
			p=p->next;
		}
	}
	
}

static void add_block (FILE *inp, int reverse, char *bname, long block_ind, long cast_ind)
{
	long orig_pos = ftell(inp);
	long block_pos=-1;
	long block_size=-1;
	long cast_pos=-1;
	char name[5]="1234";

	fseek (inp, 0x4C+20*block_ind, SEEK_SET); /* Go to the block info */
	get_4b_string (inp, name, reverse);
	if (strcmp (bname, name)) /* Is it what it is supposed to be? */
	{
		printf ("Block type mismatch (%s != %s)! Block is #%ld (@ %ld), CASt #%ld. Was reading from %ld.\n", name, bname, block_ind, 0x4C+20*block_ind, cast_ind, orig_pos-8);
		fseek (inp, orig_pos, SEEK_SET);
		return;
	}
	block_size = get_4b_int (inp, reverse);
	block_pos = get_4b_int (inp, reverse);
	fseek (inp, 0x4C+20*cast_ind, SEEK_SET); /* Go to the cast block info */
	get_4b_string (inp, name, reverse); /* get 'CASt' */
	if (strcmp (name, "CASt")) /* It is 'CASt', right? */
	{
		//printf ("Not CASt at %d! (%ld). Refer block %d (%d)\n", cast_ind, 0x4C+20*cast_ind, block_ind, block_pos);
		fseek (inp, orig_pos, SEEK_SET);
		return;
	}
	fseek (inp, 4, SEEK_CUR); /* drop the size */
	cast_pos = get_4b_int (inp, reverse);
	fseek (inp, cast_pos, SEEK_SET); /* Go to the CASt */
	process_cast_block (inp, reverse, bname, block_pos+8, block_size); /* load the CASt */
	fseek (inp, orig_pos, SEEK_SET); /* Set it all back nicely */
}

static void read_key(FILE *inp, int reverse)
{
	char name[5]="1234";
	long block_ind=-1;
	long cast_ind=-1;

	block_ind = get_4b_int (inp, reverse);
	cast_ind = get_4b_int (inp, reverse);
	get_4b_string (inp, name, reverse);
	add_block (inp, reverse, name, block_ind, cast_ind);
}

static void process_key(FILE *inp, int reverse)
{
	long count=-1;
	int i;

	fseek (inp, 8, SEEK_CUR);
	count = get_4b_int (inp, reverse);
	for (i=0;i<count;i++)
		read_key (inp, reverse);
}

static void load_cast_tables (FILE *inp, int reverse)
{
	struct st_block *p = NULL;

	p = blocks;
	while (p)
	{
		if (!strncmp (p->name, "CastTable", 9))
		{
			fseek (inp, p->start + 16, SEEK_SET);
			load_cast_table (inp);
		}
		p = p->next;
	}
}

static void fill_block_ids()
{
	struct st_block *p=NULL;
	int curr_id=0;

	p = blocks;
	while (p)
	{
		if (p->start_id)
			curr_id = p->start_id;
		else if (curr_id)
		{
//			printf ("Filled id %d to '%s' (%ld)\n",curr_id+1,p->name,p->start);
			p->start_id = ++curr_id;
		}
		p=p->next;
	}
}

static void search_file (FILE *inp, int reverse)
{
	struct block *b=NULL;
	char c;

	/* Jump to the first useful block    */
	/* this seems to always be 'imap'    */
	/* ('pami' in some, due to reversal) */
	fseek (inp, 12, SEEK_SET);
	while ((b = read_block (inp, reverse)))
	{
		if (!strcmp (b->name, "KEY*"))
		{
			process_key (inp, reverse);
			break;
		}
		else
			fseek (inp, b->size, SEEK_CUR);

		/* if the next byte is a NULL, get rid of it */
		if ((c = getc (inp)))
			ungetc (c, inp);
		free (b);
	}

	sort_blocks ();
	load_cast_tables(inp, reverse);
	fill_block_ids();
}

static void clean_up ()
{
	blocks = NULL;
	blast = NULL;
}

int integrity_ok (FILE *inp, int reverse)
{
	// fixme:
	// check mmap @ 44 etc.

	return 1;
}

struct st_block *scan_file (FILE *inp)
{
	char start[5];
	int reverse=0;
	struct st_block *ret;

	fread (start, 4, 1, inp);
	start[4] = 0;

	if (!strcmp (start, "XFIR"))
		reverse = 1;
	else if (strcmp (start, "RIFX"))
		return NULL; /* not a valid ency (AFAWCT) */

	fseek (inp, 0, SEEK_SET);
	if (!integrity_ok (inp, reverse))
		return NULL;

	search_file (inp, reverse);

	ret = blocks;
	clean_up ();
	return ret;
}
