/*
 *	  Copyright 2010-2014 Jiri Techet <techet@gmail.com>
 *
 *	  This program is free software; you can redistribute it and/or modify
 *	  it under the terms of the GNU General Public License as published by
 *	  the Free Software Foundation; either version 2 of the License, or
 *	  (at your option) any later version.
 *
 *	  This program is distributed in the hope that it will be useful,
 *	  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	  GNU General Public License for more details.
 *
 *	  You should have received a copy of the GNU General Public License
 *	  along with this program; if not, write to the Free Software
 *	  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <geanyplugin.h>
#include "dislin.h"

#include <errno.h>
#include <glib/gstdio.h>

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <math.h>


GeanyPlugin	*geany_plugin;
GeanyData	*geany_data;

/*
enum
{
	KB_COUNT
};
*/

static GtkWidget *s_context_fdec_item, *s_context_sep_item, *s_sep_item;

#define MAX_LINES 250000

PLUGIN_VERSION_CHECK(GEANY_API_VERSION)


PLUGIN_SET_TRANSLATABLE_INFO(LOCALEDIR, GETTEXT_PACKAGE,
	_("TVC print"), _(" "
	"\n "
	"  "),
	"0.0.1", " ")

static GtkWidget *main_menu_item = NULL;
static gint start_pos, start_line;
static gint end_pos, end_line;

static float xray[300], yray[300], y1ray[300], y2ray[300], zmat[50][50];
static char  cl1[500], cl2[500];
static int id_lis1, id_lis2, id_pbut;


static int n_rows = 200;
static int n_cols = 50;
static float kernel[300][50];

// TODO : falls erste zeile namen hat, einlesen für die legende?
// oder legende standardmäßig mit spalte 1, spalte 2 ...beschriften


static void set_widgets_sensitive(gboolean sensitive)
{
	gtk_widget_set_sensitive(GTK_WIDGET(s_context_fdec_item), TRUE);
}

static float getMax(int c, int n, float a[200][50])
    {
        //for (int i = 0; i < n; i++)
        //{
        float max = a[1][n];

        for (int j = 1; j <= n; j++)
        {
            //printf("%.f",a[j][c]);
            fflush(stdout);
            if (a[j][c] > max)
                max = a[j][c];
        }
        // printf("%f ",max);
        //}

        return(max);
    }

static float getMin(int c,int n, float b[200][50])
    {

        //for (int i = 0; i < n; i++) {
        float min = b[1][c];
        for (int j = 1; j <= n; j++)
        {
            if (b[j][c] < min)
                min = b[j][c];
        }
        //  printf("%f ",min);
        // }
        return(min);
    }


void exa_1 (void)
{ 
    int n = (int) kernel[0][0], i;
    int m = (int) kernel[0][1];
	float xsteps,ysteps,xstart,ystart,xstop,ystop;
    if (m<2) return;
    float xmax, xmin, ymax, ymin;
    float tempymax,tempymin;

    for (i = 0; i < n; i++)
    { xray[i] = (float) kernel[i+1][0];

     // y1ray[i] = (float) kernel[i+1][1];
    //y2ray[i] = (float) ;
    }

    metafl ("cons");
    scrmod ("revers");
    window (20, 20, 1200, 850);
    disini ();
    pagera ();
    complx ();
    axspos (350, 1800); // to the right , down
    axslen (2500, 1500);

    name   ("X-axis", "x");
    name   ("Y-axis", "y");

    labdig (-1, "x");
    ticks  (10, "xy");

    titlin ("Demo", 2);
    //titlin ("SIN(X), COS(X)", 3);

    xmax = getMax(0,n,kernel);
    xmin = getMin(0,n,kernel);
    for (i = 1; i < m; i++){
        tempymax = getMax(i,n,kernel);
        tempymin = getMin(i,n,kernel);
        if(i==1){
            ymin=tempymin;
            ymax=tempymax;
        }else{
            if(ymin>tempymin){
                ymin = tempymin;
            }
            if(ymax<tempymax){
                ymax=tempymax;
            }
        }
    }

    
    
	xsteps = round(xmax-xmin)/8.0;
	ysteps = round(ymax-ymin)/8.0;
	xstart = xmin;
	ystart = ymin;
	if(xmin>0){
		xmin=xmin*0.95;
	}
	if(xmax<0){
		xmax=xmax*0.95;
	}else{
		xmax=xmax*1.05;
	}

	if(ymin>0){
        ystart=0.0;
		ymin=0.0;
    }else{
		ymin=ymin*1.05;
	}
	if(ymax>0){
		ymax=ymax*1.05;
	}else{
		ymax=0.0;
	}

    
    graf   (xmin, xmax, xstart, xsteps, ymin, ymax, ystart, ysteps);
    title  ();

    color  ("red");
	chncrv ("COLOR");
	inccrv (1);	
    for(int h = 1;h < m; h++)
    {
        for (i = 0; i < n; i++)
        {
            y1ray[i] = (float) kernel[i+1][h];
        }
        curve  (xray, y1ray, n);
    }

    color  ("fore");
    dash   ();
    xaxgit ();
    disfin ();
}


static gboolean can_insert_numbers(void)
{
	GeanyDocument *doc = document_get_current();

	if (doc && !doc->readonly)
	{
		ScintillaObject *sci = doc->editor->sci;

		if (sci_has_selection(sci) && (sci_get_selection_mode(sci) == SC_SEL_RECTANGLE ||
			sci_get_selection_mode(sci) == SC_SEL_THIN))
		{
			start_pos = sci_get_selection_start(sci);
			start_line = sci_get_line_from_position(sci, start_pos);
			end_pos = sci_get_selection_end(sci);
			end_line = sci_get_line_from_position(sci, end_pos);

			return end_line - start_line < MAX_LINES;
		}
	}

	return FALSE;
}

#define sci_point_x_from_position(sci, position) \
	scintilla_send_message(sci, SCI_POINTXFROMPOSITION, 0, position)
#define sci_get_pos_at_line_sel_start(sci, line) \
	scintilla_send_message(sci, SCI_GETLINESELSTARTPOSITION, line, 0)
#define sci_get_endpos_at_line_sel_start(sci, line) \
	scintilla_send_message(sci, SCI_GETLINESELENDPOSITION, line, 0)

static void collect_data(gboolean *cancel)
{
    can_insert_numbers();
	/* editor */
	ScintillaObject *sci = document_get_current()->editor->sci;
	gint xinsert = sci_point_x_from_position(sci, start_pos);
	gint xend = sci_point_x_from_position(sci, end_pos);
	gint *line_pos = g_new(gint, end_line - start_line + 1);
	gint line_endpos=0;
	gint line, i;
	/* generator */
	gint64 value;
	unsigned count = 0;
	size_t prefix_len = 0;
	int plus = 0, minus;
	size_t length, lend;
	gchar *buffer;
	gchar *buffer2,*buffer3;
	gint numlength;
	gdouble sum=0.0;

	if (xend < xinsert)
		xinsert = xend;

	ui_progress_bar_start(_("Counting..."));
	/* lines shorter than the current selection are skipped */
	line_endpos = sci_get_endpos_at_line_sel_start(sci,start_line) -
				sci_get_position_from_line(sci, start_line);

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

		if (cancel && i % 2500 == 0)
		{
			//update_display();
			if (*cancel)
			{
				ui_progress_bar_stop();
				g_free(line_pos);
				return;
			}
		}
	}

	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(geany->main_widgets->progressbar),
		_("Preparing..."));

	//update_display();
	sci_start_undo_action(sci);
	//sci_replace_sel(sci, "");

	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(geany->main_widgets->progressbar),
		_("Inserting..."));
	for (line = start_line, i = 0; line <= end_line; line++, i++)
	{
		gchar *beg, *end;
		gint insert_pos;
		gint old_pos_end;
		gchar **parts;
		char * test;


		if (line_pos[i] < 0)
			continue;

		insert_pos = sci_get_position_from_line(sci, line) + line_pos[i];
		buffer2 = sci_get_contents_range(sci,insert_pos, insert_pos + ( line_endpos - line_pos[i]) );

		buffer3 = buffer2;
		test = strtok(buffer3," ");
		int j = 0;
		while( test != NULL ) {
			kernel[i+1][j] = strtod(test, NULL);
			sum+=strtod(test, NULL);
			test = strtok(NULL," ");
            //printf("%f \n",kernel[i][j]);
			j++;
		}

        kernel[0][1] = j;
        kernel[0][0] = i + 1;
		//printf("\n");

		//fflush(stdout);
	}
    /*
	for (int i = 0; i < 20; i++){
		for (int j = 0; j < 3; j++){
			printf("%f ", kernel[i][j]);
		}
		printf("\n");
	}
    */
	ui_set_statusbar(TRUE,"%.2f",sum);
	sci_end_undo_action(sci);
	g_free(buffer);
	g_free(line_pos);
	ui_progress_bar_stop();
}

static void printer(GtkMenuItem *menuitem, gpointer user_data)
{
    printf(" test 3 ");
    fflush(stdout);
    collect_data(FALSE);
	exa_1();
}


void plugin_init(G_GNUC_UNUSED GeanyData *data)
{
    GeanyKeyGroup *key_group;
    

	//key_group = plugin_set_key_group(geany_plugin, "GeanyCtags", KB_COUNT, kb_callback);

    printf("test1");
    fflush(stdout);
    
    s_context_sep_item = gtk_separator_menu_item_new();
	gtk_widget_show(s_context_sep_item);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(geany->main_widgets->editor_menu), s_context_sep_item);
    
    s_context_fdec_item = gtk_menu_item_new_with_mnemonic(_("2D graph printer"));
	gtk_widget_show(s_context_fdec_item);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(geany->main_widgets->editor_menu), s_context_fdec_item);
	g_signal_connect((gpointer) s_context_fdec_item, "activate", G_CALLBACK(printer), NULL);


	gtk_widget_set_sensitive(GTK_WIDGET(s_context_fdec_item), TRUE);
}


void plugin_cleanup(void)
{
    gtk_widget_destroy(s_context_fdec_item);
	gtk_widget_destroy(s_context_sep_item);
	gtk_widget_destroy(s_sep_item);

}