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
#include <unistd.h>
#include <getopt.h>
#include "ency.h"

extern int optind;		/* for getopt() */

int words = 0;

struct st_ency_formatting *loopies (char *txt, struct st_ency_formatting *fmt, FILE * output)
{
	int word_finish = 0;
	char smeg[50];

	while ((*txt == '\n') || (*txt == ' '))
		txt++;

	while (strlen (txt))
	{
		sscanf (txt, "%s", smeg);
		words++;

		if (fmt)
			if (fmt->firstword == words)
			{
				if (fmt->bold)
					fprintf (output, "<b>");
				if (fmt->italic)
					fprintf (output, "<i>");
				if (fmt->underline)
					fprintf (output, "<u>");

				word_finish = words + fmt->words - 1;
			}

		fprintf (output, "%s", smeg);

		if ((strlen (txt) <= 2) && (word_finish - words))
			words = word_finish;

		if (words == word_finish)
		{
			if (fmt->underline)
				fprintf (output, "</u>");
			if (fmt->italic)
				fprintf (output, "</i>");
			if (fmt->bold)
				fprintf (output, "</b>");
			fmt = fmt->next;
		}

		txt += strlen (smeg);
		while ((txt[0] == '\n') || (txt[0] == ' '))
		{
			if (txt[0] == '\n')
				fprintf (output, "<br>");
			fprintf (output, "%c", *(txt++));
		}
	}

	return (fmt);
}

int printoff (struct ency_titles *stuff, FILE * output)
{
	char *tmp;
	struct st_ency_formatting *fmt1;

	tmp = stuff->title;
	fmt1 = stuff->fmt;
	words = 0;

	fprintf (output, "<h2>");
	fmt1 = loopies (tmp, fmt1, output);
	fprintf (output, "</h2>\n");

	tmp = stuff->text;

	loopies (tmp, fmt1, output);

	fprintf (output, "<br>\n");

	return (0);
}

void print_usage (void)
{
	printf (" htmlenc - Searches the Star Trek encyclopedias\nUsage: htmlenc [OPTION...] [search string]\n\n  --chronology\t(-c)\tSearches the chronology section\n  --episode\t(-e)\tSearches the episode guide section\n   (Default: Search the encyclopedia section)\n  --media\t(-m)\tDisplays associated media\n  --save FILE\t(-s)\tSaves to a given file\n");
	exit (0);
}

int main (int argc, char *argv[])
{
	char search_string[50];
	char *temp_fn = NULL;
	struct ency_titles *thingy = NULL, *full_body = NULL;
	struct st_media *media = NULL;
	char base_path[] = "/cdrom";	/* where the media dirs etc. are */
	int i = 0;
	int use_media = 0;
	int count = 0;
	int section = ST_SECT_ENCY;
	int options = ST_OPT_MATCH_SUBSTRING | ST_OPT_NO_CACHE | ST_OPT_NO_FMT;
	FILE *out = stdout;
	char *filename = NULL;

	static struct option long_opts[] =
	{
		{"help", 0, 0, 'h'},
		{"media", 0, 0, 'm'},
		{"episode", 0, 0, 'e'},
		{"chronology", 0, 0, 'c'},
		{"save", 0, 0, 's'},
		{"fulltext", 0, 0, 'f'},
		{0, 0, 0, 0}};

	while ((i = getopt_long (argc, argv, "ecmhs:f", long_opts, 0)) != EOF)
		switch (i)
		{
		case 'm':
			use_media = 1;
			break;
		case 'e':
			section = ST_SECT_EPIS;
			break;
		case 'c':
			if (section == ST_SECT_EPIS)
				options = options | ST_OPT_SORTEPIS;
			else
				section = ST_SECT_CHRO;
			break;
		case 's':
			filename = optarg;
			break;
		case 'f':
			options |= ST_OPT_FT;
			break;
		case 'h':
		default:
			print_usage ();
		}

	if (!st_init ())
	{
		printf ("Error opening data file.\n");
		exit (1);
	}

	if (use_media)
		st_load_media ();

	if (filename)
		if (!(out = fopen (filename, "w")))
		{
			perror ("Error opening file");
			exit (1);
		}

	if (argc > optind)
	{
		strcpy (search_string, argv[optind]);
	}
	else
	{
		printf ("Enter search string :");
		scanf ("%[a-zA-Z0-9.\"\'() -]", search_string);
	}

	thingy = st_find (search_string, section, options);

	i = 0;
	fprintf (out, "<html>\n");
	fprintf (out, "<head><title>Search results for: %s</title></head>", search_string);
	fprintf (out, "<h1>Star Trek %s</h1>\n", st_fileinfo_get_name (ST_FILE_CURR));
	fprintf (out, "You searched for <b>%s</b>.<br>\n", search_string);
	fprintf (out, "<hr><b>Found:</b><br>\n");

	full_body = thingy;
	while (full_body)
	{
		fprintf (out, "<a href=\"#%d\">%s</a><br>\n", count++, full_body->title);
		full_body = full_body->next;
	}

	count = 0;
	if ((thingy != NULL) && (thingy->title != NULL))
	{
		do
		{
			full_body = st_get_title_at (thingy->filepos);

			fprintf (out, "<hr>\n<a name=\"%d\">\n", count++);
			printoff (full_body, out);

			if (use_media)
				media = st_get_media (thingy->title);

			if (media)
			{
				fprintf (out, "<b>Associated media</b>\n<ul>");
				for (i = 0; i < 5; i++)
					if (strlen (media->photos[i].file))
					{	/* if there is photos #i */
						temp_fn = st_format_filename (media->photos[i].file, base_path, picture);
						fprintf (out, "<li><a href=\"%s\">%s</a> (picture)</li>\n", temp_fn, media->photos[i].caption);
						free (temp_fn);
					}
				if (strlen (media->video.file))
				{
					temp_fn = st_format_filename (media->video.file, base_path, video);
					fprintf (out, "<li><a href=\"%s\">%s</a> (video)</li>\n", temp_fn, media->video.caption);
					free (temp_fn);
				}
				if (strlen (media->swf.file))
				{
					temp_fn = st_format_filename (media->swf.file, base_path, swf);
					fprintf (out, "<li><a href=\"%s\">%s</a> (video)</li>\n", temp_fn, media->swf.caption);
					free (temp_fn);
				}
				free (media);
				media = NULL;

				fprintf (out, "</ul>");
			}

			st_free_entry_and_advance (&thingy);
			st_free_entry (full_body);
		}
		while (thingy != NULL);
	}
	else
		fprintf (out, "No matches<br>\n");

	fprintf (out, "<hr>\nThe Star Trek ency reader: ");
	fprintf (out, "<a href=\"http://users.bigpond.com/mibus/ency/\">http://users.bigpond.com/mibus/ency/</a><br>\n");
	fprintf (out, "Queries, comments, and flames, to <a href=\"mailto:mibus@bigpond.com\">Robert Mibus &lt;mibus@bigpond.com&gt;</a>");

	fprintf (out, "</html>\n");

	st_finish ();

	return (0);
}
