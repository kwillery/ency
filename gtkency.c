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
 gtk_exit(0);
}

int
main (int argc, char *argv[])
{
  GtkWidget *window, *entry;
  GtkWidget *mybox, *button;
  GtkWidget *box2, *encylistbox;
  GtkWidget *box3, *encyquit;
  GtkWidget *encytextbox, *alignbot;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title ((GtkWindow *) window, "Test App.");
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      GTK_SIGNAL_FUNC (bye), NULL);


  button = gtk_button_new_with_label ("Find now");
  encylistbox = gtk_button_new_with_label ("<insert list box here...>");
  encytextbox = gtk_button_new_with_label ("<insert text box here...>");

  alignbot = gtk_alignment_new(1,0.5,0,0);
  encyquit = gtk_button_new_with_label ("Exit");
  gtk_signal_connect (GTK_OBJECT (encyquit), "clicked",
                      GTK_SIGNAL_FUNC (bye), NULL);

  entry = gtk_entry_new ();
  gtk_entry_set_text ((GtkEntry *) entry, "Hi!");
  gtk_entry_set_visibility ((GtkEntry *) entry, TRUE);

 // Box2 init starts
  box2 = gtk_vbox_new (FALSE, 5);
  gtk_container_add (GTK_CONTAINER (window), box2);

  mybox = gtk_hbox_new (FALSE, 5);
  gtk_container_add (GTK_CONTAINER (box2), mybox);  
  gtk_box_pack_start_defaults (GTK_BOX (mybox), entry);
  gtk_box_pack_start_defaults (GTK_BOX (mybox), button);

//  box3 = gtk_hbox_new (FALSE, 5);
//  gtk_container_add (GTK_CONTAINER (box2), box3);
//  gtk_box_pack_start_defaults (GTK_BOX (box3), encylistbox);
  gtk_box_pack_start_defaults (GTK_BOX (box2), encylistbox);
  gtk_box_pack_start_defaults (GTK_BOX (box2), encytextbox);
  gtk_box_pack_start_defaults (GTK_BOX (box2), alignbot);
//  gtk_box_pack_start_defaults (GTK_BOX (alignbot), encyquit);
  gtk_container_add (GTK_CONTAINER (alignbot), encyquit);

// box2 init stops...

 gtk_widget_show (window);
// gtk_widget_show (box3);
 gtk_widget_show (box2);
 gtk_widget_show (mybox);
 gtk_widget_show (button);
 gtk_widget_show (entry);
 gtk_widget_show (encylistbox);
 gtk_widget_show (encytextbox);
 gtk_widget_show (alignbot);
 gtk_widget_show (encyquit);

  gtk_main ();

  return 0;
}
