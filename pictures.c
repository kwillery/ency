/* This file is under the GPL
 * see http://www.gnu.org/copyleft/gpl.html
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ency.h"
#include "encyfuncs.h"

int loaded_cmap=0;
char *matches[256];

void free_cmap ()
{
	int i;

	if (!loaded_cmap)
		return;

	for (i=0;i<256;i++)
		free (matches[i]);

	loaded_cmap = 0;
}

void load_CLUT (FILE *inp)
{
	int i;

	free_cmap ();

	for (i=0;i<256;i++)
	{
		char temp[16];
		int r,g,b;

		r = getc (inp);
		getc (inp);
		g = getc (inp);
		getc (inp);
		b = getc (inp);
		getc (inp);
		sprintf (temp, "%d %d %d", r, g, b);
		matches[i] = (char*) strdup (temp);
	}
	loaded_cmap = 1;
}

int is_cmap_loaded ()
{
	return loaded_cmap;
}

static char *get_colour (unsigned char c)
{
	return matches[c];
}

static void repeat (unsigned char **d, unsigned char c, int n)
{
	unsigned char *e=*d;
	int i;
	for (i=0;i<n;i++)
		*e++ = c;
	*d += n;
}

static void write_n_bytes (unsigned char **d, FILE *inp, int n)
{
	int i;
	unsigned char *e=*d;

	for (i=0;i<n;i++)
		*e++ = getc (inp);
	*d += n;
}

void process_bytes (unsigned char **d, FILE *inp, int max_left)
{
	unsigned char c;

	c = getc (inp);

	if (c & 0x80) /* Repeating */
	{
		if (!feof (inp))
		{
			if (0xFF-c+2 > max_left)
				repeat (d, getc (inp), max_left);
			else
				repeat (d, getc (inp), 0xFF-c+2);
		}
	} else {
		if (c+1 > max_left)
			write_n_bytes (d, inp, max_left);
		else
			write_n_bytes (d, inp, c+1);
	}
}

long get_decompressed_size (FILE *inp, int csize)
{
	int i,j;
	unsigned char c;
	long dsize=0;

	for (i=0;i<csize;i++)
	{
		c = getc (inp);
		if (c & 0x80)
		{
			getc (inp);
			dsize += 0xFF-c+2;
			i++;
		} else {
			for (j=0;j<c+1;j++)
				getc (inp);
			dsize += c+1;
			i += c+1;
		}
	}
	return dsize;
}

void write_ppm (FILE *out, unsigned char *img, long width, long height)
{
	unsigned char *d;
	int size = width * height;

	fprintf (out, "P3\n%ld %ld\n255\n", width, height);

	d = img;
	for (d=img;d<img+size;d++)
		fprintf (out, "%s\n", get_colour (*d));
}

unsigned char *decompress (FILE *inp, int csize, int size)
{
	int i;
	unsigned char *decompressed, *d;

	DBG ((stderr, "Decompressing from %d to %d\n", csize, size));
	d = decompressed = (char *) malloc (size);
	for (i=0;i<csize;i++)
		process_bytes (&d, inp, size-(d-decompressed));
	return decompressed;
}

int create_ppm_from_image (char *file, FILE *inp, long width, long height, long csize)
{
	unsigned char *decompressed=NULL;
	int size;
	FILE *out;

	if (!file || !inp || width<=0 || height <=0)
		return 4;

	out = fopen (file, "w b");
	if (!out)
		return 5;

	/* Decompressed size */
	size = width*height;

	if (csize == size) /* Its complete */
	{
		decompressed = (char *) malloc (size);
		fread (decompressed, size, 1, inp);
	}
	else
		decompressed = decompress (inp, csize, size);

	write_ppm (out, decompressed, width, height);
	fclose (out);

	return 0;
}
