#include <stdlib.h>
#include <gtk/gtk.h>
#include "data.h"
#include "scan.h"

gchar *titles[]=
{
	"Name",
	"BType",
	"Type",
	"Section",
	"Start ID",
	"Start",
	"Size"
};

char *fn;

GtkWidget *win;
GtkWidget *lst;
GtkWidget *txt;

void load (char *fn)
{
	FILE *inp;
	struct st_block *b;
	int i=0;

	gtk_clist_freeze (GTK_CLIST(lst));
	gtk_clist_clear (GTK_CLIST(lst));

	inp = fopen (fn, "r b");

	b = scan_file (inp);
	while (b)
	{
		char *temp[7];

		temp[0] = b->name;
		temp[1] = (char*) b->btype;
		temp[2] = g_strdup_printf ("%s",
					   b->type == ST_BLOCK_DATA ? "DATA" :
					   b->type == ST_BLOCK_ATTRIB ? "Attrib" :
					   b->type == ST_BLOCK_FTLIST ? "FTList" :
					   b->type == ST_BLOCK_STOL ? "SToL" :
					   b->type == ST_BLOCK_LTOS ? "LToS" :
					   b->type == ST_SECT_PCPT ? "PCpt" :
					   b->type == ST_SECT_VCPT ? "VCpt" :
					   b->type == ST_SECT_VLST ? "VLst" :
					   b->type == ST_BLOCK_FLASHEXCEPT ? "FlshExcpt" :
					   "Unknown");

		temp[3] = g_strdup_printf ("%s",
					   b->section == ST_SECT_ENCY ? "ENCY" :
					   b->section == ST_SECT_EPIS ? "EPIS" :
					   b->section == ST_SECT_CHRO ? "CHRO" :
					   "???");
		temp[4] = g_strdup_printf ("%d", b->start_id);
		temp[5] = g_strdup_printf ("%ld", b->start);
		temp[6] = g_strdup_printf ("%ld", b->size);

		gtk_clist_append (GTK_CLIST(lst), temp);
		gtk_clist_set_row_data (GTK_CLIST(lst), i++, (gpointer)(b));

		b = b->next;
	}

	fclose (inp);
	gtk_clist_thaw (GTK_CLIST(lst));
}

int clist_select_row (GtkCList *lst, int row, int col, GdkEventButton event, gpointer data)
{
	struct st_block *b;
	FILE *inp;
	char *d;
	int i;

	gtk_text_freeze (GTK_TEXT(txt));

	b = (struct st_block *)gtk_clist_get_row_data (lst, row);

	/* read the block */
	inp = fopen (fn, "r b");
	fseek (inp, b->start, SEEK_SET);
	d = (char *) malloc (b->size);
	fread (d, 1, b->size, inp);

	/* clean it up */
	for (i=0;i<b->size;i++)
	{
		d[i] = st_cleantext (d[i]);
	}

	/* insert it */
	gtk_text_set_point (GTK_TEXT(txt), 0);
	gtk_text_forward_delete (GTK_TEXT(txt), gtk_text_get_length (GTK_TEXT(txt)));
	gtk_text_insert (GTK_TEXT(txt), NULL, NULL, NULL, d, b->size);

	free (d);
	fclose (inp);

	gtk_text_thaw (GTK_TEXT(txt));
	return TRUE;
}

int win_resize (GtkWidget *win, gpointer data)
{
	gtk_clist_columns_autosize (GTK_CLIST(lst));
	return TRUE;
}

void create_interface ()
{
	GtkWidget *sw;
	GtkWidget *pane;

	win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_usize (win, 300, 400);
	gtk_signal_connect(GTK_OBJECT(win), "destroy", gtk_main_quit, NULL);
	gtk_signal_connect(GTK_OBJECT(win), "check_resize", (GtkSignalFunc)win_resize, NULL);
	pane = gtk_vpaned_new ();

	/* Top pane */
	sw = gtk_scrolled_window_new (NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	lst = gtk_clist_new_with_titles (7,titles);
	gtk_signal_connect(GTK_OBJECT(lst), "select-row", (GtkSignalFunc)clist_select_row, NULL);
	gtk_container_add (GTK_CONTAINER(sw), lst);
	gtk_paned_add1 (GTK_PANED(pane), sw);

	/* Bottom pane */
	sw = gtk_scrolled_window_new (NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	txt = gtk_text_new (NULL,NULL);
	gtk_text_set_line_wrap (GTK_TEXT(txt), TRUE);
	gtk_container_add (GTK_CONTAINER(sw), txt);
	gtk_paned_add2 (GTK_PANED(pane), sw);

	gtk_container_add (GTK_CONTAINER(win), pane);
	gtk_widget_show_all (win);
}

int main(int argc, char *argv[])
{
	gtk_init (&argc, &argv);

	if (argc < 2)
	{
		fprintf (stderr, "Usage: gtkscan [filename]\n");
		exit(1);
	}

	create_interface ();

	fn = argv[1];

	load (fn);

	gtk_main();

	return 0;
}
