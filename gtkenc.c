#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gtkenc.h"
#include "ency.h"

void on_hdr_list_select_child(GtkWidget *test, gint row, gint column )
{
  gtk_text_set_point(GTK_TEXT (results), 0);
  gtk_text_forward_delete(GTK_TEXT (results), gtk_text_get_length(
			  GTK_TEXT (results)));
  gtk_text_insert (GTK_TEXT (results), NULL, NULL, NULL,
		  rtnresults[row], -1);

}

void CB_exit(GtkWidget *widget, GtkWidget *event)
{
  exit(0);
}

void copy1 (char *s1, const char *s2){
  int i;
  for (i=0; s1[i] = s2[i]; i++);
}

void search_cxt()
{
  //char *search_string;
  struct ency_titles *thingy;
  struct ency_titles *kill_me;
  struct st_ency_formatting *fmt1, *fmt2;
  gtk_clist_clear(GTK_CLIST (hdr_list));
  gtk_text_set_point(GTK_TEXT (results), 0);
  gtk_text_forward_delete(GTK_TEXT (results), gtk_text_get_length(
			  GTK_TEXT (results)));

//  printf ("Enter search string :");
//  scanf ("%s", search_string);
//  scanf ("%[a-zA-Z0-9.\"\'() -]", search_string);
  search_string = gtk_entry_get_text(GTK_ENTRY (entry1)); 
  thingy = ency_find_titles (search_string);
  if ((thingy != NULL) && (thingy->title != NULL))
    {
      do
	{
{
fmt1=thingy->fmt;
while (fmt1 != NULL)
 {
 fmt2=fmt1;
 fmt1=fmt2->next;
 free(fmt2);
 }
}
          //sprintf(rtnresults, "\n%s\n\n%s\n", thingy->title, thingy->text);
	  //gtk_text_insert (GTK_TEXT (results), NULL, NULL, NULL,
	//		  rtnresults, -1);
	  rtntitle[0] = thingy->title;
	  sprintf(rtnresults[counter], "%s", thingy->text);
	  crap = gtk_clist_append (GTK_CLIST (hdr_list), rtntitle);
	  kill_me = thingy;
	  thingy = thingy->next;
          free(kill_me->text);
	  free(kill_me);
	  counter++;
	}
      while (thingy != NULL);
    }
  else
     gtk_text_insert (GTK_TEXT (results), NULL, NULL, NULL,
			  "No matches\n", -1);
}

void make_gui()
{
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_usize (GTK_WIDGET (window), 500, 200);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		  GTK_SIGNAL_FUNC(CB_exit), &window);
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		  GTK_SIGNAL_FUNC(CB_exit), &window);
  gtk_window_set_title (GTK_WINDOW (window), "Startrek Ency");
  gtk_container_border_width (GTK_CONTAINER (window), 10);
  box0 = gtk_vbox_new(FALSE, 0);
  box1 = gtk_hbox_new(FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), box0);
  gtk_container_add (GTK_CONTAINER (box0), box1);
  label1text = "Enter Search String:";
  label1 = gtk_label_new(label1text);
  gtk_container_add (GTK_CONTAINER (box1), label1);
  entry1 = gtk_entry_new_with_max_length(49);
  gtk_container_add (GTK_CONTAINER (box1), entry1);
  button1 = gtk_button_new_with_label("Submit");
  gtk_signal_connect (GTK_OBJECT (button1), "clicked",
		  GTK_SIGNAL_FUNC (search_cxt), (gpointer) "button");
  gtk_container_add (GTK_CONTAINER (box1), button1);

  hdr_list_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (hdr_list_scrolled_window),
		  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (box0), hdr_list_scrolled_window);

  hdr_list = gtk_clist_new(1);
  gtk_clist_set_column_width (GTK_CLIST (hdr_list), 0, 180);
  gtk_container_add (GTK_CONTAINER (hdr_list_scrolled_window), hdr_list);
  gtk_signal_connect (GTK_OBJECT (hdr_list), "select_row",
                      GTK_SIGNAL_FUNC (on_hdr_list_select_child),
                      (gpointer) hdr_list);

  
  results_table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacing (GTK_TABLE (results_table), 0, 2);
  gtk_table_set_col_spacing (GTK_TABLE (results_table), 0, 2);
  gtk_box_pack_start (GTK_BOX (box0), results_table, TRUE, TRUE, 0);
  gtk_widget_show (results_table);

  results = gtk_text_new(NULL,NULL);
  gtk_text_set_word_wrap(GTK_TEXT (results), TRUE);
  gtk_table_attach_defaults (GTK_TABLE (results_table), results, 0, 1, 0, 1);
  
  results_vscroll = gtk_vscrollbar_new (GTK_TEXT (results)->vadj);
  gtk_table_attach (GTK_TABLE (results_table), results_vscroll, 1, 2, 0, 1,
		  GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  //gtk_container_add (GTK_CONTAINER (box0), results);
   
  gtk_widget_show (box0);
  gtk_widget_show (box1);
  gtk_widget_show (label1);
  gtk_widget_show (entry1);
  gtk_widget_show (button1);
  gtk_widget_show (results);
  gtk_widget_show (results_vscroll);
  gtk_widget_show (hdr_list_scrolled_window);
  gtk_widget_show (hdr_list);
  gtk_widget_show (window);
}

/*
void make_menu()
{
  file_menu = gtk_menu_new();
  open_item = gtk_menu_item_new_with_label("Open");
  gtk_menu_append( GTK_MENU(file_menu), open_item);
  gtk_widget_show( open_item);
  menu_bar = gtk_menu_bar_new();
  gtk_container_add (GTK_CONTAINER (window), menu_bar);
  gtk_widget_show (menu_bar);
  file_item = gtk_menu_item_new_with_label("File");
  gtk_widget_show(file_item);
  gtk_menu_item_set_submenu( GTK_MENU_ITEM(menu_item), GTK_WIDGET(submenu));
  gtk_menu_item_set_submenu( GTK_MENU_ITEM(file_item), file_menu );
  gtk_menu_bar_append( GTK_MENU_BAR(menu_bar), GTK_WIDGET(menu_item));
  gtk_menu_bar_append (GTK_MENU_BAR(menu_bar), file_item);
  gtk_menu_item_right_justify( GTK_MENU_ITEM(menu_item)); 
}
*/

void main(int argc, char *argv[])
{
  gtk_init(&argc, &argv);
  counter=0;
  make_gui();
//  make_menu();
  gtk_main();
}






