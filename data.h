#ifndef DATA_H
#define DATA_H

#include "ency.h"

/* internal */
#define ST_SECT_PTBL 10
#define ST_SECT_VTBL 11
#define ST_SECT_PCPT 12
#define ST_SECT_VCPT 13

#define ST_PART_OPT_EPISLIST 1
#define ST_PART_OPT_FTLIST 2
struct st_part
{
	long start;
	long count;
	int start_id;
	int bcount;
};

int load_file_info (char *filename);
int count_files (void);
void free_xml_doc (void);
int st_fingerprint (void);
char *get_name_of_file (int file_type);
const char *st_fileinfo_get_data (int file, st_filename_type type);
struct st_part *get_part (int file, int section, int number, int options);
#endif
