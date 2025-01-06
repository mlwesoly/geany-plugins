
#include <gtk/gtk.h>

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gdk/gdkkeysyms.h>

#include <geanyplugin.h>
#include <geany.h>
#include <document.h>



static void first_dialog(void)
{
    GtkWidget *vbox01, *vbox02, *vboxbig;
    // This creates (but does not yet display) a message dialog with
    // the given text as the title.
    vbox01 = gtk_vbox_new(GTK_ORIENTATION_VERTICAL, 12);
    vbox02 = gtk_vbox_new(GTK_ORIENTATION_VERTICAL, 12);


    GtkWidget* hello = gtk_dialog_new_with_buttons("My dialog",
                                       NULL,
                                       GTK_DIALOG_MODAL,
                                       _("_OK"),
                                       GTK_RESPONSE_ACCEPT,
                                       _("_Cancel"),
                                       GTK_RESPONSE_REJECT,
                                       NULL);

    
    // This displays our message dialog as a modal dialog, waiting for
    // the user to click a button before moving on. The return value
    // comes from the :response signal emitted by the dialog. By
    // default, the dialog only has an OK button, so we'll get a
    // GTK_RESPONSE_OK if the user clicked the button. But if the user
    // destroys the window, we'll get a GTK_RESPONSE_DELETE_EVENT.
    int response = gtk_dialog_run(GTK_DIALOG(hello));

    printf("response was %d (OK=%d, DELETE_EVENT=%d)\n",
           response, GTK_RESPONSE_OK, GTK_RESPONSE_DELETE_EVENT);

    // If we don't destroy the dialog here, it will still be displayed
    // (in back) when the second dialog below is run.
    gtk_widget_destroy(hello);
}

static void second_dialog(void)
{
    // In the second dialog, we use markup (just bold and italics in
    // this case).
    GtkWidget* hello_markup = gtk_message_dialog_new_with_markup(
        NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        "Hi, I'm a <b>message dialog</b> with <i>markup</i>!");

    // We *could* add secondary text, either plain text or with
    // markup, but we haven't done it here, just to show what the end
    // result looks like. Either way, printf-style formatting is
    // available.

    // gtk_message_dialog_format_secondary_markup(
    //     GTK_MESSAGE_DIALOG(hello_markup),
    //     "This is <i>secondary</i> markup.");

    // Again, this displays the second dialog as a modal dialog.
    gtk_dialog_run(GTK_DIALOG(hello_markup));

    gtk_widget_destroy(hello_markup);
}

int calldialog()
{
    // Standard boilerplate: initialize the toolkit.
    //gtk_init(&argc, &argv);

    first_dialog();
    second_dialog();

    return 0;
}

GtkWidget * call_plugin_configure (GtkDialog *dialog)
{
  GtkWidget        *box1;
  
  /* Top-level box, containing the different frames */
  box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  
  
  return box1;
}