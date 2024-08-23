
#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>

#include <string.h>
#include <regex.h>
#include <dirent.h> 
#include <stdio.h> 
#include <glob.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "geany.h"
#include <geanyplugin.h>

#include "tvctreebrowser.h"
#include "tvcaddition.h"


static     GtkWidget 			*toolbar2;


void create_sidebar_addition(){
    
    GtkWidget 			*wid;

    toolbar2 = gtk_toolbar_new();
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar2), GTK_ICON_SIZE_MENU);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar2), GTK_TOOLBAR_ICONS);

#if GTK_CHECK_VERSION(3, 10, 0)
	wid = gtk_image_new_from_icon_name("go-up", GTK_ICON_SIZE_SMALL_TOOLBAR);
	wid = GTK_WIDGET(gtk_tool_button_new(wid, NULL));
#else
	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_GO_UP));
#endif
	gtk_widget_set_tooltip_text(wid, _("Go up"));
	//g_signal_connect(wid, "clicked", G_CALLBACK(on_button_go_up), NULL);
	gtk_container_add(GTK_CONTAINER(toolbar2), wid);
    
    
}

void add_to_sidebar_addition(){
    gtk_box_pack_start(GTK_BOX(sidebar_vbox_bars), 			toolbar2, 			FALSE, TRUE,  1);
}


static void open_selected_pdffiles(GList *list, gboolean do_not_focus)
{
	//GSList *files = NULL;
	GList *item;
	//GeanyDocument *doc;
	GError* err = NULL;
	//gchar *fullpath = NULL;
	for (item = list; item != NULL; item = g_list_next(item))
	{
		GtkTreePath *treepath = item->data;
		gchar *fname = get_tree_path_filename(treepath);
		gchar *fpath = "file://";
		
		//gchar *fullpath = NULL;

		gchar *fullpath = malloc(strlen(fpath) + strlen(fname) + 1);
		//ui_set_statusbar(TRUE, "%s", fullpath);
		//ui_set_statusbar(TRUE, "\n");
		strcpy(fullpath,fpath);
		strcat(fullpath,fname);
		
		//ui_set_statusbar(TRUE, "%s", fullpath);
		//ui_set_statusbar(TRUE, fpath);
		gtk_show_uri_on_window(NULL, fullpath, GDK_CURRENT_TIME, &err);
		//gtk_show_uri(gdk_screen_get_default(), fname, GDK_CURRENT_TIME, &err);
		//files = g_slist_prepend(files, fname);
		free(fullpath);
	}
	//files = g_slist_reverse(files);
	//document_open_files(files, FALSE, NULL, NULL);
	//doc = document_get_current();
	//if (doc != NULL && ! do_not_focus)
	//	keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);

	//g_slist_foreach(files, (GFunc) g_free, NULL);	/* free filenames */
	//g_slist_free(files);
	
}