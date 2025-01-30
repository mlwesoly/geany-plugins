#include <geanyplugin.h>


#include "tvcformatter.h"
#include "tvcprint.h"
#include "tvcmatrixreader.h"
#include <locale.h>


#define MAX_LINES 250000

static gint start_pos, start_line;
static gint end_pos, end_line;
float kernel[300][50];

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
		test = strtok(buffer3," \t");
		int j = 0;
		while( test != NULL ) {
			kernel[i+1][j] = strtod(test, NULL);
			sum+=strtod(test, NULL);
			test = strtok(NULL," \t");
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


void matrixreader()
{
    collect_data(FALSE);
}