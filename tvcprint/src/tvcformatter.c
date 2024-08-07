#include <geanyplugin.h>
#include <unistd.h>

#include "tvcprint.h"
#include "tvcformatter.h"


static void start_formatter(void)
{
	gchar *locale_path, *locale_filename;
	GeanyDocument *doc = document_get_current();
	//locale_path = g_path_get_dirname(doc->file_name); // only the current path
    
    if (doc != NULL)
	{
		locale_path = g_path_get_dirname(doc->file_name); // only the current path
		locale_filename = doc->file_name; 
	}

    //printf("%s",locale_filename);

	//char* command = "/home/miki/awk/tvcformatter";
    //char* argument_list[] = {"/home/miki/awk/tvcformatter -F ", locale_filename, NULL}; //| xclip -selection clipboard
	
	char* command = "/bin/bash";
    char* argument_list[] = {"/bin/bash", "-c", "cat " , locale_filename, NULL}; //"| xclip -selection clipboard" 

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
}



void formatterfunc()
{
    ui_set_statusbar(TRUE,"hello from the formatter");
    start_formatter();
}
