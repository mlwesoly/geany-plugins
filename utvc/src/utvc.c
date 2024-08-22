/*
 *      filebrowser.c - this file is part of Geany, a fast and lightweight IDE
 *
 *      Copyright 2007 The Geany contributors
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License along
 *      with this program; if not, write to the Free Software Foundation, Inc.,
 *      51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* Sidebar file browser plugin. */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "geanyplugin.h"
#include "gtkcompat.h"
#include <string.h>
#include <regex.h>
#include <dirent.h> 
#include <stdio.h> 
#include <glob.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


#include <gdk/gdkkeysyms.h>

#ifdef G_OS_WIN32
# include <windows.h>

# define OPEN_CMD "explorer \"%d\""
#elif defined(__APPLE__)
# define OPEN_CMD "open \"%d\""
#else
# define OPEN_CMD "nautilus \"%d\""
#endif

GeanyPlugin *geany_plugin;
GeanyData *geany_data;


PLUGIN_VERSION_CHECK(GEANY_API_VERSION)

PLUGIN_SET_INFO(_("TVC File Browser"), _("Adds a file browser tab to the sidebar."),
	PACKAGE_VERSION, _("The Geany developer team"))


/* Keybinding(s) */
enum
{
	KB_FOCUS_FILE_LIST,
	KB_FOCUS_PATH_ENTRY,
	OPENTER_KB,
	KB_COUNT
};


enum
{
	FILEVIEW_COLUMN_ICON = 0,
	FILEVIEW_COLUMN_NAME,
	FILEVIEW_COLUMN_FILENAME, /* the full filename, including path for display as tooltip */
	FILEVIEW_COLUMN_IS_DIR,
	FILEVIEW_N_COLUMNS
};

static gboolean fb_set_project_base_path = FALSE;
static gboolean fb_follow_path = FALSE;
static gboolean show_hidden_files = FALSE;
static gboolean hide_object_files = TRUE;

static GtkWidget *file_view_vbox;
static GtkWidget *helper_box;
static GtkWidget *file_view;
static GtkListStore *file_store;
static GtkTreeIter *last_dir_iter = NULL;
static GtkEntryCompletion *entry_completion = NULL;

static GtkWidget *filter_combo;
static GtkWidget *TVCnumberEntry;
static GtkWidget *coupentry;
static GtkWidget *filter_entry;
static GtkWidget *path_combo;
static GtkWidget *path_entry;
static gchar *current_dir = NULL; /* in locale-encoding */
static gchar *open_cmd; /* in locale-encoding */
static gchar *config_file;
static gchar **filter = NULL;
static gchar *hidden_file_extensions = NULL;
static gchar *currentTVCNumber = NULL;
static gchar *coupling_folder = NULL;
static gchar *coupling_prefix = NULL;
static gchar *transfer_folder = NULL;
static gchar *reporttype = NULL;
static gchar *templatetype = NULL;

static gint page_number = 0;
static gint page_number2 = 0;

static struct
{
	GtkWidget *open;
	GtkWidget *open_external;
	GtkWidget *openpdf;
	GtkWidget *openfolder;
	GtkWidget *find_in_files;
	GtkWidget *show_hidden_files;
} popup_items;

//report_init(void);

static void project_open_cb(GObject *obj, GKeyFile *config, gpointer data);

/* note: other callbacks connected in plugin_init */
PluginCallback plugin_callbacks[] =
{
	{ "project-open", (GCallback) &project_open_cb, TRUE, NULL },
	{ NULL, NULL, FALSE, NULL }
};


#ifdef G_OS_WIN32
static gboolean win32_check_hidden(const gchar *filename)
{
	DWORD attrs;
	static wchar_t w_filename[MAX_PATH];
	MultiByteToWideChar(CP_UTF8, 0, filename, -1, w_filename, sizeof(w_filename));
	attrs = GetFileAttributesW(w_filename);
	if (attrs != INVALID_FILE_ATTRIBUTES && attrs & FILE_ATTRIBUTE_HIDDEN)
		return TRUE;
	return FALSE;
}
#endif


/* Returns: whether name should be hidden. */
static gboolean check_hidden(const gchar *filename, const gchar *base_name)
{
	gsize len;

#ifdef G_OS_WIN32
	if (win32_check_hidden(filename))
		return TRUE;
#else
	if (base_name[0] == '.')
		return TRUE;
#endif

	len = strlen(base_name);
	return base_name[len - 1] == '~';
}


static gboolean check_object(const gchar *base_name)
{
	gboolean ret = FALSE;
	gchar **ptr;
	gchar **exts = g_strsplit(hidden_file_extensions, " ", -1);

	foreach_strv(ptr, exts)
	{
		if (g_str_has_suffix(base_name, *ptr))
		{
			ret = TRUE;
			break;
		}
	}
	g_strfreev(exts);
	return ret;
}


/* Returns: whether filename should be removed. */
static gboolean check_filtered(const gchar *base_name)
{
	gchar **filter_item;

	if (filter == NULL)
		return FALSE;

	foreach_strv(filter_item, filter)
	{
		if (utils_str_equal(*filter_item, "*") || g_pattern_match_simple(*filter_item, base_name))
		{
			return FALSE;
		}
	}
	return TRUE;
}


static GIcon *get_icon(const gchar *fname)
{
	GIcon *icon = NULL;
	gchar *ctype;

	ctype = g_content_type_guess(fname, NULL, 0, NULL);

	if (ctype)
	{
		icon = g_content_type_get_icon(ctype);
		if (icon)
		{
			GtkIconInfo *icon_info;

			icon_info = gtk_icon_theme_lookup_by_gicon(gtk_icon_theme_get_default(), icon, 16, 0);
			if (!icon_info)
			{
				g_object_unref(icon);
				icon = NULL;
			}
			else
				gtk_icon_info_free(icon_info);
		}
		g_free(ctype);
	}

	if (!icon)
		icon = g_themed_icon_new("text-x-generic");

	return icon;
}


/* name is in locale encoding */
static void add_item(const gchar *name)
{
	GtkTreeIter iter;
	gchar *fname, *utf8_name, *utf8_fullname;
	const gchar *sep;
	gboolean dir;
	GIcon *icon;

	if (G_UNLIKELY(EMPTY(name)))
		return;

	/* root directory doesn't need separator */
	sep = (utils_str_equal(current_dir, "/")) ? "" : G_DIR_SEPARATOR_S;
	fname = g_strconcat(current_dir, sep, name, NULL);
	dir = g_file_test(fname, G_FILE_TEST_IS_DIR);
	utf8_fullname = utils_get_utf8_from_locale(fname);
	utf8_name = utils_get_utf8_from_locale(name);
	g_free(fname);

	if (! show_hidden_files && check_hidden(utf8_fullname, utf8_name))
		goto done;

	if (dir)
	{
		if (last_dir_iter == NULL)
			gtk_list_store_prepend(file_store, &iter);
		else
		{
			gtk_list_store_insert_after(file_store, &iter, last_dir_iter);
			gtk_tree_iter_free(last_dir_iter);
		}
		last_dir_iter = gtk_tree_iter_copy(&iter);
	}
	else
	{
		if (! show_hidden_files && hide_object_files && check_object(utf8_name))
			goto done;
		if (check_filtered(utf8_name))
			goto done;

		gtk_list_store_append(file_store, &iter);
	}

	icon = dir ? g_themed_icon_new("folder") : get_icon(utf8_name);
	gtk_list_store_set(file_store, &iter,
		FILEVIEW_COLUMN_ICON, icon,
		FILEVIEW_COLUMN_NAME, utf8_name,
		FILEVIEW_COLUMN_FILENAME, utf8_fullname,
		FILEVIEW_COLUMN_IS_DIR, dir,
		-1);
	g_object_unref(icon);
done:
	g_free(utf8_name);
	g_free(utf8_fullname);
}


/* adds ".." to the start of the file list */
static void add_top_level_entry(void)
{
	GtkTreeIter iter;
	gchar *utf8_dir;
	GIcon *icon;

	if (EMPTY(g_path_skip_root(current_dir)))
		return;	/* ignore 'C:\' or '/' */

	utf8_dir = g_path_get_dirname(current_dir);
	SETPTR(utf8_dir, utils_get_utf8_from_locale(utf8_dir));

	gtk_list_store_prepend(file_store, &iter);
	last_dir_iter = gtk_tree_iter_copy(&iter);

	icon = g_themed_icon_new("folder");
	gtk_list_store_set(file_store, &iter,
		FILEVIEW_COLUMN_ICON, icon,
		FILEVIEW_COLUMN_NAME, "..",
		FILEVIEW_COLUMN_FILENAME, utf8_dir,
		FILEVIEW_COLUMN_IS_DIR, TRUE,
		-1);
	g_object_unref(icon);
	g_free(utf8_dir);
}


static void clear(void)
{
	gtk_list_store_clear(file_store);

	/* reset the directory item pointer */
	if (last_dir_iter != NULL)
		gtk_tree_iter_free(last_dir_iter);
	last_dir_iter = NULL;
}


/* recreate the tree model from current_dir. */
static void refresh(void)
{
	gchar *utf8_dir;
	GSList *list, *node;


	/* don't clear when the new path doesn't exist */
	if (! g_file_test(current_dir, G_FILE_TEST_EXISTS))
		return;

	clear();

	utf8_dir = utils_get_utf8_from_locale(current_dir);
	gtk_entry_set_text(GTK_ENTRY(path_entry), utf8_dir);
	gtk_widget_set_tooltip_text(path_entry, utf8_dir);
	ui_combo_box_add_to_history(GTK_COMBO_BOX_TEXT(path_combo), utf8_dir, 0);
	g_free(utf8_dir);

	add_top_level_entry();	/* ".." item */

	list = utils_get_file_list(current_dir, NULL, NULL);
	if (list != NULL)
	{
		/* free filenames as we go through the list */
		foreach_slist(node, list)
		{
			gchar *fname = node->data;

			add_item(fname);
			g_free(fname);
		}
		g_slist_free(list);
	}
	gtk_entry_completion_set_model(entry_completion, GTK_TREE_MODEL(file_store));
}


static void on_go_home(void)
{
	SETPTR(current_dir, g_strdup(g_get_home_dir()));
	refresh();
}

static void on_go_tvchome(void)
{
	/* here change with an preference parameter, standard is /home/tvc/TVC.DATA/ */
	SETPTR(current_dir, g_strdup("/home/tvc/TVC.DATA/"));
	refresh();
}


/* TODO: use utils_get_default_dir_utf8() */
static gchar *get_default_dir(void)
{
	const gchar *dir = NULL;
	GeanyProject *project = geany->app->project;

	if (project)
		dir = project->base_path;
	else
		dir = geany->prefs->default_open_path;

	if (!EMPTY(dir))
		return utils_get_locale_from_utf8(dir);

	return g_get_current_dir();
}


static void on_current_path(void)
{
	gchar *fname;
	gchar *dir;
	GeanyDocument *doc = document_get_current();

	if (doc == NULL || doc->file_name == NULL || ! g_path_is_absolute(doc->file_name))
	{
		SETPTR(current_dir, get_default_dir());
		refresh();
		return;
	}
	fname = doc->file_name;
	fname = utils_get_locale_from_utf8(fname);
	dir = g_path_get_dirname(fname);
	g_free(fname);

	SETPTR(current_dir, dir);
	refresh();
}


static void on_realized(void)
{
	GeanyProject *project = geany->app->project;

	/* if fb_set_project_base_path and project open, the path has already been set */
	if (! fb_set_project_base_path || project == NULL || EMPTY(project->base_path))
		on_current_path();
}


static void on_go_up(void)
{
	gsize len = strlen(current_dir);
	if (current_dir[len-1] == G_DIR_SEPARATOR)
		current_dir[len-1] = '\0';
	/* remove the highest directory part (which becomes the basename of current_dir) */
	SETPTR(current_dir, g_path_get_dirname(current_dir));
	refresh();
}


static gboolean check_single_selection(GtkTreeSelection *treesel)
{
	if (gtk_tree_selection_count_selected_rows(treesel) == 1)
		return TRUE;

	ui_set_statusbar(FALSE, _("Too many items selected!"));
	return FALSE;
}


/* Returns: TRUE if at least one of selected_items is a folder. */
static gboolean is_folder_selected(GList *selected_items)
{
	GList *item;
	GtkTreeModel *model = GTK_TREE_MODEL(file_store);
	gboolean dir_found = FALSE;

	for (item = selected_items; item != NULL; item = g_list_next(item))
	{
		GtkTreeIter iter;
		GtkTreePath *treepath;

		treepath = (GtkTreePath*) item->data;
		gtk_tree_model_get_iter(model, &iter, treepath);
		gtk_tree_model_get(model, &iter, FILEVIEW_COLUMN_IS_DIR, &dir_found, -1);

		if (dir_found)
			break;
	}
	return dir_found;
}


/* Returns: the full filename in locale encoding. */
static gchar *get_tree_path_filename(GtkTreePath *treepath)
{
	GtkTreeModel *model = GTK_TREE_MODEL(file_store);
	GtkTreeIter iter;
	gchar *name, *fname;

	gtk_tree_model_get_iter(model, &iter, treepath);
	gtk_tree_model_get(model, &iter, FILEVIEW_COLUMN_FILENAME, &name, -1);

	fname = utils_get_locale_from_utf8(name);
	g_free(name);

	return fname;
}


static void open_external(const gchar *fname, gboolean dir_found)
{
	gchar *cmd;
	gchar *locale_cmd;
	gchar *dir;
	GString *cmd_str = g_string_new(open_cmd);
	GError *error = NULL;

	if (! dir_found)
		dir = g_path_get_dirname(fname);
	else
		dir = g_strdup(fname);

	utils_string_replace_all(cmd_str, "%f", fname);
	utils_string_replace_all(cmd_str, "%d", dir);

	cmd = g_string_free(cmd_str, FALSE);
	locale_cmd = utils_get_locale_from_utf8(cmd);
	if (! spawn_async(NULL, locale_cmd, NULL, NULL, NULL, &error))
	{
		gchar *c = strchr(cmd, ' ');

		if (c != NULL)
			*c = '\0';
		ui_set_statusbar(TRUE,
			_("Could not execute configured external command '%s' (%s)."),
			cmd, error->message);
		g_error_free(error);
	}
	g_free(locale_cmd);
	g_free(cmd);
	g_free(dir);
}


static void on_openfolder_clicked(GtkMenuItem *menuitem, gpointer user_data)
{
	char s[100];
	snprintf(s, sizeof s, "%s %s", "xdg-open", current_dir);
	system(s);
}


static void on_external_open(GtkMenuItem *menuitem, gpointer user_data)
{
	GtkTreeSelection *treesel;
	GtkTreeModel *model;
	GList *list;
	gboolean dir_found;

	treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(file_view));

	list = gtk_tree_selection_get_selected_rows(treesel, &model);
	dir_found = is_folder_selected(list);

	if (! dir_found || check_single_selection(treesel))
	{
		GList *item;

		for (item = list; item != NULL; item = g_list_next(item))
		{
			GtkTreePath *treepath = item->data;
			gchar *fname = get_tree_path_filename(treepath);

			open_external(fname, dir_found);
			g_free(fname);
		}
	}

	g_list_free_full(list, (GDestroyNotify) gtk_tree_path_free);
}


/* We use document_open_files() as it's more efficient. */
static void open_selected_files(GList *list, gboolean do_not_focus)
{
	GSList *files = NULL;
	GList *item;
	GeanyDocument *doc;

	for (item = list; item != NULL; item = g_list_next(item))
	{
		GtkTreePath *treepath = item->data;
		gchar *fname = get_tree_path_filename(treepath);

		files = g_slist_prepend(files, fname);
	}
	files = g_slist_reverse(files);
	document_open_files(files, FALSE, NULL, NULL);
	doc = document_get_current();
	if (doc != NULL && ! do_not_focus)
		keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);

	g_slist_free_full(files, g_free);
}


static void open_folder(GtkTreePath *treepath)
{
	gchar *fname = get_tree_path_filename(treepath);

	SETPTR(current_dir, fname);
	refresh();
}


static void on_open_clicked(GtkMenuItem *menuitem, gpointer user_data)
{
	GtkTreeSelection *treesel;
	GtkTreeModel *model;
	GList *list;
	gboolean dir_found;

	treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(file_view));

	list = gtk_tree_selection_get_selected_rows(treesel, &model);
	dir_found = is_folder_selected(list);

	if (dir_found)
	{
		if (check_single_selection(treesel))
		{
			GtkTreePath *treepath = list->data;	/* first selected item */

			open_folder(treepath);
		}
	}
	else
		open_selected_files(list, GPOINTER_TO_INT(user_data));

	g_list_free_full(list, (GDestroyNotify) gtk_tree_path_free);
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
/*
static void on_openpdf_clicked(GtkMenuItem *menuitem, gpointer user_data)
{
	printf("open folder");
}
*/
static void on_openpdf_clicked(GtkMenuItem *menuitem, gpointer user_data)
{
	GtkTreeSelection *treesel;
	GtkTreeModel *model;
	GList *list;
	gboolean dir_found;
	
	treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(file_view));

	list = gtk_tree_selection_get_selected_rows(treesel, &model);
	dir_found = is_folder_selected(list);

	if (dir_found)
	{
		if (check_single_selection(treesel))
		{
			GtkTreePath *treepath = list->data;	/* first selected item */

			open_folder(treepath);
		}
	}
	else
		open_selected_pdffiles(list, GPOINTER_TO_INT(user_data));
		//ui_set_statusbar(TRUE, "hello");

	g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(list);
}


static void on_find_in_files(GtkMenuItem *menuitem, gpointer user_data)
{
	GtkTreeSelection *treesel;
	GtkTreeModel *model;
	GList *list;
	gchar *dir;
	gboolean is_dir = FALSE;

	treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(file_view));
	/* allow 0 or 1 selections */
	if (gtk_tree_selection_count_selected_rows(treesel) > 0 &&
		! check_single_selection(treesel))
		return;

	list = gtk_tree_selection_get_selected_rows(treesel, &model);
	is_dir = is_folder_selected(list);

	if (is_dir)
	{
		GtkTreePath *treepath = list->data;	/* first selected item */

		dir = get_tree_path_filename(treepath);
	}
	else
		dir = g_strdup(current_dir);

	g_list_free_full(list, (GDestroyNotify) gtk_tree_path_free);

	SETPTR(dir, utils_get_utf8_from_locale(dir));
	search_show_find_in_files_dialog(dir);
	g_free(dir);
}


static void on_hidden_files_clicked(GtkCheckMenuItem *item)
{
	show_hidden_files = gtk_check_menu_item_get_active(item);
	refresh();
}


static void on_hide_sidebar(void)
{
	keybindings_send_command(GEANY_KEY_GROUP_VIEW, GEANY_KEYS_VIEW_SIDEBAR);
}


static void on_show_preferences(void)
{
	plugin_show_configure(geany_plugin);
}


static GtkWidget *create_popup_menu(void)
{
	GtkWidget *item, *menu;

	menu = gtk_menu_new();

	item = ui_image_menu_item_new(GTK_STOCK_OPEN, _("Open in _Geany"));
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(on_open_clicked), NULL);
	popup_items.open = item;

	item = ui_image_menu_item_new(GTK_STOCK_OPEN, _("Open file with defaul_t"));
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(on_openpdf_clicked), NULL);
	popup_items.openpdf = item;
	
	item = ui_image_menu_item_new(GTK_STOCK_OPEN, _("Open _Folder"));
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(on_openfolder_clicked), NULL);
	popup_items.openfolder = item;
	
	item = ui_image_menu_item_new(GTK_STOCK_OPEN, _("Open _Externally"));
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(on_external_open), NULL);
	popup_items.open_external = item;

	item = gtk_separator_menu_item_new();
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REFRESH, NULL);
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(refresh), NULL);

	item = ui_image_menu_item_new(GTK_STOCK_FIND, _("_Find in Files..."));
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(on_find_in_files), NULL);
	popup_items.find_in_files = item;

	item = gtk_separator_menu_item_new();
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);

	item = gtk_check_menu_item_new_with_mnemonic(_("Show _Hidden Files"));
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(on_hidden_files_clicked), NULL);
	popup_items.show_hidden_files = item;

	item = gtk_separator_menu_item_new();
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(on_show_preferences), NULL);

	item = gtk_separator_menu_item_new();
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);

	item = ui_image_menu_item_new(GTK_STOCK_CLOSE, _("H_ide Sidebar"));
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(on_hide_sidebar), NULL);

	return menu;
}


static void on_tree_selection_changed(GtkTreeSelection *selection, gpointer data)
{
	gboolean have_sel = (gtk_tree_selection_count_selected_rows(selection) > 0);
	gboolean multi_sel = (gtk_tree_selection_count_selected_rows(selection) > 1);

	if (popup_items.open != NULL)
		gtk_widget_set_sensitive(popup_items.open, have_sel);
	if (popup_items.openpdf != NULL)
		gtk_widget_set_sensitive(popup_items.openpdf, have_sel && ! multi_sel);
	if (popup_items.open_external != NULL)
		gtk_widget_set_sensitive(popup_items.open_external, have_sel);
	if (popup_items.find_in_files != NULL)
		gtk_widget_set_sensitive(popup_items.find_in_files, have_sel && ! multi_sel);
}


static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	if (event->button == 1 && event->type == GDK_2BUTTON_PRESS)
	{
		on_openpdf_clicked(NULL, NULL);
		return TRUE;
	}
	else if (event->button == 3)
	{
		static GtkWidget *popup_menu = NULL;

		if (popup_menu == NULL)
			popup_menu = create_popup_menu();

		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(popup_items.show_hidden_files),
			show_hidden_files);
		gtk_menu_popup_at_pointer(GTK_MENU(popup_menu), (GdkEvent *) event);
		/* don't return TRUE here, unless the selection won't be changed */
	}
	return FALSE;
}

static gboolean on_key_press_combo(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	ui_set_statusbar(TRUE,"key press event");
	if (ui_is_keyval_enter_or_return(event->keyval))
	{
		//on_open_clicked(NULL, NULL);
		ui_set_statusbar(TRUE,"strg+enter");
		return TRUE;
	}
	
	if (event->keyval == GDK_KEY_space)
	{
		//on_open_clicked(NULL, GINT_TO_POINTER(TRUE));
		ui_set_statusbar(TRUE,"space");
		return TRUE;
	}
	
	/*
	if (( (event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_KP_Up) && (event->state & GDK_MOD1_MASK)) || 
		(event->keyval == GDK_KEY_BackSpace) )
	{
		on_go_up();
		return TRUE;
	}
	*/
	
	/*
	if ((event->keyval == GDK_KEY_F10 && event->state & GDK_SHIFT_MASK) || event->keyval == GDK_KEY_Menu)
	{
		GdkEventButton button_event;

		button_event.time = event->time;
		button_event.button = 3;

		on_button_press(widget, &button_event, data);
		return TRUE;
	}
	*/
	return FALSE;
}

static void ui_textEntry_changed(GtkEntry *TVCnumberEntry, gpointer user_data)
{
	gchar *text;
	char temptext[15]; // 24_24_2.009
	regex_t regex;
	int reti;
	size_t maxGroups = 3;
	regmatch_t groupArray[maxGroups];
	reti = regcomp(&regex, ".*([0-9]{2}_[0-9]{2}_[0-9]\.[0-9]{3}).*", REG_EXTENDED);
	
	text = gtk_entry_get_text(GTK_ENTRY(TVCnumberEntry));
	
	reti = regexec(&regex, text, maxGroups, groupArray, 0);
	if (!reti) {
		char sourcecopy[strlen(text)+1];
		strcpy(sourcecopy, text);
		sourcecopy[groupArray[1].rm_eo] = 0;
		strcpy(temptext, sourcecopy + groupArray[1].rm_so);
		ui_set_statusbar(TRUE, "Nummer eingegeben: %s", temptext);
		currentTVCNumber= sourcecopy + groupArray[1].rm_so;
	}
	
	regfree(&regex);
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	if (ui_is_keyval_enter_or_return(event->keyval))
	{
		on_openpdf_clicked(NULL, NULL);
		return TRUE;
	}

	if (event->keyval == GDK_KEY_space)
	{
		on_open_clicked(NULL, GINT_TO_POINTER(TRUE));
		return TRUE;
	}

	if (( (event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_KP_Up) && (event->state & GDK_MOD1_MASK)) || /* FIXME: Alt-Up doesn't seem to work! */
		(event->keyval == GDK_KEY_BackSpace) )
	{
		on_go_up();
		return TRUE;
	}

	if ((event->keyval == GDK_KEY_F10 && event->state & GDK_SHIFT_MASK) || event->keyval == GDK_KEY_Menu)
	{
		GdkEventButton button_event;

		button_event.time = event->time;
		button_event.button = 3;

		on_button_press(widget, &button_event, data);
		return TRUE;
	}

	return FALSE;
}


static void clear_filter(void)
{
	if (filter != NULL)
	{
		g_strfreev(filter);
		filter = NULL;
	}
}


static void on_clear_filter(GtkEntry *entry, gpointer user_data)
{
	clear_filter();

	gtk_entry_set_text(GTK_ENTRY(filter_entry), "");

	refresh();
}


static void on_path_entry_activate(GtkEntry *entry, gpointer user_data)
{
	gchar *new_dir = (gchar*) gtk_entry_get_text(entry);

	if (!EMPTY(new_dir))
	{
		if (g_str_has_suffix(new_dir, ".."))
		{
			on_go_up();
			return;
		}
		else if (new_dir[0] == '~')
		{
			GString *str = g_string_new(new_dir);
			utils_string_replace_first(str, "~", g_get_home_dir());
			new_dir = g_string_free(str, FALSE);
		}
		else
			new_dir = utils_get_locale_from_utf8(new_dir);
	}
	else
		new_dir = g_strdup(g_get_home_dir());

	SETPTR(current_dir, new_dir);

	on_clear_filter(NULL, NULL);
}

static void ui_textEntry_activate(GtkEntry *TVCNumberentry, gpointer user_data)
{
	gchar *text;
	char temptext[15]; // 24_24_2.009
	char newfolder[100];
	regex_t regex;
	int reti;
	size_t maxGroups = 3;
	regmatch_t groupArray[maxGroups];
	reti = regcomp(&regex, ".*([0-9]{2}_[0-9]{2}_[0-9]\.[0-9]{3}).*", REG_EXTENDED);
	
	text = gtk_entry_get_text(GTK_ENTRY(TVCnumberEntry));
	
	reti = regexec(&regex, text, maxGroups, groupArray, 0);
	if (!reti) {
		char sourcecopy[strlen(text)+1];
		strcpy(sourcecopy, text);
		sourcecopy[groupArray[1].rm_eo] = 0;
		strcpy(temptext, sourcecopy + groupArray[1].rm_so);
		// ui_set_statusbar(TRUE, "%s", temptext);
		// currentTVCNumber= sourcecopy + groupArray[1].rm_so;
		// ui_set_statusbar(TRUE, "Wechsel den Pfad zu TVC:%s", currentTVCNumber);
		sprintf(newfolder,"/home/tvc/TVC.DATA/%s", temptext);
	}

	SETPTR(current_dir, g_strdup(newfolder));
	
	regfree(&regex);
	refresh();
}

int cp(const char *to, const char *from)
{
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    int saved_errno;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
	{
        return -1;
	}

    fd_to = open(to, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd_to < 0)
	{
        goto out_error;
	}

    while (nread = read(fd_from, buf, sizeof buf), nread > 0)
    {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR)
            {
                goto out_error;
            }
        } while (nread > 0);
    }

    if (nread == 0)
    {
        if (close(fd_to) < 0)
        {
            fd_to = -1;
            goto out_error;
        }
        close(fd_from);

        /* Success! */
        return 0;
    }

  out_error:
    saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0)
        close(fd_to);

    errno = saved_errno;
    return -1;
}

static void ui_CoupEntry_activate(GtkEntry *coupentry, gpointer user_data)
{
	gchar *text;
	char temptext[15]; // 24_24_2.009
	regex_t regex, regexasterix, regexasterixone;
	int reti, retiasterix,retiasterixone ;
	size_t maxGroups = 3;
	regmatch_t groupArray[maxGroups], groupArray2[maxGroups];
	reti = regcomp(&regex, "([A-Z][0-9]{2}([1-4]|D)[0-9A-Z])", REG_EXTENDED | REG_ICASE  ); //REG_ICASE
	retiasterix = regcomp(&regexasterix, "([A-Z][0-9][0-9A-Z][1-4A-Z])", REG_EXTENDED | REG_ICASE  ); //REG_ICASE
	
	text = gtk_entry_get_text(GTK_ENTRY(coupentry));
	
	reti = regexec(&regex, text, maxGroups, groupArray, 0);
	retiasterix = regexec(&regexasterix, text, maxGroups, groupArray2, 0);
	ui_set_statusbar(TRUE, "checking for coupling");

	if (!reti) {
		char sourcecopy[strlen(text)+1];
		strcpy(sourcecopy, text);
		sourcecopy[groupArray[1].rm_eo] = 0;
		strcpy(temptext, sourcecopy + groupArray[1].rm_so);
		ui_set_statusbar(TRUE, "%s", temptext);
		// currentTVCNumber= sourcecopy + groupArray[1].rm_so;
	} else if (!retiasterix){
		char sourcecopy[strlen(text)+1];
		strcpy(sourcecopy, text);
		sourcecopy[groupArray2[1].rm_eo] = 0;
		strcpy(temptext, sourcecopy + groupArray2[1].rm_so);
		ui_set_statusbar(TRUE, "%s", temptext);
		// currentTVCNumber= sourcecopy + groupArray[1].rm_so;
	}
	//ui_set_statusbar(TRUE, "after");
	if(!reti){
		DIR *d;
		struct dirent *dir;
		d = opendir(coupling_folder);
		if (d) {
			while ((dir = readdir(d)) != NULL) {
				if (strcmp(dir->d_name, g_strconcat(coupling_prefix, temptext, ".dat", NULL)) == 0 ){
					// printf("%s\n", dir->d_name);
					// printf("%s\n",current_dir);
					cp(g_strconcat(current_dir, "/", dir->d_name, NULL), g_strconcat(coupling_folder, dir->d_name, NULL) );
				}
			}
			closedir(d);
		}
	} else if(!retiasterix){
		DIR *d;
		struct dirent *dir;
		d = opendir(coupling_folder);
		if (d) {
			while ((dir = readdir(d)) != NULL) {
				retiasterix = 1;
				//sprintf(test);
				retiasterix = regcomp(&regexasterixone, temptext , REG_EXTENDED | REG_ICASE  );
				retiasterix = regexec(&regexasterixone, dir->d_name, 0, 0, 0);
				if(!retiasterix){
					ui_set_statusbar(TRUE, "%d %s", retiasterix, dir->d_name);
					cp(g_strconcat(current_dir, "/", dir->d_name, NULL), g_strconcat(coupling_folder, dir->d_name, NULL) );
					//ui_set_statusbar(TRUE, "%s", dir->d_name);
				}
				/*
				if (strcmp(dir->d_name, g_strconcat(coupling_prefix, temptext, ".dat", NULL)) == 0 ){
					// printf("%s\n", dir->d_name);
					// printf("%s\n",current_dir);
					//cp(g_strconcat(current_dir, "/", dir->d_name, NULL), g_strconcat(coupling_folder, dir->d_name, NULL) );
				}
				*/
			}
			closedir(d);
		}
	}

	regfree(&regex);
	regfree(&regexasterix);
	refresh();
}

static void ui_combo_box_changed(GtkComboBox *combo, gpointer user_data)
{
	/* we get this callback on typing as well as choosing an item */
	if (gtk_combo_box_get_active(combo) >= 0)
		gtk_widget_activate(gtk_bin_get_child(GTK_BIN(combo)));
}


static void on_filter_activate(GtkEntry *entry, gpointer user_data)
{
	/* We use spaces for consistency with Find in Files file patterns
	 * ';' also supported like original patch. */
	filter = g_strsplit_set(gtk_entry_get_text(entry), "; ", -1);
	if (filter == NULL || g_strv_length(filter) == 0)
	{
		clear_filter();
	}
	ui_combo_box_add_to_history(GTK_COMBO_BOX_TEXT(filter_combo), NULL, 0);
	refresh();
}


static void on_filter_clear(GtkEntry *entry, gint icon_pos,
							GdkEvent *event, gpointer data)
{
	clear_filter();
	refresh();
}


static void prepare_file_view(void)
{
	GtkCellRenderer *text_renderer, *icon_renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

	file_store = gtk_list_store_new(FILEVIEW_N_COLUMNS, G_TYPE_ICON, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);

	gtk_tree_view_set_model(GTK_TREE_VIEW(file_view), GTK_TREE_MODEL(file_store));
	g_object_unref(file_store);

	icon_renderer = gtk_cell_renderer_pixbuf_new();
	text_renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(column, icon_renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, icon_renderer, "gicon", FILEVIEW_COLUMN_ICON, NULL);
	gtk_tree_view_column_pack_start(column, text_renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, text_renderer, "text", FILEVIEW_COLUMN_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(file_view), column);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(file_view), FALSE);

	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(file_view), TRUE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(file_view), FILEVIEW_COLUMN_NAME);

	ui_widget_modify_font_from_string(file_view, geany->interface_prefs->tagbar_font);

	/* tooltips */
	ui_tree_view_set_tooltip_text_column(GTK_TREE_VIEW(file_view), FILEVIEW_COLUMN_FILENAME);

	/* selection handling */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(file_view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

	/* Show the current path when the FB is first needed */
	g_signal_connect(file_view, "realize", G_CALLBACK(on_realized), NULL);
	g_signal_connect(selection, "changed", G_CALLBACK(on_tree_selection_changed), NULL);
	g_signal_connect(file_view, "button-press-event", G_CALLBACK(on_button_press), NULL);
	g_signal_connect(file_view, "key-press-event", G_CALLBACK(on_key_press), NULL);
}


#define GEANYTEST_STOCK "newreportodt"

static void add_stock_item(void)
{
	GtkIconSet *icon_set;
	GtkIconFactory *factory = gtk_icon_factory_new();
	GtkIconTheme *theme = gtk_icon_theme_get_default();
	GtkStockItem item = { (gchar*)(GEANYTEST_STOCK), (gchar*)(N_("Newreport")), 0, 0, (gchar*)(GETTEXT_PACKAGE) };

	if (gtk_icon_theme_has_icon(theme, "document-new"))
	{
		GtkIconSource *icon_source = gtk_icon_source_new();
		icon_set = gtk_icon_set_new();
		gtk_icon_source_set_icon_name(icon_source, "view-refresh");
		gtk_icon_set_add_source(icon_set, icon_source);
		gtk_icon_source_free(icon_source);
	}

	gtk_icon_factory_add(factory, item.stock_id, icon_set);
	gtk_stock_add(&item, 1);
	gtk_icon_factory_add_default(factory);

	g_object_unref(factory);
	gtk_icon_set_unref(icon_set);
}

static void fill_combo_entry (GtkWidget *combo)
{
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "EP");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "EA");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "EMP");
  //gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "industry");
}

static void copynewreport(void)
{
	gchar *locale_path;
	GeanyDocument *doc = document_get_current();
	locale_path = g_path_get_dirname(doc->file_name);
	//execlp("python", "python", "/home/michael/my_script.py", "test", (char*) NULL);
	char* command = "python3";
    char* argument_list[] = {"python3", "/home/tvc/tools/python/copArename.py", current_dir, reporttype, NULL};
	
	int pid = fork();
    if (pid == 0) {
        // Newly spawned child Process.
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
	waitpid(pid, NULL, 0);
	refresh();
}

static void finalizereport(void)
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
    char* argument_list[] = {"/home/tvc/PycharmProjects/PrepareReport/main.py", current_dir, locale_filename, NULL};
	printf("%s", locale_filename);
	
	int pid = fork();
    if (pid == 0) {
        // Newly spawned child Process.
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
	waitpid(pid, NULL, 0);
	refresh();
}

static void makereport_copy(void)
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
	char* command = "python3";
    char* argument_list[] = {"python3", "/home/tvc/tools/python/makereportcopy.py", current_dir, reporttype, NULL};
	
	//char* command = "/home/tvc/PycharmProjects/PrepareReport/main.py";
    //char* argument_list[] = {"/home/tvc/PycharmProjects/makereport/main.py", current_dir, NULL};
	//printf("%s", locale_filename);
	
	int pid = fork();
    if (pid == 0) {
        // Newly spawned child Process.
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
	waitpid(pid, NULL, 0);
	refresh();
}

static void update_current_shell(void)
{
	gchar *locale_path;
	gchar *locale_filename;
	gchar *file_basename;
	GeanyDocument *doc = document_get_current();
	if (doc != NULL)
	{
		locale_path = g_path_get_dirname(doc->file_name);
		locale_filename = doc->file_name;
		file_basename = g_path_get_basename(doc->file_name);
		
	}
	char* command = "/bin/bash";
    char* argument_list[] = {"/bin/bash", "/home/tvc/tools/shell/update_tvc_script", file_basename, locale_filename, NULL}; //"/bin/bash", 
	
	//char* command = "/mnt/hgfs/Scripts/BASH_LINUX_TVC/update_script.sh";
    //char* argument_list[] = {"/mnt/hgfs/Scripts/BASH_LINUX_TVC/update_script.sh ", locale_filename, NULL};
	
	int pid = fork();
    if (pid == 0) {
        // Newly spawned child Process.
        int status_code = execv(command, argument_list);
		
        if (status_code == -1) {
            printf("Terminated Incorrectly\n");
            return;
        }
    }
    else {
        // Old Parent process. The C program will come here
        printf("This line will be printed\n");
    }
	waitpid(pid, NULL, 0);
	
	refresh();
}

static void set_report_style(GtkComboBox * combobox, G_GNUC_UNUSED gpointer user_data)
{
	gint index;
	// get the value from 
	/*
	get_active_iter();
	get_entry_text_column();
	printf("%s",combobox.get_active_iter());
	*/
	index = gtk_combo_box_get_active(combobox);
	//printf("%d",gtk_combo_box_get_active(combobox));
	switch ( index )
	{
		// declarations
		// . . .
		case 0:
			printf("EP Anlage");
			reporttype = "EP";
			break;
		case 1:
			printf("EA Anlage");
			reporttype = "EA";
			break;
		case 2:
			printf("EMP Anlage");
			reporttype = "EMP";
			break;
		case 3:
			printf("industry");
			reporttype = "industry";
			break;
		default:
			printf("default\n");
	}
	fflush(stdout);
}

static void set_template_style(GtkComboBox * combobox, G_GNUC_UNUSED gpointer user_data)
{
	gint index;
	// get the value from 
	/*
	get_active_iter();
	get_entry_text_column();
	printf("%s",combobox.get_active_iter());
	*/
	index = gtk_combo_box_get_active(combobox);
	//printf("%d",gtk_combo_box_get_active(combobox));
	switch ( index )
	{
		// declarations
		// . . .
		case 0:
			templatetype = "EP";
			break;
		case 1:
			templatetype = "EA";
			break;
		case 2:
			templatetype = "EMP";
			break;
		case 3:
			templatetype = "industry";
			break;
		default:
			printf("default\n");
	}
	fflush(stdout);
}

static void copy_new_project_template()
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
	char* command = "python3";
    char* argument_list[] = {"python3", "/home/tvc/tools/python/copy_project_template.py", current_dir, templatetype, NULL};
	
	//char* command = "/home/tvc/PycharmProjects/PrepareReport/main.py";
    //char* argument_list[] = {"/home/tvc/PycharmProjects/makereport/main.py", current_dir, NULL};
	//printf("%s", locale_filename);
	
	int pid = fork();
    if (pid == 0) {
        // Newly spawned child Process.
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
	waitpid(pid, NULL, 0);
	refresh();	
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

static GtkWidget *make_tvcbar(void)
{
	GtkWidget *wid, *toolbar, *newreport, *iconwidget, *choosereportstyle;
	GtkWidget *label, *top, *topsub1, *topsub2;
	top = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
	topsub1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	topsub2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	
	add_stock_item();
	toolbar = gtk_toolbar_new();
	//topsub2 = gtk_toolbar_new();
	
	label = gtk_label_new(_("TVC:"));
	gtk_box_pack_start(GTK_BOX(topsub1), label, FALSE, FALSE, 0);

	TVCnumberEntry = gtk_entry_new();
	//ui_entry_add_clear_icon(GTK_ENTRY(filter_entry));
	g_signal_connect(TVCnumberEntry, "changed", G_CALLBACK(ui_textEntry_changed), NULL);
	g_signal_connect(TVCnumberEntry, "activate", G_CALLBACK(ui_textEntry_activate), NULL);
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_MENU);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	gtk_box_pack_start(GTK_BOX(topsub1), TVCnumberEntry, TRUE, TRUE, 0);
	
	// makereport_copy
	label = gtk_label_new(_("rep:"));
	gtk_box_pack_start(GTK_BOX(topsub2), label, FALSE, FALSE, 0);
	//gtk_toolbar_set_icon_size(GTK_TOOLBAR(topsub2), GTK_ICON_SIZE_MENU);
	//gtk_toolbar_set_style(GTK_TOOLBAR(topsub2), GTK_TOOLBAR_ICONS);

	choosereportstyle = gtk_combo_box_text_new();
    fill_combo_entry (choosereportstyle);
	g_signal_connect(choosereportstyle, "changed", G_CALLBACK(set_report_style), NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(choosereportstyle),0);
	gtk_box_pack_start(GTK_BOX(topsub2), choosereportstyle, FALSE, FALSE, 0);
	
	//newreport = gtk_tool_button_new(GEANYTEST_STOCK);
	//gtk_box_pack_start(GTK_BOX(topsub2), newreport, TRUE, TRUE, 0);
	//g_signal_connect(filter_combo, "changed", G_CALLBACK(ui_combo_box_changed), NULL);

	// document-edit
	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_NEW));
	gtk_widget_set_tooltip_text(wid, _("New report odt"));
	g_signal_connect(wid, "clicked", G_CALLBACK(copynewreport), NULL);
	gtk_box_pack_start(GTK_CONTAINER(topsub2), wid, FALSE, FALSE, 0);

	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_APPLY));
	gtk_widget_set_tooltip_text(wid, _("Fill report odt"));
	g_signal_connect(wid, "clicked", G_CALLBACK(finalizereport), NULL);
	gtk_box_pack_start(GTK_CONTAINER(topsub2), wid, FALSE, FALSE, 0);

	//wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_EXECUTE));  //view_refresh GEANYTEST_STOCK
	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GEANYTEST_STOCK));
	gtk_widget_set_tooltip_text(wid, _("update current shell"));
	g_signal_connect(wid, "clicked", G_CALLBACK(update_current_shell), NULL);
	gtk_box_pack_end(GTK_CONTAINER(topsub2), wid, FALSE, FALSE, 0);

	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_PROPERTIES));
	gtk_widget_set_tooltip_text(wid, _("copy makereport.sh"));
	g_signal_connect(wid, "clicked", G_CALLBACK(makereport_copy), NULL);
	//gtk_container_add(GTK_CONTAINER(topsub2), wid);
	gtk_box_pack_end(GTK_CONTAINER(topsub2), wid, FALSE, FALSE, 1);
	
	gtk_box_pack_start(GTK_BOX(top), topsub1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(top), topsub2, FALSE, FALSE, 0);

	//return toolbar;
	return top;
}

static GtkWidget *make_toolbar(void)
{
	GtkWidget *wid, *toolbar;

	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_MENU);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_GO_UP));
	gtk_widget_set_tooltip_text(wid, _("Up"));
	g_signal_connect(wid, "clicked", G_CALLBACK(on_go_up), NULL);
	gtk_container_add(GTK_CONTAINER(toolbar), wid);

	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_REFRESH));
	gtk_widget_set_tooltip_text(wid, _("Refresh"));
	g_signal_connect(wid, "clicked", G_CALLBACK(refresh), NULL);
	gtk_container_add(GTK_CONTAINER(toolbar), wid);

	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_HOME));
	gtk_widget_set_tooltip_text(wid, _("Home"));
	g_signal_connect(wid, "clicked", G_CALLBACK(on_go_home), NULL);
	gtk_container_add(GTK_CONTAINER(toolbar), wid);

	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_JUMP_TO));
	gtk_widget_set_tooltip_text(wid, _("Set path from document"));
	g_signal_connect(wid, "clicked", G_CALLBACK(on_current_path), NULL);
	gtk_container_add(GTK_CONTAINER(toolbar), wid);

	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_CONNECT));
	gtk_widget_set_tooltip_text(wid, _("TVC.DATA"));
	g_signal_connect(wid, "clicked", G_CALLBACK(on_go_tvchome), NULL);
	gtk_container_add(GTK_CONTAINER(toolbar), wid);

	return toolbar;
}


static GtkWidget *make_filterbar(void)
{
	GtkWidget *label, *filterbar;

	filterbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);

	label = gtk_label_new(_("Filter:"));

	filter_combo = gtk_combo_box_text_new_with_entry();
	filter_entry = gtk_bin_get_child(GTK_BIN(filter_combo));

	ui_entry_add_clear_icon(GTK_ENTRY(filter_entry));
	g_signal_connect(filter_entry, "icon-release", G_CALLBACK(on_filter_clear), NULL);

	gtk_widget_set_tooltip_text(filter_entry,
		_("Filter your files with the usual wildcards. Separate multiple patterns with a space."));
	g_signal_connect(filter_entry, "activate", G_CALLBACK(on_filter_activate), NULL);
	g_signal_connect(filter_combo, "changed", G_CALLBACK(ui_combo_box_changed), NULL);

	gtk_box_pack_start(GTK_BOX(filterbar), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(filterbar), filter_combo, TRUE, TRUE, 0);

	return filterbar;
}

static void fill_combo_entry2 (GtkWidget *combo)
{
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "EP");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "EA");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "EMP");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "industry");
}

static GtkWidget *make_start_project(void)
{
	/*
		fill with standardfiles for (EMP/ EP / EC  Industrie )
		fill with only the stadard script
		connect to upadt_script and optimiziing script
		copy pre odt report (EMP/EP/..) different version combobox 
	*/
	GtkWidget *wid, *toolbar, *newreport, *iconwidget, *choosereportstyle;
	GtkWidget *label, *top, *topsub1, *topsub2;
	top = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
	topsub1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	topsub2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);

	toolbar = gtk_toolbar_new();
	//topsub2 = gtk_toolbar_new();
	
	label = gtk_label_new(_("Start Project"));
	gtk_box_pack_start(GTK_BOX(topsub1), label, FALSE, FALSE, 0);

	// makereport_copy
	label = gtk_label_new(_("type:"));
	gtk_box_pack_start(GTK_BOX(topsub2), label, FALSE, FALSE, 0);
	//gtk_toolbar_set_icon_size(GTK_TOOLBAR(topsub2), GTK_ICON_SIZE_MENU);
	//gtk_toolbar_set_style(GTK_TOOLBAR(topsub2), GTK_TOOLBAR_ICONS);

	choosereportstyle = gtk_combo_box_text_new();
    fill_combo_entry2(choosereportstyle);
	g_signal_connect(choosereportstyle, "changed", G_CALLBACK(set_template_style), NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(choosereportstyle),0);
	gtk_box_pack_start(GTK_BOX(topsub2), choosereportstyle, FALSE, FALSE, 0);
	
	//newreport = gtk_tool_button_new(GEANYTEST_STOCK);
	//gtk_box_pack_start(GTK_BOX(topsub2), newreport, TRUE, TRUE, 0);
	//g_signal_connect(filter_combo, "changed", G_CALLBACK(ui_combo_box_changed), NULL);

	// document-edit
	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_NEW));
	gtk_widget_set_tooltip_text(wid, _("Project template files copied"));
	g_signal_connect(wid, "clicked", G_CALLBACK(copy_new_project_template), NULL);
	gtk_container_add(GTK_CONTAINER(topsub2), wid);
	
	/*
	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_APPLY));
	gtk_widget_set_tooltip_text(wid, _("Fill report odt"));
	g_signal_connect(wid, "clicked", G_CALLBACK(finalizereport), NULL);
	gtk_container_add(GTK_CONTAINER(topsub2), wid);
	*/
	gtk_box_pack_start(GTK_BOX(top), topsub1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(top), topsub2, FALSE, FALSE, 0);
	//gtk_box_pack_start(GTK_BOX(top), toolbar, TRUE, TRUE, 0);


	//return toolbar;
	return top;
}


static GtkWidget *make_coupbar(void)
{
	GtkWidget *wid, *toolbar;
	GtkWidget *label, *top;
	top = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);

	toolbar = gtk_toolbar_new();
	
	label = gtk_label_new(_("Kupplungsbezeichner:"));

	coupentry = gtk_entry_new();
	gtk_entry_set_placeholder_text( GTK_ENTRY(coupentry),"X3012, F5014, ... ");
	//ui_entry_add_clear_icon(GTK_ENTRY(filter_entry));

	g_signal_connect(coupentry, "activate", G_CALLBACK(ui_CoupEntry_activate), NULL);

	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_MENU);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	gtk_box_pack_start(GTK_BOX(top), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(top), coupentry, TRUE, TRUE, 0);

	//g_signal_connect(filter_combo, "changed", G_CALLBACK(ui_combo_box_changed), NULL);


	/*
	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_GO_UP));
	gtk_widget_set_tooltip_text(wid, _("Up"));
	//g_signal_connect(wid, "clicked", G_CALLBACK(on_go_up), NULL);
	gtk_container_add(GTK_CONTAINER(toolbar), wid);

	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_REFRESH));
	gtk_widget_set_tooltip_text(wid, _("Refresh"));
	//g_signal_connect(wid, "clicked", G_CALLBACK(refresh), NULL);
	gtk_container_add(GTK_CONTAINER(toolbar), wid);

	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_HOME));
	gtk_widget_set_tooltip_text(wid, _("Home"));
	//g_signal_connect(wid, "clicked", G_CALLBACK(on_go_home), NULL);
	gtk_container_add(GTK_CONTAINER(toolbar), wid);

	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_JUMP_TO));
	gtk_widget_set_tooltip_text(wid, _("Set path from document"));
	//g_signal_connect(wid, "clicked", G_CALLBACK(on_current_path), NULL);
	gtk_container_add(GTK_CONTAINER(toolbar), wid);
	*/
	//gtk_box_pack_start(GTK_BOX(top), topsub1, FALSE, FALSE, 0);
	//gtk_box_pack_start(GTK_BOX(top), toolbar, TRUE, TRUE, 0);


	//return toolbar;
	return top;
}

static gboolean completion_match_func(GtkEntryCompletion *completion, const gchar *key,
									  GtkTreeIter *iter, gpointer user_data)
{
	gchar *str;
	gboolean is_dir;
	gboolean result = FALSE;

	gtk_tree_model_get(GTK_TREE_MODEL(file_store), iter,
		FILEVIEW_COLUMN_IS_DIR, &is_dir, FILEVIEW_COLUMN_NAME, &str, -1);

	if (str != NULL && is_dir && !g_str_has_suffix(key, G_DIR_SEPARATOR_S))
	{
		/* key is something like "/tmp/te" and str is a filename like "test",
		 * so strip the path from key to make them comparable */
		gchar *base_name = g_path_get_basename(key);
		gchar *str_lowered = g_utf8_strdown(str, -1);
		result = g_str_has_prefix(str_lowered, base_name);
		g_free(base_name);
		g_free(str_lowered);
	}
	g_free(str);

	return result;
}


static gboolean completion_match_selected(GtkEntryCompletion *widget, GtkTreeModel *model,
										  GtkTreeIter *iter, gpointer user_data)
{
	gchar *str;
	gtk_tree_model_get(model, iter, FILEVIEW_COLUMN_NAME, &str, -1);
	if (str != NULL)
	{
		gchar *text = g_strconcat(current_dir, G_DIR_SEPARATOR_S, str, NULL);
		gtk_entry_set_text(GTK_ENTRY(path_entry), text);
		gtk_editable_set_position(GTK_EDITABLE(path_entry), -1);
		/* force change of directory when completion is done */
		on_path_entry_activate(GTK_ENTRY(path_entry), NULL);
		g_free(text);
	}
	g_free(str);

	return TRUE;
}


static void completion_create(void)
{
	entry_completion = gtk_entry_completion_new();

	gtk_entry_completion_set_inline_completion(entry_completion, FALSE);
	gtk_entry_completion_set_popup_completion(entry_completion, TRUE);
	gtk_entry_completion_set_text_column(entry_completion, FILEVIEW_COLUMN_NAME);
	gtk_entry_completion_set_match_func(entry_completion, completion_match_func, NULL, NULL);

	g_signal_connect(entry_completion, "match-selected",
		G_CALLBACK(completion_match_selected), NULL);

	gtk_entry_set_completion(GTK_ENTRY(path_entry), entry_completion);
}


static void load_settings(void)
{
	GKeyFile *config = g_key_file_new();

	config_file = g_strconcat(geany->app->configdir, G_DIR_SEPARATOR_S, "plugins", G_DIR_SEPARATOR_S,
		"filebrowser", G_DIR_SEPARATOR_S, "filebrowser.conf", NULL);
	g_key_file_load_from_file(config, config_file, G_KEY_FILE_NONE, NULL);

	open_cmd = utils_get_setting_string(config, "filebrowser", "open_command", OPEN_CMD);
	/* g_key_file_get_boolean defaults to FALSE */
	show_hidden_files = g_key_file_get_boolean(config, "filebrowser", "show_hidden_files", NULL);
	hide_object_files = utils_get_setting_boolean(config, "filebrowser", "hide_object_files", TRUE);
	hidden_file_extensions = utils_get_setting_string(config, "filebrowser", "hidden_file_extensions",
		".o .obj .so .dll .a .lib .pyc");
	fb_follow_path = g_key_file_get_boolean(config, "filebrowser", "fb_follow_path", NULL);
	fb_set_project_base_path = g_key_file_get_boolean(config, "filebrowser", "fb_set_project_base_path", NULL);
	coupling_prefix = utils_get_setting_string(config, "filebrowser", "coupling_prefix", NULL);
	coupling_folder = utils_get_setting_string(config, "filebrowser", "coupling_folder", NULL);
	transfer_folder = utils_get_setting_string(config, "filebrowser", "transfer_folder", NULL);

	

	g_key_file_free(config);
}


static void project_open_cb(G_GNUC_UNUSED GObject *obj, G_GNUC_UNUSED GKeyFile *config,
							G_GNUC_UNUSED gpointer data)
{
	gchar *new_dir;
	GeanyProject *project = geany->app->project;

	if (! fb_set_project_base_path || project == NULL || EMPTY(project->base_path))
		return;

	/* TODO this is a copy of project_get_base_path(), add it to the plugin API */
	if (g_path_is_absolute(project->base_path))
		new_dir = g_strdup(project->base_path);
	else
	{	/* build base_path out of project file name's dir and base_path */
		gchar *dir = g_path_get_dirname(project->file_name);

		new_dir = g_strconcat(dir, G_DIR_SEPARATOR_S, project->base_path, NULL);
		g_free(dir);
	}
	/* get it into locale encoding */
	SETPTR(new_dir, utils_get_locale_from_utf8(new_dir));

	if (! utils_str_equal(current_dir, new_dir))
	{
		SETPTR(current_dir, new_dir);
		refresh();
	}
	else
		g_free(new_dir);
}


static gpointer last_activate_path = NULL;

static void document_activate_cb(G_GNUC_UNUSED GObject *obj, GeanyDocument *doc,
								 G_GNUC_UNUSED gpointer data)
{
	gchar *new_dir;

	last_activate_path = doc->real_path;

	if (! fb_follow_path || doc->file_name == NULL || ! g_path_is_absolute(doc->file_name))
		return;

	new_dir = g_path_get_dirname(doc->file_name);
	SETPTR(new_dir, utils_get_locale_from_utf8(new_dir));

	if (! utils_str_equal(current_dir, new_dir))
	{
		SETPTR(current_dir, new_dir);
		refresh();
	}
	else
		g_free(new_dir);
}


static void document_save_cb(GObject *obj, GeanyDocument *doc, gpointer user_data)
{
	if (!last_activate_path)
		document_activate_cb(obj, doc, user_data);
}


static void kb_activate(guint key_id)
{
	gtk_notebook_set_current_page(GTK_NOTEBOOK(geany->main_widgets->sidebar_notebook), page_number);
	switch (key_id)
	{
		case KB_FOCUS_FILE_LIST:
			gtk_widget_grab_focus(file_view);
			break;
		case KB_FOCUS_PATH_ENTRY:
			gtk_widget_grab_focus(path_entry);
			break;
	}
}


void plugin_init(GeanyData *data)
{
	GeanyKeyGroup *key_group;
	GtkWidget *scrollwin, *toolbar, *filterbar, *tvcbar, *coupbar;
	GtkWidget *NewTVCbar, *NewProjectBar;

	filter = NULL;

	file_view_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	helper_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	
	tvcbar = make_tvcbar();
	gtk_box_pack_start(GTK_BOX(file_view_vbox), tvcbar, FALSE, FALSE, 0);

	toolbar = make_toolbar();
	gtk_box_pack_start(GTK_BOX(file_view_vbox), toolbar, FALSE, FALSE, 0);

	filterbar = make_filterbar();
	gtk_box_pack_start(GTK_BOX(file_view_vbox), filterbar, FALSE, FALSE, 0);

	path_combo = gtk_combo_box_text_new_with_entry();
	gtk_box_pack_start(GTK_BOX(file_view_vbox), path_combo, FALSE, FALSE, 2);
	g_signal_connect(path_combo, "changed", G_CALLBACK(ui_combo_box_changed), NULL);
	path_entry = gtk_bin_get_child(GTK_BIN(path_combo));
	g_signal_connect(path_entry, "activate", G_CALLBACK(on_path_entry_activate), NULL);

	file_view = gtk_tree_view_new();
	prepare_file_view();
	completion_create();

	popup_items.open = popup_items.open_external = popup_items.openpdf = popup_items.openfolder = popup_items.find_in_files = NULL;


	scrollwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(
		GTK_SCROLLED_WINDOW(scrollwin),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrollwin), file_view);
	gtk_box_pack_start(GTK_BOX(file_view_vbox), scrollwin, TRUE, TRUE, 0);

	coupbar = make_coupbar();
	gtk_box_pack_start(GTK_BOX(file_view_vbox), coupbar, FALSE, FALSE, 0);

	NewProjectBar = make_start_project();
	gtk_box_pack_start(GTK_BOX(file_view_vbox), NewProjectBar, FALSE, FALSE, 0);



	/* load settings before file_view "realize" callback */
	load_settings();

	gtk_widget_show_all(file_view_vbox);
	page_number = gtk_notebook_append_page(GTK_NOTEBOOK(geany->main_widgets->sidebar_notebook),
		file_view_vbox, gtk_label_new(_("Files")));

	gtk_widget_show_all(helper_box);
	page_number2 = gtk_notebook_append_page(GTK_NOTEBOOK(geany->main_widgets->sidebar_notebook),
		helper_box, gtk_label_new(_("Helper")));

	/* setup keybindings */
	key_group = plugin_set_key_group(geany_plugin, "file_browser", KB_COUNT, NULL);
	keybindings_set_item(key_group, KB_FOCUS_FILE_LIST, kb_activate,
		0, 0, "focus_file_list", _("Focus File List"), NULL);
	keybindings_set_item(key_group, KB_FOCUS_PATH_ENTRY, kb_activate,
		0, 0, "focus_path_entry", _("Focus Path Entry"), NULL);
	keybindings_set_item(key_group, OPENTER_KB, on_openTerminal, 0, 0, "openTermin", _("openTermin"), NULL);
	

	plugin_signal_connect(geany_plugin, NULL, "document-activate", TRUE,
		(GCallback) &document_activate_cb, NULL);
	plugin_signal_connect(geany_plugin, NULL, "document-save", TRUE,
		(GCallback) &document_save_cb, NULL);
}


static void save_settings(void)
{
	GKeyFile *config = g_key_file_new();
	gchar *data;
	gchar *config_dir = g_path_get_dirname(config_file);

	g_key_file_load_from_file(config, config_file, G_KEY_FILE_NONE, NULL);

	g_key_file_set_string(config, "filebrowser", "open_command", open_cmd);
	g_key_file_set_boolean(config, "filebrowser", "show_hidden_files", show_hidden_files);
	g_key_file_set_boolean(config, "filebrowser", "hide_object_files", hide_object_files);
	g_key_file_set_string(config, "filebrowser", "hidden_file_extensions", hidden_file_extensions);
	g_key_file_set_boolean(config, "filebrowser", "fb_follow_path", fb_follow_path);
	g_key_file_set_boolean(config, "filebrowser", "fb_set_project_base_path",
		fb_set_project_base_path);
	g_key_file_set_string(config, "filebrowser", "coupling_prefix", coupling_prefix);
	g_key_file_set_string(config, "filebrowser", "coupling_folder", coupling_folder);
	g_key_file_set_string(config, "filebrowser", "transfer_folder", transfer_folder);

	

	if (! g_file_test(config_dir, G_FILE_TEST_IS_DIR) && utils_mkdir(config_dir, TRUE) != 0)
	{
		dialogs_show_msgbox(GTK_MESSAGE_ERROR,
			_("Plugin configuration directory could not be created."));
	}
	else
	{
		/* write config to file */
		data = g_key_file_to_data(config, NULL, NULL);
		utils_write_file(config_file, data);
		g_free(data);
	}
	g_free(config_dir);
	g_key_file_free(config);
}


static struct
{
	GtkWidget *open_cmd_entry;
	GtkWidget *show_hidden_checkbox;
	GtkWidget *hide_objects_checkbox;
	GtkWidget *hidden_files_entry;
	GtkWidget *follow_path_checkbox;
	GtkWidget *set_project_base_path_checkbox;
	GtkWidget *coupling_folder;
	GtkWidget *coupling_prefix;
	GtkWidget *transfer_folder;
}
pref_widgets;

static void
on_configure_response(GtkDialog *dialog, gint response, gpointer user_data)
{
	if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_APPLY)
	{
		g_free(open_cmd);
		open_cmd = g_strdup(gtk_entry_get_text(GTK_ENTRY(pref_widgets.open_cmd_entry)));
		show_hidden_files = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pref_widgets.show_hidden_checkbox));
		hide_object_files = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pref_widgets.hide_objects_checkbox));
		g_free(hidden_file_extensions);
		hidden_file_extensions = g_strdup(gtk_entry_get_text(GTK_ENTRY(pref_widgets.hidden_files_entry)));
		fb_follow_path = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pref_widgets.follow_path_checkbox));
		fb_set_project_base_path = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
			pref_widgets.set_project_base_path_checkbox));
		// 
		coupling_folder = g_strdup(gtk_entry_get_text(GTK_ENTRY(pref_widgets.coupling_folder)));
		coupling_prefix = g_strdup(gtk_entry_get_text(GTK_ENTRY(pref_widgets.coupling_prefix)));
		transfer_folder = g_strdup(gtk_entry_get_text(GTK_ENTRY(pref_widgets.transfer_folder)));

		/* apply the changes */
		refresh();
	}
}


static void on_toggle_hidden(void)
{
	gboolean enabled = !gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(pref_widgets.show_hidden_checkbox));

	gtk_widget_set_sensitive(pref_widgets.hide_objects_checkbox, enabled);
	enabled &= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pref_widgets.hide_objects_checkbox));
	gtk_widget_set_sensitive(pref_widgets.hidden_files_entry, enabled);
}


GtkWidget *plugin_configure(GtkDialog *dialog)
{
	GtkWidget *label, *entry, *checkbox_of, *checkbox_hf, *checkbox_fp, *checkbox_pb, *vbox;
	GtkWidget *box, *align, *box2;

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
	box2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);

	label = gtk_label_new(_("External open command:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

	entry = gtk_entry_new();
	if (open_cmd != NULL)
		gtk_entry_set_text(GTK_ENTRY(entry), open_cmd);
	gtk_widget_set_tooltip_text(entry,
		_("The command to execute when using \"Open with\". You can use %f and %d wildcards.\n"
		  "%f will be replaced with the filename including full path\n"
		  "%d will be replaced with the path name of the selected file without the filename"));
	gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
	pref_widgets.open_cmd_entry = entry;

	gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, FALSE, 3);

	checkbox_hf = gtk_check_button_new_with_label(_("Show hidden files"));
	gtk_button_set_focus_on_click(GTK_BUTTON(checkbox_hf), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox_hf), show_hidden_files);
	gtk_box_pack_start(GTK_BOX(vbox), checkbox_hf, FALSE, FALSE, 0);
	pref_widgets.show_hidden_checkbox = checkbox_hf;
	g_signal_connect(checkbox_hf, "toggled", on_toggle_hidden, NULL);

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
	checkbox_of = gtk_check_button_new_with_label(_("Hide file extensions:"));
	gtk_button_set_focus_on_click(GTK_BUTTON(checkbox_of), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox_of), hide_object_files);
	gtk_box_pack_start(GTK_BOX(box), checkbox_of, FALSE, FALSE, 0);
	pref_widgets.hide_objects_checkbox = checkbox_of;
	g_signal_connect(checkbox_of, "toggled", on_toggle_hidden, NULL);

	entry = gtk_entry_new();
	if (hidden_file_extensions != NULL)
		gtk_entry_set_text(GTK_ENTRY(entry), hidden_file_extensions);
	gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
	pref_widgets.hidden_files_entry = entry;

	align = gtk_alignment_new(1, 0.5, 1, 1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align), 0, 0, 12, 0);
	gtk_container_add(GTK_CONTAINER(align), box);
	gtk_box_pack_start(GTK_BOX(vbox), align, FALSE, FALSE, 0);
	on_toggle_hidden();

	checkbox_fp = gtk_check_button_new_with_label(_("Follow the path of the current file"));
	gtk_button_set_focus_on_click(GTK_BUTTON(checkbox_fp), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox_fp), fb_follow_path);
	gtk_box_pack_start(GTK_BOX(vbox), checkbox_fp, FALSE, FALSE, 0);
	pref_widgets.follow_path_checkbox = checkbox_fp;

	checkbox_pb = gtk_check_button_new_with_label(_("Use the project's base directory"));
	gtk_button_set_focus_on_click(GTK_BUTTON(checkbox_pb), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox_pb), fb_set_project_base_path);
	gtk_widget_set_tooltip_text(checkbox_pb,
		_("Change the directory to the base directory of the currently opened project"));
	gtk_box_pack_start(GTK_BOX(vbox), checkbox_pb, FALSE, FALSE, 0);
	pref_widgets.set_project_base_path_checkbox = checkbox_pb;

	label = gtk_label_new(_("Foldername of Coupling Collection:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(box2), label, FALSE, FALSE, 0);

	entry = gtk_entry_new();
	if (coupling_folder != NULL)
		gtk_entry_set_text(GTK_ENTRY(entry), coupling_folder);
	gtk_widget_set_tooltip_text(entry,
		_("root folder of the coupling"));
	gtk_box_pack_start(GTK_BOX(box2), entry, FALSE, FALSE, 0);
	pref_widgets.coupling_folder = entry;

	label = gtk_label_new(_("Coupling filename prefix"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(box2), label, FALSE, FALSE, 0);

	entry = gtk_entry_new();
	if (coupling_prefix != NULL)
		gtk_entry_set_text(GTK_ENTRY(entry), coupling_prefix);
	gtk_widget_set_tooltip_text(entry,
		_("for example cn(X3012) or none(X3012)"));
	gtk_box_pack_start(GTK_BOX(box2), entry, FALSE, FALSE, 0);
	pref_widgets.coupling_prefix = entry;

	label = gtk_label_new(_("TransferFolder Win <-> Linux"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(box2), label, FALSE, FALSE, 0);

	entry = gtk_entry_new();
	if (transfer_folder != NULL)
		gtk_entry_set_text(GTK_ENTRY(entry), transfer_folder);
	gtk_widget_set_tooltip_text(entry,
		_("Optional"));
	gtk_box_pack_start(GTK_BOX(box2), entry, FALSE, FALSE, 0);
	pref_widgets.transfer_folder = entry;



	gtk_box_pack_start(GTK_BOX(vbox), box2, FALSE, FALSE, 3);

	gtk_widget_show_all(vbox);

	g_signal_connect(dialog, "response", G_CALLBACK(on_configure_response), NULL);
	return vbox;
}


void plugin_cleanup(void)
{
	save_settings();

	g_free(config_file);
	g_free(open_cmd);
	g_free(hidden_file_extensions);
	// TODO : free coupling variables
	clear_filter();
	gtk_widget_destroy(file_view_vbox);
	gtk_widget_destroy(helper_box);
	g_object_unref(G_OBJECT(entry_completion));
}