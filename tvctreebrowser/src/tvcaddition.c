/*
 *      tvctreebrowser.c
 *
 *
 */

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <string.h>
#include <regex.h>
#include <dirent.h>
#include <stdio.h>
#include <glob.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#include "geany.h"
#include <geanyplugin.h>

#include "tvcaddition.h"
#include "tvctreebrowser.h"


static GtkWidget *tvcexpander;
static GtkWidget *TVCNumbertoolbar;
static GtkWidget *TVCnumberEntry;
static gchar *currentTVCNumber = NULL;
static gchar *TVCnumbercurrentFile = NULL;
static gchar *templatetype = NULL;

static gchar *reporttype = NULL;


static void ui_textEntry_activate(GtkEntry *TVCNumberentry)
{
	GRegex *regex;
	GMatchInfo *match_info;
	const gchar *string = gtk_entry_get_text(GTK_ENTRY(TVCnumberEntry));
	const gchar *destinationfolder;

	regex = g_regex_new ("([0-9]{2}_[0-9]{2}_[0-9][.][0-9]{3})", G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, NULL);

	g_regex_match (regex, string, 0, &match_info);

	if (g_match_info_matches (match_info)) // while
	{
		gchar *word = g_match_info_fetch (match_info, 1);
		// destinationfolder = g_strconcat("/home/miki/TVC.DATA/", word, NULL);
		destinationfolder = g_strconcat("/home/tvc/TVC.DATA/", word, NULL);
		tvctreebrowser_chroot(g_strdup(destinationfolder));
		g_free (word);
		// g_match_info_next (match_info, NULL);
		g_free(destinationfolder);
	}
	g_match_info_free (match_info);
	g_regex_unref (regex);
}

static void ui_textEntry_changed(GtkEntry *TVCNumberentry)
{
	// im moment unbenutzte funktion, glaube ich ^^
	GRegex *regex;
	GMatchInfo *match_info;
	const gchar *string = gtk_entry_get_text(GTK_ENTRY(TVCnumberEntry));

	regex = g_regex_new ("([0-9]{2}_[0-9]{2}_[0-9][.][0-9]{3})", G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, NULL);

	g_regex_match (regex, string, 0, &match_info);
	if (g_match_info_matches (match_info)) // while
	{
		// gchar *word = g_match_info_fetch (match_info, 0);
		currentTVCNumber = g_match_info_fetch (match_info, 0);
		// g_free (word);
	}
	g_match_info_free (match_info);
	g_regex_unref (regex);
}


static gboolean TVCnumberofCurrentFile(gchar *locale_path)
{
	GRegex *regex;
	GMatchInfo *match_info;

	regex = g_regex_new ("([0-9]{2}_[0-9]{2}_[0-9][.][0-9]{3})", G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, NULL);

	g_regex_match (regex, locale_path, 0, &match_info);
	if (g_match_info_matches (match_info)) // while
	{
		gchar *word = g_match_info_fetch (match_info, 0);
		//g_strlcpy(TVCnumbercurrentFile, word, 11);
		TVCnumbercurrentFile = g_match_info_fetch (match_info, 0);
		ui_set_statusbar(TRUE, "Nummer eingegeben: %s", TVCnumbercurrentFile);
		g_free (word);
		g_match_info_free (match_info);
		g_regex_unref (regex);
		return(TRUE);
	}else
	{
		g_match_info_free (match_info);
		g_regex_unref (regex);
		return(FALSE);
	}
}


static void set_template_style(GtkComboBox * combobox, G_GNUC_UNUSED gpointer user_data)
{
	gint index;
	index = gtk_combo_box_get_active(combobox);
	switch ( index )
	{
		case 0:
			templatetype = g_strdup("EP");
			break;
		case 1:
			templatetype = g_strdup("EA");
			break;
		case 2:
			templatetype = g_strdup("EMP");
			break;
		case 3:
			templatetype = g_strdup("industry");
			break;
		default:
			printf("default\n");
	}
}


static void copy_new_project_template()
{
	GError *error = NULL;
	GPid child_pid;

	gchar *locale_path;
	gchar *locale_filename;
	GeanyDocument *doc = document_get_current();
	if (doc != NULL)
	{
		locale_path = g_path_get_dirname(doc->file_name);
		locale_filename = doc->file_name;
	}

	char* command = "python3";
	char* argument_list[] = {"python3", "/home/tvc/tools/python/copy_project_template.py", (char*) addressbar_last_address, (char*)templatetype, NULL};

	// Spawn
	g_spawn_async(NULL, argument_list, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &child_pid, &error); //G_SPAWN_SEARCH_PATH

	if (error != NULL) {
		g_error("Failed to spawn: %s", error->message);
		ui_set_statusbar(TRUE, "Failed to copy : %s", error->message);
		g_error_free(error);
	}
	g_spawn_close_pid(child_pid);

	on_menu_refresh(NULL,NULL);
}

//
static void update_current_shell(void)
{
	GError *error = NULL;
	GPid child_pid;
	GeanyDocument *doc = document_get_current();
	if (doc != NULL)
	{
		gchar *locale_path = g_path_get_dirname(doc->file_name);
		gchar *locale_filename = doc->file_name;
		gchar *file_basename = g_path_get_basename(doc->file_name);

		gchar *command = "/bin/bash";
		gchar *argument_list[] = {"/bin/bash", "/home/tvc/tools/shell/update_tvc_script", file_basename, locale_filename,(char *) NULL}; //"/bin/bash",

		// Spawn
		g_spawn_async(NULL, argument_list, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &child_pid, &error); //G_SPAWN_SEARCH_PATH

		if (error != NULL) {
			g_error("Failed to spawn: %s", error->message);
			ui_set_statusbar(TRUE, "Failed to copy : %s", error->message);
			g_error_free(error);
		}
		g_spawn_close_pid(child_pid);
	}

}


static void makereport_copy(void)
{
	const GeanyDocument *doc = document_get_current();
	gchar *locale_path = g_path_get_dirname(doc->file_name);

	g_autoptr (GFile) source, destination;
	char *source_path, *destination_path;
	g_autoptr (GError) error = NULL;
	gboolean success;

	if (locale_path != NULL)
	{
		source_path = "/home/tvc/tools/extra/makereport.sh";
		destination_path = g_strconcat(locale_path,(gchar*)"/",(gchar*)"makereport.sh",NULL);

		source = g_file_new_for_path (source_path);
		destination = g_file_new_for_path (destination_path);

		success = g_file_copy (source, destination, G_FILE_COPY_BACKUP, NULL, NULL, NULL, &error);

		if (!success) {
			g_warning ("Failed to copy '%s' to '%s': %s",
					   source_path,
			  destination_path,
			  error->message);
			ui_set_statusbar(TRUE, "Failed to copy : %s", error->message);
		}

	}else{
		gboolean test = TVCnumberofCurrentFile(addressbar_last_address);
		if (test == TRUE )
		{
			source_path = "/home/tvc/tools/extra/makereport.sh";
			destination_path = g_strconcat(addressbar_last_address,(gchar*)"/",(gchar*)"makereport.sh",NULL);

			source = g_file_new_for_path (source_path);
			destination = g_file_new_for_path (destination_path);

			success = g_file_copy (source, destination, G_FILE_COPY_BACKUP, NULL, NULL, NULL, &error);

			if (!success) {
				g_warning ("Failed to copy '%s' to '%s': %s",
						   source_path,
			   destination_path,
			   error->message);
				ui_set_statusbar(TRUE, "Failed to copy %s", error->message);
			}
		}

	}
	on_menu_refresh(NULL,NULL);

}


static void set_report_style(GtkComboBox * combobox, G_GNUC_UNUSED gpointer user_data)
{
	const gint index = gtk_combo_box_get_active(combobox);
	switch ( index )
	{
		case 0:
			reporttype = g_strdup("EP");
			break;
		case 1:
			reporttype = g_strdup("EA");
			break;
		case 2:
			reporttype = g_strdup("EMP");
			break;
		default:
			reporttype = g_strdup("EP");
			printf("default\n");
	}
}


static void copynewreport(void)
{
	const GeanyDocument *doc = document_get_current();
	gchar *locale_path = g_path_get_dirname(doc->file_name);
	// alternative einbauen, falls keine datei ausgewaehlt, schau root folder
	if (locale_path != NULL) {
		// regex current TVC number out of path string
		TVCnumberofCurrentFile(locale_path);
		if (TVCnumbercurrentFile != NULL )
		{
			g_autoptr (GFile) source, destination;
			char *source_path, *destination_path;
			g_autoptr (GError) error = NULL;
			gboolean success;

			source_path = g_strconcat((char*)"/home/tvc/Templates/XX_YY_A.nnn_", reporttype, (char*)".odt", NULL);
			destination_path = g_strconcat(locale_path,(char*)"/", TVCnumbercurrentFile, (char*)"-I.odt", NULL);

			// g_file_copy () operates on GFile instances, not raw strings, so we need
			// to construct such objects from our paths before we can do any copying.
			source = g_file_new_for_path (source_path);
			destination = g_file_new_for_path (destination_path);

			// Now actually copy the file.
			success = g_file_copy (source, destination, G_FILE_COPY_NONE, NULL, NULL, NULL, &error);

			// And then check for errors.
			if (!success) {
				g_warning ("Failed to copy '%s' to '%s': %s",
						   source_path,
			   destination_path,
			   error->message);
				ui_set_statusbar(TRUE, "Failed to copy : %s", error->message);
				g_error_free(error);
			}
			on_menu_refresh(NULL,NULL);
		}
		else
		{
			ui_set_statusbar(TRUE, "couldn't find a TVC number");
		}
	}
}


void TVCnumberFolderChange()
{

	// const GtkWidget *wid;

	TVCNumbertoolbar = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(TVCNumbertoolbar), GTK_ICON_SIZE_MENU);
	gtk_toolbar_set_style(GTK_TOOLBAR(TVCNumbertoolbar), GTK_TOOLBAR_ICONS);


	/* #if GTK_CHECK_VERSION(3, 10, 0)
	 *    wid = gtk_image_new_from_icon_name("go-up", GTK_ICON_SIZE_SMALL_TOOLBAR);
	 *    wid = GTK_WIDGET(gtk_tool_button_new(wid, NULL));
	 *    #else
	 *    wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_GO_UP));
	 *    #endif
	 */
	//gtk_widget_set_tooltip_text(wid, _("Go up"));
	//g_signal_connect(wid, "clicked", G_CALLBACK(on_button_go_up), NULL);


	TVCnumberEntry = gtk_entry_new();
	//gtk_entry_set_max_length(GTK_ENTRY(TVCnumberEntry), 11);
	gtk_entry_set_placeholder_text( GTK_ENTRY(TVCnumberEntry),"TVC Number AA_BB_C.XYZ");

	g_signal_connect(TVCnumberEntry, "changed", G_CALLBACK(ui_textEntry_changed), NULL);
	g_signal_connect(TVCnumberEntry, "activate", G_CALLBACK(ui_textEntry_activate), NULL);

	gtk_box_pack_start(GTK_BOX(TVCNumbertoolbar), TVCnumberEntry, TRUE, TRUE, 0);
}


static void fill_comboreport_entry (GtkWidget *combo)
{
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "EP");
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "EA");
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "EMP");
}


GtkWidget* make_tvcbar(void)
{
	GtkWidget *wid, *toolbar, *newreport, *iconwidget;
	GtkWidget *label, *top, *topsub1, *topsub2;
	top = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
	topsub1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	topsub2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);

	//add_stock_item();
	tvcexpander = gtk_expander_new ("addition");

	label = gtk_label_new(_("rep:"));
	gtk_box_pack_start(GTK_BOX(topsub2), label, FALSE, FALSE, 0);
	//gtk_toolbar_set_icon_size(GTK_TOOLBAR(topsub2), GTK_ICON_SIZE_MENU);
	//gtk_toolbar_set_style(GTK_TOOLBAR(topsub2), GTK_TOOLBAR_ICONS);

	GtkWidget *choosereportstyle = gtk_combo_box_text_new();
	fill_comboreport_entry (choosereportstyle);
	g_signal_connect(choosereportstyle, "changed", G_CALLBACK(set_report_style), NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(choosereportstyle),0);
	gtk_box_pack_start(GTK_BOX(topsub2), choosereportstyle, FALSE, FALSE, 0);

	// document-edit
	#if GTK_CHECK_VERSION(3, 10, 0)
	wid = gtk_image_new_from_icon_name("accessories-text-editor-symbolic", GTK_ICON_SIZE_MENU);
	wid = GTK_WIDGET(gtk_tool_button_new(wid, NULL));
	#else
	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_NEW));
	#endif
	gtk_widget_set_tooltip_text(wid, _("New report odt"));
	g_signal_connect(wid, "clicked", G_CALLBACK(copynewreport), NULL);
	gtk_box_pack_start(GTK_BOX(topsub2), wid, FALSE, FALSE, 0);

	/* weg damit -> context menu
	 *    #if GTK_CHECK_VERSION(3, 10, 0)
	 *        wid = gtk_image_new_from_icon_name("insert-text-symbolic", GTK_ICON_SIZE_MENU);
	 *        wid = GTK_WIDGET(gtk_tool_button_new(wid, NULL));
	 *    #else
	 *        wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_APPLY));
	 *    #endif
	 *    gtk_widget_set_tooltip_text(wid, _("Fill report odt"));
	 *    g_signal_connect(wid, "clicked", G_CALLBACK(finalizereport), NULL);
	 *    gtk_box_pack_start(GTK_BOX(topsub2), wid, FALSE, FALSE, 0);
	 */

	#if GTK_CHECK_VERSION(3, 10, 0)
	wid = gtk_image_new_from_icon_name("software-update-available-symbolic", GTK_ICON_SIZE_MENU);
	wid = GTK_WIDGET(gtk_tool_button_new(wid, NULL));
	#else
	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GEANYTEST_STOCK));
	#endif
	gtk_widget_set_tooltip_text(wid, _("update current shell"));
	g_signal_connect(wid, "clicked", G_CALLBACK(update_current_shell), NULL);
	gtk_box_pack_end(GTK_BOX(topsub2), wid, FALSE, FALSE, 0);


	#if GTK_CHECK_VERSION(3, 10, 0)
	wid = gtk_image_new_from_icon_name("system-run-symbolic", GTK_ICON_SIZE_MENU);
	wid = GTK_WIDGET(gtk_tool_button_new(wid, NULL));
	#else
	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_PROPERTIES));
	#endif
	//wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_PROPERTIES));
	gtk_widget_set_tooltip_text(wid, _("copy makereport.sh"));
	g_signal_connect(wid, "clicked", G_CALLBACK(makereport_copy), NULL);
	gtk_box_pack_end(GTK_BOX(topsub2), wid, FALSE, FALSE, 1);

	gtk_box_pack_start(GTK_BOX(top), topsub1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(top), topsub2, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(tvcexpander), top);
	gtk_expander_set_expanded(GTK_EXPANDER(tvcexpander),TRUE);

	gtk_widget_show_all(tvcexpander);
	return(tvcexpander);
}



void add_to_sidebar_addition(GtkWidget *sidebar_vbox_bars){
	gtk_box_pack_start(GTK_BOX(sidebar_vbox_bars), TVCNumbertoolbar, FALSE, TRUE, 1);
	gtk_box_pack_start(GTK_BOX(sidebar_vbox_bars), tvcexpander, FALSE, TRUE, 1);
}


/*
 * static void ui_CoupEntry_activate(GtkEntry *coupentry, gpointer user_data)
 * {
 *    gchar *text;
 *    char temptext[15]; // 24_24_2.009
 *    regex_t regex, regexasterix, regexasterixone;
 *    int reti, retiasterix,retiasterixone ;
 *    size_t maxGroups = 3;
 *    regmatch_t groupArray[maxGroups], groupArray2[maxGroups];
 *    reti = regcomp(&regex, "([A-Z][0-9]{2}([1-4]|D)[0-9A-Z])", REG_EXTENDED | REG_ICASE  ); //REG_ICASE
 *    retiasterix = regcomp(&regexasterix, "([A-Z][0-9][0-9A-Z][1-4A-Z])", REG_EXTENDED | REG_ICASE  ); //REG_ICASE
 *
 *    text = gtk_entry_get_text(GTK_ENTRY(coupentry));
 *
 *    reti = regexec(&regex, text, maxGroups, groupArray, 0);
 *    retiasterix = regexec(&regexasterix, text, maxGroups, groupArray2, 0);
 *    ui_set_statusbar(TRUE, "checking for coupling");
 *
 *    if (!reti) {
 *        char sourcecopy[strlen(text)+1];
 *        strcpy(sourcecopy, text);
 *        sourcecopy[groupArray[1].rm_eo] = 0;
 *        strcpy(temptext, sourcecopy + groupArray[1].rm_so);
 *        ui_set_statusbar(TRUE, "%s", temptext);
 *        // currentTVCNumber= sourcecopy + groupArray[1].rm_so;
 *    } else if (!retiasterix){
 *        char sourcecopy[strlen(text)+1];
 *        strcpy(sourcecopy, text);
 *        sourcecopy[groupArray2[1].rm_eo] = 0;
 *        strcpy(temptext, sourcecopy + groupArray2[1].rm_so);
 *        ui_set_statusbar(TRUE, "%s", temptext);
 *        // currentTVCNumber= sourcecopy + groupArray[1].rm_so;
 *    }
 *    //ui_set_statusbar(TRUE, "after");
 *    if(!reti){
 *        DIR *d;
 *        struct dirent *dir;
 *        d = opendir(coupling_folder);
 *        if (d) {
 *            while ((dir = readdir(d)) != NULL) {
 *                if (strcmp(dir->d_name, g_strconcat(coupling_prefix, temptext, ".dat", NULL)) == 0 ){
 *                    // printf("%s\n", dir->d_name);
 *                    // printf("%s\n",current_dir);
 *                    cp(g_strconcat(current_dir, "/", dir->d_name, NULL), g_strconcat(coupling_folder, dir->d_name, NULL) );
 *                }
 *            }
 *            closedir(d);
 *        }
 *    } else if(!retiasterix){
 *        DIR *d;
 *        struct dirent *dir;
 *        d = opendir(coupling_folder);
 *        if (d) {
 *            while ((dir = readdir(d)) != NULL) {
 *                retiasterix = 1;
 *                //sprintf(test);
 *                retiasterix = regcomp(&regexasterixone, temptext , REG_EXTENDED | REG_ICASE  );
 *                retiasterix = regexec(&regexasterixone, dir->d_name, 0, 0, 0);
 *                if(!retiasterix){
 *                    ui_set_statusbar(TRUE, "%d %s", retiasterix, dir->d_name);
 *                    cp(g_strconcat(current_dir, "/", dir->d_name, NULL), g_strconcat(coupling_folder, dir->d_name, NULL) );
 *                    //ui_set_statusbar(TRUE, "%s", dir->d_name);
 *                }
 *
 *                //if (strcmp(dir->d_name, g_strconcat(coupling_prefix, temptext, ".dat", NULL)) == 0 ){
 *                //  // printf("%s\n", dir->d_name);
 *                //  // printf("%s\n",current_dir);
 *                //  //cp(g_strconcat(current_dir, "/", dir->d_name, NULL), g_strconcat(coupling_folder, dir->d_name, NULL) );
 *                //}
 *
 *            }
 *            closedir(d);
 *        }
 *    }
 *
 *    regfree(&regex);
 *    regfree(&regexasterix);
 *    refresh();
 * }
 *
 * void overwrite_with_spaces() // in xtvc untergebracht
 * {
 *    GeanyDocument *doc = document_get_current();
 *    ScintillaObject *sci = doc->editor->sci;
 *
 *    gint start = sci_get_selection_start(sci);
 *    gint end = sci_get_selection_end(sci);
 *    gint line_start = sci_get_line_from_position(sci,start);
 *    gint line_end = sci_get_line_from_position(sci,end);
 *
 *    if(line_end == line_start){
 *        if(sci_get_selection_mode(sci) == SC_SEL_RECTANGLE){
 *            ui_set_statusbar(TRUE,"bitte nur in einer zeile markieren");
 *        }else{
 *            gchar *text = sci_get_selection_contents(sci);
 *
 *            memset(text, ' ', end-start);
 *            sci_replace_sel(sci, text);
 *            g_free(text);
 *        }
 *    }
 * }
 *
 * static void fill_combo_entry2 (GtkWidget *combo)
 * {
 *  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "EP");
 *  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "EA");
 *  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "EMP");
 *  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "industry");
 * }
 *
 * static GtkWidget *make_start_project(void)
 * {
 *
 *    GtkWidget *wid, *toolbar, *newreport, *iconwidget, *choosereportstyle;
 *    GtkWidget *label, *top, *topsub1, *topsub2;
 *    top = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
 *    topsub1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
 *    topsub2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
 *
 *    toolbar = gtk_toolbar_new();
 *    //topsub2 = gtk_toolbar_new();
 *
 *    label = gtk_label_new(_("Start Project"));
 *    gtk_box_pack_start(GTK_BOX(topsub1), label, FALSE, FALSE, 0);
 *
 *    // makereport_copy
 *    label = gtk_label_new(_("type:"));
 *    gtk_box_pack_start(GTK_BOX(topsub2), label, FALSE, FALSE, 0);
 *    //gtk_toolbar_set_icon_size(GTK_TOOLBAR(topsub2), GTK_ICON_SIZE_MENU);
 *    //gtk_toolbar_set_style(GTK_TOOLBAR(topsub2), GTK_TOOLBAR_ICONS);
 *
 *    choosereportstyle = gtk_combo_box_text_new();
 *    fill_combo_entry2(choosereportstyle);
 *    g_signal_connect(choosereportstyle, "changed", G_CALLBACK(set_template_style), NULL);
 *    gtk_combo_box_set_active(GTK_COMBO_BOX(choosereportstyle),0);
 *    gtk_box_pack_start(GTK_BOX(topsub2), choosereportstyle, FALSE, FALSE, 0);
 *
 *    //newreport = gtk_tool_button_new(GEANYTEST_STOCK);
 *    //gtk_box_pack_start(GTK_BOX(topsub2), newreport, TRUE, TRUE, 0);
 *    //g_signal_connect(filter_combo, "changed", G_CALLBACK(ui_combo_box_changed), NULL);
 *
 *    // document-edit
 *    wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_NEW));
 *    gtk_widget_set_tooltip_text(wid, _("Project template files copied"));
 *    g_signal_connect(wid, "clicked", G_CALLBACK(copy_new_project_template), NULL);
 *    gtk_container_add(GTK_CONTAINER(topsub2), wid);
 *
 *
 *    gtk_box_pack_start(GTK_BOX(top), topsub1, FALSE, FALSE, 0);
 *    gtk_box_pack_start(GTK_BOX(top), topsub2, FALSE, FALSE, 0);
 *    //gtk_box_pack_start(GTK_BOX(top), toolbar, TRUE, TRUE, 0);
 *
 *
 *    //return toolbar;
 *    return top;
 * }
 *
 *
 * static GtkWidget *make_coupbar(void)
 * {
 *    GtkWidget *wid, *toolbar;
 *    GtkWidget *label, *top;
 *    top = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
 *
 *    toolbar = gtk_toolbar_new();
 *
 *    label = gtk_label_new(_("Kupplungsbezeichner:"));
 *
 *    coupentry = gtk_entry_new();
 *    gtk_entry_set_placeholder_text( GTK_ENTRY(coupentry),"X3012, F5014, ... ");
 *    //ui_entry_add_clear_icon(GTK_ENTRY(filter_entry));
 *
 *    g_signal_connect(coupentry, "activate", G_CALLBACK(ui_CoupEntry_activate), NULL);
 *
 *    gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_MENU);
 *    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
 *
 *    gtk_box_pack_start(GTK_BOX(top), label, FALSE, FALSE, 0);
 *    gtk_box_pack_start(GTK_BOX(top), coupentry, TRUE, TRUE, 0);
 *
 *    //g_signal_connect(filter_combo, "changed", G_CALLBACK(ui_combo_box_changed), NULL);
 *
 *
 *
 *    //gtk_box_pack_start(GTK_BOX(top), topsub1, FALSE, FALSE, 0);
 *    //gtk_box_pack_start(GTK_BOX(top), toolbar, TRUE, TRUE, 0);
 *
 *
 *    //return toolbar;
 *    return top;
 * }
 *
 */
/*
 * static void open_selected_pdffiles(GList *list, gboolean do_not_focus)
 * {
 *    //GSList *files = NULL;
 *    GList *item;
 *    //GeanyDocument *doc;
 *    GError* err = NULL;
 *    //gchar *fullpath = NULL;
 *    for (item = list; item != NULL; item = g_list_next(item))
 *    {
 *        GtkTreePath *treepath = item->data;
 *        gchar *fname = get_tree_path_filename(treepath);
 *        gchar *fpath = "file://";
 *
 *        //gchar *fullpath = NULL;
 *
 *        gchar *fullpath = malloc(strlen(fpath) + strlen(fname) + 1);
 *        //ui_set_statusbar(TRUE, "%s", fullpath);
 *        //ui_set_statusbar(TRUE, "\n");
 *        strcpy(fullpath,fpath);
 *        strcat(fullpath,fname);
 *
 *        //ui_set_statusbar(TRUE, "%s", fullpath);
 *        //ui_set_statusbar(TRUE, fpath);
 *        gtk_show_uri_on_window(NULL, fullpath, GDK_CURRENT_TIME, &err);
 *        //gtk_show_uri(gdk_screen_get_default(), fname, GDK_CURRENT_TIME, &err);
 *        //files = g_slist_prepend(files, fname);
 *        free(fullpath);
 *    }
 *    //files = g_slist_reverse(files);
 *    //document_open_files(files, FALSE, NULL, NULL);
 *    //doc = document_get_current();
 *    //if (doc != NULL && ! do_not_focus)
 *    //  keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
 *
 *    //g_slist_foreach(files, (GFunc) g_free, NULL); // free filenames
 *    //g_slist_free(files);
 *
 * }
 */

// on_menu_fillreport

