/* This file is under the GPL
 * see http://www.gnu.org/copyleft/gpl.html
 */
#include <stdio.h>
#include <stdlib.h>
#include "ency.h"
#include "data.h"

char *decompressed=NULL;
char *d=NULL;
int verbose=0;
int allow_compression=1;
int size=0;

char *matches[256]=
{
"255 255 255",
"198 199 198",
"33 8 8",
"90 113 165",
"255 248 0",
"0 255 0",
"0 0 0",
"128 128 128",
"165 166 165",
"255 255 247",
"255 207 255",
"239 182 255",
"255 174 99",
"206 158 255",
"206 166 189",
"255 158 0",
"173 113 255",
"206 97 148",
"156 97 255",
"189 113 0",
"115 73 198",
"115 65 140",
"255 207 255",
"123 73 0",
"99 65 49",
"79 48 127",
"74 48 99",
"90 56 0",
"48 48 63",
"49 32 66",
"74 48 99",
"49 24 33",
"49 24 0",
"33 16 57",
"16 8 24",
"74 142 74",
"255 247 255",
"255 255 206",
"231 255 255",
"239 112 175",
"239 255 206",
"240 232 240",
"255 231 198",
"255 231 156",
"198 239 247",
"208 224 207",
"214 215 222",
"247 223 74",
"239 215 123",
"222 215 189",
"214 215 156",
"159 216 224",
"214 199 198",
"239 182 148",
"214 166 198",
"231 174 82",
"222 174 123",
"198 182 156",
"156 199 165",
"144 176 208",
"239 142 140",
"222 166 8",
"239 134 107",
"156 174 156",
"90 190 222",
"156 174 90",
"198 142 140",
"247 121 82",
"247 121 49",
"198 134 107",
"144 136 176",
"148 150 132",
"148 142 148",
"156 142 99",
"198 121 82",
"99 158 156",
"198 113 57",
"148 121 107",
"140 121 132",
"66 150 206",
"148 121 74",
"95 128 144",
"165 105 90",
"99 134 99",
"156 105 66",
"206 81 49",
"132 105 99",
"90 113 165",
"198 73 66",
"99 113 132",
"132 105 57",
"96 112 96",
"165 73 115",
"107 105 90",
"95 104 95",
"148 81 82",
"41 150 173",
"111 96 64",
"148 81 41",
"64 104 128",
"148 89 16",
"41 105 173",
"132 81 66",
"64 104 96",
"82 97 90",
"123 81 49",
"222 32 41",
"99 81 115",
"99 81 66",
"64 96 95",
"107 81 49",
"181 48 41",
"90 81 82",
"198 40 24",
"79 80 80",
"66 81 132",
"99 81 33",
"79 80 64",
"90 81 49",
"66 81 99",
"123 65 16",
"57 81 82",
"99 65 66",
"140 48 41",
"74 73 66",
"99 65 41",
"82 97 16",
"57 73 66",
"74 73 41",
"66 65 82",
"41 73 90",
"99 56 41",
"16 73 156",
"74 65 41",
"000 000 000", /* unknown */
"64 64 63",
"115 48 8",
"49 65 66",
"165 32 24",
"66 65 16",
"57 65 41",
"16 65 123",
"41 65 41",
"99 40 41",
"16 73 90",
"66 56 49",
"66 56 33",
"90 48 16",
"66 48 66",
"41 65 66",
"90 48 0",
"24 56 107",
"49 56 57",
"57 56 16",
"115 190 99",
"66 48 33",
"41 56 41",
"24 56 66",
"66 48 8",
"24 48 90",
"24 56 41",
"41 48 49",
"41 40 66",
"41 48 33",
"107 16 8",
"66 32 41",
"47 40 63",
"66 40 8",
"41 48 16",
"41 40 41",
"24 48 41",
"132 150 222",
"82 24 8",
"41 40 24",
"33 40 41",
"41 40 8",
"8 40 66",
"49 32 16",
"24 40 41",
"000 000 000", /* unknown */
"123 16 16",
"16 40 16",
"000 000 000", /* unknown */
"33 32 24",
"0 65 0",
"8 40 41",
"49 16 16",
"255 121 0",
"8 24 90",
"24 24 57",
"33 24 8",
"49 16 8",
"90 0 0",
"16 32 24",
"000 000 000", /* unknown */
"000 000 000", /* unknown */
"16 32 8",
"24 16 33",
"0 105 255",
"57 8 0",
"24 24 16",
"0 24 41",
"24 24 0",
"0 24 41",
"16 24 16",
"33 8 16",
"000 000 000", /* unknown */
"0 16 57",
"000 000 000", /* unknown */
"33 8 8",
"24 16 16",
"255 97 0",
"0 24 16",
"132 113 49",
"0 105 0",
"000 000 000", /* unknown */
"000 000 000", /* unknown */
"33 8 0",
"222 65 41",
"16 16 0",
"000 000 000", /* unknown */
"33 0 8",
"132 190 140",
"206 81 16",
"24 8 0",
"0 16 8",
"33 0 0",
"000 000 000", /* unknown */
"0 16 0",
"0 8 47",
"16 8 0",
"8 8 16",
"000 000 000", /* unknown */
"8 8 8",
"24 0 0",
"255 65 41",
"16 0 8",
"000 000 000", /* unknown */
"90 121 66",
"156 150 41",
"189 199 41",
"239 166 49",
"173 207 90",
"0 150 148",
"255 40 24",
"160 0 80",
"160 200 240",
"198 223 198",
"198 199 198",
"0 190 189",
"0 0 0",
"148 142 148",
"49 24 33",
"0 190 0",
"189 0 0",
"0 0 0"
};

static char *get_colour (unsigned char c)
{
	return matches[c];
}

static void repeat (unsigned char c, int n)
{
	int i;
	for (i=0;i<n;i++)
		if (decompressed)
			if (d < decompressed+size)
				*d++ = c;
	if (!decompressed)
		size += n;
}

static void write_n_bytes (FILE *inp, int n)
{
	int i;
	unsigned char c;

	for (i=0;i<n;i++)
	{
		c = getc (inp);
		if (feof (inp))
			return;
		if (decompressed)
			if (d < decompressed+size)
				*d++ = c;
	}
	if (!decompressed)
		size += n;
}

void process_bytes (FILE *inp)
{
	unsigned char c;

	c = getc (inp);

	if (c & 0x80) /* Repeating */
	{
		if (!feof (inp))
			repeat (getc (inp), 0xFF-c+2);
	} else
		write_n_bytes (inp, c+1);
}

void write_final_image (FILE *out, long width, long height)
{
	fprintf (out, "P3\n%ld %ld\n255\n", width, height);

	d = decompressed;
	for (d=decompressed;d<decompressed+size;d++)
		fprintf (out, "%s\n", get_colour (*d));
}

int create_ppm_from_image (char *file, FILE *inp, long width, long height, long csize)
{
	FILE *out;
	int i;

	if (!file || !inp || width<=0 || height <=0)
		return 1;

	out = fopen (file, "w b");
	if (!out)
		return 1;

	/* Decompressed size */
	size = width*height;

	if (csize == size) /* Its complete */
	{
		decompressed = (char *) malloc (size);
		fread (decompressed, size, 1, inp);
	}
	else
	{
		d = decompressed = (char *) malloc (size);
		for (i=0;i<csize;i++)
			process_bytes (inp);
	}

	write_final_image (out, width, height);
	return 0;
}