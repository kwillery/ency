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

struct part
{
	int section;
	long start;
	int count;
	struct part *next;
};

int is_all_ascii (char *string)
{
	while (*string)
		if (isascii (*string++) == 0) return 0;
	return 1;
}

int ends_in_number (char *string, int expected_length)
{
	return (isdigit(string[expected_length]) || (string[expected_length] == 'F'));
}
int ends_in_quote (char *string, int expected_length)
{
	return ((string[1] == '\"') || (string[expected_length] == '\"'));
}

int not_embedding_bracket (char *string)
{
	return ((string[0] != '[') && (string[1] != '['));
}

int doesnt_have_junk (char *string)
{
	return (isalpha (string[0]) || isalpha (string[1]) || ispunct (string[0]) || ispunct (string[1]));
}

void check_for_captions (FILE *inp, FILE *outp, FILE *data)
{
	char c=0;
	char fnbase[9]="        ";
	char temp[9] = "        ";
	long found_at;

	found_at = ftell (inp);

	fread (fnbase, 8, 1, inp);
	st_cleanstring (fnbase);

	if (is_all_ascii (fnbase) && ends_in_quote (fnbase, 7) && ends_in_number (fnbase, 6))
	{
		if (fnbase[1] == '\"')
			fseek (inp, found_at + 2, SEEK_SET);
		c = getc (inp);

		if ((c == ':') || ((c == ' ') && (getc(inp) == ':')))
		{
			fread (temp, 8, 1, inp);
			if (not_embedding_bracket(temp))
			{
				if (doesnt_have_junk(temp))
				{
					*strchr (fnbase, '\"') = 0;

					fseek (inp, found_at, SEEK_SET);
					while ((c != ']') && (c != '\"'))
						c = getc (inp);
					if (c == ']')
						return;
					while ((c = getc (inp)) != '\"')
						;
					while ((c = getc (inp)) != '\"')
						;
					while ((c = getc (inp)) != '\"')
						;

					fread (temp, 8, 1, inp);
					if (!is_all_ascii (temp))
						return;
					if (!ends_in_quote (temp, 7))
						return;
					if (!ends_in_number (temp, 6))
						return;

					printf ("found cpt @ 0x%lx\t%s\n", found_at, fnbase);
					if (outp)
						fprintf (outp, "found cpt @ 0x%lx\t%s\n", found_at, fnbase);
					if (data)
						fprintf (data, "  <pcaption start=\"0x%lx\" count=\"1\"/>\n", found_at-2);
					fseek (inp, found_at, SEEK_SET);
					return;
				}
			}
		}
	}
	fseek (inp, found_at, SEEK_SET);
}

int check_for_table (FILE *inp, FILE *outp, FILE *data)
{
	char c=0;
	char fnbase[8]="       ";
	char temp[8] = "       ";
	long found_at;

	found_at = ftell (inp);

	fread (fnbase, 7, 1, inp);
	st_cleanstring (fnbase);

	if (is_all_ascii (fnbase) && ends_in_quote (fnbase, 6))
	{
		if (fnbase[1] == '\"')
			fseek (inp, found_at + 2, SEEK_SET);

		if (getc (inp) == ':')
		{
			fread (temp, 7, 1, inp);
			if (not_embedding_bracket(temp))
			{
				if (doesnt_have_junk(temp))
				{

					fseek (inp, found_at, SEEK_SET);
					while ((c != ']') && (c != '\"'))
						c = getc (inp);
					if (c == ']')
						return 0;
					while ((c = getc (inp)) != '\"')
						;
					while ((c = getc (inp)) != '\"')
						;
					while ((c = getc (inp)) != '\"')
						;

					fread (temp, 7, 1, inp);
					if (!is_all_ascii (temp))
						return 0;
					if (!ends_in_quote (temp, 6))
						return 0;

					*strchr (fnbase, '\"') = 0;
					printf ("found tbl @ 0x%lx\t%s\n", found_at-2, fnbase);
					if (outp)
						fprintf (outp, "found tbl @ 0x%lx\t%s\n", found_at-2, fnbase);
					if (data)
						fprintf (data, "  <ptable start=\"0x%lx\" count=\"1\"/>\n", found_at-2);
					fseek (inp, found_at, SEEK_SET);
					return 1;
				}
			}
		}
	}
	fseek (inp, found_at, SEEK_SET);
	return 0;
}

void check_for_old_captions (FILE *inp, FILE *outp, FILE *data)
{
	long found_at;
	char temp[32];
	char fnbasen[8];

	found_at = ftell(inp);

#if 0
	fseek (inp, 16, SEEK_CUR);
#endif

	fread (temp, 32, 1, inp);

	if ((temp[0] == '[') && doesnt_have_junk (temp + 1))
	{
		if (isdigit (temp[7]) && temp[8] == ':')
			if ((temp[9] == ' ') && (temp[10] == '\"'))
			{
				strncpy (fnbasen, temp+1, 7);
				fnbasen[7] = 0;
				printf ("found [old] cpt @ 0x%lx\t%s\n", found_at+16, fnbasen);
				if (outp)
					fprintf (outp, "found [old] cpt @ 0x%lx\t%s\n", found_at+16, fnbasen);
				if (data)
					fprintf (data, "  <pcaption start=\"0x%lx\" count=\"1\" old_type=\"true\"/>\n", found_at+16);
			}
	}

	fseek (inp, found_at, SEEK_SET);
}
#if 0
void find_media_tables (FILE *inp, FILE *outp, FILE *data)
{
	unsigned char c=0, old_c=0, old_old_c=0;
	unsigned char old_old_old_c=0;

	while (!feof(inp))
	{
		c = getc(inp);
		if ((old_c == '[') && (old_old_c != 0x20))
		{
			if (c == '\"')
			{
				if (!check_for_table(inp, outp, data))
					check_for_captions(inp, outp, data);
			} else if (c == '3')
			{
				if ((getc (inp) == '9') && (getc (inp) == '5') && (getc (inp) == ':'))
				{
					fseek (inp, 0x21, SEEK_CUR);
					if (getc (inp) == '\"')
						check_for_table(inp, outp, data);
				}
			}
		} else if ((c == 'S') && (old_c == 'T') && (old_old_c == 'X') && (old_old_old_c == 'T'))
		{
			check_for_old_captions(inp, outp, data);
		}

		old_old_old_c = old_old_c;
		old_old_c = old_c;
		old_c = c;
	}
}
#endif
static int find_next_stxt_block (FILE *inp, char *stxt)
{
	char d[5]="    ";

	while (!feof (inp))
	{
		d[3] = getc (inp);

		if ((!strcmp (d, stxt)))
			return 1;
		d[0] = d[1];
		d[1] = d[2];
		d[2] = d[3];
	}
	return 0;
}

void find_media_tables (FILE *inp, FILE *outp, FILE *data)
{
	char temp[5]="1234";
	char c;
	int reverse;

	fread (temp, 4, 1, inp);
	if (!strcmp (temp, "XFIR"))
		reverse = 1;
	else
		reverse = 0;
	while (find_next_stxt_block (inp, reverse ? "TXTS" : "STXT"))
	{
		fseek (inp, 16, SEEK_CUR);
		c = getc (inp);
		if (c == '[')
			c = getc (inp);
		if (c == '\"')
		{
			if (!check_for_table(inp, outp, data))
				check_for_captions(inp, outp, data);
		} else
		{
			ungetc (c, inp);
			ungetc ('[', inp);
			check_for_old_captions(inp, outp, data);
		}
	}
}
void save_match (FILE *inp, FILE *outp)
{
	int i;
	fseek (inp,-8, SEEK_CUR);
	for (i=0;i<16;i++)
		fprintf (outp, "%2x ", getc(inp));
	fseek (inp, -8, SEEK_CUR);
	fprintf (outp, "\n");
}

void print_parts (struct part *this_part, int sect, char *name, FILE *data)
{
	fprintf (data, "  <section name=\"%s\">\n", name);
	while (this_part)
	{
		if (this_part->section == sect)
			fprintf (data, "    <part start=\"0x%lx\" count=\"%d\"/>\n", this_part->start, this_part->count);
		this_part = this_part->next;
	}
	fprintf (data, "  </section>\n");
}

int guess_section (char *title, char *text, int last_section)
{
	char *episode_starts[7] =
	{
		"Original Series",
		"Next Generation",
		"Deep Space Nine",
		"Voyager episode",
		"No episodes",
		"There are no episodes",
		"There were no episodes"
	};
	int i;

	/* Episodes */
	if ((*title == '\"') || (strlen (title) == 1))
		for (i = 0; i < 7; i++)
		{
			if (!strncmp (text, episode_starts[i], strlen (episode_starts[i])))
				return 1;
			if (!strncmp (text + 1, episode_starts[i], strlen (episode_starts[i])))
				return 1;
		}
	if ((!strncmp (title, "Star Trek", 9)) && (last_section == 1))
		return 1;

	/* Chronology */
	if (*title == '\"')
		return 2;
	if (!strncmp (text, "\n\n", 2))
		return 2;
	if ((!strncmp (title, "Star Trek", 9)) && (last_section == 2))
		return 2;

	/* Encyclopedia */
	return 0;
}

void usage()
{
	printf ("scanenc [-s outfile] [-x savefile] scanfile\n");
	exit (1);

}

int main (int argc, char *argv[])
{
	FILE *inp=NULL, *outp=NULL, *data=NULL;
	char *sections[3] =
	{"Encyclopedia", "Episodes", "Chronology"};
	int counts[3] =
	{0, 0, 0};
	long returned = 0;
	int i = 0;
	char last_start = 0;
	int new_section;
	int last_section = -1;
	long this_start=0;
	int this_count=0;
	struct part *root_part=NULL, *this_part=NULL, *last_part=NULL;
	char *filename;
	char *save_file=NULL;
	char *save_data=NULL;
	struct ency_titles *entry;
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
		fprintf (data, "  <prepend_year/>\n  <append_series/>\n");
	}
	if (inp == 0)
	{
		printf ("You must supply the main data file as the first parameter\n");
		exit (1);
	}
#ifndef QUIET
	printf ("Fingerprint: ");
	for (i = 0; i < 16; i++)
		printf ("%x;", getc (inp));
	printf ("\n");
#endif
	if (save_file)
	{
		rewind(inp);
		fprintf (outp, "Fingerprint: ");
		for (i = 0; i < 16; i++)
			fprintf (outp, "%x;", getc (inp));
		fprintf (outp, "\n");
	}

	if (save_data)
	{
		rewind(inp);
		fprintf (data, "  <fingerprint>");
		for (i = 0; i < 16; i++)
			fprintf (data, "%x;", getc (inp));
		fprintf (data, "</fingerprint>\n");
	}
	
	st_init();
	st_force_unknown_file (1);
	st_set_filename (filename);
	printf ("Scanning for entries...\n");
	if (save_file)
		fprintf (outp, "Scanning for entries...\n");
	do
	{
		returned = st_find_start (inp);
		if (returned)
		{
			entry = st_get_title_at (ftell (inp));
			if ((!last_start) || (last_start > tolower (*entry->title)) || ((last_start == '\"') && (*entry->title != '\"')))
			{
				new_section = guess_section (entry->title, entry->text, last_section);
				if (new_section != last_section)
				{
					if (this_count)
					{
						if (save_file)
							fprintf (outp, "Total for this part: %d\n", this_count);

						this_part = (struct part *) malloc (sizeof (struct part));
						this_part->section = last_section;
						this_part->start = this_start;
						this_part->count = this_count;
						this_part->next = NULL;
						if (!root_part)
							root_part = this_part;
						if (last_part)
							last_part->next = this_part;
						last_part = this_part;
					}
					this_start = ftell(inp);

					if (save_file)
						fprintf (outp, "\nNew section (%s)\n", sections[new_section]);
#ifndef QUIET
					if (this_count)
						printf ("Total for this part: %d\n", this_count);
					printf ("\nNew section (%s)\n", sections[last_section = new_section]);
#else
					last_section = new_section;
#endif
					this_count = 0;
				}
			}
			last_start = tolower (*entry->title);
#ifndef QUIET
			printf ("found @ 0x%lx:  \t%s\n", ftell (inp), entry->title);
#endif
			if (save_file)
			{
				fprintf (outp ,"found @ 0x%lx:  \t%s\n", ftell (inp), entry->title);
				save_match (inp, outp);
			}
			counts[last_section]++;
			this_count++;
			st_free_entry (entry);
			getc (inp);	/* make sure it doesnt pick the same one up again */
		}
	}
	while (returned);
	if ((save_file) && (this_count))
		fprintf (outp, "Total for this part: %d\n", this_count);
	if (this_count)
		printf ("Total for this part: %d\n", this_count);

	printf ("Found entries:\n");
	if (save_file)
		fprintf (outp, "Found entries:\n");
	for (i = 0; i < 3; i++)
	{
		printf ("\t%s\t%d\n", sections[i], counts[i]);
		if (save_file)
			fprintf (outp, "\t%s\t%d\n", sections[i], counts[i]);
	}


	if (this_part)
	{
		this_part = (struct part *) malloc (sizeof (struct part));
		this_part->section = last_section;
		this_part->start = this_start;
		this_part->count = this_count;
		this_part->next = NULL;
		if (!root_part)
			root_part = this_part;
		if (last_part)
			last_part->next = this_part;
		last_part = this_part;
	}

	if (data)
	{
		for (i=0;i<3;i++)
			print_parts (root_part, i, sections[i], data);
	}

	rewind (inp);

	printf ("Scanning for media lists...\n");
	if (save_file)
		fprintf (outp, "Scanning for media lists...\n");

	find_media_tables (inp, outp, data);

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
