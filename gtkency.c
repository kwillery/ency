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
#include <gtk/gtk.h>

void bye(void)
{
gtk_main_quit;
}

int
main (int argc, char *argv[])
{
  GtkWidget *window, *entry;
  GtkWidget *mybox, *button;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title ((GtkWindow *) window, "Test App.");
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      GTK_SIGNAL_FUNC (bye), NULL);


  button = gtk_button_new_with_label ("hi");

  entry = gtk_entry_new ();
  gtk_entry_set_text ((GtkEntry *) entry, "Hi!");
  gtk_entry_set_visibility ((GtkEntry *) entry, TRUE);

  mybox = gtk_hbox_new (FALSE, 5);
  gtk_container_add (GTK_CONTAINER (window), mybox);  
  gtk_box_pack_start_defaults (GTK_BOX (mybox), entry);
  gtk_box_pack_start_defaults (GTK_BOX (mybox), button);

// gtk_container_add (GTK_CONTAINER (window), mybox);

 gtk_widget_show (window);
 gtk_widget_show (mybox);
 gtk_widget_show (button);
 gtk_widget_show (entry);

  gtk_main ();

  return 0;
}
