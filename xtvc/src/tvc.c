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

#include <geanyplugin.h>

GeanyPlugin	*geany_plugin;
GeanyData	*geany_data;

PLUGIN_VERSION_CHECK(224)

PLUGIN_SET_TRANSLATABLE_INFO(LOCALEDIR, GETTEXT_PACKAGE,
	_("TVCX"), _("Plugin helping to check input file,"
	"\ncalculate inline, "
	""),
	"0.0.1", "")

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
	COUNT_KB
};

static void on_calculateX(int Stellen)
{
	GeanyDocument *doc = document_get_current();
	GtkClipboard *clipboard, *primary;
	gchar *f_content; //, *f_content_tmp;
	int error;
	size_t maxGroups = 5;
	regmatch_t groupArray[maxGroups];
	regex_t regex01,regex02,regex03;
	int reti01,reti02,reti03,reti04;
	float Pkv30, Pkvtemp, Pkvresult;	
	long double b=0.0;

	gint selection_end;
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
			sci_insert_text(sci, selection_end, buf);
			ui_set_statusbar(TRUE, "result: %Lf", buf);
			sci_set_current_position (sci, selection_end, TRUE);
			
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


//G_GNUC_UNUSED GtkMenuItem *menuitem, G_GNUC_UNUSED	gpointer gdata
static void on_tvc_check()
{
	GeanyDocument *doc = document_get_current();
	gchar *my_string;
	// ScintillaObject *sci = doc->editor->sci;
	gint len = sci_get_length(doc->editor->sci) + 1;
	my_string = sci_get_contents(doc->editor->sci, len);

	size_t maxGroups = 3;
	regmatch_t groupArray[maxGroups];
	char systemanz[5];

	int dampmodel=0, dampdefine=0, systemline=0, gearmeshes=0, gearmeshdefs=0;
	int AnzahlModelLines=0, zerosnotfours=0;
	
	regex_t regex01,regex02,regex03,regex04,regex05,regex06,regex07,regex08,regex09,regex10,regex11,regex12,regex13;
	int reti01,reti02,reti03,reti3,reti4,reti5,reti6,reti7,reti8,reti9,reti10,reti11,reti12;	

	/* Compile regular expression */
    const char * curLine = my_string;
	
	// Gesamtanzahl der Modellzeilen 
	reti01 = regcomp(&regex01, "^(1[0-9][0-9][0-9]) *('.*')+ *(([+-]?([0-9]+[.])+[0-9]+ *)|( *-1 *)){3} [0-9]((\n)*.*)", REG_EXTENDED);
	
	//element 2. 
    reti02 = regcomp(&regex02, "^(1[0-9][0-9][0-9]) *('.*')+ *(([+-]?([0-9]+[.])+[0-9]+ *)|( *-1 *)){3} 2((\n)*.*)", REG_EXTENDED);
    reti03 = regcomp(&regex03, "^22[0-9][0-9] .*", REG_EXTENDED);
	
	//Systemstellen auszuwerten
	reti3 = regcomp(&regex04, "^3100 * '(systems|SYSTEMS)' *([0-9]+).*", REG_EXTENDED); // issue: erkennt die nummer nicht
    reti4 = regcomp(&regex05, "^31[0-9][0-9] *[0-9]{1,4} .*", REG_EXTENDED);
	
	//Hooke
	reti5 = regcomp(&regex06, "^99999999", REG_EXTENDED);	// ?? verstehe ich hier gerade nicht
    reti6 = regcomp(&regex07, "42[1-9][0-9] *([+-]?([0-9]*[.])?[0-9]+ *)*.*", REG_EXTENDED);

	//Gear Meshes
	reti7 = regcomp(&regex08, "^(1[0-9][0-9][0-9]) *('.*')+ *(([+-]?([0-9]+[.])+[0-9]+ *)|( *-1 *)){3} 10((\n)*.*)", REG_EXTENDED);
    reti8 = regcomp(&regex09, "^410[0-9] .*([+-]?([0-9]*[.])?[0-9]+ *)*.*", REG_EXTENDED);
	
	//engine min drehzahl 2110 260.0 385.0 4 1 1920.0 750.0 500.0     " *(([+-]?([0-9]+[.])+[0-9]+ *))" negative lookahead *(?!([+-]?([0-9]+[.])+[0-9]+))
	reti9 = regcomp(&regex10, "^(211[0-9]) *([+-]?([0-9]+[.])+[0-9]+ *){2} *[0-9] *[0-9] *([+-]?([0-9]+[.])+[0-9]+ *){2} *((\n).*)", REG_EXTENDED);
	
	//powercurves number of columns 
    reti10 = regcomp(&regex11, "^40[1-9][0-9] .*([+-]?([0-9]*[.])?[0-9]+ *)*.*", REG_EXTENDED);
	
	// wenn eine zeile 5 dann auch 2510 definition

	// wenn eine zeile 15 dann auch 4150 definition, wenn zwei, dann mehr spalten

	// wenn steifigkeit 0 oder 0.0 dann muss element in der zeile sein
	reti11 = regcomp(&regex12, "^(1[0-9][0-9][0-9]) *('.*')+ *(([+-]?([0-9]+[.])+[0-9]+ *)|( *-1 *)) [0]+([.][0]*)* *(([+-]?([0-9]+[.])+[0-9]+ *)|( *-1 *)) ([0-3]|[5-9])((\n)*.*)", REG_EXTENDED);
   
	//reti7 = regcomp(&regex7, "^(1[0-9][0-9][0-9]) *('.*')+ *(([+-]?([0-9]+[.])+[0-9]+ *)|( *-1 *)){3} 5((\n)*.*)", REG_EXTENDED); 2510 R1 R2
	
	//reti7 = regcomp(&regex7, "^(1[0-9][0-9][0-9]) *('.*')+ *(([+-]?([0-9]+[.])+[0-9]+ *)|( *-1 *)){3} 15((\n)*.*)", REG_EXTENDED); 4150  ...
	/*
		first counting the elemente 15 elements
		then checking if really 10 inputlines for the damper are there
		?? checking if damperdata inside / 
	*/
    reti12 = regcomp(&regex13, "^410[0-9] .*([+-]?([0-9]*[.])?[0-9]+ *)*.*", REG_EXTENDED);

	if ( reti01 || reti02 || reti03 || reti3 || reti4 || reti5 || reti6) {
		fprintf(stderr, "Could not compile regex\n");
		exit(1);
	}
	if (reti9 || reti10 || reti8 || reti7 || reti11 || reti12) {
		fprintf(stderr, "Could not compile regex ab 7\n");
		exit(1);
	}
	ui_set_statusbar(TRUE, "\n\t\t ----Modellcheck :----");
	while(curLine)
    {
      	const char * nextLine = strchr(curLine, '\n');
      	int curLineLen = nextLine ? (nextLine-curLine) : strlen(curLine);
      	char * tempStr = (char *) malloc(curLineLen+1);
		if (tempStr)
      	{
        	memcpy(tempStr, curLine, curLineLen);
         	tempStr[curLineLen] = '\0';  // NUL-terminate!

			// Gesamtanzahl der Modellzeilen
			reti01 = regexec(&regex01, tempStr, 0, NULL, 0);
			if (!reti01) {
				AnzahlModelLines=AnzahlModelLines+1;
			}
			
			// Anzahl von damping lines im Modell
		 	reti02 = regexec(&regex02, tempStr, 0, NULL, 0);
		 	if (!reti02) {
				dampmodel=dampmodel+1;
			}
			// Anzahl von damping lines in den Definitionen
			reti03 = regexec(&regex03, tempStr, 0, NULL, 0);
		 	if (!reti03) {
				dampdefine=dampdefine+1;
			}

			reti3 = regexec(&regex04, tempStr, maxGroups, groupArray, 0);
		 	if (!reti3) {
				char sourcecopy[strlen(tempStr)+1];
				strcpy(sourcecopy, tempStr);
				sourcecopy[groupArray[2].rm_eo] = 0;
				strcpy(systemanz,sourcecopy + groupArray[2].rm_so);
				// printf("string: %s  rest: %s",sourcecopy, systemanz);
				// fflush(stdout);
			}
			reti4 = regexec(&regex05, tempStr, 0, NULL, 0);
		 	if (!reti4) {
				systemline=systemline+1;
			}
			
			reti7 = regexec(&regex08, tempStr, 0, NULL, 0);
		 	if (!reti7) {
				gearmeshes=gearmeshes+1;
			}
			reti8 = regexec(&regex09, tempStr, 0, NULL, 0);
		 	if (!reti8) {
				gearmeshdefs=gearmeshdefs+1;
			}

			reti9 = regexec(&regex10, tempStr, 0, NULL, 0);
		 	if (!reti9) {
				ui_set_statusbar(TRUE, "\t\t---Engine has no minimal rpm");
			}
			reti11 = regexec(&regex12, tempStr, 0, NULL, 0);
		 	if (!reti11) {
				zerosnotfours=zerosnotfours+1;
			}
        	free(tempStr);
      	}
      	else printf("malloc() failed!?\n");
		curLine = nextLine ? (nextLine+1) : NULL;
	}
	/* Execute regular expression */
	/*
	- if stiffness == 0.0 or 0, dann muss es auch element 4 sein
	- motor testen: minimaldrehzahl vorhanden?
	- 2130 falls 0, keine 2135 oder 26.. 
		   falls 1,2 dann 2135 und 26..
	- checking if everywhere same amount of power factors
	- if 15 then there should be some definitions 
	- if 5 then there should be a definition
	- if 6 also check for definition
	*/
	
	ui_set_statusbar(TRUE, "\t Gesamtanzahl der Elemente vom Model \t %d", AnzahlModelLines);

	ui_set_statusbar(TRUE, "\t\t ----Damping Element 2:----");
	ui_set_statusbar(TRUE, "\t im Modell \t %d", dampmodel);
	ui_set_statusbar(TRUE, "\t definiert \t %d ", dampdefine);


	ui_set_statusbar(TRUE, "\t\t ----3100 Systemstellen:----");
	ui_set_statusbar(TRUE, "\t auszuwerten \t %s ",systemanz);
	ui_set_statusbar(TRUE, "\t definiert \t %d ",systemline);

	if(gearmeshes>0 || gearmeshdefs>0) //spaeter if gearmeshes == gearmeshdefs, nur kurze ausgabe
	{
		ui_set_statusbar(TRUE, "\t\t ----Gear-Meshes 10:----");
		ui_set_statusbar(TRUE, "\t im Modell \t %d ",gearmeshes);
		ui_set_statusbar(TRUE, "\t definiert \t %d ",gearmeshdefs);
	}
	if(zerosnotfours)
	{
		ui_set_statusbar(TRUE, "\t\t-  %d-mal 0 Steifigkeit die nicht 4Elemente sind",zerosnotfours);
	}

	/* Free memory allocated to the pattern buffer by regcomp() */
	regfree(&regex01);
	regfree(&regex02);
	regfree(&regex03);
	regfree(&regex04);
	regfree(&regex05);
	regfree(&regex06);
	regfree(&regex07);
	regfree(&regex08);
	regfree(&regex09);
	regfree(&regex11);



}

// G_GNUC_UNUSED GtkMenuItem *menuitem, G_GNUC_UNUSED gpointer gdata
static void on_branches_check()
{ 
	GeanyDocument *doc = document_get_current();
	ScintillaObject *sci = doc->editor->sci;

	gint numberoflines = sci_get_line_count(sci);

	regex_t regex01, regex02, regex03;
	int reti01, reti02, reti03;
	int id = 0, j = 0;
	int AnzahlModelLines = 0, lastModelLine = 1;
	int modelrow = 1051, branchcount=1; // anzahl der branches, 
	int matrix [25][2] = {0};
	int schalter = 0;
	
	/* Compile regular expression */
	// normale Zeile im SystemModell 
	reti01 = regcomp(&regex01, "^([0-9][0-9][0-9][0-9]) *('.*')+ *(([+-]?([0-9]+[.])+[0-9]+ *)|( *-1 *)){3} [0-9]((\n)*.*)", REG_EXTENDED);
    // Leerzeile im SystemModell
	reti02 = regcomp(&regex02, "^[ \t]*\n\0", REG_EXTENDED);
	reti03 = regcomp(&regex03, "^105[0-9] *[0-9]* *[0-9]((\n)*.*)", REG_EXTENDED);
	
	matrix[0][0] = 1;

	if (reti01 || reti02 || reti03 ) 
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
				AnzahlModelLines++;
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
				if(compareLines <= AnzahlModelLines)
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
				if((compareLines < AnzahlModelLines) && (compareLines > 0))
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
		gint einfuegezeile=lastModelLine-(AnzahlModelLines+branchcount-1);
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
			if(matrix[row][0]>0)
			{
				einfuegezeile=einfuegezeile+1;
				startofline = sci_get_position_from_line(sci,einfuegezeile);
				sprintf(helper,"%d %02d %02d\n", modelrow, matrix[row][0]%100, matrix[row][1]%100 );
				ui_set_statusbar(TRUE, "%s",helper);
				sci_insert_text(sci, startofline, helper);
				modelrow=modelrow+1;
			}
		}

		fflush(stdout);
		regfree(&regex02);
		regfree(&regex01);
	}
	/* Free memory allocated to the pattern buffer by regcomp() */
	
	// falls ^105x zeilen vorhanden, dann loeschen!
	/*
	for(gint i=AnzahlModelLines; i > 0; i=i-1)
	{
		gchar * test = sci_get_line(sci,i);
		
		reti03 = regexec(&regex03, test, 0, NULL, 0);
		if (!reti01) {
			
		}
	}
	*/
	
}


// G_GNUC_UNUSED GtkMenuItem *menuitem, G_GNUC_UNUSED gpointer gdata
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
		int elem[] = {4159,4158,4157,4156,4155,4154,4153,4152,4151,4150};

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
		ui_set_statusbar(TRUE, "input nicht okay ( aC bC aD bD := Integer Float Integer Float )");
	}
	regfree(&regex01);
}


/*
TODO: static void CATERPILLAR_damper()

checking by dampernumber, if data is already in list
then placing before holzer area


*/

/*
static void on_tools_show(G_GNUC_UNUSED GtkMenuItem *menuitem, G_GNUC_UNUSED gpointer gdata)
{
	gtk_widget_set_sensitive(main_menu_item, TRUE); //can_insert_numbers());
}



void plugin_help(void)
{
	GtkWidget *dialog,*label,*scroll;

	dialog=gtk_dialog_new_with_buttons(_("Numbered Bookmarks help"),
	                                   GTK_WINDOW(geany->main_widgets->window),
	                                   GTK_DIALOG_DESTROY_WITH_PARENT,
	                                   GTK_STOCK_OK,GTK_RESPONSE_ACCEPT,NULL);

	label=gtk_label_new(
_("This Plugin implements Numbered Bookmarks in Geany, as well as remembering the state of folds, \
and positions of standard non-numbered bookmarks when a file is saved.\n\n"
"It allows you to use up to 10 numbered bookmarks. To set a numbered bookmark press Ctrl+Shift+a n\
"));
	gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
	gtk_widget_show(label);


	scroll=gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy((GtkScrolledWindow*)scroll,GTK_POLICY_NEVER,
	                               GTK_POLICY_AUTOMATIC);
#if GTK_CHECK_VERSION(3, 8, 0)
	gtk_widget_set_size_request(label, 800, -1);
	gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
	gtk_widget_set_vexpand(label, TRUE);
	gtk_container_add(GTK_CONTAINER(scroll),label);
	gtk_scrolled_window_set_shadow_type((GtkScrolledWindow*)scroll, GTK_SHADOW_IN);
#else
	gtk_scrolled_window_add_with_viewport((GtkScrolledWindow*)scroll,label);
#endif

	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),scroll);
	gtk_widget_show_all(dialog);

	gtk_widget_set_size_request(dialog,-1,600);


	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

*/


void plugin_init(G_GNUC_UNUSED GeanyData *data)
{
	GeanyKeyGroup *plugin_key_group;
	setlocale(LC_NUMERIC, "C");

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

	keybindings_set_item(plugin_key_group, BRANCHES_INPUT_KB, on_branches_check,
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

	keybindings_set_item(plugin_key_group, PERCENT_OF_RATEDSPEED_KB, percent_of_ratedspeed,
		0, 0, "percent_of_ratedspeed", _("percent_of_ratedspeed"), NULL);
	
	/*
	plugin_signal_connect(geany_plugin, G_OBJECT(geany->main_widgets->tools_menu), "show",
		TRUE, (GCallback) on_tools_show, NULL);
	*/
}

void plugin_cleanup(void)
{
	//gtk_widget_destroy(main_menu_item);
}
