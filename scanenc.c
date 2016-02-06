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

/* Oooh, Aaah - How Naughty :-P */
#define PRINT_ALL
#define BLOCK_EXPORT
#include "scan.c"
#undef PRINT_ALL
#undef BLOCK_EXPORT

void usage()
{
	printf ("scanenc [-x blockname] scanfile\n");
	exit (1);
}

int main (int argc, char *argv[])
{
	FILE *inp=NULL;
	int i = 0;
	char *filename;
	static struct option long_opts[] =
	{
		{"help", 0, 0, 'h'},
		{"export", 0, 0, 'x'},
		{0, 0, 0, 0}};

	while ((i = getopt_long (argc, argv, "x:h", long_opts, 0)) != EOF)
		switch (i)
		{
		case 'x':
			blockname = optarg;
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

	if (inp == 0)
	{
		printf ("You must supply the main data file as the last parameter\n");
		exit (1);
	}

	scan_file (inp);

	fclose (inp);
	return 0;
}
