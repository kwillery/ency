/******************************************************************************/
/* Mibus's Ency 98 Reader: Reads the Star Trek Encyclopedia (1998 version)    */
/* Copyright (C) 1998 Robert Mibus                                            */
/*                                                                            */
/* This program is free software; you can redistribute it and/or              */
/* modify it under the terms of the GNU General Public License                */
/* as published by the Free Software Foundation; either version 2             */
/* of the License, or (at your option) any later version.                     */
/*                                                                            */
/* This program is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of             */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              */
/* GNU General Public License for more details.                               */
/*                                                                            */
/* You should have received a copy of the GNU General Public License          */
/* along with this program; if not, write to the Free Software                */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */
/*                                                                            */
/* Author:                                                                    */
/*      Email   mibus@hallett-cove.schools.sa.edu.au                          */
/*              beemer@picknowl.com.au                                        */
/*      webpage www.picknowl.com.au/homepages/beemer/robonly.html             */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ency.h"

void CB_exit(GtkWidget *widget, GtkWidget *event)
{
 exit(0);
}

void search_cxt()
{
  char *search_string;
  struct ency_titles *thingy;
  struct ency_titles *kill_me;
  struct st_ency_formatting *fmt1, *fmt2;
  gint ret_val1;

//  printf ("Enter search string :");
//  scanf ("%s", search_string);
//  scanf ("%[a-zA-Z0-9.\"\'() -]", search_string);

ret_val1=gtk_text_backward_delete(GTK_TEXT(results), gtk_text_get_length(GTK_TEXT(results)));
  search_string = gtk_entry_get_text(GTK_ENTRY(entry1)); 
  thingy = ency_find_titles (search_string);
  if ((thingy != NULL) && (thingy->title != NULL))
    {
      do
	{
fmt1=thingy->fmt;
while (fmt1 != NULL)
 {
 fmt2=fmt1;
 fmt1=fmt2->next;
 free(fmt2);
 }
	  //printf ("\n%s\n\n%s\n", thingy->title, thingy->text);
	  gtk_text_insert (GTK_TEXT (results), NULL, NULL, NULL,
			  thingy->title, -1);
	  gtk_text_insert (GTK_TEXT (results), NULL, NULL, NULL,
			  "\n\n", -1);
	  gtk_text_insert (GTK_TEXT (results), NULL, NULL, NULL,
			  thingy->text, -1);
	  gtk_text_insert (GTK_TEXT (results), NULL, NULL, NULL,
			  "\n\n", -1);
	  kill_me = thingy;
	  thingy = thingy->next;
          free(kill_me->text);
          free(kill_me->title);
	  free(kill_me);
// if (thingy->next == NULL) printf("thingy->next == NULL");
	}
      while (thingy != NULL);
    }
  else {
  search_string = "test"; 
  // printf ("No matches\n");
   rtnresults = "No matches\n";  
   gtk_text_insert (GTK_TEXT (results), NULL, NULL, NULL, rtnresults, -1);
    //printf ("No matches\n");
}
}

void testfunc()
{
  rtnresults = "test";
  gtk_text_insert (GTK_TEXT (results), NULL, NULL, NULL, rtnresults, -1);
}

void make_gui()
{
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_usize (GTK_WIDGET (window), 500, 200);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		  GTK_SIGNAL_FUNC(CB_exit), &window);
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		  GTK_SIGNAL_FUNC(CB_exit), &window);
  gtk_window_set_title (GTK_WINDOW (window), "Startrek Ensyc");
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

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);
  gtk_box_pack_start (GTK_BOX (box0), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  results = gtk_text_new(NULL,NULL);
  gtk_text_set_word_wrap(GTK_TEXT (results), TRUE);
  gtk_table_attach_defaults (GTK_TABLE (table), results, 0, 1, 0, 1);
  
  vscroll = gtk_vscrollbar_new (GTK_TEXT (results)->vadj);
  gtk_table_attach (GTK_TABLE (table), vscroll, 1, 2, 0, 1,
		  GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  //gtk_container_add (GTK_CONTAINER (box0), results);
  gtk_widget_show (box0);
  gtk_widget_show (box1);
  gtk_widget_show (label1);
  gtk_widget_show (entry1);
  gtk_widget_show (button1);
  gtk_widget_show (results);
  gtk_widget_show (vscroll);
  gtk_widget_show (window);
}

int main(int argc, char *argv[])
{
  gtk_init(&argc, &argv);
  make_gui();
  gtk_main();
return(0);
}


