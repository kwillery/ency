#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk_imlib.h>
#include "data.h"
#include "scan.h"
#include "pictures.h"

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

enum
{
	DISPLAY_TYPE_TEXT,
	DISPLAY_TYPE_HEX,
	DISPLAY_TYPE_IMAGE
} display_type;

char *fn;

struct st_block *current_block;
char *current_data;

GtkWidget *win;
GtkWidget *lst;
GtkWidget *vb;
GtkWidget *omenu;


/*********
	  Data displays
**********/
/** Text **/
void display_data_text ()
{
	GtkWidget *sw, *txt;
	int i;
	struct st_block *b;
	char *d;
	b = current_block;
	d = current_data;

	sw = gtk_scrolled_window_new (NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	txt = gtk_text_new (NULL,NULL);
	gtk_text_set_line_wrap (GTK_TEXT(txt), TRUE);
	gtk_container_add (GTK_CONTAINER(sw), txt);
	gtk_box_pack_start_defaults (GTK_BOX(vb), sw);

	/* clean it up */
	for (i=0;i<b->size;i++)
	{
		d[i] = st_cleantext (d[i]);
	}

	/* insert it */
	gtk_text_insert (GTK_TEXT(txt), NULL, NULL, NULL, d, b->size);

	gtk_widget_show_all (vb);
}

/** Hex **/
void display_data_hex ()
{
	GtkWidget *sw, *txt;
	int i;
	struct st_block *b;
	char *d;
	b = current_block;
	d = current_data;

	sw = gtk_scrolled_window_new (NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	txt = gtk_text_new (NULL,NULL);
	gtk_text_set_line_wrap (GTK_TEXT(txt), TRUE);
	gtk_container_add (GTK_CONTAINER(sw), txt);
	gtk_box_pack_start_defaults (GTK_BOX(vb), sw);

	for (i=0;i<b->size;i++)
	{
		char temp[16];
		sprintf(temp, "%2x ", d[i]);
		gtk_text_insert (GTK_TEXT(txt), NULL, NULL, NULL, temp, -1);
		if (i % 16 == 15)
			gtk_text_insert (GTK_TEXT(txt), NULL,  NULL, NULL, "\n", 1);
	}

	gtk_widget_show_all (vb);
}

/** Image **/
void redisplay_image (int w, int h, int autodc)
{
#define TEMP_FN "/tmp/gtkscan_XXXXXX"
	GtkWidget *hbox, *img=NULL;
	FILE *inp;
	GdkPixmap *pm;
	GdkBitmap *bm;
	char file[] = TEMP_FN;
	int z;
	struct st_block *b;
	char *d;
	GList *l;
	b = current_block;
	d = current_data;

	/* Remove the pixmap */
	l = gtk_container_children (GTK_CONTAINER(vb));
	l = gtk_container_children (GTK_CONTAINER(hbox=l->data));
	gtk_container_remove (GTK_CONTAINER(hbox),GTK_WIDGET(l->data));

	/* Get a temporary file name */
	if ((z = mkstemp (file)) == -1)
		return;
	close (z);

	inp = fopen (fn, "r b");
	fseek (inp, b->start, SEEK_SET);
	printf ("start:%ld\n",b->start);
	create_ppm_from_image (file, inp, w, h, b->size);
	fclose (inp);

	if (gdk_imlib_load_file_to_pixmap(file, &pm, &bm))
		img = gtk_pixmap_new (pm,bm);
	else
		img = gtk_label_new ("Error");

	gtk_box_pack_start_defaults (GTK_BOX(hbox), img);

	unlink (file);

	gtk_widget_show_all (vb);
}

void btn_refresh_image (GtkWidget *btn, gpointer data)
{
	GtkWidget *dc, *size;
	int w, h, autodc;
	char *wxh;

	dc = gtk_object_get_data (GTK_OBJECT(btn), "dc");
	autodc = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(dc));

	size = gtk_object_get_data (GTK_OBJECT(btn), "size");
	wxh = gtk_entry_get_text (GTK_ENTRY(size));
	sscanf (wxh, "%dx%d", &w, &h);
	redisplay_image (w,h,autodc);
}

void count_factors (GtkWidget *lst)
{
	FILE *inp;
	long dsize;
	char *s;
	long i,j;

	inp = fopen (fn, "r b");
	fseek (inp, current_block->start, SEEK_SET);
	dsize = get_decompressed_size (inp, current_block->size);
	fclose (inp);

	j=current_block->size;
	for (i=1;i<j;i++)
		if (j % i == 0)
		{
			s = g_strdup_printf("%ldx%ld\n", i, j/i);
			gtk_clist_append (GTK_CLIST(lst), &s);
			free (s);
		}

	j=dsize;
	for (i=1;i<j;i++)
		if (j % i == 0)
		{
			s = g_strdup_printf("%ldx%ld\n", i, j/i);
			gtk_clist_append (GTK_CLIST(lst), &s);
			free (s);
		}
}

void lst_sizes_select_row (GtkCList *clist, gint row, gint column,
			   GdkEventButton *event, gpointer user_data)
{
	char *text;
	GtkWidget *size;
	gtk_clist_get_text(clist,row,column,&text);
	size = gtk_object_get_data (GTK_OBJECT(clist), "size");
	gtk_entry_set_text (GTK_ENTRY(size),text);
	btn_refresh_image(GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(clist),"btn")), NULL);
}

void display_data_image ()
{
	GtkWidget *hbox, *img=NULL;
	GtkWidget *vbox;
	GtkWidget *dc, *size, *btn;
	GtkWidget *lst, *sw;

	hbox = gtk_hbox_new (0,0);
	vbox = gtk_vbox_new (0,0);

	img = gtk_label_new ("Error!");
	gtk_box_pack_start_defaults (GTK_BOX(hbox), img);

	size = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY(size), "60x40");
	gtk_box_pack_start (GTK_BOX(vbox), size, 0, 0, 0);

	dc = gtk_check_button_new_with_label ("Auto decompress");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(dc), TRUE);
	gtk_box_pack_start (GTK_BOX(vbox), dc, 0, 0, 0);

	btn = gtk_button_new_with_label ("Refresh");
	gtk_box_pack_start (GTK_BOX(vbox), btn, 0, 0, 0);
	gtk_signal_connect (GTK_OBJECT(btn), "clicked", (GtkSignalFunc)btn_refresh_image, NULL);
	gtk_object_set_data (GTK_OBJECT(btn), "dc", dc);
	gtk_object_set_data (GTK_OBJECT(btn), "size", size);

	sw = gtk_scrolled_window_new (NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	lst = gtk_clist_new (1);
	gtk_signal_connect (GTK_OBJECT(lst), "select-row",
			    (GtkSignalFunc)lst_sizes_select_row, NULL);
	gtk_object_set_data (GTK_OBJECT(lst), "size", size);
	gtk_object_set_data (GTK_OBJECT(lst), "btn", btn);
	gtk_container_add (GTK_CONTAINER(sw), lst);
	gtk_box_pack_start (GTK_BOX(vbox), sw, 1, 1, 0);

	count_factors (lst);

	gtk_box_pack_end_defaults (GTK_BOX(hbox), vbox);
	gtk_box_pack_start_defaults (GTK_BOX(vb), hbox);
	gtk_widget_show_all (vb);

	redisplay_image (60,40,1);
}

/** Controller **/
void display_data (struct st_block *b, char *d)
{
	GList *l;
	l = gtk_container_children (GTK_CONTAINER(vb));
	while (l)
	{
		gtk_container_remove (GTK_CONTAINER(vb), GTK_WIDGET(l->data));
		l=l->next;
	}

	current_data = d;
	current_block = b;

	switch (display_type)
	{
	case DISPLAY_TYPE_TEXT:
		display_data_text ();
		break;
	case DISPLAY_TYPE_HEX:
		display_data_hex ();
		break;
	case DISPLAY_TYPE_IMAGE:
		display_data_image ();
		break;
	}
}

/*********
	  Other
**********/
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

		if (!is_cmap_loaded() && !strcmp(b->btype, "CLUT"))
		{
			fseek (inp, b->start, SEEK_SET);
			load_CLUT (inp);
		}

		b = b->next;
	}

	fclose (inp);

	gtk_clist_thaw (GTK_CLIST(lst));

	if (!is_cmap_loaded())
		printf ("Warning, no colourmap loaded\n");
}

/*********
	  GUI Callbacks
**********/
int clist_select_row (GtkCList *lst, int row, int col, GdkEventButton event, gpointer data)
{
	struct st_block *b;
	FILE *inp;
	char *d;

	b = (struct st_block *)gtk_clist_get_row_data (lst, row);

	/* read the block */
	inp = fopen (fn, "r b");
	fseek (inp, b->start, SEEK_SET);
	d = (char *) malloc (b->size);
	fread (d, 1, b->size, inp);

	display_data (b, d);

	free (d);
	fclose (inp);

	return TRUE;
}

int win_resize (GtkWidget *win, gpointer data)
{
	gtk_clist_columns_autosize (GTK_CLIST(lst));
	return TRUE;
}

void change_menu (GtkMenuItem *menuitem, gpointer name)
{
	if (!strcmp(name, "Text"))
		display_type = DISPLAY_TYPE_TEXT;
	if (!strcmp(name, "Hex"))
		display_type = DISPLAY_TYPE_HEX;
	if (!strcmp(name, "Image"))
		display_type = DISPLAY_TYPE_IMAGE;
}

/*********
	  UI Building
**********/
GtkWidget *create_item (char *name)
{
	GtkWidget *item;
	item = gtk_menu_item_new_with_label (name);
	gtk_signal_connect (GTK_OBJECT(item), "activate", (GtkSignalFunc)change_menu, (char*)strdup(name));
	return item;
}

GtkWidget *create_display_menu ()
{
	GtkWidget *menu, *om;

        menu = gtk_menu_new ();
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), create_item ("Text"));
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), create_item ("Hex"));
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), create_item ("Image"));
	om = gtk_option_menu_new ();
        gtk_option_menu_set_menu (GTK_OPTION_MENU(om), menu);

	return om;
}

void create_interface ()
{
	GtkWidget *sw, *bvb;
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
	bvb = gtk_vbox_new (0,0);
	vb = gtk_vbox_new (0,0); /* VBox used for data display */
	gtk_container_add (GTK_CONTAINER(bvb), vb);
	omenu = create_display_menu ();
	gtk_box_pack_end (GTK_BOX(bvb), omenu, 0, 0, 0); /* omenu for choosing data display */

	gtk_paned_add2 (GTK_PANED(pane), bvb);

	gtk_container_add (GTK_CONTAINER(win), pane);
	gtk_widget_show_all (win);
}

/*********
	  main()
**********/
int main(int argc, char *argv[])
{
	gtk_init (&argc, &argv);
	gdk_imlib_init ();

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
