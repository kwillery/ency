/*
 * ency2 data.c file
 * data-related stuff goes here 
 * (C) 2000 Robert Mibus <mibus@bigpond.com>
 *
 * licensed under the GNU GPL 2.0 or greater
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>

#include "ency.h"
#include "encyfuncs.h"
#include "data.h"
#include "scan.h"
#include "rcfile.h"

/* strdup() barfs on NULLs, so if we want to be able to
 * copy a possibly NULL string, do this instead.
 * N.B. We return NULL not "" if the input is NULL. */
static char *strdup_if_valid (char *t)
{
	if (t)
		return strdup (t);
	else
		return NULL;
}

/* The list of files we support. */
struct st_data_filenode *files=NULL;

/* Returns one file out of 'files'.
 * If file_type is higher than the number we have
 * (starting at 0, so count_filetypes()-1) then it
 * returns NULL. If file_type is < 0 we just return the
 * first one. */
static struct st_data_filenode *get_filenode (int file_type)
{
	struct st_data_filenode *tmp;
	int i;

	if (file_type >= count_files())
		return NULL;

	tmp = files;

	for (i=0;i<file_type;i++)
		tmp = tmp->next;

	return tmp;
}

/* Frees a part struct and things it points to. */
void free_part (struct st_part *part)
{
	if (part->name)
		free (part->name);
	if (part->dir)
		free (part->dir);
	free (part);
}

/* Frees an exception. */
void free_exception (struct st_data_exception *ex)
{
	if (ex->type)
		free (ex->type);
	if (ex->from)
		free (ex->from);
	if (ex->to)
		free (ex->to);
	free (ex);
}

/* Frees a data_filenode, its parts, exceptions,
 * etc. etc. etc. */
void free_data_filenode (struct st_data_filenode *file)
{
	struct st_part *part,*tmp_part;
	struct st_data_exception *ex, *tmp_ex;
	part = file->parts;
	while (part)
	{
		tmp_part = part;
		part = part->next;
		free_part (tmp_part);
	}
	ex = file->exceptions;
	while (ex)
	{
		tmp_ex = ex;
		ex = ex->next;
		free_exception (tmp_ex);
	}
	if (file->name)
		free (file->name);
	if (file->mainfile)
		free (file->mainfile);
	if (file->datadir)
		free (file->datadir);
	if (file->photodir)
		free (file->photodir);
	if (file->videodir)
		free (file->videodir);
	if (file->fingerprint)
		free (file->fingerprint);
	free (file);

}

/* This clears all of our loaded file nodes. */
void st_data_clear (void)
{
	struct st_data_filenode *tmp=NULL;

	while (files)
	{
		tmp = files;
		files = files->next;
		free_data_filenode (tmp);
	}
}

/* A quick & safe way to make a part. */
struct st_part *new_part()
{
	struct st_part *part=NULL;
	part = (struct st_part *) malloc (sizeof (struct st_part));

	if (part)
	{
		part->name = NULL;
		part->type = 0;
		part->section = 0;
		part->start = 0;
		part->count = 0;
		part->start_id = 0;
		part->bcount = 1;
		part->dir = NULL;
		part->next = NULL;
		strcpy (part->btype, "");
	}

	return part;
}

/* A quick, easy & safe way to make an exception */
struct st_data_exception *new_exception (char *type, char *from, char *to)
{
	struct st_data_exception *ex;

	ex = malloc (sizeof (struct st_data_exception));

	ex->type = strdup (type);
	ex->from = strdup (from);
	ex->to = strdup (to);
	ex->next = NULL;

	return ex;
}

/* Returns how many files we know about. */
int count_files (void)
{
	struct st_data_filenode *tmp;
	int i=0;

	tmp = files;

	while (tmp)
	{
		tmp = tmp->next;
		i++;
	}

	return i;
}

/* Turns a 16-byte array (from the start of a file) into
 * a string (eg. 1;23;a3;d2;...) so that it can be easily
 * stored or compared */
static void make_text_fingerprint (unsigned char fp[16], unsigned char text_fp[16 * 3 + 1])
{
	char temp_ptr[4];
	int i;
	
	text_fp[0]=0;

	for (i=0;i<16;i++)
	{
		sprintf (temp_ptr, "%x;", fp[i]);
		strcat (text_fp, temp_ptr);
	}
}

/* Compares the currently active file to all known
 * encyclopediae. It returns the file node's number
 * if it is recognised, else 254 or 255.
 * (unrecognised or unopenable respectively.) */
int st_fingerprint (void)
{
	int i = 0;
	FILE *inp;
	struct st_data_filenode *filenode=NULL;
	unsigned char input_fp[16];
	unsigned char text_fp[16 * 3 + 1]="";
	char *match_fp=NULL;

	inp = (FILE *) curr_open (0);

	if (inp)
	{
		fread (input_fp,1,16,inp);
		fclose (inp);

		make_text_fingerprint (input_fp, text_fp);
		for (i=0;i<count_files();i++)
		{
			filenode = get_filenode(i);
			match_fp = filenode->fingerprint;
			if (!match_fp)
			{
				fprintf (stderr, "No fingerprint for file #%d ('%s')\n", i, filenode->name ? filenode->name : "Unnamed");
				continue;
			}
			if (!strcmp(match_fp, text_fp))
				return (i);
		}
	} else {
		return (255);
	}
	return 254;
}

/* Gets the 'nice' name from a data node.
 * e.g. "Encyclopedia 3.0". */
char *get_name_of_file (int file_type)
{
	return (get_filenode (file_type)->name);
}

/* Gets specific bits of info from a data node. */
const char *st_fileinfo_get_data (int file, st_filename_type type)
{
	struct st_data_filenode *node=NULL;

	node = get_filenode (file);

	if (!node)
		return NULL;

	switch (type)
	{
		case mainfilename:
			return node->mainfile;
		case data_dir:
			return node->datadir;
		case picture_dir:
			return node->photodir;
		case video_dir:
			return node->videodir;
		case append_char:
			return (node->append_char ? "yes" : NULL);
		default:
			return NULL;
	}
}

/* Gets a 'part' based on its type, section, and number in the list.
 * (The first one matching type & section is '0', the next that matches
 * is '1', etc.) */
struct st_part *get_part (int file, int type, int section, int number, int options)
{
	FILE *inp;
	struct st_data_filenode *file_node=NULL;
	struct st_part *part=NULL, *tmp=NULL;
	int i=0;

	file_node = get_filenode (file);

	if (!file_node)
		return NULL;

	part = file_node->parts;

	while (part)
	{
		if ((part->type == type) && ((part->section == section) || (section == -1)))
		{
			if (i == number)
				return part;
			i++;
		}

		/* ST_BLOCK_SCAN is a pseudo-block entry. It triggers off
		 * a call to scan_file() so that less info needs to be
		 * kept in the rcfile. */
		if (part->type == ST_BLOCK_SCAN)
		{
			inp = (FILE *) curr_open (0);
			tmp = scan_file (inp);
			fclose (inp);
			/* NB. we don't do 
				tmp->next = part->next;
			   because we don't want to have to screw
			   around w/ the returned list,
			   so <needscan/> should be the last
			   parts tag for that file
			   (not that you need others when using it
			   :-) */
			part->next = tmp;
			part->type = ST_BLOCK_SCANNED;
		}

		if (part)
			part = part->next;
	}

	return NULL;
}

/* Similar to get_part(), but does it by block number.
 * (Usually 500+). This is for getting the part an entry
 * is in. */
struct st_part *get_part_by_id (int file, int block_id)
{
	struct st_data_filenode *file_node=NULL;
	struct st_part *part=NULL;

	file_node = get_filenode (file);

	if (!file_node)
		return NULL;

	part = file_node->parts;

	while (part)
	{
		if (part->start_id == block_id)
			return part;

		part = part->next;
	}

	return NULL;
}

/* Similar to get_part(), but does it by block name.
 * (eg. "LU_A_ENCY"). This is for getting the part a thumbnail
 * is in. */
struct st_part *get_part_by_name (int file, char *name)
{
        struct st_data_filenode *file_node=NULL;
        struct st_part *part=NULL;

	if (!name)
		return NULL;

        file_node = get_filenode (file);

        if (!file_node)
                return NULL;

        part = file_node->parts;

        while (part)
        {
                if (part->name)
			if (!strcasecmp (part->name, name))
	                        return part;
                part = part->next;
        }

        return NULL;
}

/* Looks up an exception in a given file. */
char *get_exception (int file, char *type, char *from)
{
	struct st_data_exception *ex=NULL;
	struct st_data_filenode *filenode=NULL;

	filenode = get_filenode (file);

	if (!filenode)
		return NULL;

	ex = filenode->exceptions;

	while (ex)
	{
		if ((!strcmp (type, ex->type)) && (!strcmp (from, ex->from)))
			return ex->to;
		ex = ex->next;
	}

	return NULL;
}

/* A quick & safe way to make new filenodes. */
struct st_data_filenode *st_data_new_filenode (void)
{
	struct st_data_filenode *new_node=NULL;

	new_node = (struct st_data_filenode *) malloc (sizeof (struct st_data_filenode));
	if (new_node)
	{
		new_node->name = NULL;
		new_node->mainfile = NULL;
		new_node->datadir = NULL;
		new_node->photodir = NULL;
		new_node->videodir = NULL;
		new_node->fingerprint = NULL;
		new_node->append_char = 1;
		new_node->parts = NULL;
		new_node->exceptions = NULL;
		new_node->next = NULL;
	}

	return new_node;
}

/* Appends a new filenode to the end of the list of files
 * that we know about. */
void st_data_append_filenode (struct st_data_filenode *new_file)
{
	struct st_data_filenode *tmp=files;

	if (!tmp)
		files = new_file;
	else
	{
		while (tmp->next)
			tmp = tmp->next;
		tmp->next = new_file;
	}
}
