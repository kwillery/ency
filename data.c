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

static char *strdup_if_valid (char *t)
{
	if (t)
		return strdup (t);
	else
		return NULL;
}

struct st_data_filenode *files=NULL;

static struct st_data_filenode *get_filenode (int file_type)
{
	struct st_data_filenode *tmp;
	int i;

	if (file_type > count_files())
		return NULL;

	tmp = files;

	for (i=0;i<file_type;i++)
		tmp = tmp->next;

	return tmp;
}

void free_part (struct st_part *part)
{
	if (part->name)
		free (part->name);
	if (part->dir)
		free (part->dir);
	free (part);
}

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
	}

	return part;
}

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

char *get_name_of_file (int file_type)
{
	return (get_filenode (file_type)->name);
}

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

		if (part->type == ST_BLOCK_SCAN)
		{
			inp = (FILE *) curr_open (0);
			tmp = scan_file (inp);
			fclose (inp);
			/* NB. we don't do 
				tmp->next = part->next;
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
