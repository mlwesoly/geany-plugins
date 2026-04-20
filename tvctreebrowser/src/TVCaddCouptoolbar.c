//
// Created by tvc on 16.10.25.
//

#ifdef HAVE_CONFIG_H
#	include "config.h"
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
#include <sys/sendfile.h>

#include "geany.h"
#include <geanyplugin.h>


#include "TVCaddCouptoolbar.h"
#include "tvctreebrowser.h"
#include "tvcaddition.h"


#define BUF_SIZE 4096*1000

GtkWidget *coupbar;

static gchar *coupling_folder = NULL;
static gchar *coupling_prefix = NULL;
static gchar *couppath = NULL;
static gchar *coupusage = NULL;
static gchar *coupmaterial  = NULL;
static gchar *siliconeduty = NULL;
static gchar *preletter = NULL;

static GtkWidget *choosecouptype, *chooseusagetype, *materialtype;
static GtkWidget *coupentry;


static void copy_coupling_file(gchar *couplingname, gchar *couplingdestname, gchar *path)
{
	const GeanyDocument *doc = document_get_current();

	if (doc != NULL) {
		const gchar *locale_path = g_path_get_dirname(doc->file_name);

		gchar *destinationfile = g_strconcat(locale_path,(gchar*)"/", couplingdestname, NULL);
		gchar *sourcefile = g_strconcat(path, couplingname, NULL);

		ui_set_statusbar(TRUE, "%s", sourcefile);
		ui_set_statusbar(TRUE, "%s", destinationfile);

		const int pid = fork();
		if (pid == 0) {
			// Newly spawned child Process.
			// int status_code = execvp(command, argument_list);
			int status_code = 0;
			struct stat stat_buf;
			int n = 1;

			const int fromfd = open(sourcefile, O_RDONLY);
			fstat(fromfd, &stat_buf);
			int tofd = open(destinationfile, O_RDWR | O_CREAT, stat_buf.st_mode);

			while (n > 0) {
				n = sendfile(tofd, fromfd, 0, BUF_SIZE);
			}

			if (status_code == -1) {
				printf("Terminated >copy_coupling_file< Incorrectly\n");
				return;
			}
			//return(0);
		}
		else {
			// Old Parent process. The C program will come here
			printf("This line will be printed\n");
		}
		waitpid(pid, NULL, 0);

		on_menu_refresh(NULL,NULL);
	}else {
		ui_set_statusbar(TRUE, "\n\n     >>   Open a file from the destination folder   <<  \n");
	}
}

static void ui_CoupEntry_activate()
{
	gchar *text;
	gchar temptext[15]; // 24_24_2.009
	const gchar *temptextpre = NULL;
	regex_t regex, regexasterix, regexasterixone, regexsilicone;
	regex_t regexpre, regexasterixpre, regexsiliconepre;
	int reti, retiasterix, retiasterixone, retisilicone;
	int retipre, retiasterixpre, retisiliconepre;

	size_t maxGroups = 3;
	regmatch_t groupArray[maxGroups], groupArray2[maxGroups], groupArray3[maxGroups];
	regmatch_t groupArraypre[maxGroups], groupArray2pre[maxGroups], groupArray3pre[maxGroups];
	reti = regcomp(&regex, "([A-Z][0-9][0-9A-Z]([1-4]|D)[0-9A-Z])", REG_EXTENDED | REG_ICASE  ); //REG_ICASE
	retiasterix = regcomp(&regexasterix, "([A-Z][0-9][0-9A-Z][1-4A-Z])", REG_EXTENDED | REG_ICASE  ); //REG_ICASE
	// retisilicone = regcomp(&regexsilicone, "([A-Z][0-9]{2}[1-4][0-9]S)", REG_EXTENDED | REG_ICASE  ); //REG_ICASE

	retipre = regcomp(&regexpre, "([0-9][0-9A-Z]([1-4]|D)[0-9A-Z])", REG_EXTENDED | REG_ICASE  ); //REG_ICASE
	retiasterixpre = regcomp(&regexasterixpre, "([0-9][0-9A-Z][1-4A-Z])", REG_EXTENDED | REG_ICASE  ); //REG_ICASE
	retisiliconepre = regcomp(&regexsiliconepre, "([0-9]{2}[1-4][0-9]S)", REG_EXTENDED | REG_ICASE  ); //REG_ICASE

	text = gtk_entry_get_text(GTK_ENTRY(coupentry));

	reti = regexec(&regex, text, maxGroups, groupArray, 0);
	retiasterix = regexec(&regexasterix, text, maxGroups, groupArray2, 0);
	// retisilicone = regexec(&regexsilicone, text, maxGroups, groupArray3, 0);
	retipre = regexec(&regexpre, text, maxGroups, groupArraypre, 0);
	retiasterixpre = regexec(&regexasterixpre, text, maxGroups, groupArray2pre, 0);
	retisiliconepre = regexec(&regexsiliconepre, text, maxGroups, groupArray3pre, 0);

	ui_set_statusbar(TRUE, "checking for coupling");

	//preletter
	/*
	if (!retisilicone){
		gchar sourcecopy[strlen(text)+1];
		strcpy(sourcecopy, text);
		sourcecopy[groupArray3[1].rm_eo] = 0;
		strcpy(temptext, sourcecopy + groupArray3[1].rm_so);
		// ui_set_statusbar(TRUE, "Silikon %s", temptext);
		reti=1;
		retiasterix=1;
		// currentTVCNumber= sourcecopy + groupArray[1].rm_so;
	} else*/
	if (!reti) {
		gchar sourcecopy[strlen(text)+1];
		strcpy(sourcecopy, text);
		sourcecopy[groupArray[1].rm_eo] = 0;
		strcpy(temptext, sourcecopy + groupArray[1].rm_so);
		// ui_set_statusbar(TRUE, "Gummi %s", temptext);
		// currentTVCNumber= sourcecopy + groupArray[1].rm_so;
	} else if (!retiasterix){
		gchar sourcecopy[strlen(text)+1];
		strcpy(sourcecopy, text);
		sourcecopy[groupArray2[1].rm_eo] = 0;
		strcpy(temptext, sourcecopy + groupArray2[1].rm_so);
		ui_set_statusbar(TRUE, "wildcard %s", temptext);
		// currentTVCNumber= sourcecopy + groupArray[1].rm_so;
	} else if (!retisiliconepre){
		gchar sourcecopy[strlen(text)+1];
		strcpy(sourcecopy, text);
		sourcecopy[groupArray3pre[1].rm_eo] = 0;
		strcpy(temptext, sourcecopy + groupArray3pre[1].rm_so);
		temptextpre = g_strconcat(preletter, temptext, NULL);
		strcpy(temptext, temptextpre);
		ui_set_statusbar(TRUE, "Silikon %s", temptextpre);
		reti=1;
		retiasterix=1;
		// currentTVCNumber= sourcecopy + groupArray[1].rm_so;
	} else if (!retipre) {
		gchar sourcecopy[strlen(text)+1];
		strcpy(sourcecopy, text);
		sourcecopy[groupArraypre[1].rm_eo] = 0;
		strcpy(temptext, sourcecopy + groupArraypre[1].rm_so);
		temptextpre = g_strconcat(preletter, temptext, NULL);
		strcpy(temptext, temptextpre);
		ui_set_statusbar(TRUE, "%s", temptextpre);
		// currentTVCNumber= sourcecopy + groupArray[1].rm_so;
	} else if (!retiasterixpre){
		gchar sourcecopy[strlen(text)+1];
		strcpy(sourcecopy, text);
		sourcecopy[groupArray2pre[1].rm_eo] = 0;
		strcpy(temptext, sourcecopy + groupArray2pre[1].rm_so);
		temptextpre = g_strconcat(preletter, temptext, NULL);
		strcpy(temptext, temptextpre);
		ui_set_statusbar(TRUE, "wildcard %s", temptextpre);
		// currentTVCNumber= sourcecopy + groupArray[1].rm_so;
	}


    coupling_folder = g_strdup("/home/tvc/Kupplungen/coups");
    //coupling_folder = g_strdup("/home/miki/Kupplungen");
	coupling_prefix = g_strdup("cn");

	gchar *wholepath = g_strconcat(coupling_folder, couppath, coupusage, coupmaterial, NULL);

	if( !reti || !retipre || !retisiliconepre ){
		DIR *d;
		struct dirent *dir;
		d = opendir(g_strconcat(coupling_folder, couppath, coupusage, coupmaterial, NULL));
		if (d) {
			while ((dir = readdir(d)) != NULL) {
				if (strcmp(dir->d_name, g_strconcat(coupling_prefix, temptext, ".dat", NULL)) == 0 ){
					//printf("%s\n", dir->d_name);
					ui_set_statusbar(TRUE, "%s", dir->d_name);
					copy_coupling_file(dir->d_name,dir->d_name,wholepath);
				}
				else if(strcmp(dir->d_name, g_strconcat(coupling_prefix, temptext, siliconeduty, ".dat", NULL)) == 0 ){
					ui_set_statusbar(TRUE, "%s", dir->d_name);

					copy_coupling_file(dir->d_name,g_strconcat(coupling_prefix, temptext, "S.dat", NULL) ,wholepath);
				}
			}
			closedir(d);
		}
	}

	/*else if(!retiasterix || !retiasterixpre){
		DIR *d;
		struct dirent *dir;
		d = opendir(g_strconcat(coupling_folder,couppath,coupusage,coupmaterial,NULL));
		if (d) {
			while ((dir = readdir(d)) != NULL) {
				retiasterix = 1;
				//sprintf(test);
				retiasterix = regcomp(&regexasterixone, temptext , REG_EXTENDED | REG_ICASE  );
				retiasterix = regexec(&regexasterixone, dir->d_name, 0, 0, 0);
				if(!retiasterix){
					ui_set_statusbar(TRUE, "%d %s", retiasterix, dir->d_name);

					copy_coupling_file(dir->d_name,wholepath);

					ui_set_statusbar(TRUE, "%s", dir->d_name);
				}

			}
			closedir(d);
		}
	}*/

	regfree(&regex);
	regfree(&regexasterix);

	on_menu_refresh(NULL, NULL);
}

static void coupfolderchooser()
{
	const int index = gtk_combo_box_get_active(GTK_COMBO_BOX(choosecouptype));
	switch ( index )
	{
		case 0:
			couppath = g_strdup("/VULASTIK_XT/");
			preletter = g_strdup("Y");
			break;
		case 1:
			couppath = g_strdup("/VULKARDAN_E/");
			preletter = g_strdup("K");
			break;
		case 2:
			couppath = g_strdup("/MEGIFLEX_B/");
			preletter = g_strdup("J");
			break;
		case 3:
			couppath = g_strdup("/RATO_S/");
			preletter = g_strdup("G");
			break;
		case 4:
			couppath = g_strdup("/RATO_R/");
			preletter = g_strdup("G");
			break;
		case 5:
			couppath = g_strdup("/RATO_DS/");
			preletter = g_strdup("A");
			break;
		case 6:
			couppath = g_strdup("/VULKARDAN_L/");
			preletter = g_strdup("K");
			break;
		case 7:
			couppath = g_strdup("/VULKARDAN_G/");
			preletter = g_strdup("K");
			break;
		case 8:
			couppath = g_strdup("/VULKARDAN_F/");
			preletter = g_strdup("F");
			break;
		case 9:
			couppath = g_strdup("/VULASTIK_L/");
			// strcpy(preletter, "X");
			preletter = g_strdup("X");
			break;
		default:
			printf("default\n");
	}
}

static void coupusagechooser()
{
	const int index = gtk_combo_box_get_active(GTK_COMBO_BOX(chooseusagetype));
	switch ( index )
	{
		case 0:
			coupusage = g_strdup("Marine/");
			break;
		case 1:
			coupusage = g_strdup("Industrie/");
			break;
		default:
			printf("default\n");
	}
}

static void materialchooser()
{
	const int index = gtk_combo_box_get_active(GTK_COMBO_BOX(materialtype));
	switch ( index )
	{
		case 0:
			coupmaterial = g_strdup("Gummi/");
			siliconeduty = g_strdup("");
			break;
		case 1:
			coupmaterial = g_strdup("Silicone/");
			siliconeduty = g_strdup("SL");
			break;
		case 2:
			coupmaterial = g_strdup("Silicone/");
			siliconeduty = g_strdup("SM");
			break;
		case 3:
			coupmaterial = g_strdup("Silicone/");
			siliconeduty = g_strdup("SC");
			break;
		default:
			printf("default\n");
	}
}

static void fill_combocoup_entry (GtkWidget *combo)
{
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "VULASTIK XT");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "VULKARDAN E");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "MEGIFLEX B");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "RATO S/S+");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "RATO R/R+");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "RATO DS/DS+");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "VULKARDAN L");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "VULKARDAN G");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "VULKARDAN F");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "VULASTIK L"); //0
}

static void fill_combocoupusage_entry (GtkWidget *combo)
{
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "Marine"); //0
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "Industrie");
}

static void fill_material_entry (GtkWidget *combo)
{
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "Rubber"); //0
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "Silicon L duty");
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "Silicon M duty");
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "Silicon C duty");
}

GtkWidget* make_coupbar(void)
{
	// GtkWidget *wid, *toolbar, *combocopubox;  delete

	coupbar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
	GtkWidget *combocoupbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);

	GtkWidget *label = gtk_label_new(_("Kupplungsbezeichner:"));

	choosecouptype = gtk_combo_box_text_new();
    fill_combocoup_entry (choosecouptype);
	g_signal_connect(choosecouptype, "changed", G_CALLBACK(coupfolderchooser), NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(choosecouptype),9);

	chooseusagetype = gtk_combo_box_text_new();
    fill_combocoupusage_entry (chooseusagetype);
	g_signal_connect(chooseusagetype, "changed", G_CALLBACK(coupusagechooser), NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(chooseusagetype),0);

	materialtype = gtk_combo_box_text_new();
	fill_material_entry (materialtype);
	g_signal_connect(materialtype, "changed", G_CALLBACK(materialchooser), NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(materialtype),0);

	gtk_box_pack_start(GTK_BOX(combocoupbox), choosecouptype, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(combocoupbox), chooseusagetype, TRUE, FALSE, 0);

	coupentry = gtk_entry_new();
	gtk_entry_set_placeholder_text( GTK_ENTRY(coupentry),"X3012, F5014, ... ");

	g_signal_connect(coupentry, "activate", G_CALLBACK(ui_CoupEntry_activate), NULL);

	gtk_box_pack_start(GTK_BOX(coupbar), label, FALSE, FALSE, 0);
	// gtk_box_pack_start(GTK_BOX(coupbar), coupentry, TRUE, TRUE, 0);
	// gtk_box_pack_start(GTK_BOX(coupbar), combocoupbox, TRUE, TRUE, 0);
	// gtk_box_pack_start(GTK_BOX(coupbar), materialtype, TRUE, TRUE, 0);

	// //g_signal_connect(filter_combo, "changed", G_CALLBACK(ui_combo_box_changed), NULL);

	return(coupbar);
}
