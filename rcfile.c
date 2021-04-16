/*
 * ency2 rcfile.c file
 * the rc file loader is in here
 * (C) 2001 Robert Mibus <mibus@bigpond.com>
 *
 * licensed under the GNU GPL 2.0
 */

#include <stdlib.h>
#include <string.h>
#include "encyfuncs.h"
#include "rcfile.h"
#include "data.h"

/* Try and locate an rc file to open.
 * If we can, open it and return the file
 * handle */
static FILE *open_rc_file (char *filename)
{
	FILE *file=NULL;
	char *temp=NULL;
	char *home=NULL;
	char *fn=NULL;
	int i;

	char *home_locations[] =
	{
		"/.ency/encyfiles.rc",
		"/.encyfiles.rc",
		NULL
	};

	char *locations[] = 
	{
		"encyfiles.rc",
		"/etc/encyfiles.rc",
		"/usr/local/etc/encyfiles.rc",
		"/usr/share/ency/encyfiles.rc",
		"/usr/local/share/ency/encyfiles.rc",
		NULL
	};

	if (filename)
	{
		if ((file = fopen (filename, "r")))
			return file;
		else
			return 0;
	}

	if ((fn = getenv ("ENCY_RC_FILENAME")))
	{
		if ((file = fopen (fn, "r")))
			return file;
	}

	home = getenv ("HOME");
	if (home)
		for (i=0;home_locations[i];i++)
		{
			fn = home_locations[i];
			temp = malloc (strlen (fn) + strlen (home) + 1);
			strcpy (temp, home);
			strcat (temp, fn);
			if ((file = fopen (temp, "r")))
			{
				free (temp);
				return file;
			}
			free (temp);
		}

	for (i=0;locations[i];i++)
	{
		if ((file = fopen (locations[i], "r")))
			return file;
	}

	return NULL;
}

/* A sort-of do-all get-me-some-text from this line function.
 * N.B. it takes the pointer to the pointer to the start of what you
 * want to read. */
static char *rcfile_get_text (char **line, char stop_at, int stop_at_whitespace)
{
	char *l = *line;
	char *m;
	int wait_until_quote;

	/* swallow leading whitespace */
	while (*l == ' ' || *l == '\t')
		l++;

	if (*l == '\"')
	{
		wait_until_quote = 1;
		l++;
	}
	else 
		wait_until_quote = 0;

	m = l;

	while (*l)
	{
		if (*l == 0)
			break;

		if (wait_until_quote)
		{
			if (*l == '\"')
			{
				*l++ = 0;
				break;
			}
		} else {
			if (*l == stop_at)
				break;
			if (stop_at_whitespace && (*l == ' ' || *l == '\t'))
				break;
		}

		l++;
	}
	if (*l)
		*l++ = 0;

	*line = l;

	return strdup (m);
}

/* Create an argument list out a line in the rc file. */
static struct rcfile_args *new_rcfile_args (char *line)
{
	struct rcfile_args *arg_root=NULL;
	struct rcfile_args *arg_last=NULL;
	struct rcfile_args *arg_curr=NULL;
	char *l;

	l = line;

	while (*l)
	{
		arg_curr = (struct rcfile_args *) malloc (sizeof (struct rcfile_args));
		arg_curr->name = NULL;
		arg_curr->value = NULL;
		arg_curr->next = NULL;

		/* read the name */
		arg_curr->name = rcfile_get_text (&l, '=', 1);

		/* read the value */
		arg_curr->value = rcfile_get_text (&l, ',', 1);

		/* linked list stuff */
		if (arg_last)
			arg_last->next = arg_curr;
		else
			arg_root = arg_curr;

		arg_last = arg_curr;
	}
	return arg_root;
}

/* Reads the command part of a line, and calls a function to get
 * each argument. */
static struct rcfile_cmd *new_rcfile_cmd (char *line)
{
	struct rcfile_cmd *cmd=NULL;
	char *l, *n;

	cmd = (struct rcfile_cmd *) malloc (sizeof (struct rcfile_cmd));

	if (!cmd)
		return NULL;

	cmd->name = NULL;
	cmd->args = NULL;

	l = line;

	/* swallow leading whitespace */
	while (*l == ' ' || *l == '\t')
		l++;

	/* remember where the cmd name starts */
	n = l;

	/* skip past the command name */
	while (*l && *l != ':' && *l != '\n')
		l++;

	/* get the args if it has them */
	if (*l == ':')
	{
		*l++ = 0;
		cmd->args = new_rcfile_args (l);
	}

	/* copy the cmd name */
	/* note we dont do it earlier 'cos it
	   might not be NULL terminated */
	cmd->name = strdup (n);

	return cmd;
}

/* Load a command and it's arguments from the rc file. */
static struct rcfile_cmd *rc_file_get_cmd (FILE *inp)
{
	char line[256]="";
	char c, *t;

	t = line;
	while ((c = getc (inp)) != '\n' && t-line < 255 && !feof (inp))
		*t++ = c;
	*t = 0;

	if (*line == '#' || (!strlen(line) && !feof (inp)))
		return rc_file_get_cmd (inp);

	if (!strlen (line) && feof (inp))
		return NULL;

	return new_rcfile_cmd (line);
}

/* Look in an rc argument list for a given argument. */
static char *get_rc_arg (struct rcfile_args *arg, char *name)
{
	if (!arg)
		return NULL;

	/* if they dont specify name, we give them the 1st
	   arg->name 'cos itll be 'nameless' options.
	   (eg. "name" tag). */
	if (!name)
		return arg->name;

	while (arg)
	{
		if (arg->name)
			if (!strcmp (arg->name, name))
				return arg->value;
		arg = arg->next;
	}

	return NULL;
}

/* Makes an exception out of an rc arg list. */
static struct st_data_exception *append_exception_from_rc_file (struct st_data_exception *ex_root, struct rcfile_args *arg)
{
	struct st_data_exception *ex=NULL, *ex_last=NULL;
	char *type, *from, *to;

	ex = ex_root;
	while (ex)
	{
		ex_last = ex;
		ex = ex->next;
	}

	type = get_rc_arg (arg, "type");
	from = get_rc_arg (arg, "from");
	to = get_rc_arg (arg, "to");

	if (type == NULL || from == NULL || to == NULL)
	{
		fprintf (stderr, "Bogus exception in rc file!\n");
		fprintf (stderr, "Bombing out...\n");
		exit (1);
	}

	ex = new_exception (type, from, to);

	if (ex_last)
		ex_last->next = ex;
	else
		return ex;

	return ex_root;
}

/* Adds a videolist lookup */
void add_videolist (struct st_data_filenode *node, struct rcfile_args *args)
{
	struct st_vidlist *vl=NULL;
	char *name=NULL, *dir=NULL;

	name = get_rc_arg (args, "name");
	dir = get_rc_arg (args, "dir");

	if (!name || !dir)
		return;

	vl = malloc (sizeof(struct st_vidlist));

	if (!vl)
		return;

	vl->name = strdup(name);
	vl->dir = strdup(dir);

	/* prepend the vid list - easier than
	 * appending */
	vl->next = node->videolist;
	node->videolist = vl;
}

/* Add a data file to an encyclopedia */
void add_file (struct st_data_filenode *node, struct rcfile_args *args)
{
	struct st_dfile *df=NULL;
	int dfiletype=ST_DFILE_UNKNOWN;
	char *type=NULL, *filename=NULL;

	type = get_rc_arg (args, "type");

	if (type == NULL)
		dfiletype = ST_DFILE_UNKNOWN;
	else if (!strcasecmp (type, "ency"))
		dfiletype = ST_DFILE_ENCY;
	else if (!strcasecmp (type, "data"))
		dfiletype = ST_DFILE_DATA;
	else if (!strcasecmp (type, "picon"))
		dfiletype = ST_DFILE_PICON;

	filename = get_rc_arg (args, "filename");
	DBG ((stderr, "Have '%s' file '%s'.\n", type, filename));
	df = node->dfiles;
	if (!df)
	{
		df = node->dfiles = new_dfile();
	} else
	{
		while (df->next)
			df = df->next;
		df->next = new_dfile();
		df = df->next;
	}

	df->type = dfiletype;
	df->filename = strdup(filename);
	df->blocks = new_block();
	df->blocks->type = ST_BLOCK_SCAN;
}

/* Frees an rc argument list. */
static void free_rc_args (struct rcfile_args *arg)
{
	struct rcfile_args *old_arg;

	while (arg)
	{
		old_arg = arg;
		arg = arg->next;

		if (old_arg->name)
			free (old_arg->name);
		if (old_arg->value)
			free (old_arg->value);
		free (old_arg);
	}
}

/* Frees the rc command. */
static void free_rc_cmd (struct rcfile_cmd *cmd)
{
	free_rc_args (cmd->args);

	if (cmd->name)
		free (cmd->name);

	free (cmd);
}

/* Loop through every rc command and set up new
 * file nodes. */
static struct st_data_filenode *make_filenode_from_rc_file (FILE *inp)
{
	struct st_data_filenode *new_node=NULL;
	struct rcfile_cmd *cmd=NULL;

	while ((cmd = rc_file_get_cmd (inp)))
	{
		/* if we get to the end block, stop */
		if (new_node && !strcmp (cmd->name, "endfile"))
		{
			free_rc_cmd (cmd);
			return new_node;
		}

		/* We allocate the node here just in case there is nothing left
		   in the rcfile when we start */
		if (!new_node)
			new_node = st_data_new_filenode ();

		if (!strcasecmp (cmd->name, "name"))
			new_node->name = strdup (get_rc_arg (cmd->args, NULL));
		if (!strcasecmp (cmd->name, "dfile"))
			add_file (new_node, cmd->args);
		if (!strcasecmp (cmd->name, "datadir"))
			new_node->datadir = strdup (get_rc_arg (cmd->args, NULL));
		if (!strcasecmp (cmd->name, "photodir"))
			new_node->photodir = strdup (get_rc_arg (cmd->args, NULL));
		if (!strcasecmp (cmd->name, "videodir"))
			new_node->videodir = strdup (get_rc_arg (cmd->args, NULL));
		if (!strcasecmp (cmd->name, "videolist"))
			add_videolist (new_node, cmd->args);
		if (!strcasecmp (cmd->name, "fingerprint"))
			new_node->fingerprint = strdup (get_rc_arg (cmd->args, NULL));
		if (!strcasecmp (cmd->name, "append_char"))
			new_node->append_char = 1;
		if (!strcasecmp (cmd->name, "exception"))
			new_node->exceptions = append_exception_from_rc_file (new_node->exceptions, cmd->args);

		free_rc_cmd (cmd);
	}


	return new_node;
}

/* Load an rcfile into file nodes. */
int load_rc_file_info (char *filename)
{
	FILE *inp;
	struct st_data_filenode *node=NULL;

	inp = open_rc_file (filename);

	if (!inp)
		return -1;

	while ((node = make_filenode_from_rc_file (inp)))
		st_data_append_filenode (node);

	fclose (inp);
	return 1;
}
