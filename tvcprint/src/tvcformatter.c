#include <geanyplugin.h>
#include <unistd.h>
#include <stdlib.h>

#include "tvcprint.h"
#include "tvcformatter.h"

static void start_formatter(void)
{
    int des_p[2];
	gchar *locale_path, *locale_filename;
	GeanyDocument *doc = document_get_current();
	//locale_path = g_path_get_dirname(doc->file_name); // only the current path
    
    if (doc != NULL)
	{
		locale_path = g_path_get_dirname(doc->file_name); // only the current path
		locale_filename = doc->file_name; 
	}

    if(pipe(des_p) == -1) {
        perror("Pipe failed");
        exit(1);
    }

	//char* command = "/home/miki/awk/tvcformatter";
    //char* argument_list[] = {"/home/miki/awk/tvcformatter -F ", locale_filename, " | " , "xclip", "-selection clipboard" , NULL}; //| xclip -selection clipboard
	
	//char* command = "/bin/bash";
    //char* argument_list[] = {"/bin/bash", "-c", "cat " , locale_filename, NULL}; //"| xclip -selection clipboard" 
    /*
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
    */
    int pid = fork();
    if(pid == 0)            //first fork
    {
        close(STDOUT_FILENO);  //closing stdout
        dup(des_p[1]);         //replacing stdout with pipe write 
        close(des_p[0]);       //closing pipe read
        close(des_p[1]);

        char* prog1[] = { "/home/tvc/tools/awk/tvcformatter", locale_filename, 0};
        execvp(prog1[0], prog1);
        perror("execvp of formatter failed");
        exit(1);
    }

    pid = fork();
    if(pid == 0)            //creating 2nd child
    {
        close(STDIN_FILENO);   //closing stdin
        dup(des_p[0]);         //replacing stdin with pipe read
        close(des_p[1]);       //closing pipe write
        close(des_p[0]);

        char* prog2[] = { "/usr/bin/xclip", "-selection" , "clipboard", 0};
        execvp(prog2[0], prog2);
        perror("execvp of xclip failed");
        exit(1);
    }

    close(des_p[0]);
    close(des_p[1]);
    wait(0);
    wait(0);
    //return 0;
}



void formatterfunc()
{
    ui_set_statusbar(TRUE,"hello from the formatter");
    start_formatter();
}

