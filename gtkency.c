     #include <gtk/gtk.h>
     
     int
     main (int argc, char *argv[])
     {
       GtkWidget *window, *entry;
       GtkWidget *mybox, *button;

       gtk_init (&argc, &argv);
     
     
       window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
gtk_window_set_title((GtkWindow *)window,"Test App.");
       gtk_widget_show (window);

button=gtk_button_new_with_label("hi");
mybox=gtk_hbox_new(1,1);
entry = gtk_entry_new();
gtk_entry_set_text((GtkEntry *)entry,"Hi!");
gtk_entry_set_visibility((GtkEntry *)entry,TRUE);
gtk_container_add(GTK_CONTAINER (window),mybox);
gtk_box_pack_start_defaults(GTK_BOX(mybox),entry);
gtk_box_pack_start_defaults(GTK_BOX(mybox),button);
gtk_widget_show(mybox);
gtk_widget_show(button);
gtk_widget_show(entry);
       gtk_main ();
     
       return 0;
     }

