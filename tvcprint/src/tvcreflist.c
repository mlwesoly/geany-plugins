

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <geanyplugin.h>

#include "tvcprint.h"
#include "tvcreflist.h"



void searchinreflist2()
{
    GtkWidget *dialog, *label, *content_area;
    GtkDialogFlags flags;

    // Create the widgets
    flags = GTK_DIALOG_DESTROY_WITH_PARENT;
    dialog = gtk_dialog_new_with_buttons ("Message",
                                         GTK_WINDOW(geany->main_widgets->window),
                                        flags,
                                        _("_OK"),
                                        GTK_RESPONSE_NONE,
                                        NULL);


    content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

    label = gtk_label_new ("message");

    // Ensure that the dialog box is destroyed when the user responds

    g_signal_connect_swapped (dialog,
                            "response",
                            G_CALLBACK (gtk_widget_destroy),
                            dialog);

    // Add the label, and show everything we’ve added

    gtk_container_add (GTK_CONTAINER (content_area), label);

    gtk_widget_show_all (dialog);

}


static GtkWidget *window = NULL;


void searchinreflist ()
{
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *expander;
  

    window = gtk_dialog_new_with_buttons ("GtkExpander",
                    GTK_WINDOW(geany->main_widgets->window),
                    0,
                    GTK_STOCK_CLOSE,
                    GTK_RESPONSE_NONE,
                    NULL);
                    
    gtk_window_set_resizable (GTK_WINDOW (window), FALSE);

    g_signal_connect (window, "response",
            G_CALLBACK (gtk_widget_destroy), NULL);
    g_signal_connect (window, "destroy",
            G_CALLBACK (gtk_widget_destroyed), &window);


    vbox = gtk_vbox_new (FALSE, 5);
    gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (window))), vbox, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

    label = gtk_label_new ("Expander demo. Click on the triangle for details.");
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

    /* Create the expander */
    expander = gtk_expander_new ("Details");
    gtk_box_pack_start (GTK_BOX (vbox), expander, FALSE, FALSE, 0);

    label = gtk_label_new ("Details can be shown or hidden.");
    gtk_container_add (GTK_CONTAINER (expander), label);
    

    if (!gtk_widget_get_visible (window))
    gtk_widget_show_all (window);
    else
    gtk_widget_destroy (window);

 
}
