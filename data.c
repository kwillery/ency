/*
 * ency2 data.c file
 * data-related stuff goes here 
 * (C) 2000 Robert Mibus <mibus@bigpond.com>
 *
 * licensed under the GNU GPL 2.0 or greater
 */

#include <stdio.h>
#include <stdlib.h>
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

struct st_dfile *get_dfile (int file_type, int dfile)
{
	struct st_data_filenode *node;
	struct st_dfile *df;

	node = get_filenode (file_type);

	if (!node)
		return NULL;

	df = node->dfiles;
	while (df)
	{
		if (df->type == dfile)
			break;
		df = df->next;
	}
	return df;
}

/* Frees a block struct and things it points to. */
void free_block (struct st_block *block)
{
	if (block->name)
		free (block->name);
	free (block);
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

/* Frees a data_filenode, its blocks, exceptions,
 * etc. etc. etc. */
void free_data_filenode (struct st_data_filenode *file)
{
	struct st_block *block,*tmp_block;
	struct st_data_exception *ex, *tmp_ex;
	struct st_dfile *df, *tmp_df;
	struct st_vidlist *vl, *tmp_vl;

	df = file->dfiles;
	while (df)
	{
		block = df->blocks;
		while (block)
		{
			tmp_block = block;
			block = block->next;
			free_block (tmp_block);
		}
		if (df->filename)
			free (df->filename);
		tmp_df = df;
		df = df->next;
		free (tmp_df);
	}

	ex = file->exceptions;
	while (ex)
	{
		tmp_ex = ex;
		ex = ex->next;
		free_exception (tmp_ex);
	}

	vl = file->videolist;
	while (vl)
	{
		tmp_vl = vl;
		vl = vl->next;
		if (tmp_vl->name)
			free (tmp_vl->name);
		if (tmp_vl->dir)
			free (tmp_vl->dir);
		free (tmp_vl);
	}
	if (file->name)
		free (file->name);
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

/* A quick & safe way to make a block. */
struct st_block *new_block()
{
	struct st_block *block=NULL;
	block = (struct st_block *) malloc (sizeof (struct st_block));

	if (block)
	{
		block->name = NULL;
		block->type = 0;
		block->section = 0;
		block->start = 0;
		block->start_id = 0;
		block->next = NULL;
		strcpy (block->btype, "");
	}

	return block;
}

/* A quick & safe way to make a dfile. */
struct st_dfile *new_dfile()
{
	struct st_dfile *df=NULL;
	df = (struct st_dfile *) malloc (sizeof (struct st_dfile));

	if (df)
	{
		df->type = 0;
		df->filename = NULL;
		df->next = NULL;
	}

	return df;
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

/* Compares the file to all known encyclopediae.
 * It returns the file node's number
 * if it is recognised, else ST_FILE_ERROR or ST_FILE_UNKNOWN.
 * (unopenable or unrecognised respectively.) */
int st_fingerprint (char *filename)
{
	int i = 0;
	FILE *inp;
	struct st_data_filenode *filenode=NULL;
	unsigned char input_fp[16];
	unsigned char text_fp[16 * 3 + 1]="";
	char *match_fp=NULL;

	DBG((stderr, "Beginning fingerprint on '%s'...\n", filename));

	inp = fopen (filename, "r b");

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
				DBG((stderr, "No fingerprint for file #%d ('%s')\n", i, filenode->name ? filenode->name : "Unnamed"));
				continue;
			}
			if (!strcmp(match_fp, text_fp))
			{
				DBG ((stderr, "Found match (%d)\n", i));
				return (i);
			}
		}
	} else {
		DBG((stderr, "... couldn't open file\n"));
		return ST_FILE_ERROR;
	}
	DBG((stderr, "... unknown file\n"));
	return ST_FILE_UNKNOWN;
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
	struct st_dfile *df=NULL;

	node = get_filenode (file);

	if (!node)
		return NULL;

	switch (type)
	{
		case mainfilename:
			df = get_dfile (file, ST_DFILE_DATA);
			if (!df)
				return NULL;
			return df->filename;
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

/* ST_BLOCK_SCAN is a pseudo-block entry. It triggers off
 * a call to scan_file() so that less info needs to be
 * kept in the rcfile. */
static void data_scan (char *filename, struct st_block *block)
{
	FILE *inp=NULL;
	struct st_block *tmp=NULL;

	inp = open_file (filename, 0);

	if (!inp)
		return;

	tmp = scan_file (inp);
	fclose (inp);
	/* NB. we don't do 
		tmp->next = block->next;
	   because we don't want to have to screw
	   around w/ the returned list,
	   so "needscan" should be the last
	   block tag for that file
	   (not that you need others when using it
	   :-) */
	block->next = tmp;
	block->type = ST_BLOCK_SCANNED;

	return;
}

struct st_vidlist *get_vidlist (int file, int number)
{
	struct st_data_filenode *file_node=NULL;
	struct st_vidlist *vl=NULL;
	int i;

	file_node = get_filenode (file);

	if (!file_node)
		return NULL;

	vl = file_node->videolist;
	for (i=0;i<number;i++)
	{
		if (!vl)
			return NULL;

		vl = vl->next;
	}

	return vl;
}

/* Videolist blocks are referenced by ->videolist in struct st_data_filenode,
 * this runs along that, gets the name of the block and finds that. */
struct st_block *get_videolist_block (int file, int number)
{
	struct st_vidlist *vl=NULL;

	vl = get_vidlist (file, number);

	if (vl && vl->name)
		return get_block_by_name (file, ST_DFILE_DATA, vl->name, 0);

	return NULL;
}

/* Gets the directory for a video list */
char *get_videolistblock_dir(int file, int number)
{
	struct st_vidlist *vl=NULL;

	vl = get_vidlist (file, number);
	if (vl)
		return (vl->dir);

	return NULL;
}

/* Gets a 'block' based on its type, section, and number in the list.
 * (The first one matching type & section is '0', the next that matches
 * is '1', etc.) */
struct st_block *get_block (int file, int dfiletype, int type, int section, int number, int options)
{
	struct st_dfile *df=NULL;
	struct st_block *block=NULL;
	int i=0;

	/* Video lists are handled differently ATM */
	if (type == ST_SECT_VLST)
		return get_videolist_block (file, number);

	df = get_dfile (file, dfiletype);

	if (!df)
		return NULL;

	block = df->blocks;

	while (block)
	{
		if ((block->type == type) && ((block->section == section) || (section == -1)))
		{
			if (i == number)
				return block;
			i++;
		} else if (block->type == ST_BLOCK_SCAN)
			data_scan (df->filename, block);

		block = block->next;
	}

	return NULL;
}

/* Similar to get_block(), but does it by block number.
 * (Usually 500+). This is mainly for getting the block an
 * entry is in. */
struct st_block *get_block_by_id (int file, int dfiletype, int block_id)
{
	struct st_dfile *df=NULL;
	struct st_block *block=NULL;

	df = get_dfile (file, dfiletype);

	if (!df)
		return NULL;

	block = df->blocks;

	while (block)
	{
		if (block->start_id == block_id)
			return block;
		else if (block->type == ST_BLOCK_SCAN)
			data_scan (df->filename, block);

		block = block->next;
	}

	return NULL;
}

/* Similar to get_block(), but does it by block name.
 * (eg. "LU_A_ENCY"). This is mainly for getting the
 * block a thumbnail is in. */
struct st_block *get_block_by_name (int file, int dfiletype, char *name, int options)
{
	struct st_dfile *df=NULL;
        struct st_block *block=NULL;

	if (!name)
		return NULL;

	df = get_dfile (file, dfiletype);

	if (!df)
		return NULL;

	block = df->blocks;

        while (block)
        {
                if (block->name)
		{
			if (options & ST_DATA_OPT_PREFIX)
			{
				if (!strncasecmp (block->name, name, strlen (name)))
					return block;
			} else
				if (!strcasecmp (block->name, name))
					return block;
		}

		if (block->type == ST_BLOCK_SCAN)
			data_scan (df->filename, block);

                block = block->next;
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
		new_node->datadir = NULL;
		new_node->photodir = NULL;
		new_node->videodir = NULL;
		new_node->videolist = NULL;
		new_node->fingerprint = NULL;
		new_node->append_char = 1;
		new_node->dfiles = NULL;
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
