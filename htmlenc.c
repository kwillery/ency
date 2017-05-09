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

void print_one_media (struct st_photo media, char *base_path, media_type mtype, char *stype, FILE *out)
{
	char *temp_fn;
	if (strlen (media.file))
	{
		temp_fn = st_format_filename (media.file, base_path, mtype);
		fprintf (out, "<li><a href=\"%s\">%s</a> (%s)</li>\n", temp_fn, media.caption, stype ? stype : "unknown");
		free (temp_fn);
	}
}

void print_media(struct st_media *media, char *base_path, FILE *out)
{
	int i;

	if (!media)
		return;

	fprintf (out, "<b>Associated media</b>\n<ul>");

	for (i = 0; i < 6; i++)
		print_one_media (media->photos[i], base_path, picture, "picture", out);

	print_one_media (media->video, base_path, video, "video", out);

	print_one_media (media->swf, base_path, swf, "flash", out);

	print_one_media (media->audio, base_path, audio, "audio", out);

	if (strlen(media->resource))
		fprintf (out, "<li>There is an associated resource: %s (DXR)</li>\n", media->resource);

	fprintf (out, "</ul>");

	free (media);
}

void print_usage (void)
{
	printf (" htmlenc - Searches the Star Trek encyclopedias\nUsage: htmlenc [OPTION...] [search string]\n\n  --chronology\t(-c)\tSearches the chronology section\n  --episode\t(-e)\tSearches the episode guide section\n   (Default: Search the encyclopedia section)\n  --media\t(-m)\tDisplays associated media\n  --fulltext\t(-f)\tPerform a fulltext search\n  --ultraclean\t(-u)\tRemove accented characters\n  --save FILE\t(-s)\tSaves to a given file\n");
	exit (0);
}

int main (int argc, char *argv[])
{
	char search_string[256]="";
	struct ency_titles *thingy = NULL, *full_body = NULL;
	struct st_media *media = NULL;
	char base_path[] = "/cdrom";	/* where the media dirs etc. are */
	int i = 0;
	int use_media = 0;
	int count = 0;
	int section = ST_SECT_ENCY;
	int options = ST_OPT_MATCH_SUBSTRING | ST_OPT_NO_FMT;
	FILE *out = stdout;
	char *filename = NULL;
	int ultraclean=0;
	static struct option long_opts[] =
	{
		{"help", 0, 0, 'h'},
		{"media", 0, 0, 'm'},
		{"episode", 0, 0, 'e'},
		{"chronology", 0, 0, 'c'},
		{"fulltext", 0, 0, 'f'},
		{"ultraclean", 0, 0, 'u'},
		{"save", 0, 0, 's'},
		{0, 0, 0, 0}};

	while ((i = getopt_long (argc, argv, "ecmhfus:", long_opts, 0)) != EOF)
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
		case 'u':
			ultraclean = 1;
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

	/* If there is an output file, open it */
	if (filename)
		if (!(out = fopen (filename, "w")))
		{
			perror ("Error opening file");
			exit (1);
		}

	/* Get a string to search for, either from the command line or read from the user */
	if (argc > optind)
	{
		strcpy (search_string, argv[optind]);
		for (i=optind+1;i<argc;i++)
		{
			strcat (search_string, " ");
			strcat (search_string, argv[i]);
		}
	}
	else
	{
		printf ("Enter search string :");
		if (scanf ("%[a-zA-Z0-9.\"\'() -]", search_string) == 0) {
			printf ("No search term provided, exiting...\n");
			exit (0);
		}
	}

	/* Do the search */
	thingy = st_find (search_string, section, options);

	/* Header */
	fprintf (out, "<html>\n");
	fprintf (out, "<head><title>Search results for: %s</title></head>", search_string);
	fprintf (out, "<h1>Star Trek %s</h1>\n", st_fileinfo_get_name (ST_FILE_CURR));
	fprintf (out, "You searched for <b>%s</b>%s.<br>\n", search_string, (options & ST_OPT_FT) ? " in full-text mode" : "");
	fprintf (out, "<hr><b>Found:</b><br>\n");

	/* Go through the list once, to have a TOC of sorts at the top of the page */
	full_body = thingy; /* N.B. full_body is a temporary variable here, not like later */
	while (full_body)
	{
		if (ultraclean)
			st_ultraclean_string (full_body->name);
		if (options & ST_OPT_FT)
			fprintf (out, "<a href=\"#%d\">%s</a> [%.0f%%]<br>\n", count++, full_body->name, full_body->score);
		else
			fprintf (out, "<a href=\"#%d\">%s</a><br>\n", count++, full_body->name);
		full_body = full_body->next;
	}

	/* Print the actual entries */
	count = 0;
	if ((thingy != NULL) && (thingy->name != NULL))
	{
		do
		{
			/* Get the text, formatting, etc. */
			full_body = st_get_title_at (thingy->filepos);

			/* Remove accented characters etc. if we have to */
			if (ultraclean)
			{
				st_ultraclean_string (full_body->name);
				st_ultraclean_string (full_body->title);
				st_ultraclean_string (full_body->text);
			}

			/* Anchor for above TOC-like links */
			fprintf (out, "<hr>\n<a name=\"%d\">\n", count++);

			/* Print the entry */
			printoff (full_body, out);

			if (use_media)
			{
				media = st_get_media (thingy->name);
				print_media (media, base_path, out);
			}

			st_free_entry (full_body);
			st_free_entry_and_advance (&thingy);
		}
		while (thingy != NULL);
	}
	else
		fprintf (out, "No matches<br>\n");

	/* Footer */
	fprintf (out, "<hr>\nThe Star Trek ency reader: ");
	fprintf (out, "<a href=\"http://users.bigpond.com/mibus/ency/\">http://users.bigpond.com/mibus/ency/</a><br>\n");
	fprintf (out, "Queries, comments, and flames, to <a href=\"mailto:mibus@bigpond.com\">Robert Mibus &lt;mibus@bigpond.com&gt;</a>");

	fprintf (out, "</html>\n");

	/* Clean up and go home! :-) */
	st_finish ();

	return (0);
}



