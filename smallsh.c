// Author: Jacob Russell
// Date: 01-24-22
// Class: CS344 Operating Systems
// Description: a small shell

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>


struct CommandLine {
    char* command;    
    int args_array[2100];
    char* input_file;
    char* output_file;
    char* background_flag;
};

char * get_command(char command[]);

int main()
{
    char command[2100];

    while(1){
        
        get_command(command);
        
        printf("Your command: %s", command);  //for debugging
        
    }

    return 0;
}

char * get_command(char command[])
{
    //Set up getline()
    char *curr_line = NULL;
    size_t buff_size = 0;
    //ssize_t num_char;
    curr_line = malloc(buff_size * sizeof(char));

    putchar(':');
    getline(&curr_line, &buff_size, stdin);

    strcpy(command, curr_line);

    return command;
}
