#ifndef DATA_H
#define DATA_H

#include "ency.h"

/* internal */
#define ST_BLOCK_TEXT 0
#define ST_BLOCK_ATTRIB 1
#define ST_BLOCK_FTLIST 2
#define ST_BLOCK_FLASHEXCEPT 3
#define ST_BLOCK_SCAN 254 /* If we should scan this file */
#define ST_BLOCK_SCANNED 255 /* If we have scanned this file */

#define ST_SECT_PTBL 10
#define ST_SECT_VTBL 11
#define ST_SECT_PCPT 12
#define ST_SECT_VCPT 13
#define ST_SECT_VLST 14
#define ST_SECT_BLK  15

struct st_part
{
	char *name;
	char btype[5];
	int type;
	int section;
	long start;
	long count;
	int start_id;
	int bcount;
	long size;
	char *dir;
	struct st_part *next;
};

struct st_data_exception
{
	char *from;
	char *type;
	char *to;
	struct st_data_exception *next;
};

struct st_data_filenode
{
	char *name;
	char *mainfile;
	char *datadir;
	char *photodir;
	char *videodir;
	char *fingerprint;
	int append_char;
	struct st_part *parts;
	struct st_data_exception *exceptions;

	struct st_data_filenode *next;
};

int load_rc_file_info (char *filename);
void st_data_clear (void);
int count_files (void);
int st_fingerprint (void);

struct st_data_filenode *st_data_new_filenode (void);
void free_data_filenode (struct st_data_filenode *file);
void st_data_append_filenode (struct st_data_filenode *new_file);

char *get_name_of_file (int file_type);
const char *st_fileinfo_get_data (int file, st_filename_type type);
struct st_part *get_part (int file, int type, int section, int number, int options);
struct st_part *get_part_by_id (int file, int block_id);
struct st_part *get_part_by_name (int file, char *name);
char *get_exception (int file, char *type, char *from);
void free_exception (struct st_data_exception *ex);

struct st_part *new_part(void);
struct st_data_exception *new_exception(char *type, char *from, char *to);
void free_part (struct st_part *part);

#endif

