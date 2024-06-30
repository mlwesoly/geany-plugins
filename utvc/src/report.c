
#ifdef HAVE_CONFIG_H
	#include "config.h" /* for the gettext domain */
#endif

#include "report.h"
#include <geanyplugin.h>
#include "mail-icon.xpm"
#include <glib.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gstdio.h>
#include <errno.h>
/*

GeanyPlugin		*geany_plugin2;
//GeanyData *geany_data;

enum
{
	OPENTER_KB,
	COUNT_KB
};

GtkWidget *mailbutton = NULL;
GtkWidget *button2 = NULL;
GtkWidget *newreportbutton = NULL;

#define GEANYSENDMAIL_STOCK_MAIL "geanysendmail-mail"
#define GEANYSENDBUTTON2 "geanybutton2"

static void add_stock_item(void)
{
	GtkIconSet *icon_set;
	GtkIconFactory *factory = gtk_icon_factory_new();
	GtkIconTheme *theme = gtk_icon_theme_get_default();
	GtkStockItem item = { (gchar*)(GEANYSENDMAIL_STOCK_MAIL), (gchar*)(N_("Mail")), 0, 0, (gchar*)(GETTEXT_PACKAGE) };

	if (gtk_icon_theme_has_icon(theme, "document-new"))
	{
		GtkIconSource *icon_source = gtk_icon_source_new();
		icon_set = gtk_icon_set_new();
		gtk_icon_source_set_icon_name(icon_source, "document-new");
		gtk_icon_set_add_source(icon_set, icon_source);
		gtk_icon_source_free(icon_source);
	}
	else
	{
		GdkPixbuf *pb = gdk_pixbuf_new_from_xpm_data(mail_icon);
		icon_set = gtk_icon_set_new_from_pixbuf(pb);
		g_object_unref(pb);
	}
	gtk_icon_factory_add(factory, item.stock_id, icon_set);
	gtk_stock_add(&item, 1);
	gtk_icon_factory_add_default(factory);

	g_object_unref(factory);
	gtk_icon_set_unref(icon_set);
}

static void add_stock_item2(void)
{
	GtkIconSet *icon_set;
	GtkIconFactory *factory = gtk_icon_factory_new();
	GtkIconTheme *theme = gtk_icon_theme_get_default();
	GtkStockItem item = { (gchar*)(GEANYSENDBUTTON2), (gchar*)(N_("button2")), 0, 0, (gchar*)(GETTEXT_PACKAGE) };

	if (gtk_icon_theme_has_icon(theme, "insert-link"))
	{
		GtkIconSource *icon_source = gtk_icon_source_new();
		icon_set = gtk_icon_set_new();
		gtk_icon_source_set_icon_name(icon_source, "insert-link");
		gtk_icon_set_add_source(icon_set, icon_source);
		gtk_icon_source_free(icon_source);
	}
	else
	{
		GdkPixbuf *pb = gdk_pixbuf_new_from_xpm_data(mail_icon);
		icon_set = gtk_icon_set_new_from_pixbuf(pb);
		g_object_unref(pb);
	}
	gtk_icon_factory_add(factory, item.stock_id, icon_set);
	gtk_stock_add(&item, 1);
	gtk_icon_factory_add_default(factory);

	g_object_unref(factory);
	gtk_icon_set_unref(icon_set);
}

static void excecute01(void)
{
	gchar *locale_path;
	GeanyDocument *doc = document_get_current();
	locale_path = g_path_get_dirname(doc->file_name);
	//execlp("python", "python", "/home/michael/my_script.py", "test", (char*) NULL);
	char* command = "python3";
    char* argument_list[] = {"python3", "/home/tvc/tools/python/copArename.py", locale_path , NULL};
	

    if (fork() == 0) {
        // Newly spawned child Process. This will be taken over by "ls -l"
        int status_code = execvp(command, argument_list);
		
        if (status_code == -1) {
            printf("Terminated Incorrectly\n");
            return;
        }
    }
    else {
        // Old Parent process. The C program will come here
        printf("This line will be printed\n");
    }
}

static void excecute02(void)
{
	gchar *locale_path;
	gchar *locale_filename;
	GeanyDocument *doc = document_get_current();
	if (doc != NULL)
	{
		locale_path = g_path_get_dirname(doc->file_name);
		locale_filename = doc->file_name;
	}
	//printf("%s",locale_path);
	//execlp("python", "python", "/home/michael/my_script.py", "test", (char*) NULL);
	char* command = "/home/tvc/PycharmProjects/PrepareReport/main.py";
    char* argument_list[] = {"/home/tvc/PycharmProjects/PrepareReport/main.py", locale_path, locale_filename, NULL};
	printf("%s", locale_filename);
	

    if (fork() == 0) {
        // Newly spawned child Process. This will be taken over by "ls -l"
        int status_code = execvp(command, argument_list);
		
        if (status_code == -1) {
            printf("Terminated Incorrectly\n");
            return;
        }
    }
    else {
        // Old Parent process. The C program will come here
        printf("This line will be printed\n");
    }
}

static void on_openTerminal()
{
	GeanyDocument *doc = document_get_current();
	gchar *locale_path2;
	if (doc == NULL)
	{
		locale_path2 = "~";
	}
	else 
	{
		locale_path2 = g_path_get_dirname(doc->file_name);
	}

	const gchar *prefix="--working-directory=" ;
	gchar *workingdic = g_strconcat(prefix , locale_path2, NULL);

	char* command = "gnome-terminal";
    //char* argument_list[] = {"gnome-terminal",  NULL};
	char* argument_list[] = {"gnome-terminal ", workingdic , NULL};

    if (fork() == 0) {
        // Newly spawned child Process. This will be taken over by "ls -l"
        int status_code = execvp(command, argument_list);
        if (status_code == -1) {
            printf("Terminated Incorrectly\n");
            return;
        }
    }
    else {
        // Old Parent process. The C program will come here
        printf("This line will be printed\n");
    }
	
}

static void show_icon(void)
{
	mailbutton = GTK_WIDGET(gtk_tool_button_new_from_stock(GEANYSENDMAIL_STOCK_MAIL));
	// mailbutton = GTK_WIDGET(gtk_tool_button_new_from_stock(GEANYSENDMAIL_STOCK_MAIL));
	plugin_add_toolbar_item(geany_plugin2, GTK_TOOL_ITEM(mailbutton));
	ui_add_document_sensitive(mailbutton);
#if GTK_CHECK_VERSION(2, 12, 0)
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(mailbutton), _("Copy new TVCreport Template"));
#endif
	g_signal_connect (G_OBJECT(mailbutton), "clicked", G_CALLBACK(excecute01), NULL);
	gtk_widget_show_all (mailbutton);
}

static void show_icon2(void)
{
	button2 = GTK_WIDGET(gtk_tool_button_new_from_stock(GEANYSENDBUTTON2));
	plugin_add_toolbar_item(geany_plugin2, GTK_TOOL_ITEM(button2));
	ui_add_document_sensitive(button2);
#if GTK_CHECK_VERSION(2, 12, 0)
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(button2), _("button2"));
#endif
	g_signal_connect (G_OBJECT(button2), "clicked", G_CALLBACK(excecute02), NULL);
	gtk_widget_show_all (button2);
}

static void cleanup_icon(void)
{
	if (mailbutton != NULL)
	{
		gtk_container_remove(GTK_CONTAINER (geany->main_widgets->toolbar), mailbutton);
	}
	if (button2 != NULL)
	{
		gtk_container_remove(GTK_CONTAINER (geany->main_widgets->toolbar), button2);
	}
}


void report_init(GeanyData G_GNUC_UNUSED *data)
{
	GeanyKeyGroup *plugin_key_group;
	setlocale(LC_NUMERIC, "C");
	plugin_key_group = plugin_set_key_group(geany_plugin2, "yTVC_Group", COUNT_KB, NULL);

	keybindings_set_item(plugin_key_group, OPENTER_KB, on_openTerminal, 0, 0, "openTermin", _("openTermin"), NULL);
	
	add_stock_item();
	add_stock_item2();
	show_icon();
	show_icon2();
}


void plugin_cleanup(void)
{
	cleanup_icon();
}
*/
void addingtwo(void)
{
	int i;
	i=2+2;
	printf("2+2");
	fflush(stdout);
	ui_set_statusbar(TRUE, ("Too many items selected!"));

}