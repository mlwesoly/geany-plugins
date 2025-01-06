
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
static	GtkWidget *TVCnumberEntry;
static gchar *currentTVCNumber = NULL;


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
		sprintf(newfolder,"/home/tvc/TVC.DATA/%s", temptext);
	}

	// SETPTR(current_dir, g_strdup(newfolder));
	// hier ordner neu setzen 
	tvctreebrowser_chroot(g_strdup(newfolder));

	regfree(&regex);
	//refresh();
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



void create_sidebar_addition(){
    
    GtkWidget 			*wid;

    //toolbar2 = gtk_toolbar_new();
	toolbar2 = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar2), GTK_ICON_SIZE_MENU);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar2), GTK_TOOLBAR_ICONS);

#if GTK_CHECK_VERSION(3, 10, 0)
	wid = gtk_image_new_from_icon_name("go-up", GTK_ICON_SIZE_SMALL_TOOLBAR);
	wid = GTK_WIDGET(gtk_tool_button_new(wid, NULL));
#else
	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_GO_UP));
#endif
	// gtk_widget_set_tooltip_text(wid, _("Go up"));
	//g_signal_connect(wid, "clicked", G_CALLBACK(on_button_go_up), NULL);
	
    TVCnumberEntry = gtk_entry_new();
	gtk_entry_set_placeholder_text( GTK_ENTRY(TVCnumberEntry),"TVC Number AA_BB_C.XYZ");

	g_signal_connect(TVCnumberEntry, "changed", G_CALLBACK(ui_textEntry_changed), NULL);
	g_signal_connect(TVCnumberEntry, "activate", G_CALLBACK(ui_textEntry_activate), NULL);

	gtk_box_pack_start(GTK_BOX(toolbar2), TVCnumberEntry, TRUE, TRUE, 0);
    //gtk_container_add(GTK_CONTAINER(toolbar2), TVCnumberEntry);
}

void add_to_sidebar_addition(){
    gtk_box_pack_start(GTK_BOX(sidebar_vbox_bars), 			toolbar2, 			FALSE, TRUE,  1);
}


/*
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

	//g_slist_foreach(files, (GFunc) g_free, NULL);	// free filenames
	//g_slist_free(files);
	
}
*/

void on_openTerminal()
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
/*
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
				
				//if (strcmp(dir->d_name, g_strconcat(coupling_prefix, temptext, ".dat", NULL)) == 0 ){
				//	// printf("%s\n", dir->d_name);
				//	// printf("%s\n",current_dir);
				//	//cp(g_strconcat(current_dir, "/", dir->d_name, NULL), g_strconcat(coupling_folder, dir->d_name, NULL) );
				//}
				
			}
			closedir(d);
		}
	}

	regfree(&regex);
	regfree(&regexasterix);
	refresh();
}

void overwrite_with_spaces() // in xtvc untergebracht
{
	GeanyDocument *doc = document_get_current();
	ScintillaObject *sci = doc->editor->sci;

	gint start = sci_get_selection_start(sci);
	gint end = sci_get_selection_end(sci);
	gint line_start = sci_get_line_from_position(sci,start);
	gint line_end = sci_get_line_from_position(sci,end);

	if(line_end == line_start){
		if(sci_get_selection_mode(sci) == SC_SEL_RECTANGLE){
			ui_set_statusbar(TRUE,"bitte nur in einer zeile markieren");
		}else{
			gchar *text = sci_get_selection_contents(sci);

			memset(text, ' ', end-start);
			sci_replace_sel(sci, text);
			g_free(text);
		}
	}
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



	//gtk_box_pack_start(GTK_BOX(top), topsub1, FALSE, FALSE, 0);
	//gtk_box_pack_start(GTK_BOX(top), toolbar, TRUE, TRUE, 0);


	//return toolbar;
	return top;
}

*/