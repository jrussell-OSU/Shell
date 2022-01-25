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
    char * cmd;    
    char * arguments;
    char * input_file;
    char * output_file;
    int background_flag;
};

char * get_command(char command_line[]);
struct CommandLine *parse_command(char command_line[]);

int main()
{
    char command_line[2100];

    while(1){
        
        get_command(command_line);
        //printf("Your command: %s", command_line);  //for debugging
        
        parse_command(command_line);
        
    }

    return 0;
}

char * get_command(char command_line[])
{
    //Set up getline()
    char *curr_line = NULL;
    size_t buff_size = 0;
    //ssize_t num_char;
    curr_line = malloc(buff_size * sizeof(char));

    putchar(':');
    getline(&curr_line, &buff_size, stdin);

    strcpy(command_line, curr_line);
    
    free(curr_line);
    return command_line;
}


struct CommandLine *parse_command(char command_line[])
{
    char *saveptr; //holds place for strtok_r
    char *token; //holds delim'd tokens from strtok_r
    char args_array[1000];
    
    struct CommandLine *curr_command = malloc(sizeof(struct CommandLine)); //create (allocate memory for) CommandLine structure
    
    token = strtok_r(command_line, " ", &saveptr); //holds current delimn'd token from the command_line user input

    //Get "command" portion of user command (first delim'd token)
    curr_command->cmd = calloc(strlen(token) + 1, sizeof(char));
    strcpy(curr_command->cmd, token);
    printf("Command from line is: %s\n", curr_command->cmd); // for debug

    //Get "arguments" (if any)
    while (!strncmp("-", (token = strtok_r(NULL, " ", &saveptr)), 1)){
        strcat(args_array, token);
        //strcat(curr_command->arguments, token);
        //printf("Arguments: %s\n", token);  //DEBUGGING
    }
    printf("Arguments: %s\n", args_array);  //DEBUGGING
    //printf("Arguments: %s\n", curr_command->arguments);  //DEBUGGING

    //Get "input_file" (if any)
    if (!strcmp("<", token)){ //if it begins with "<" it's an input file
        token = strtok_r(NULL, " ", &saveptr);
        curr_command->input_file = calloc(strlen(token) + 1, sizeof(char));
        strcpy(curr_command->input_file, token);
        printf("Input file: %s\n", curr_command->input_file); // for debug
        token = strtok_r(NULL, " ", &saveptr);
    }

    //Get "output_file" (if any)
    if (!strcmp(">", token)){ //if it begins with "<" it's an output file
        token = strtok_r(NULL, " ", &saveptr);
        curr_command->output_file = calloc(strlen(token) + 1, sizeof(char));
        strcpy(curr_command->output_file, token);
        printf("Output file: %s\n", curr_command->output_file); // for debug
        token = strtok_r(NULL, " ", &saveptr);
    }

    //Get if background or foreground process
    if (!strncmp("&", token, 1))
        printf("Background process: %s", token);
    else
        printf("Foreground process: %s", token);

    return curr_command;
}

