#ifndef DATA_H
#define DATA_H

#include "ency.h"

/* Block types */
/* The default - no guarantees on what's in it */ 
#define ST_BLOCK_DATA 0
/* ATTRIB list */
#define ST_BLOCK_ATTRIB 1
/* Full-text work lookup table */
#define ST_BLOCK_FTLIST 2
/* Lookup table, long entry name to 6 letter code */
#define ST_SECT_PTBL 3
/* Captions for photos & swfs */
#define ST_SECT_PCPT 4
/* Captions for videos */
#define ST_SECT_VCPT 5
/* A list of videos on a different CD */
#define ST_SECT_VLST 6
/* A list of swfs that have a numeric suffix (instead of the 'F') */
#define ST_BLOCK_FLASHEXCEPT 7
/* Pseudo-blocks */
#define ST_BLOCK_SCAN 254 /* If we should scan this file */
#define ST_BLOCK_SCANNED 255 /* If we have scanned this file */

/* Data file types */
#define ST_DFILE_TYPES 3
#define ST_DFILE_UNKNOWN 0
#define ST_DFILE_DATA 1
#define ST_DFILE_PICON 2

struct st_block
{
	char *name;
	char btype[5];
	int type;
	int section;
	long start;
	int start_id;
	long size;
	char *dir;
	struct st_block *next;
};

struct st_data_exception
{
	char *from;
	char *type;
	char *to;
	struct st_data_exception *next;
};

struct st_dfile
{
	int type;
	struct st_block *blocks;
	struct st_dfile *next;
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
	struct st_dfile *dfiles;
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
struct st_block *get_block (int file, int dfiletype, int type, int section, int number, int options);
struct st_block *get_block_by_id (int file, int dfiletype, int block_id);
struct st_block *get_block_by_name (int file, int dfiletype, char *name);
char *get_exception (int file, char *type, char *from);
void free_exception (struct st_data_exception *ex);

struct st_block *new_block(void);
struct st_dfile *new_dfile(void);
struct st_data_exception *new_exception(char *type, char *from, char *to);
void free_block (struct st_block *block);

#endif

