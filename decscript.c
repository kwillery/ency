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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "ency.h"
#include "encyfuncs.h"
#include "scan.h"

void loopies (char *txt, struct st_ency_formatting *fmt, FILE *output)
{
	int words = 0;
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
                while ((txt[0] == '\n') || (txt[0] == ' ') || (txt[0] == '\t'))
                {
                        if (txt[0] == '\n')
                                fprintf (output, "<br>");
                        fprintf (output, "%c", *(txt++));
                }
        }
}

int main (int argc, char **argv)
{
	FILE *inp;
	struct st_block *b;

	if (argc != 2)
	{
		printf ("Usage: decscript [filename]\n");
		exit (1);
	}

	inp = fopen (argv[1], "r b");

	if (!inp)
	{
		printf ("Cannot open file '%s'\n", argv[1]);
		exit (2);
	}

	printf ("<html><head><title>Script %s\n</title></head><body>", argv[1]);
	b = scan_file (inp);
	while (b)
	{
		struct st_ency_formatting *fmt=NULL;
		char *txt=NULL;
		fseek (inp, b->start + 12, SEEK_SET);
		fmt = st_return_fmt (inp);
		txt = st_return_text (inp);

		printf ("fmt = %p\n", fmt);
		printf ("txt = %s\n", txt);

//		loopies (txt, fmt, stdout);

		if (b->name && !strncmp (b->name, "finis", 5))
			break;
		b = b->next;
		printf ("\n<hr>\n");
	}
	printf("</body></html>");
	return 0;
}
