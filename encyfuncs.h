#ifndef ENCYFUNCS_H
#define ENCYFUNCS_H

#include "ency.h"

FILE *curr_open (char *filename, long start);
char *get_ency_dir ();

#define free(A) {if (A) free (A); A=NULL;}
#ifdef DEBUG
#define DBG(a) fprintf a;
#else
#define DBG(a) ;
#endif

#endif
