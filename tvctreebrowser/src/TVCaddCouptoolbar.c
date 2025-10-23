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

#include "geany.h"
#include <geanyplugin.h>


#include "TVCaddCouptoolbar.h"
#include "tvctreebrowser.h"
#include "tvcaddition.h"


static gchar *coupling_folder = NULL;
static gchar *coupling_prefix = NULL;
static gchar *transfer_folder = NULL;

static gchar *couppath = NULL;

static gchar *coupusage = NULL;
static gchar *preletter = NULL;
static GtkComboBox * combobox;

GtkWidget *coupbar;
static GtkWidget *coupentry;

static void ui_CoupEntry_activate()
{
	gchar *text;
	gchar temptext[15]; // 24_24_2.009
	const gchar *temptextpre = NULL;
	regex_t regex, regexasterix, regexasterixone, regexsilicone;
	regex_t regexpre, regexasterixpre, regexsiliconepre;
	int reti, retiasterix, retiasterixone, retisilicone;
	int retipre, retiasterixpre, retisiliconepre;
	gchar *material="";
	size_t maxGroups = 3;
	regmatch_t groupArray[maxGroups], groupArray2[maxGroups], groupArray3[maxGroups];
	regmatch_t groupArraypre[maxGroups], groupArray2pre[maxGroups], groupArray3pre[maxGroups];
	reti = regcomp(&regex, "([A-Z][0-9][0-9A-Z]([1-4]|D)[0-9A-Z])", REG_EXTENDED | REG_ICASE  ); //REG_ICASE
	retiasterix = regcomp(&regexasterix, "([A-Z][0-9][0-9A-Z][1-4A-Z])", REG_EXTENDED | REG_ICASE  ); //REG_ICASE
	retisilicone = regcomp(&regexsilicone, "([A-Z][0-9]{2}[1-4][0-9]S)", REG_EXTENDED | REG_ICASE  ); //REG_ICASE

	retipre = regcomp(&regexpre, "([0-9][0-9A-Z]([1-4]|D)[0-9A-Z])", REG_EXTENDED | REG_ICASE  ); //REG_ICASE
	retiasterixpre = regcomp(&regexasterixpre, "([0-9][0-9A-Z][1-4A-Z])", REG_EXTENDED | REG_ICASE  ); //REG_ICASE
	retisiliconepre = regcomp(&regexsiliconepre, "([0-9]{2}[1-4][0-9]S)", REG_EXTENDED | REG_ICASE  ); //REG_ICASE

	text = gtk_entry_get_text(GTK_ENTRY(coupentry));

	reti = regexec(&regex, text, maxGroups, groupArray, 0);
	retiasterix = regexec(&regexasterix, text, maxGroups, groupArray2, 0);
	retisilicone = regexec(&regexsilicone, text, maxGroups, groupArray3, 0);
	retipre = regexec(&regexpre, text, maxGroups, groupArraypre, 0);
	retiasterixpre = regexec(&regexasterixpre, text, maxGroups, groupArray2pre, 0);
	retisiliconepre = regexec(&regexsiliconepre, text, maxGroups, groupArray3pre, 0);

	ui_set_statusbar(TRUE, "checking for coupling");

	//preletter
	if (!retisilicone){
		gchar sourcecopy[strlen(text)+1];
		strcpy(sourcecopy, text);
		sourcecopy[groupArray3[1].rm_eo] = 0;
		strcpy(temptext, sourcecopy + groupArray3[1].rm_so);
		ui_set_statusbar(TRUE, "Silikon %s", temptext);
		reti=1;
		retiasterix=1;
		// currentTVCNumber= sourcecopy + groupArray[1].rm_so;
	} else if (!reti) {
		gchar sourcecopy[strlen(text)+1];
		strcpy(sourcecopy, text);
		sourcecopy[groupArray[1].rm_eo] = 0;
		strcpy(temptext, sourcecopy + groupArray[1].rm_so);
		ui_set_statusbar(TRUE, "Gummi %s", temptext);
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
		ui_set_statusbar(TRUE, "Gummi %s", temptextpre);
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

	if(!reti||!retipre){
		material = "Gummi/";
	}else if(!retisilicone||!retisiliconepre){
		material = "Silikon/";
	}

    coupling_folder = "/home/tvc/Kupplungen/coups";
	coupling_prefix = "cn";

	const gchar *wholepath = g_strconcat(coupling_folder,couppath,coupusage,material,NULL);

	if(!reti||!retisilicone||!retipre||!retisiliconepre){
		DIR *d;
		struct dirent *dir;
		d = opendir(g_strconcat(coupling_folder,couppath,coupusage,material,NULL));
		if (d) {
			while ((dir = readdir(d)) != NULL) {
				if (strcmp(dir->d_name, g_strconcat(coupling_prefix, temptext, ".dat", NULL)) == 0 ){
					//printf("%s\n", dir->d_name);
					ui_set_statusbar(TRUE, "%s", dir->d_name);

					cp(g_strconcat(addressbar_last_address, "/", dir->d_name, NULL), g_strconcat(wholepath, dir->d_name, NULL) );
				}
			}
			closedir(d);
		}
	}
	else if(!retiasterix||!retiasterixpre){
		DIR *d;
		struct dirent *dir;
		d = opendir(g_strconcat(coupling_folder,couppath,coupusage,material,NULL));
		if (d) {
			while ((dir = readdir(d)) != NULL) {
				retiasterix = 1;
				//sprintf(test);
				retiasterix = regcomp(&regexasterixone, temptext , REG_EXTENDED | REG_ICASE  );
				retiasterix = regexec(&regexasterixone, dir->d_name, 0, 0, 0);
				if(!retiasterix){
					ui_set_statusbar(TRUE, "%d %s", retiasterix, dir->d_name);
					cp(g_strconcat(addressbar_last_address, "/", dir->d_name, NULL), g_strconcat(wholepath,dir->d_name, NULL) );
					ui_set_statusbar(TRUE, "%s", dir->d_name);
				}

			}
			closedir(d);
		}
	}

	regfree(&regex);
	regfree(&regexasterix);

	on_menu_refresh(NULL,NULL);
}

static void coupfolderchooser()
{
	int index = gtk_combo_box_get_active(combobox);
	switch ( index )
	{
		case 0:
			couppath = g_strdup("/VULASTIK_L/");
			// strcpy(preletter, "X");
			preletter = "X";
			break;
		case 1:
			couppath = g_strdup("/VULASTIK_XT/");
			preletter = "Y";
			break;
		case 2:
			couppath = g_strdup("/VULKARDAN_F/");
			preletter = "F";
			break;
		case 3:
			couppath = g_strdup("/VULKARDAN_E/");
			preletter = "K";
			break;
		case 4:
			couppath = g_strdup("/MEGIFLEX_B/");
			preletter = "J";
			break;
		case 5:
			couppath = g_strdup("/RATO_S/");
			preletter = "G";
			break;
		case 6:
			couppath = g_strdup("/RATO_R/");
			preletter = "G";
			break;
		case 7:
			couppath = g_strdup("/RATO_DS/");
			preletter = "A";
			break;
		case 8:
			couppath = g_strdup("/VULKARDAN_L/");
			preletter = "K";
			break;
		case 9:
			couppath = g_strdup("/VULKARDAN_G/");
			preletter = "K";
			break;
		default:
			printf("default\n");
	}
}

static void coupusagechooser()
{
	int index = gtk_combo_box_get_active(combobox);
	switch ( index )
	{
		case 0:
			coupusage = "Marine/";
			break;
		case 1:
			coupusage = "Industrie/";
			break;
		default:
			printf("default\n");
	}
}

static void fill_combocoup_entry (GtkWidget *combo)
{
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "VULASTIK L"); //0
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "VULASTIK XT");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "VULKARDAN F");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "VULKARDAN E");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "MEGIFLEX B");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "RATO S/S+");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "RATO R/R+");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "RATO DS/DS+");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "VULKARDAN L");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "VULKARDAN G");
}

static void fill_combocoupusage_entry (GtkWidget *combo)
{

  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "Marine"); //0
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "Industrie");
}

void make_coupbar(void)
{
	// GtkWidget *wid, *toolbar, *combocopubox;  delete
	GtkWidget *label, *combocoupbox;

	coupbar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
	combocoupbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);

	label = gtk_label_new(_("Kupplungsbezeichner:"));

	GtkWidget *choosecouptype = gtk_combo_box_text_new();
    fill_combocoup_entry (choosecouptype);
	g_signal_connect(choosecouptype, "changed", G_CALLBACK(coupfolderchooser), NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(choosecouptype),0);

	GtkWidget *chooseusagetype = gtk_combo_box_text_new();
    fill_combocoupusage_entry (chooseusagetype);
	g_signal_connect(chooseusagetype, "changed", G_CALLBACK(coupusagechooser), NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(chooseusagetype),0);

	gtk_box_pack_start(GTK_BOX(combocoupbox), choosecouptype, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(combocoupbox), chooseusagetype, TRUE, FALSE, 0);

	coupentry = gtk_entry_new();
	gtk_entry_set_placeholder_text( GTK_ENTRY(coupentry),"X3012, F5014, ... ");


	g_signal_connect(coupentry, "activate", G_CALLBACK(ui_CoupEntry_activate), NULL);

	gtk_box_pack_start(GTK_BOX(coupbar), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(coupbar), coupentry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(coupbar), combocoupbox, TRUE, TRUE, 0);

	// //g_signal_connect(filter_combo, "changed", G_CALLBACK(ui_combo_box_changed), NULL);

}