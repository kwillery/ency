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

struct st_file *fileinfo=NULL;
static xmlDocPtr xmlroot;

static xmlNode *find_xml_node (xmlNode *node, char *name)
{
	while (node)
	{
		if (!strcmp (node->name, name))
			return (node);
		node = node->next;
	}
	return NULL;
		
}

static xmlNode *get_nth_xml_node (xmlNode *node, char *name, int n)
{
	int i;
	xmlNode *last=NULL;

	for (i=0;i<n+1;i++)
	{
		node = find_xml_node (node, name);
		last = node;
		if (node)
			node = node->next;
		else
			return NULL;
	}
	return last;
}

static char *get_xml_content (xmlNode *node, char *name)
{
	node = find_xml_node (node, name);
	if (node)
		return (xmlNodeGetContent (node));
	else
		return (NULL);
}

static char *get_xml_next_content (xmlNode **node, char *name)
{
	*node = find_xml_node (*node, name);
	if (*node)
		return (xmlNodeGetContent (*node));
	else
		return (NULL);
}

static struct st_part *get_xml_next_part (xmlNode **node, char *name)
{
	struct st_part *ret;
	char *start=NULL, *count=NULL;

	*node = find_xml_node (*node, name);

	if (*node)
	{
		ret = (struct st_part *) malloc (sizeof (struct st_part));
		if (!ret)
			return (NULL);

		start = xmlGetProp (*node, "start");
		count = xmlGetProp (*node, "count");
		sscanf (start, "%ld", &(ret->start));
		sscanf (count, "%ld", &(ret->count));
		return (ret);
	} else
		return (NULL);
}

int count_files ()
{
	xmlNode *node;
	int count = 0;

	if (!xmlroot)
		return 1;

	node = find_xml_node (xmlroot->root, "files");
	node = node->childs;
	while (node)
	{
		find_xml_node (node, "file");
		if (node)
		{
			node = node->next;
			count++;
		}
	}
	return count;
}

static xmlNode *get_file_info (int number)
{
	xmlNode *node;

	node = find_xml_node (xmlroot->root, "files");
	node = node->childs;
	if (!node)
		return NULL;

	node = get_nth_xml_node (node, "file", number);

	return (node);
}

void load_file_info()
{
	xmlroot = xmlParseFile ("encyfiles.xml");
}

static char *get_text_fingerprint (xmlNode *file_data)
{
	xmlNode *node;
	node=file_data->childs;
	return (get_xml_content (node, "fingerprint"));
}

static char *get_file_name (xmlNode *file_data)
{
	xmlNode *node;
	node=file_data->childs;
	return (get_xml_content (node, "name"));
}

char *get_name_of_file (int file_type)
{
	xmlNode *node=NULL;

	node = get_file_info (file_type);
	if (node)
		return get_file_name (node);
	else
		return NULL;
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
	xmlNode *file_data;

	unsigned char input_fp[16];
	unsigned char text_fp[16 * 3 + 1]="";
	char *match_fp=NULL;

/*	inp = fopen (filename, "r b");*/
	inp = (FILE *) curr_open (0);

	if (inp) {
		fread (input_fp,1,16,inp);
		fclose (inp);

		make_text_fingerprint (input_fp, text_fp);
		for (i=0;i<count_files();i++)
		{
			file_data = get_file_info (i);
			match_fp = get_text_fingerprint (file_data);
			if (!strcmp(match_fp, text_fp))
			{
				free (match_fp);
				return (i);
			}
			free (match_fp);
		}
	} else {
		return (255);
	}
	return 254;
}

const char *st_fileinfo_get_data (int file, st_filename_type type)
{
	xmlNode *node=NULL;
	
	node = get_file_info(file);

	if (!node)
		return NULL;

	node = node->childs;
	
	if (!node)
		return NULL;

	switch (type)
	{
		case mainfilename:
			return get_xml_content (node, "mainfile");
		case data_dir:
			return get_xml_content (node, "datadir");
		case picture_dir:
			return get_xml_content (node, "photodir");
		case video_dir:
			return get_xml_content (node, "videodir");
		case append_char:
			return (find_xml_node (node, "append_char") ? "yes" : NULL);
		case prepend_year:
			return (find_xml_node (node, "prepend_year") ? "yes" : NULL);
		case append_series:
			return (find_xml_node (node, "append_series") ? "yes" : NULL);
		default:
			return NULL;
	}
}

struct st_part *get_part (int file, int section, int number)
{
	struct st_part *part=NULL;
	char *start=NULL,*count=NULL;
	xmlNode *node=NULL;
	int i;

	node = get_file_info(file);

	if (node)
		node = node->childs;
	else
		return NULL;
	if (!node)
		return NULL;

	switch (section)
	{
	case ST_SECT_ENCY:
	case ST_SECT_EPIS:
	case ST_SECT_CHRO:
		node = get_nth_xml_node (node, "section", section);

		if (!node)
			return NULL;

		node = node->childs;

		for (i=0;i<number;i++)
			if (node)
				node = node->next;
		break;
	case ST_SECT_PTBL:
		node = get_nth_xml_node (node, "ptable", number);
		break;
	case ST_SECT_VTBL:
		node = get_nth_xml_node (node, "vtable", number);
		break;
	case ST_SECT_PCPT:
		node = get_nth_xml_node (node, "pcaption", number);
		break;
	case ST_SECT_VCPT:
		node = get_nth_xml_node (node, "vcaption", number);
		break;
	default:
		return NULL;
	}

	if (!node)
		return NULL;


	start = xmlGetProp (node, "start");
	count = xmlGetProp (node, "count");
	if (!(start && count))
		return NULL;

	part = (struct st_part *) malloc (sizeof (struct st_part));

	if (strncmp (start, "0x", 2) == 0)
		sscanf (start, "%lx", &(part->start));
	else
		sscanf (start, "%ld", &(part->start));
	sscanf (count, "%ld", &(part->count));

	return part;
}
