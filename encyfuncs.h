#ifndef ENCYFUNCS_H
#define ENCYFUNCS_H

#include "ency.h"

FILE *curr_open (char *filename, long start);
FILE *open_file (char *fn, long start);

#define free(A) {if (A) free (A); A=NULL;}
#ifdef DEBUG
#define DBG(a) fprintf a;
#else
#define DBG(a) ;
#endif

#endif
