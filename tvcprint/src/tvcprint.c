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


static void set_widgets_sensitive(gboolean sensitive)
{
	gtk_widget_set_sensitive(GTK_WIDGET(s_context_fdec_item), TRUE);
}

void exa_1 (void)
{ int n = 100, i;
  double fpi = 3.1415926 / 180., step, x;

  step = 360. / (n-1);

  for (i = 0; i < n; i++)
  { xray[i] = (float) (i * step);
    x = xray[i] * fpi;
    y1ray[i] = (float) sin (x);
    y2ray[i] = (float) cos (x);
  }

  disini ();
  pagera ();
  complx ();
  axspos (450, 1800);
  axslen (2200, 1200);

  name   ("X-axis", "x");
  name   ("Y-axis", "y");

  labdig (-1, "x");
  ticks  (10, "xy");

  titlin ("Demonstration of CURVE", 1);
  titlin ("SIN(X), COS(X)", 3);

  graf   (0.f, 360.f, 0.f, 90.f, -1.f, 1.f, -1.f, 0.5f);
  title  ();

  color  ("red");
  curve  (xray, y1ray, n);
  color  ("green");
  curve  (xray, y2ray, n);

  color  ("fore");
  dash   ();
  xaxgit ();
  disfin ();
}

/*
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
*/
static void printer(GtkMenuItem *menuitem, gpointer user_data)
{
    printf(" test 3 ");
    fflush(stdout);
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