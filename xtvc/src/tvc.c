/*
 *  tvc.c
 *
 *  Copyright 20 <>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <tinyexpr.h>
#include <locale.h>
#include <regex.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include <geanyplugin.h>

GeanyPlugin	*geany_plugin;
GeanyData	*geany_data;

PLUGIN_VERSION_CHECK(224)

PLUGIN_SET_TRANSLATABLE_INFO(LOCALEDIR, GETTEXT_PACKAGE,
	_("TVCX"), _("Plugin helping to check input file,"
	"\ncalculate inline, "
	""),
	"0.0.1", "")

static int AnzahlModelLines=0;
static int springdampmodel=0;
static int springdampdefined=0;
static int gearmeshesmodel=0;
static int gearmeshesdefined=0;
static int systemlinestoevaluatemodel=0;
static int systemlinestoevaluatedefined=0;
static int minrpmofenginedefined=0;
static int hookejointsmodel=0;
static int hookejointsdefined=0;
static int zerostiffnessesinmodel=0;
static int freqdepdampmodel=0;
static int freqdepdampdefined=0;
static int freqdepdampdefinedd=0;
static int viskodampermodel=0;
static int viskodamperdefined=0;
static int propsmodel=0;
static int propsdefined=0;
static int number_of_powercurves=0;
static int couplingsmodel=0;
static int couplingsdefined=0;
static int cylindersmodel=0;
static int cylindersdefined=0;

static GtkWidget *s_context_sep_item, *s_context_checkinput_item, *s_context_recount_item;

/* Keybinding(s) */
enum
{
	CALCULATEfast_KB,
	CHECK_INPUT_KB,
	BRANCHES_INPUT_KB,
	PERCENT_OF_RATEDSPEED_KB,
	SCANIADAMPER_KB,
	CALCULATE0_KB,
	CALCULATE1_KB,
	CALCULATE2_KB,
	CALCULATE3_KB,
	CALCULATE4_KB,
	CALCULATE5_KB,
	CALCULATE6_KB,
	CALCULATE7_KB,
	CALCULATE8_KB,
	CALCULATE9_KB,
	CALCULATE10_KB,
	//OVERWRITE_WITH_SPACES_KB,
	OVERWRITE_BLOCK_SPACES_KB,
	OPEN_DRW_IN_VIEWER_KB,
	OPEN_TVCFOLDER_ARCHIVE_KB,
	COUNT_KB
};

#define sci_point_x_from_position(sci, position) \
	scintilla_send_message(sci, SCI_POINTXFROMPOSITION, 0, position)
#define sci_get_pos_at_line_sel_start(sci, line) \
	scintilla_send_message(sci, SCI_GETLINESELSTARTPOSITION, line, 0)
#define sci_get_pos_at_line_sel_end(sci, line) \
	scintilla_send_message(sci, SCI_GETLINESELENDPOSITION, line, 0)

static void update_display(void)
{
	while (gtk_events_pending())
		gtk_main_iteration();
}

static void overwrite_block_spaces()
{
	GeanyDocument *doc = document_get_current();
	ScintillaObject *sci = doc->editor->sci;

	gint start_pos = sci_get_selection_start(sci); // startpos der auswahl
	gint start_line = sci_get_line_from_position(sci, start_pos); //startzeile
	gint end_pos = sci_get_selection_end(sci); // endpos der auswahl
	gint end_line = sci_get_line_from_position(sci, end_pos); //endzeile

	gint xinsert = sci_point_x_from_position(sci, start_pos);
	gint xend = sci_point_x_from_position(sci, end_pos);
	gint insertcol = sci_get_pos_at_line_sel_start(sci, start_line);
	gint endcol = sci_get_pos_at_line_sel_end(sci, start_line);

	gint *line_pos = g_new(gint, end_line - start_line + 1);

	gint line, i;
	gint64 start_value=1;
	gint64 start = start_value;
	gint64 value;
	unsigned count = 0;
	size_t length, lend;
	

	// ui_set_statusbar(TRUE,"startline %d %d",start_line,end_line);
	// ui_set_statusbar(TRUE,"xinsert %d %d => %d ",xinsert,xend,xend-xinsert-1);  // xinsert xend 
	// ui_set_statusbar(TRUE,"startpos %d %d",start_pos,end_pos);

	if (xend < xinsert)
		xinsert = xend;

	for (line = start_line, i = 0; line <= end_line; line++, i++)
	{
		if (sci_point_x_from_position(sci,
			scintilla_send_message(sci, SCI_GETLINEENDPOSITION, line, 0)) >= xinsert)
		{
			line_pos[i] = sci_get_pos_at_line_sel_start(sci, line) -
				sci_get_position_from_line(sci, line);
			count++;
		}
		else
			line_pos[i] = -1;
	}
	
	sci_start_undo_action(sci);
	sci_replace_sel(sci, "");

	gchar buffer[endcol-insertcol];
	memset(buffer,' ',(endcol-insertcol));
	length=(endcol-insertcol);
	buffer[length] = '\0';

	for (line = start_line, i = 0; line <= end_line; line++, i++)
	{
		gint insert_pos;

		if (line_pos[i] < 0)
			continue;

		insert_pos = sci_get_position_from_line(sci, line) + line_pos[i];
		sci_insert_text(sci, insert_pos, buffer);
	}

	sci_end_undo_action(sci);
	
	g_free(line_pos);
}

/*
void overwrite_with_spaces()
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
}*/

static void on_calculateX(int Stellen)
{
	GeanyDocument *doc = document_get_current();
	GtkClipboard *clipboard, *primary;
	gchar *f_content; //, *f_content_tmp;
	int error;
	size_t maxGroups = 5;
	regmatch_t groupArray[maxGroups];
	regex_t regex01,regex02;
	int reti01,reti02,reti03,reti04;
	float Pkv30, Pkvtemp, Pkvresult;	
	long double b=0.0;

	gint selection_end;
	gint selection_start;

	if (doc && !doc->readonly)
	{
		// einlesen des selektierten bereichs 
		ScintillaObject *sci = doc->editor->sci;
		f_content = sci_get_selection_contents(doc->editor->sci);

		/*
		rubber:
		Pkv*(110-tu)/80
		silicone:
		Pkv*(150-tu)/120

		rubber:
		L=1 M=0.89 C=0.77
		silicone
		L=1 M=0.75 C=0.62
		*/
		reti01 = regcomp(&regex01, "([0-9]*([.][0-9]*)?)(R|G)([0-9]{2,3})", REG_EXTENDED | REG_ICASE);
		reti02 = regcomp(&regex02, "([0-9]*([.][0-9]*)?)(S|S)([0-9]{2,3})", REG_EXTENDED | REG_ICASE);
	
		if ( reti01 | reti02 ) {
			ui_set_statusbar(TRUE, "Could not compile Temp regex");
			//fprintf(stderr, "Could not compile regex\n");
			//exit(1);
		}
		// wenn gerechnet hinten an die zeile die temperatur ranschreiben
		// vielleicht ueder zweite keybinding mit paramter aufrufen und 
		// den berechneten term ersetzen
		reti03 = regexec(&regex01, f_content, maxGroups, groupArray, 0);
		if (!reti03) {
			char sourcecopy[strlen(f_content)+1];
			strcpy(sourcecopy, f_content);
			sourcecopy[groupArray[1].rm_eo] = 0;
			Pkv30 = atof(sourcecopy + groupArray[1].rm_so);
			
			char sourcecopy2[strlen(f_content)+1];
			strcpy(sourcecopy2, f_content);
			sourcecopy2[groupArray[4].rm_eo] = 0;
			Pkvtemp = atof(sourcecopy2 + groupArray[4].rm_so);

			Pkvresult = Pkv30*(110.0-Pkvtemp)/80.0;
			ui_set_statusbar(TRUE,"%.3f*(110-%.3f)/80 -> %.3f", Pkv30, Pkvtemp ,Pkvresult);
		}
	
		reti04 = regexec(&regex02, f_content, maxGroups, groupArray, 0);
		if (!reti04) {
			char sourcecopy[strlen(f_content)+1];
			strcpy(sourcecopy, f_content);
			sourcecopy[groupArray[1].rm_eo] = 0;
			Pkv30 = atof(sourcecopy + groupArray[1].rm_so);
			
			char sourcecopy2[strlen(f_content)+1];
			strcpy(sourcecopy2, f_content);
			sourcecopy2[groupArray[4].rm_eo] = 0;
			Pkvtemp = atof(sourcecopy2 + groupArray[4].rm_so);

			Pkvresult = Pkv30*(150.0-Pkvtemp)/120.0;
			ui_set_statusbar(TRUE,"%.3f*(150-%.3f)/120 -> %.3f", Pkv30, Pkvtemp ,Pkvresult);
		}
		regfree(&regex01);
		regfree(&regex02);


		/*
		Falls keine Temperaturfaktorberechnung dann normale berechnung durchfuehren
		*/
		if ((!reti03) || (!reti04)){
			b = Pkvresult;
			error = 0;
		}else{
			b = te_interp(f_content, &error);
		}
		if(!error)
		{
			char buf[sizeof(b)+1];
			switch (Stellen)
			{
				case 0:
					sprintf(buf, "%.0Lf", b);
					break;
				case 1:
					sprintf(buf, "%.1Lf", b);
					break;
				case 2:
					sprintf(buf, "%.2Lf", b);
					break;
				case 3:
					sprintf(buf, "%.3Lf", b);
					break;
				case 4:
					sprintf(buf, "%.4Lf", b);
					break;
				case 5:
					sprintf(buf, "%.5Lf", b);
					break;
				case 6:
					sprintf(buf, "%.6Lf", b);
					break;
				case 7:
					sprintf(buf, "%.7Lf", b);
					break;
				case 8:
					sprintf(buf, "%.8Lf", b);
					break;
				case 9:
					sprintf(buf, "%.9Lf", b);
					break;
				case 10:
					sprintf(buf, "%.10Lf", b);
					break;
			}
			selection_end = sci_get_selection_end(sci);
			selection_start = sci_get_selection_start(sci);
			sci_insert_text(sci, selection_end, " ");
			sci_insert_text(sci, selection_end+1, buf);
			ui_set_statusbar(TRUE, "result: %s", buf);

			if ((!reti03) || (!reti04)){
				char buf2[50];
				int line = sci_get_line_from_position(sci, selection_end);
				int line_end = sci_get_line_end_position(sci,line);
				sprintf(buf2, "  %.2f@%.3f°C", Pkv30, Pkvtemp);
				//sprintf(buf, " test");
				sci_insert_text(sci, line_end, buf2);
			}
			sci_set_current_position (sci, selection_end+1, TRUE);
			sci_set_selection_start (sci,selection_start);
			sci_set_selection_end (sci,selection_end+1);
			
			clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
			primary = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
			gtk_clipboard_set_text(clipboard,buf,sizeof(buf));
			gtk_clipboard_set_text(primary, buf,sizeof(buf));
		}		
	}
}

/*
static void on_calculate4()
{
	GeanyDocument *doc = document_get_current();
	gchar *f_content;
	int error;
	gint selection_end;
	if (doc && !doc->readonly)
	{
		ScintillaObject *sci = doc->editor->sci;
		f_content = sci_get_selection_contents(doc->editor->sci);
		double b = te_interp(f_content, &error);
		if(!error)
		{
			char buf[sizeof(b)+1];
			sprintf(buf, "%.4f", b);
			selection_end = sci_get_selection_end(sci);
			sci_insert_text(sci, selection_end, buf);
			ui_set_statusbar(TRUE, "result: %f", b);
			sci_set_current_position (sci, selection_end, TRUE);
		}
	}
}

static void on_calculaten()
{
	GeanyDocument *doc = document_get_current();
	gchar *f_content;
	//gchar *f_content01;
	int error;
	gint selection_end;
	
	if (doc && !doc->readonly)
	{
		ScintillaObject *sci = doc->editor->sci;

		f_content = sci_get_selection_contents(doc->editor->sci);
		// gchar **f_content01 = g_strsplit(f_content,";",-1);
 
		long double b = te_interp(f_content, &error);
		if(!error)
		{
			char buf[sizeof(b)+1];
		
			sprintf(buf, "%.10Lf", b);
			selection_end = sci_get_selection_end(sci);
		
			sci_insert_text(sci, selection_end, buf );

			ui_set_statusbar(TRUE, "result: %Lf", b);
			sci_set_current_position (sci, selection_end, TRUE);
		}		
		
	}
}
*/

static void on_calculateY1()
{
	on_calculateX(1);
}

static void on_calculateY2()
{
	on_calculateX(2);
}

static void on_calculateY3()
{
	on_calculateX(3);
}

static void on_calculateY4()
{
	on_calculateX(4);
}

static void on_calculateY5()
{
	on_calculateX(5);
}

static void on_calculateY6()
{
	on_calculateX(6);
}

static void on_calculateY7()
{
	on_calculateX(7);
}

static void on_calculateY8()
{
	on_calculateX(8);
}

static void on_calculateY9()
{
	on_calculateX(9);
}

static void on_calculateY10()
{
	on_calculateX(10);
}

static void on_calculateY0()
{
	on_calculateX(0);
}

static void reset_all_countingvalues()
{
	AnzahlModelLines=0;
	springdampmodel=0;
	springdampdefined=0;
	gearmeshesmodel=0;
	gearmeshesdefined=0;
	systemlinestoevaluatemodel=0;
	systemlinestoevaluatedefined=0;
	minrpmofenginedefined=0;
	hookejointsmodel=0;
	hookejointsdefined=0;
	zerostiffnessesinmodel=0;
	freqdepdampmodel=0;
	freqdepdampdefined=0;
	freqdepdampdefinedd=0;
	viskodampermodel=0;
	viskodamperdefined=0;
	propsmodel=0;
	propsdefined=0;
	couplingsmodel=0;
    couplingsdefined=0;
	cylindersmodel=0;
	cylindersdefined=0;
}

static void count_all_modellines(char * tempStr)
{
	regex_t regex;
	int reti;

	reti = regcomp(&regex, "^(1[0-9]{3}) +('.*') +([0-9]+[.]?[0-9]*) +(([^-][0-9]+[.]?[0-9]*)|-1) +([0-9]+[.]?[0-9]*) +[0-9]{1,2}.*", REG_EXTENDED);

	reti = regexec(&regex, tempStr, 0, NULL, 0);
	if (!reti) {
		AnzahlModelLines++;
	}
	regfree(&regex);
}

static void count_all_springdamperlines(char * tempStr)
{
	regex_t regexm, regexd;
	int retim,retid;
	
	//element 2. 
    retim = regcomp(&regexm, "^(1[0-9]{3}) +('.*') +([0-9]+[.]?[0-9]*) +(([^-][0-9]+[.]?[0-9]*)|-1) +([0-9]+[.]?[0-9]*) +2(.*)", REG_EXTENDED);
    retid = regcomp(&regexd, "^22[0-9]{2} +([0-9]+[.]?[0-9]*) +([-]?[0-9]+[.]?[0-9]*) +([0-9]+[.]?[0-9]*) +([0-9]+[.]?[0-9]*) +([0-9]+[.]?[0-9]*).*", REG_EXTENDED); // liest nicht immer alle 

	retim = regexec(&regexm, tempStr, 0, NULL, 0);
	if (!retim) {
		springdampmodel++;
	}

	retid = regexec(&regexd, tempStr, 0, NULL, 0);
	if (!retid) {
		springdampdefined++;
	}

	regfree(&regexm);
	regfree(&regexd);
}

static void count_all_viskodamperlines(char * tempStr)
{
	regex_t regexm, regexd;
	int retim,retid;
	
	//element 2. 
    retim = regcomp(&regexm, "^(1[0-9]{3}) +('.*') +([0-9]+[.]?[0-9]*) +(([^-][0-9]+[.]?[0-9]*)|-1) +([0-9]+[.]?[0-9]*) +5(.*)", REG_EXTENDED);
    retid = regcomp(&regexd, "^25[0-9]{2} +([0-9]+[.]?[0-9]*) +([0-9]+[.]?[0-9]*).*", REG_EXTENDED);

	retim = regexec(&regexm, tempStr, 0, NULL, 0);
	if (!retim) {
		viskodampermodel++;
	}

	retid = regexec(&regexd, tempStr, 0, NULL, 0);
	if (!retid) {
		viskodamperdefined++;
	}

	regfree(&regexm);
	regfree(&regexd);
}

static void count_all_freqdepdamplines(char * tempStr)
{
	regex_t regexm, regexd, regexdd;
	int retim,retid,retidd;
	
	//Gear Meshes
	retim = regcomp(&regexm, "^(1[0-9]{3}) +('.*') +([0-9]+[.]?[0-9]*) +(([^-][0-9]+[.]?[0-9]*)|-1) +([0-9]+[.]?[0-9]*) +15(.*)", REG_EXTENDED);
    retid = regcomp(&regexd, "^45[0-9]{2} +[0-9]+ +([+-]?([0-9]*[.])?[0-9]+) +([+-]?([0-9]*[.])?[0-9]+).*", REG_EXTENDED);
	retidd = regcomp(&regexdd, "^45[0-9]{2} +[0-9]+ +([+-]?([0-9]*[.])?[0-9]+) +([+-]?([0-9]*[.])?[0-9]+) +[0-9]+ +([+-]?([0-9]*[.])?[0-9]+) +([+-]?([0-9]*[.])?[0-9]+).*", REG_EXTENDED);

	retim = regexec(&regexm, tempStr, 0, NULL, 0);
	if (!retim) {
		freqdepdampmodel++;
	}

	retidd = regexec(&regexdd, tempStr, 0, NULL, 0);
	if (!retidd) {
		freqdepdampdefinedd++;
	}
	else
	{
		retid = regexec(&regexd, tempStr, 0, NULL, 0);
		if (!retid) {
			freqdepdampdefined++;
		}
	}

	regfree(&regexm);
	regfree(&regexd);
}

static void count_all_gearmeshlines(char * tempStr)
{
	regex_t regexm, regexd;
	int retim,retid;
	
	//Gear Meshes
	retim = regcomp(&regexm, "^(1[0-9]{3}) +('.*') +([0-9]+[.]?[0-9]*) +(([^-][0-9]+[.]?[0-9]*)|-1) +([0-9]+[.]?[0-9]*) +10(.*)", REG_EXTENDED);
    retid = regcomp(&regexd, "^41[0-9]{2}( +(([a-zA-Z]{4})|([0-9]+[.]?[0-9]*)))+.*", REG_EXTENDED);
	
	retim = regexec(&regexm, tempStr, 0, NULL, 0);
	if (!retim) {
		gearmeshesmodel++;
	}
	retid = regexec(&regexd, tempStr, 0, NULL, 0);
	if (!retid) {
		gearmeshesdefined++;
	}

	regfree(&regexm);
	regfree(&regexd);
}

static void count_all_proplines(char * tempStr)
{
	regex_t regexm, regexd, regexd1, regexd2, regexp, regexp1, regexp2;
	int retim,retid,retid1,retid2,retip,retip1,retip2;
	
	//TODO: not completed...missing is 2310 with 1 or 2
	retim = regcomp(&regexm, "^(1[0-9]{3}) +('.*') +([0-9]+[.]?[0-9]*) +(([^-][0-9]+[.]?[0-9]*)|-1) +([0-9]+[.]?[0-9]*) +6(.*)", REG_EXTENDED);
    
	retid1 = regcomp(&regexd1, "^2310 +[0-9] +[0-9] +[1-2].*", REG_EXTENDED);
	retid2 = regcomp(&regexd2, "^2310 [0-9] [0-9] 2.*", REG_EXTENDED);
	retid = regcomp(&regexd, "^2310 +[0-9] +[0-9] +-1.*", REG_EXTENDED);
	
	retip1 = regcomp(&regexp1, "^2335 +.*([+-]?([0-9]*[.])?[0-9]+ *)* .*", REG_EXTENDED);
	retip2 = regcomp(&regexp2, "^2335 +.*([+-]?([0-9]*[.])?[0-9]+ *)* .*", REG_EXTENDED);
	retip = regcomp(&regexp, "^261[0-9]( +(([a-zA-Z]{4})|([0-9]+[.]?[0-9]*)))+.*", REG_EXTENDED);
	/*
	if line 2130 has a 1 or 2 then prop def next line
	if line 2130 has a -1 then read for line 2610 
	prop can be 
	*/
	retim = regexec(&regexm, tempStr, 0, NULL, 0);
	if (!retim) {
		propsmodel++;
	}

	retid1 = regexec(&regexp1, tempStr, 0, NULL, 0);
	retid2 = regexec(&regexp2, tempStr, 0, NULL, 0);
	retid = regexec(&regexp, tempStr, 0, NULL, 0);
	
	if (!retid1) {
		retip1 = regexec(&regexp1, tempStr, 0, NULL, 0);
		if (!retip1) {
			propsdefined++;
		}
	}
	else if (!retid2) {
		retip2 = regexec(&regexp2, tempStr, 0, NULL, 0);
		if (!retip2) {
			propsdefined++;
		}
		
	}
	else if (!retid) {
		retip = regexec(&regexp, tempStr, 0, NULL, 0);
		if (!retip) {
			propsdefined++;
		}
	}

	regfree(&regexm);
	regfree(&regexd);
}

static void count_all_hookejointslines(char * tempStr)
{
	regex_t regexm, regexd;
	int retim,retid;
	
	//Hooke
	retim = regcomp(&regexm, "^(1[0-9]{3}) +('.*') +([0-9]+[.]?[0-9]*) +(([^-][0-9]+[.]?[0-9]*)|-1) +([0-9]+[.]?[0-9]*) +12(.*)", REG_EXTENDED);
    retid = regcomp(&regexd, "^42[0-9]{2} +([0-9]+[.]?[0-9]*) +([0-9]+[.]?[0-9]*).*", REG_EXTENDED);

	retim = regexec(&regexm, tempStr, 0, NULL, 0);
	if (!retim) {
		hookejointsmodel++;
	}
	retid = regexec(&regexd, tempStr, 0, NULL, 0);
	if (!retid) {
		hookejointsdefined++;
	}

	regfree(&regexm);
	regfree(&regexd);
}

static void count_all_coupinglines(char * tempStr)
{
	regex_t regexm, regexd;
	int retim,retid;
	
	retim = regcomp(&regexm, "^(1[0-9]{3}) +('.*') +([0-9]+[.]?[0-9]*) +(([^-][0-9]+[.]?[0-9]*)|-1) +([0-9]+[.]?[0-9]*) +4(.*)", REG_EXTENDED);
    retid = regcomp(&regexd, "^24[0-9]{2}( +([a-zA-Z]{4}|([0-9]+[.]?[0-9]*))).*", REG_EXTENDED);

	retim = regexec(&regexm, tempStr, 0, NULL, 0);
	if (!retim) {
		couplingsmodel++;
	}
	retid = regexec(&regexd, tempStr, 0, NULL, 0);
	if (!retid) {
		couplingsdefined++;
	}

	regfree(&regexm);
	regfree(&regexd);
}

static void count_all_cylinderslines(char * tempStr)
{
	regex_t regexm, regexd;
	int retim,retid;
	
	retim = regcomp(&regexm, "^(1[0-9]{3}) +('.*') +([0-9]+[.]?[0-9]*) +(([^-][0-9]+[.]?[0-9]*)|-1) +([0-9]+[.]?[0-9]*) +3(.*)", REG_EXTENDED);
    retid = regcomp(&regexd, "^23[0-9]{2}( +(([0-9]+[.]?[0-9]*))){2}.*", REG_EXTENDED);

	retim = regexec(&regexm, tempStr, 0, NULL, 0);
	if (!retim) {
		cylindersmodel++;
	}
	retid = regexec(&regexd, tempStr, 0, NULL, 0);
	if (!retid) {
		cylindersdefined++;
	}

	regfree(&regexm);
	regfree(&regexd);
}

static void count_all_pointstoevaluatelines(char * tempStr)
{
	regex_t regexm, regexd;
	int retim,retid;

	size_t maxGroups = 3;
	regmatch_t groupArray[maxGroups];
	char systemanz[5];

	//Systemstellen auszuwerten
	retid = regcomp(&regexd, "^3100 +'(systems|SYSTEMS)' +([0-9]+).*", REG_EXTENDED);
    retim = regcomp(&regexm, "^31[0-9]{2} +[0-9]{1,4} +.*", REG_EXTENDED);
	
	retid = regexec(&regexd, tempStr, maxGroups, groupArray, 0);
	if (!retid) {
		char sourcecopy[strlen(tempStr)+1];
		strcpy(sourcecopy, tempStr);
		sourcecopy[groupArray[2].rm_eo] = 0;
		strcpy(systemanz,sourcecopy + groupArray[2].rm_so);
		systemlinestoevaluatedefined = g_ascii_strtoll(systemanz,NULL,10);
	}
	
	retim = regexec(&regexm, tempStr, 0, NULL, 0);
	if (!retim) {
		systemlinestoevaluatemodel++;
	}

	regfree(&regexm);
	regfree(&regexd);
}

static void minimal_rpm_engine(char * tempStr)
{
	regex_t regexm;
	int retim;

	retim = regcomp(&regexm, "^(211[0-9]) +([0-9]+[.]?[0-9]*) +([0-9]+[.]?[0-9]*) +[0-9] +[0-9] +([0-9]+[.]?[0-9]*) +([0-9]+[.]?[0-9]*) +([0-9]+[.]?[0-9]*).*$", REG_EXTENDED);
	
	retim = regexec(&regexm, tempStr, 0, NULL, 0);
	if (!retim) {
		minrpmofenginedefined=1;
	}

	regfree(&regexm);
}

static void count_all_number_of_powercurves(char * tempStr)
{
	regex_t regexm;
	int retim;
	int error;
	size_t maxGroups = 3;
	regmatch_t groupArray[maxGroups];
	
	retim = regcomp(&regexm, "^3200 +'BETRIEB' +[0-9]{1,2} +([0-9]).*$", REG_EXTENDED);

	retim = regexec(&regexm, tempStr, maxGroups, groupArray, 0);
	if (!retim) {
		char sourcecopy[strlen(tempStr)+1];
		strcpy(sourcecopy, tempStr);
		sourcecopy[groupArray[1].rm_eo] = 0;
		number_of_powercurves = atof(sourcecopy + groupArray[1].rm_so);
		
	}

	regfree(&regexm);
}

static void count_all_zerostiffnesses(char * tempStr)
{
	regex_t regexm;
	int retim;

	// wenn steifigkeit 0 oder 0.0 dann muss element in der zeile sein
	retim = regcomp(&regexm, "^1[0-9]{3} +('.*') +([0-9]+[.]?[0-9]*) +([0]+[.]?[0]*) +([0-9]+[.]?[0-9]*) +(1[0-9]|([0-3]|[5-9])).*", REG_EXTENDED);
    // ^(1[0-9]{3}) +('.*') +([0-9]+[.]?[0-9]*) +(([^-][0-9]+[.]?[0-9]*)|-1) +([0-9]+[.]?[0-9]*) +12(.*)
	retim = regexec(&regexm, tempStr, 0, NULL, 0);
	if (!retim) {
		zerostiffnessesinmodel++;
	}

	regfree(&regexm);
}

static void print_input_test_results()
{
	ui_set_statusbar(TRUE, "\n\t\t ---- Modellcheck :----");
	ui_set_statusbar(TRUE, "\t Gesamtanzahl der Elemente vom Model \t %d", AnzahlModelLines);

	if(number_of_powercurves>0){

		ui_set_statusbar(TRUE, "\t number of powercurves \t %d ", number_of_powercurves);
	}

	ui_set_statusbar(TRUE, "\t\t ---- 3100 Systemstellen: ----");
	ui_set_statusbar(TRUE, "\t auszuwerten \t %d ", systemlinestoevaluatedefined);
	ui_set_statusbar(TRUE, "\t definiert \t %d ", systemlinestoevaluatemodel);
	
	if(springdampmodel>0 || springdampdefined>0)
	{
		ui_set_statusbar(TRUE, "\t\t ---- 2201 Damping Element (Elementtyp 2): ----");
		ui_set_statusbar(TRUE, "\t im Modell \t %d", springdampmodel);
		ui_set_statusbar(TRUE, "\t definiert \t %d ", springdampdefined);
	}

	if(couplingsmodel>0 || couplingsdefined>0)
	{
		ui_set_statusbar(TRUE, "\t\t ---- 2401 elst. Coupling (Elementtyp 4): ----");
		ui_set_statusbar(TRUE, "\t im Modell \t %d", couplingsmodel);
		ui_set_statusbar(TRUE, "\t definiert \t %d ", couplingsdefined);
	}

	if(cylindersmodel>0 || cylindersdefined>0)
	{
		ui_set_statusbar(TRUE, "\t\t ---- 2301 Cylinderexcitation (Elementtyp 3): ----");
		ui_set_statusbar(TRUE, "\t im Modell \t %d", cylindersmodel);
		ui_set_statusbar(TRUE, "\t definiert \t %d ", cylindersdefined);
	}

	if(gearmeshesmodel>0 || gearmeshesdefined>0)
	{
		ui_set_statusbar(TRUE, "\t\t ---- 4101 Gear-Meshes (Elementtyp 10): ----");
		ui_set_statusbar(TRUE, "\t im Modell \t %d ", gearmeshesmodel);
		ui_set_statusbar(TRUE, "\t definiert \t %d ", gearmeshesdefined);
	}

	if(hookejointsmodel>0 || hookejointsdefined>0)
	{
		ui_set_statusbar(TRUE, "\t\t ---- 4201 Hooke's Joints (Elementtyp 12): ----");
		ui_set_statusbar(TRUE, "\t im Modell \t %d ", hookejointsmodel);
		ui_set_statusbar(TRUE, "\t definiert \t %d ", hookejointsdefined);
	}

	if(propsmodel>0 || propsdefined>0)
	{
		ui_set_statusbar(TRUE, "\t\t ---- 2610 Propellers (Elementtyp 6): ----");
		ui_set_statusbar(TRUE, "\t im Modell \t %d ", propsmodel);
		ui_set_statusbar(TRUE, "\t definiert \t %d ", propsdefined);
	}
	
	if(viskodampermodel>0 || viskodamperdefined>0)
	{
		ui_set_statusbar(TRUE, "\t\t ---- 2501 viscous damper (Elementtyp 5): ----");
		ui_set_statusbar(TRUE, "\t im Modell \t %d ", viskodampermodel);
		ui_set_statusbar(TRUE, "\t definiert \t %d ", viskodamperdefined);
	}

	if(freqdepdampmodel>0 || freqdepdampdefined>0 || freqdepdampdefined>0)
	{
		ui_set_statusbar(TRUE, "\t\t ---- 4501 freq stiff damp (Elementtyp 15): ----");
		ui_set_statusbar(TRUE, "\t im Modell \t %d ", freqdepdampmodel);
		ui_set_statusbar(TRUE, "\t2x definiert \t %d ", freqdepdampdefinedd);
		ui_set_statusbar(TRUE, "\t1x definiert \t %d ", freqdepdampdefined);
	}

	if(!minrpmofenginedefined)
	{
		ui_set_statusbar(TRUE, "\t\t--- Engine has no minimal rpm, please add at the end of line 2110");
	}

	if(zerostiffnessesinmodel > 0)
	{
		ui_set_statusbar(TRUE, "\t\t-  %d-mal 0 Steifigkeit ist/sind nicht Elementtyp 4 ", zerostiffnessesinmodel);
	}
}

static void on_tvc_check()
{
	// gchar *my_string;
	GeanyDocument *doc = document_get_current();
	ScintillaObject *sci = doc->editor->sci;
	gint len = sci_get_length(sci) + 1;
	gchar *my_string = sci_get_contents(sci, len);

	reset_all_countingvalues();

    const char * curLine = my_string;
			
	while(curLine)
    {
      	const char * nextLine = strchr(curLine, '\n');
      	int curLineLen = nextLine ? (nextLine-curLine) : strlen(curLine);
      	char * tempStr = (char *) malloc(curLineLen+1);
		if (tempStr)
      	{
        	memcpy(tempStr, curLine, curLineLen);
         	tempStr[curLineLen] = '\0';  // NUL-terminate!

			// Zaehlen der Gesamtanzahl der Modellzeilen
			count_all_modellines(tempStr);

			count_all_number_of_powercurves(tempStr);

			// Anzahl von damping lines im Modell
			count_all_springdamperlines(tempStr);

			// Anzahl von elas. coupling lines im Modell
			count_all_coupinglines(tempStr);

			// Anzahl von cylinder lines im Modell
			count_all_cylinderslines(tempStr);

			// Anzahl der Auszuwertenden Systemstellen
			count_all_pointstoevaluatelines(tempStr);

			// Anzahl der Gear-meshes checken
			count_all_gearmeshlines(tempStr);

			// Anzahl von 15 damper
			count_all_freqdepdamplines(tempStr);

			// Anzahl von 5 damper
			count_all_viskodamperlines(tempStr);

			// Anzahl von 6 Propeller
			count_all_proplines(tempStr);

			// hat das model eine minimaldrehzahl 
			minimal_rpm_engine(tempStr);

			count_all_zerostiffnesses(tempStr);

			count_all_hookejointslines(tempStr);
        	
			free(tempStr);
      	}
      	else printf("malloc() failed!?\n");
		curLine = nextLine ? (nextLine+1) : NULL;
	}

	print_input_test_results();

}

static void numbering_branches()
{ 
	GeanyDocument *doc = document_get_current();
	ScintillaObject *sci = doc->editor->sci;

	gint numberoflines = sci_get_line_count(sci);

	regex_t regex01, regex02;
	int reti01, reti02;
	int id = 0;
	int Anzahlmodel2Lines = 0, lastModelLine = 1;
	int modelrow = 1051, branchcount=1;
	int matrix [25][2] = {0};
	int schalter = 0;
	
	/* Compile regular expression */
	// normale Zeile im SystemModell 
	reti01 = regcomp(&regex01, "^([0-9][0-9][0-9][0-9]) *('.*')+ *(([+-]?([0-9]+[.])+[0-9]+ *)|( *-1 *)){3} [0-9]((\n)*.*)", REG_EXTENDED);
    // Leerzeile im SystemModell
	reti02 = regcomp(&regex02, "^[ \t]*\n\0", REG_EXTENDED);
	//reti03 = regcomp(&regex03, "^105[0-9] *[0-9]* *[0-9]((\n)*.*)", REG_EXTENDED);
	
	matrix[0][0] = 1;

	if (reti01 || reti02 ) 
	{
		fprintf(stderr, "Could not compile regex\n");
		// exit(1); better ending...not THAT error! :)
	}
	else
	{
		// erster Durchlauf um die Gesamtzahl der Modellzeilen zu ermitteln
		for(gint i=0; i < numberoflines; i++)
		{
			gchar * test = sci_get_line(sci,i);
			
			reti01 = regexec(&regex01, test, 0, NULL, 0);
			if (!reti01) {
				Anzahlmodel2Lines++;
				lastModelLine=i;
			}
		}
		
		int compareLines=0;
		int linenumber=1100;

		// zweiter Durchlauf um die Branches zu ermitteln 
		for(gint i=0; i<numberoflines; i++)
		{
			gchar * test = sci_get_line(sci,i);
			
			reti01 = regexec(&regex01, test, 0, NULL, 0);
			if (!reti01) {
				if(compareLines <= Anzahlmodel2Lines)
				{
					linenumber=linenumber+1;
					if(!schalter)
					{
						matrix[id][0] = linenumber;
						schalter=1;
					}
					compareLines++;

					gint linelenght = sci_get_line_length(sci, i);
					gint startofline = sci_get_position_from_line(sci,i);

					gchar * dest = (char *) malloc(linelenght+1);
					char line[11];
					sprintf(line,"%d",linenumber);
					
					strcpy(dest,line);
					strcat(dest, test + 4 );

					sci_set_selection_start(sci,startofline);
					sci_set_selection_end(sci,startofline+linelenght);
					sci_replace_sel(sci, dest);

				}
			}

			reti02 = regexec(&regex02, test, 0, NULL, 0);
			if (!reti02) {
				if((compareLines < Anzahlmodel2Lines) && (compareLines > 0))
				{
					matrix[id][1] = linenumber;
					id=id+1;
					schalter=0;
					//ui_set_statusbar(TRUE, "\t %d",linenumber);
					linenumber=linenumber+100;
					branchcount++;
				}
			}
		}
		matrix[id][1] = linenumber;

		gchar helper[50];
		int row;
		gint einfuegezeile=lastModelLine-(Anzahlmodel2Lines+branchcount-1);
		gint startofline = sci_get_position_from_line(sci,einfuegezeile);
		if(branchcount == 1){
			// sprintf(helper,"1050 %d\n", branchcount);
			sci_insert_text(sci, startofline, "1050 0");
		}else{
			sprintf(helper,"1050 %02d \n", branchcount);
			sci_insert_text(sci, startofline, helper);
		}
		
		for (row=0; row<=24; row++)
		{
			if(matrix[row][0] > 0)
			{
				einfuegezeile++;
				startofline = sci_get_position_from_line(sci, einfuegezeile);
				sprintf(helper,"%d %02d %02d\n", modelrow, matrix[row][0]%100, matrix[row][1]%100 );
				//ui_set_statusbar(TRUE, "%s",helper);
				sci_insert_text(sci, startofline, helper);
				modelrow++;
			}
			if(row==24){
				einfuegezeile++;
				startofline = sci_get_position_from_line(sci, einfuegezeile);
				sprintf(helper,"\n");
				//ui_set_statusbar(TRUE, "%s",helper);
				sci_insert_text(sci, startofline, helper);
			}
		}

		fflush(stdout);
		regfree(&regex02);
		regfree(&regex01);
	}
	
	
}

static void percent_of_ratedspeed()
{
	GeanyDocument *doc = document_get_current();
	ScintillaObject *sci = doc->editor->sci;

	GtkClipboard *clipboard, *primary;
	gchar temptxt[30];
	gint numberoflines = sci_get_line_count(sci);

	size_t maxGroups = 3;
	regmatch_t groupArray[maxGroups];
	char * succ;
	double minspeed=0, maxspeed=0, ratedspeed=1;
	regex_t regex01;
	int reti01;
	gint lastline=0;

	/* Compile regular expression */
	// normale Zeile im SystemModell 
	reti01 = regcomp(&regex01, "^3[0-9]{3} *([0-9]+[.]+[0-9]+) *.*\n.*", REG_EXTENDED);

	for(gint i=0; i < numberoflines; i=i+1)
	{
		gchar * test = sci_get_line(sci,i);
		
		reti01 = regexec(&regex01, test, 0, NULL, 0);
		if (!reti01) {
			lastline=i;
		}
	}

	gchar * first = sci_get_line(sci, 1);
	reti01 = regexec(&regex01, first, maxGroups, groupArray, 0);
	if (!reti01) {
		char sourcecopy[strlen(first)+1];
		char speed[10];
		strcpy(sourcecopy, first);
		sourcecopy[groupArray[1].rm_eo] = 0;
		strcpy(speed,sourcecopy + groupArray[1].rm_so);
		maxspeed=strtod(speed, &succ);
	}

	gchar * last = sci_get_line(sci, lastline);
	reti01 = regexec(&regex01, last, maxGroups, groupArray, 0);
	if (!reti01) {
		char sourcecopy[strlen(last)+1];
		char speed[10];
		strcpy(sourcecopy, last);
		sourcecopy[groupArray[1].rm_eo] = 0;
		strcpy(speed,sourcecopy + groupArray[1].rm_so);
		minspeed=strtod(speed, &succ);
	}
	gchar * f_content = sci_get_selection_contents(doc->editor->sci);
	ratedspeed=strtod(f_content, &succ);
	
	if(ratedspeed>0)
	{
		ui_set_statusbar(TRUE, "\t%.2f - %.2f", minspeed/ratedspeed, maxspeed/ratedspeed);

		sprintf(temptxt,"%.2f - %.2f", minspeed/ratedspeed, maxspeed/ratedspeed);

		clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
		primary = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
		gtk_clipboard_set_text(clipboard, temptxt, sizeof(temptxt));
		gtk_clipboard_set_text(primary, temptxt, sizeof(temptxt));
	}
	regfree(&regex01);
}

static void scania_damper()
{
	GeanyDocument *doc = document_get_current();

	ScintillaObject *sci = doc->editor->sci;
	gchar * f_content;
	f_content = sci_get_selection_contents(doc->editor->sci);
	
	regex_t regex01;
	int reti01;
	reti01 = regcomp(&regex01, "^[0-9]+ +([+-]?[0-9]?[.]{1}[0-9]+) +[0-9]+ +([+-]?[0-9]?[.]{1}[0-9]+)((\n)*.*)", REG_EXTENDED);

	reti01 = regexec(&regex01, f_content, 0, NULL, 0);
	if (!reti01) 
	{
		gint linenumber = sci_get_current_line(sci);
		gint startofline = sci_get_position_from_line(sci,linenumber);
		gint linelenght = sci_get_line_length(sci, linenumber);

		gchar **f_content01 = g_strsplit(f_content," ",-1);
		char * succ;
		long double ac,ad,bc,bd;

		long double stiffness=0,damping=0;
		//int freq[] = {10,45,90,140,190,240,290,340,390,440};
		long double freq[] = {440,390,340,290,240,190,140,90,45,10};
		int elem[] = {4509,4508,4507,4506,4505,4504,4503,4502,4501,4500};

		ac=strtod(f_content01[0],&succ);
		bc=strtod(f_content01[1],&succ);
		ad=strtod(f_content01[2],&succ);
		bd=strtod(f_content01[3],&succ);
		ui_set_statusbar(TRUE, "Eingabewerte:\n\t Stiffness:\t a=%.3Lf b=%.4Lf \n\t Damper:\t a=%.3Lf b=%.4Lf", ac,bc,ad,bd);

		gchar helper[50];
		for(int i=0;i<=9;i=i+1)
		{
			stiffness=ac*pow(freq[i],bc)/1000.0;
			damping=ad*pow(freq[i],-bd)/1000.0;
			sprintf(helper,"%d \t %.1Lf \t %.5Lf \t %.5Lf\n", elem[i], freq[i], stiffness,damping);
			
			sci_insert_text(sci, startofline+linelenght, helper);
		}
	}else{
		ui_set_statusbar(TRUE, "input nicht okay ( aC bC aD bD := Float Float Float Float )");
	}
	regfree(&regex01);
}

static void on_openTerminal(gchar *workingdic)
{
	GeanyDocument *doc = document_get_current();
	gchar *locale_path2;
	locale_path2 = "/home/miki/Downloads/";
	char* command = "pqiv";
	char* argument_list[] = {"pqiv ", workingdic , NULL};

    if (fork() == 0) {
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
	// ui_set_statusbar(TRUE,"drawing number: %s", drwnumber);
}

static void find_correct_drw(gchar *drwnumber)
{
	gchar *locale_path2 = "/home/miki/Downloads/";

	gchar one[10] = "";
	memset(one, '\0', sizeof(one));
	strncpy(one, drwnumber, 1);
	strncpy(one+1, "/", 1);
	strncpy(one+2, drwnumber+1, 1);
	strncpy(one+3, "/", 1);
	strncpy(one+4, drwnumber+2, 1);
	strncpy(one+5, drwnumber+3, 1);
	strncpy(one+6, "/", 1);
	strncpy(one+7, drwnumber+4, 1);
	strncpy(one+8, "/", 1);
	
	gchar *prepath = g_strconcat(locale_path2, one, NULL);

	gchar *workingdic = g_strconcat(prepath, drwnumber, ".ps", NULL);
	drwnumber[5] = '0';
	gchar *workingdic1 = g_strconcat(locale_path2, drwnumber, ".ps", NULL);

	if (access(workingdic, F_OK) == 0) {
		on_openTerminal(workingdic);
	}else if(access(workingdic1, F_OK) == 0) {
		on_openTerminal(workingdic1);
	}else{
		ui_set_statusbar(TRUE,"couldn't find drawing number: %s", drwnumber);
	}
}

static void on_opennautilus(gchar *workingdic)
{
	char* command = "nautilus";
	char* argument_list[] = {"nautilus ", workingdic , NULL};

    if (fork() == 0) {
        int status_code = execvp(command, argument_list);
        if (status_code == -1) {
            printf("Terminated Incorrectly\n");
            return;
        }
    }
    else {
        // Old Parent process. The C program will come here
        printf("This line will be printed\n");
		fflush(stdout);
    }
	// ui_set_statusbar(TRUE,"drawing number: %s", drwnumber);
}

static void open_tvcfolder_archive(gchar *tvcnumber)
{
	// 20_24_1.003
	// VK20/2024/20_24_2.003
	GError 				*error 		= NULL;
	gchar *locale_path2 = "/mnt/hgfs/tvc/"; // /mnt/hgfs/tvc/
	gchar year[3] = "";
	memset(year, '\0', sizeof(year));
	strncpy(year, tvcnumber+3, 2);
	//strncpy(year+1, tvcnumber+4, 1);
	gchar one[15] = "";
	memset(one, '\0', sizeof(one));
	strncpy(one, "VK", 2);
	strncpy(one+2,tvcnumber, 2);
	strncpy(one+4, "/", 1);
	if(atoi(year)>80){
		strncpy(one+5, "19", 2);
		strncpy(one+7, year, 2);
		strncpy(one+9, "/", 1);
	}else{
		strncpy(one+5, "20", 2);
		strncpy(one+7, year, 2);
		strncpy(one+9, "/", 1);
	}
	// gchar *prepath = g_strconcat(locale_path2, one, NULL); //
	gchar *fullpath = g_strconcat(locale_path2, one, tvcnumber, "/" ,NULL); // one, 

	DIR* dir = opendir(fullpath);
	if (dir) {
		ui_set_statusbar(TRUE," Folder  %s can be opened", fullpath);
		on_opennautilus(fullpath);
		/* Directory exists. */ 
		
		closedir(dir);
	} else if (ENOENT == errno) {
		ui_set_statusbar(TRUE,"doesn't exist: %s", fullpath);
		/* Directory does not exist. */
	} else {
		/* opendir() failed for some other reason. */
	}
}
/*
static void
on_menu_open_nautilus(GtkMenuItem *menuitem, const gchar *uri)
{ 
	gchar *cwd;
	cwd = g_strdup(addressbar_last_address);
		
	char* argument_list[] = {addressbar_last_address , NULL};
	spawn_async(cwd, "nautilus", argument_list, NULL, NULL, NULL);
	g_free(cwd);
}*/

static void open_drw_in_viewer()
{
	GeanyDocument *doc = document_get_current();
	gchar *f_content;
	GtkClipboard *clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	gchar drwnumber[10];
	gchar tvcnumber[12];
	size_t maxGroups = 5;
	regmatch_t groupArray[maxGroups];
	regex_t regex01,regextvc;
	int reti01,reti02,retitvc,retitvc2=TRUE;

	//get content from clipboard
	gchar *text = gtk_clipboard_wait_for_text(clip);

	if (doc && !doc->readonly)
	{
		// einlesen des selektierten bereichs
		ScintillaObject *sci = doc->editor->sci;
		f_content = sci_get_selection_contents(doc->editor->sci);
		
		reti01 = regcomp(&regex01, "(([0-9][A-Z])[0-9]{2}[0-9][A-Z0-9]*)", REG_EXTENDED | REG_ICASE);

		retitvc = regcomp(&regextvc, "([0-9]{2}_[0-9]{2}_[0-9]\.[0-9]{3})", REG_EXTENDED | REG_ICASE);
		//reti01 = regcomp(&regex01, "(.*)", REG_EXTENDED | REG_ICASE);

		reti01 = regexec(&regex01, f_content, maxGroups, groupArray, 0);
		if(!reti01)
		{
			char sourcecopy[strlen(f_content)+1];
			strcpy(sourcecopy, f_content);
			sourcecopy[groupArray[1].rm_eo] = 0;
			strncpy(drwnumber,sourcecopy + groupArray[1].rm_so,10);
			find_correct_drw(drwnumber);
		}
		reti02 = regexec(&regex01, text, maxGroups, groupArray, 0);
		if (!reti02 && reti01) {
			char sourcecopy[strlen(text)+1];
			strcpy(sourcecopy, text);
			sourcecopy[groupArray[1].rm_eo] = 0;
			strncpy(drwnumber,sourcecopy + groupArray[1].rm_so,10);
			find_correct_drw(drwnumber);
		}
		retitvc = regexec(&regextvc, f_content, maxGroups, groupArray, 0);
		if (!retitvc && reti02 && reti01) {
			char sourcecopy[strlen(f_content)+1];
			strcpy(sourcecopy, f_content);
			sourcecopy[groupArray[1].rm_eo] = 0;
			strncpy(tvcnumber,sourcecopy + groupArray[1].rm_so,11);
			open_tvcfolder_archive(tvcnumber);
		}
		retitvc2 = regexec(&regextvc, text, maxGroups, groupArray, 0);
		if (!retitvc2 && retitvc && reti02 && reti01) {
			char sourcecopy[strlen(text)+1];
			strcpy(sourcecopy, text);
			sourcecopy[groupArray[1].rm_eo] = 0;
			strncpy(tvcnumber,sourcecopy + groupArray[1].rm_so,11);
			open_tvcfolder_archive(tvcnumber);
		}

	}
	//ui_set_statusbar(TRUE,"drawing number: %s", drwnumber);
	//check if something is selected
	
	//open number in viewer
	//ui_set_statusbar(TRUE,"drawing number: %s", drwnumbertmp);
	
	regfree(&regextvc);
	regfree(&regex01);
}

void plugin_init(G_GNUC_UNUSED GeanyData *data)
{
	GeanyKeyGroup *plugin_key_group;
	setlocale(LC_NUMERIC, "C");
	int contextmenu = 1;

	plugin_key_group = plugin_set_key_group(geany_plugin, "TVCX_Group", COUNT_KB, NULL);

	/*
	main_menu_item = gtk_menu_item_new_with_mnemonic(_("test _TVC..."));
	gtk_widget_show(main_menu_item);

	gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu), main_menu_item);

	g_signal_connect(main_menu_item, "activate", G_CALLBACK(on_calculate4),
		NULL);
	*/

	keybindings_set_item(plugin_key_group, CHECK_INPUT_KB, on_tvc_check,
		0, 0, "check input", _("checkinput"), NULL);

	keybindings_set_item(plugin_key_group, BRANCHES_INPUT_KB, numbering_branches,
		0, 0, "branches", _("branchesinput"), NULL);

	keybindings_set_item(plugin_key_group, CALCULATE0_KB, on_calculateY0,
		0, 0, "calculate0", _("calculate0"), NULL);
	keybindings_set_item(plugin_key_group, CALCULATE1_KB, on_calculateY1,
		0, 0, "calculate1", _("calculate1"), NULL);
	keybindings_set_item(plugin_key_group, CALCULATE2_KB, on_calculateY2,
		0, 0, "calculate2", _("calculate2"), NULL);
	keybindings_set_item(plugin_key_group, CALCULATE3_KB, on_calculateY3,
		0, 0, "calculate3", _("calculate3"), NULL);
	keybindings_set_item(plugin_key_group, CALCULATE4_KB, on_calculateY4,
		0, 0, "calculate4", _("calculate4"), NULL);
	keybindings_set_item(plugin_key_group, CALCULATE5_KB, on_calculateY5,
		0, 0, "calculate5", _("calculate5"), NULL);
	keybindings_set_item(plugin_key_group, CALCULATE6_KB, on_calculateY6,
		0, 0, "calculate6", _("calculate6"), NULL);
	keybindings_set_item(plugin_key_group, CALCULATE7_KB, on_calculateY7,
		0, 0, "calculate7", _("calculate7"), NULL);
	keybindings_set_item(plugin_key_group, CALCULATE8_KB, on_calculateY8,
		0, 0, "calculate8", _("calculate8"), NULL);
	keybindings_set_item(plugin_key_group, CALCULATE9_KB, on_calculateY9,
		0, 0, "calculate9", _("calculate9"), NULL);
	keybindings_set_item(plugin_key_group, CALCULATE10_KB, on_calculateY10,
		0, 0, "calculate10", _("calculate10"), NULL);

	keybindings_set_item(plugin_key_group, SCANIADAMPER_KB, scania_damper,
		0, 0, "scania_damper", _("scania_damper"), NULL);

	keybindings_set_item(plugin_key_group, OVERWRITE_BLOCK_SPACES_KB, overwrite_block_spaces, 
		0, 0, "overwrite_block_spaces", _("overwrite_block_spaces"), NULL);


	keybindings_set_item(plugin_key_group, OPEN_DRW_IN_VIEWER_KB, open_drw_in_viewer, 
		0, 0, "open_drw_in_viewer", _("open_drw_in_viewer"), NULL);


	//keybindings_set_item(plugin_key_group, OPEN_TVCFOLDER_ARCHIVE_KB, open_tvcfolder_archive, 
	//	0, 0, "open_tvcfolder_archive", _("open_tvcfolder_archive"), NULL);

	
	//keybindings_set_item(plugin_key_group, OVERWRITE_WITH_SPACES_KB, overwrite_with_spaces, 
	//	0, 0, "overwrite_with_spaces", _("overwrite_with_spaces"), NULL);

	keybindings_set_item(plugin_key_group, PERCENT_OF_RATEDSPEED_KB, percent_of_ratedspeed,
		0, 0, "percent_of_ratedspeed", _("percent_of_ratedspeed"), NULL);


	if(contextmenu == 1){

		s_context_sep_item = gtk_separator_menu_item_new();
		gtk_widget_show(s_context_sep_item);
		gtk_menu_shell_prepend(GTK_MENU_SHELL(geany->main_widgets->editor_menu), s_context_sep_item);
		
		s_context_checkinput_item = gtk_menu_item_new_with_mnemonic(_("check inputfile"));
		gtk_widget_show(s_context_checkinput_item);
		gtk_menu_shell_prepend(GTK_MENU_SHELL(geany->main_widgets->editor_menu), s_context_checkinput_item);
		g_signal_connect((gpointer) s_context_checkinput_item, "activate", G_CALLBACK(on_tvc_check), NULL);

		s_context_recount_item = gtk_menu_item_new_with_mnemonic(_("recount branches"));
		gtk_widget_show(s_context_recount_item);
		gtk_menu_shell_prepend(GTK_MENU_SHELL(geany->main_widgets->editor_menu), s_context_recount_item);
		g_signal_connect((gpointer) s_context_recount_item, "activate", G_CALLBACK(numbering_branches), NULL);
		
	}

}

void plugin_cleanup(void)
{
	//gtk_widget_destroy(main_menu_item);
}
