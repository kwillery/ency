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
#include "ency.h"

extern int optind;		/* for getopt() */

/* define QUIET to print only the final total */

int tell = 0;
long old_ftell;

struct block
{
	char name[5];
	long size;
};

struct part
{
	char *name;
	int section;
	long start;
	int count;
	int start_id;
	struct part *next;
};

struct part *parts=NULL;
struct part *pcurr=NULL;
struct part *plast=NULL;
int curr_id = 0;

void fingerprint (FILE *inp, FILE *outp, FILE *data)
{
	int i;

	rewind (inp);
#ifndef QUIET
	printf ("Fingerprint: ");
	for (i = 0; i < 16; i++)
		printf ("%x;", getc (inp));
	printf ("\n");
#endif
	if (outp)
	{
		rewind(inp);
		fprintf (outp, "Fingerprint: ");
		for (i = 0; i < 16; i++)
			fprintf (outp, "%x;", getc (inp));
		fprintf (outp, "\n");
	}

	if (data)
	{
		rewind(inp);
		fprintf (data, "  <fingerprint>");
		for (i = 0; i < 16; i++)
			fprintf (data, "%x;", getc (inp));
		fprintf (data, "</fingerprint>\n");
	}

	rewind (inp);
}

struct block *get_block(FILE *inp, int reverse)
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

char *sections[]=
{
	"Unimportant",
	"Attrib [Ency]",
	"Attrib [Epis]",
	"Attrib [Chro]",
	"Lookup",
	"Captions",
	"FTList [Ency]",
	"FTList [Epis]",
	"FTList [Chro]",
	"Text [Ency]",
	"Text [Epis]",
	"Text [Chro]",
	"Captions [Vid]",
	"List [Epis]",
};

int identify_section (char *section)
{
	char *temp;
	int i;
	temp = strdup (section);
	for (i=0;(temp[i] = tolower (temp[i])); i++);

	/* Attrib */
	if (!strncmp (temp, "attrib_", 7))
	{
		if (strstr (temp, "_ency"))
		{
			free (temp);
			return 1;
		}
		if (strstr (temp, "_epissub"))
		{
			free (temp);
			return 13;
		}
		if (strstr (temp, "_epis"))
		{
			free (temp);
			return 2;
		}
		if (strstr (temp, "_chro"))
		{
			free (temp);
			return 3;
		}
			
		return 0;
	}

	/* Lookup */
	if (strstr (temp, "_lu_") || !strncmp (temp, "lu_", 3))
	{
		free (temp);
		return 4;
	}

	/* Captions */
	if (!strncmp (temp, "captxt", 6))
	{
		if (strstr (temp, "_vid"))
		{
			free (temp);
			return 12;
		}
		free (temp);
		return 5;
	}

	/* FT Lists */
	if (!strncmp (temp, "ftency", 6) && !strstr (temp, "breakpoint"))
	{
		free (temp);
		return 6;
	}
	if (!strncmp (temp, "ftepis", 6) && !strstr (temp, "breakpoint"))
	{
		free (temp);
		return 7;
	}
	if (!strncmp (temp, "ftchro", 6) && !strstr (temp, "breakpoint"))
	{
		free (temp);
		return 8;
	}

	/* Entries... */
	if (strstr (temp, "entrytxt_") || strstr (temp, "entrytext_") || strstr (temp, "entry_"))
	{
		free (temp);
		return 9;
	}
	if (strstr (temp, "epistxt_") || strstr (temp, "epis_txt") || strstr (temp, "newepis"))
	{
		free (temp);
		return 10;
	}
	if (strstr (temp, "chrotxt_"))
	{
		free (temp);
		return 11;
	}
	if (strstr (temp, "newentries_"))
	{
		if (strstr (temp, "chro"))
			return 11;
		if (strstr (temp, "epis"))
			return 10;
		else
			return 9;
	}
	if (!strncmp (temp, "missed new", 10))
	{
		if (strstr (temp, "entries"))
		{
			free (temp);
			return 9;
		}
	}
	if (strstr (temp, " text - insert "))
	{
		free (temp);
		return 9;
	}

	free (temp);
	return 0;
}

void process_cast_block (FILE *inp, long size)
{
	struct part *tmp;
	char *block;
	char *t;

	printf ("\tFound CASt block");

	block = malloc (size * sizeof (char));
	fread (block, size, 1, inp);

	switch (block[3])
	{
		case 0:
		case 1:
		case 3:
		case 7:
			break;
		default:
		printf (", Ignoring...");
		free (block);
		return;
	}

	tmp = (struct part *) malloc (sizeof (struct part));
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

	t = block + block[7] + 12;
	*t = 0;
	while (!*--t)
		;
	while (*--t)
		;
	t += 2;

	tmp->name = (char *) malloc (sizeof (char) * *t + 1);
	strncpy (tmp->name, t+1, *t);
	tmp->name[(int) *t] = 0;

	printf (" (%s)", tmp->name);

	tmp->section = identify_section (tmp->name);
	tmp->count = 1;
	tmp->start_id = 0;
	tmp->next = NULL;

	free (block);
}

void process_noncast_block (FILE *inp, long size)
{
	if (pcurr)
	{
		printf ("\tCASt says \"%s\"", pcurr->name);
		if (!strcmp (pcurr->name, "CastTable500"))
			curr_id = 500;
		else
			if (curr_id)
				curr_id++;

		pcurr->start = ftell (inp) + 12;
		pcurr->start_id = curr_id;
		pcurr = pcurr->next;
	} else
	{
		printf ("\tNo prior matching CASt block");
	}
	fseek (inp, size, SEEK_CUR);
}

int ignore_block (char *name, long size)
{
	if (strcmp (name, "STXT") && strcmp (name, "BITD"))
		return 1;

	return 0;
}

void search_file (FILE *inp, int reverse, FILE *outp, FILE *data)
{
	struct block *b=NULL;
	char c;

	/* Jump to the first useful block    */
	/* this seems to always be 'imap'    */
	/* ('pami' in some, due to reversal) */
	fseek (inp, 12, SEEK_SET);
	while ((b = get_block (inp, reverse)))
	{
		printf ("%ld: %s\t%ld", ftell (inp), b->name, b->size);
		if (!strcmp (b->name, "CASt"))
			process_cast_block(inp, b->size);
		else
			if (!ignore_block (b->name, b->size))
				process_noncast_block(inp, b->size);
			else
				fseek (inp, b->size, SEEK_CUR);

		printf ("\n");
		/* if the next byte is a NULL, get rid of it */
		if ((c = getc (inp)))
			ungetc (c, inp);
		free (b);
	}
}

void write_useful_information (FILE *inp, FILE *outp, FILE *data)
{
	struct part *part=NULL;
	int i;
	int count, z=1;
	char *sect[] = 
	{
		"Encyclopedia",
		"Episodes",
		"Chronology"
	};

	fingerprint (inp, outp, data);

	part = parts;
	while (part)
	{
		printf ("Block of \'%s\' start=%lx count=%d\n", sections[part->section], part->start, part->count);
		if (outp)
			fprintf (outp, "Block of \'%s\' start=%lx count=%d\n", sections[part->section], part->start, part->count);
		part = part->next;
	}

	if (data)
	{
		for (i=0;i<3;i++)
		{
			count=0;
			part = parts;
			fprintf (data, "    <section name=\"%s\">\n", sect[i]);
			while (part)
			{
				if ((part->section == i + 1) || (part->section == i + 6) || (part->section == i + 9))
				{
					switch (part->section - i)
					{
						case 1: // Attrib
							fprintf (data, "      <list start=\"0x%lx\" count=\"%d\"/>\n", part->start, part->count);
							break;
						case 6: // FT
							fprintf (data, "      <ftlist start=\"0x%lx\" count=\"%d\"/>\n", part->start, part->count);
							break;
						case 9: // Text
							fprintf (data, "      <part start=\"0x%lx\" count=\"%d\" bcount=\"%d\" start_id=\"%d\"/>\n", part->start, 1, part->count, part->start_id);
							count += z;
							break;
					}
				}
				/* Episode numbering/ordering */
				if ((i == 1) && (part->section == 13))
				{
					fprintf (data, "      <list start=\"0x%lx\" count=\"%d\"/>\n", part->start, part->count);
				}
				part = part->next;
			}
			fprintf (data, "    </section>\n");
			printf ("Found %d entries in the %s section\n", count, sect[i]);
		}

		part = parts;
		while (part)
		{
			switch (part->section)
			{
				case 1:
				case 2:
				case 3:
					fprintf (data, "    <vtable start=\"0x%lx\" count=\"%d\" section=\"%d\"/>\n", part->start, part->count, part->section);
					break;
				case 4: // LU
					fprintf (data, "    <ptable start=\"0x%lx\" count=\"%d\"/>\n", part->start, part->count);
					break;
				case 5: // CPT
					fprintf (data, "    <pcaption start=\"0x%lx\" count=\"%d\"/>\n", part->start, part->count);
					break;
				case 12: // VCPT
					fprintf (data, "    <vcaption start=\"0x%lx\" count=\"%d\"/>\n", part->start, part->count);
					break;
			}
			part = part->next;
		}
	}
	
}

void do_search (FILE *inp, FILE *outp, FILE *data)
{
	char start[5];
	int reverse=0;

	fread (start, 4, 1, inp);
	start[4] = 0;

	if (!strcmp (start, "XFIR"))
		reverse = 1;
	else if (strcmp (start, "RIFX"))
	{
		printf ("Sorry, not a valid encyclopedia.\n");
	}

	fseek (inp, 0, SEEK_SET);
	search_file (inp, reverse, outp, data);

	write_useful_information (inp, outp, data);
}


void usage()
{
	printf ("scanenc [-s outfile] [-x savefile] scanfile\n");
	exit (1);

}

int main (int argc, char *argv[])
{
	FILE *inp=NULL, *outp=NULL, *data=NULL;
	int i = 0;
	char *filename;
	char *save_file=NULL;
	char *save_data=NULL;
	static struct option long_opts[] =
	{
		{"save", 0, 0, 's'},
		{"export", 0, 0, 'x'},
		{"help", 0, 0, 'h'},
		{0, 0, 0, 0}};

	while ((i = getopt_long (argc, argv, "s:x:h", long_opts, 0)) != EOF)
		switch (i)
		{
		case 's':
			save_file = optarg;
			break;
		case 'x':
			save_data = optarg;
			break;
		case 'h':
		default:
			usage ();
		}

	if (argc > optind)
	{
		filename = argv[optind];
	} else
		usage();

	inp = fopen (filename, "r b");
	if (save_file)
	{
		outp = fopen (save_file, "w");
		if (!outp)
		{
			printf ("Error writing to %s\n",save_file);
			exit;
		}
	}

	if (save_data)
	{
		data = fopen (save_data, "w");
		if (!data)
		{
			printf ("Error writing to %s\n", save_data);
			exit;
		}
		fprintf (data, "<file>\n");
		fprintf (data, "  <name>Unknown encyclopedia</name>\n");
		fprintf (data, "  <mainfile>%s</mainfile>\n", filename);
		fprintf (data, "  <datadir>Please_fill_this_field_in</datadir>\n");
		fprintf (data, "  <photodir>Please_fill_this_field_in</photodir>\n");
		fprintf (data, "  <videodir>Please_fill_this_field_in</videodir>\n");
		fprintf (data, "  <append_char/>\n");
	}
	if (inp == 0)
	{
		printf ("You must supply the main data file as the last parameter\n");
		exit (1);
	}

	do_search (inp, outp, data);

	if (outp)
		fclose (outp);
	if (data)
	{
		fprintf (data, "</file>\n");
		fclose (data);
	}

	fclose (inp);
	return 0;
}
