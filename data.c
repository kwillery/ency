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

#include <gnome-xml/parser.h>
#include <gnome-xml/tree.h>

#include "ency.h"
#include "encyfuncs.h"
#include "data.h"
#include "scan.h"

static char *strdup_if_valid (char *t)
{
	if (t)
		return strdup (t);
	else
		return NULL;
}

struct st_data_filenode *files=NULL;

struct st_data_filenode *get_filenode (int file_type)
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

void st_data_clear (void)
{
}

static xmlDocPtr open_xml_file (char *filename)
{
	xmlDocPtr xmlroot;
	char *temp=NULL;
	char *home=NULL;
	char *fn=NULL;
	int i;

	char *home_locations[] =
	{
		"/.ency/encyfiles.xml",
		"/.encyfiles.xml",
		NULL
	};

	char *locations[] = 
	{
		"encyfiles.xml",
		"/etc/encyfiles.xml",
		"/usr/local/etc/encyfiles.xml",
		"/usr/local/etc/encyfiles.xml",
		"/usr/share/ency/encyfiles.xml",
		"/usr/lib/ency/encyfiles.xml",
		"/usr/local/share/ency/encyfiles.xml",
		"/usr/local/lib/ency/encyfiles.xml",
		NULL
	};

	if (filename)
	{
		if ((xmlroot = xmlParseFile (filename)))
			return xmlroot;
		else
			return 0;
	}

	if ((fn = getenv ("ENCY_XML_FILENAME")))
	{
		if ((xmlroot = xmlParseFile (fn)))
			return xmlroot;
	}

	home = getenv ("HOME");
	if (home)
		for (i=0;home_locations[i];i++)
		{
			fn = home_locations[i];
			temp = malloc (strlen (fn) + strlen (home) + 1);
			strcpy (temp, home);
			strcat (temp, fn);
			if ((xmlroot = xmlParseFile (temp)))
			{
				free (temp);
				return xmlroot;
			}
			free (temp);
		}

	for (i=0;locations[i];i++)
	{
		if ((xmlroot = xmlParseFile (locations[i])))
			return xmlroot;
	}

	return NULL;
}

struct st_part *new_part()
{
	struct st_part *part=NULL;
	part = (struct st_part *) malloc (sizeof (struct st_part));

	if (part)
	{
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

struct st_part *new_part_from_xmlnode (xmlNode *node)
{
	struct st_part *part=NULL;
	char *start=NULL, *start_id=NULL, *bcount=NULL;
	char *section=NULL, *count=NULL;

	start = xmlGetProp (node, "start");

	if (!start)
		return NULL;

	part = new_part ();
	if (!part)
	{
		free (start);
		return NULL;
	}

	if (!strcmp (node->name, "list"))
		part->type = ST_BLOCK_ATTRIB;
	else if (!strcmp (node->name, "ftlist"))
		part->type = ST_BLOCK_FTLIST;
	else if (!strcmp (node->name, "ptable"))
		part->type = ST_SECT_PTBL;
	else if (!strcmp (node->name, "pcaption"))
		part->type = ST_SECT_PCPT;
	else if (!strcmp (node->name, "vtable"))
		part->type = ST_SECT_VTBL;
	else if (!strcmp (node->name, "vcaption"))
		part->type = ST_SECT_VCPT;
	else if (!strcmp (node->name, "videolist"))
		part->type = ST_SECT_VLST;

	count = xmlGetProp (node, "count");
	start_id = xmlGetProp (node, "start_id");
	bcount = xmlGetProp (node, "bcount");
	section = xmlGetProp (node, "sect");

	if (strncmp (start, "0x", 2) == 0)
		sscanf (start, "%lx", &(part->start));
	else
		sscanf (start, "%ld", &(part->start));
	if (count)
		sscanf (count, "%ld", &(part->count));
	else
		part->count = 1;

	if (start_id)
		sscanf (start_id, "%d", &(part->start_id));
	else
		part->start_id = 0;

	if (bcount)
		sscanf (bcount, "%d", &(part->bcount));
	else
		part->bcount = 1;

	if (section)
		sscanf (section, "%d", &(part->section));

	part->dir = xmlGetProp (node, "dir");

	free (start);
	if (count)
		free (count);
	if (start_id)
		free (start_id);
	if (bcount)
		free (bcount);
	if (section)
		free (section);

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

struct st_data_exception *new_exception_from_xmlnode (xmlNode *node)
{
	char *type, *from, *to;
	struct st_data_exception *ex;

	type = xmlGetProp (node, "type");
	from = xmlGetProp (node, "from");
	to = xmlGetProp (node, "to");

	ex = new_exception (type, from, to);

	free (type);
	free (from);
	free (to);

	return ex;
}

int load_xmlfile_info (char *filename)
{
	xmlDocPtr xmlroot;
	xmlNode *fnode=NULL;
	xmlNode *dnode=NULL;
	xmlNode *snode=NULL;
	struct st_data_filenode *new_node=NULL;
	struct st_part *root_parts=NULL, *curr_parts=NULL, *last_parts=NULL;
	struct st_data_exception *root_ex=NULL, *curr_ex=NULL, *last_ex=NULL;
	int section=0;

	xmlroot = open_xml_file (filename);

	if (xmlroot)
	{
		fnode = xmlroot->root;

		if (!fnode)
			return 0;

		fnode = fnode->childs;

		if (!fnode)
			return 0;

		while (fnode)
		{
			dnode = fnode->childs;
			new_node = st_data_new_filenode ();

			if (!new_node)
				return 0;

			new_node->append_char = 0;
			while (dnode)
			{
				if (!strcmp (dnode->name, "name"))
					new_node->name = strdup_if_valid (xmlNodeGetContent (dnode));
				else if (!strcmp (dnode->name, "mainfile"))
					new_node->mainfile = strdup_if_valid (xmlNodeGetContent (dnode));
				else if (!strcmp (dnode->name, "datadir"))
					new_node->datadir = strdup_if_valid (xmlNodeGetContent (dnode));
				else if (!strcmp (dnode->name, "photodir"))
					new_node->photodir = strdup_if_valid (xmlNodeGetContent (dnode));
				else if (!strcmp (dnode->name, "videodir"))
					new_node->videodir = strdup_if_valid (xmlNodeGetContent (dnode));
				else if (!strcmp (dnode->name, "videolist") || !strcmp (dnode->name, "ptable") || !strcmp (dnode->name, "pcaption") || !strcmp (dnode->name, "vtable") || !strcmp (dnode->name, "vcaption"))
				{
					curr_parts = new_part_from_xmlnode (dnode);
					if (last_parts)
						last_parts->next = curr_parts;
					else
						last_parts = root_parts = curr_parts;
					last_parts = curr_parts;
				}
				else if (!strcmp (dnode->name, "fingerprint"))
					new_node->fingerprint = strdup_if_valid (xmlNodeGetContent (dnode));
				else if (!strcmp (dnode->name, "append_char"))
					new_node->append_char = 1;
				else if (!strcmp (dnode->name, "section"))
				{
					snode = dnode->childs;
					while (snode)
					{
						curr_parts = new_part_from_xmlnode (snode);
						if (!curr_parts)
							break;
						if (last_parts)
							last_parts->next = curr_parts;
						else
							root_parts = curr_parts;
						last_parts = curr_parts;
						curr_parts->section = section;
						snode = snode->next;
					}
					section++;
				}
				else if (!strcmp (dnode->name, "needscan"))
				{
					curr_parts = new_part ();
					curr_parts->type = ST_BLOCK_SCAN;
					if (last_parts)
						last_parts->next = curr_parts;
					else
						root_parts = curr_parts;
					last_parts = curr_parts;
				}
				else if (!strcmp (dnode->name, "exception"))
				{
					curr_ex = new_exception_from_xmlnode (dnode);
					if (last_ex)
						last_ex->next = curr_ex;
					else
						root_ex = curr_ex;
					last_ex = curr_ex;
				}
				dnode = dnode->next;
			}
			new_node->parts = root_parts;
			new_node->exceptions = root_ex;
			st_data_append_filenode (new_node);
			root_parts = last_parts = curr_parts = NULL;
			root_ex = last_ex = curr_ex = NULL;
			fnode = fnode->next;
		}

		return 1;
	}	

	return 0;
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

void make_text_fingerprint (unsigned char fp[16], unsigned char text_fp[16 * 3 + 1])
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
