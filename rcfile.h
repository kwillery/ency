/*
 * ency2 rcfile.h file
 * the rc file loader header is in here 
 * (C) 2001 Robert Mibus <mibus@bigpond.com>
 *
 * licensed under the GNU GPL 2.0
 */

struct rcfile_args
{
	char *name;
	char *value;
	struct rcfile_args *next;
};

struct rcfile_cmd
{
	char *name;
	struct rcfile_args *args;
};

int load_rc_file_info (char *filename);
