#include <gtk/gtk.h>

char ency_cleantext (unsigned char);
GtkWidget *window, *box0, *box1, *entry1, *results, *results_table,
	*results_vscroll, *hdr_list, *hdr_list_scrolled_window, *label1, *button1;
static char *rtntitle[] = {"int","int2"};
char *rtnresults[100][1000];
int counter;
int crap;
char *label1text;
char *search_string;

//char **rtnresults;

// more of your code
//mystring = new char[numstrings][numchars];
