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
void print_command(struct CommandLine *curr_command);

int main()
{
    char command_line[3000];

    while(1){
        
        get_command(command_line); //Get command line input from user        

        struct CommandLine *curr_command = parse_command(command_line); //Parse command input, returns structure with parsed commands

        
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

//Parse the command line input and put into a structure
struct CommandLine *parse_command(char command_line[])
{
    char *saveptr; //holds place for strtok_r
    char *token; //holds delim'd tokens from strtok_r
    int newline_flag = 0;  //if a token has a \n char
    
    struct CommandLine *curr_command = malloc(sizeof(struct CommandLine)); //create (allocate memory for) CommandLine structure
    
    curr_command->cmd = NULL;
    curr_command->arguments = NULL;
    curr_command->input_file = NULL;
    curr_command->output_file = NULL;
    curr_command->background_flag = 0;
    
    token = strtok_r(command_line, " ", &saveptr); //holds current delimn'd token from the command_line user input

    //Check for # comments and blank lines
    if (!(strcmp("\n", token)) || !(strncmp("#", token, 1))){
        //printf("Comment or blank line ignored.\n"); //DEBUGGING
        return curr_command;  //truncate function
    }

    //Get "command" portion of user command (first delim'd token)
    curr_command->cmd = calloc(strlen(token) + 1, sizeof(char));

    strcpy(curr_command->cmd, token);


    //Get "arguments" (if any)
    curr_command->arguments = calloc(1000, sizeof(char));  //allocate mem for arguments
    while (!strncmp("-", (token = strtok_r(NULL, " ", &saveptr)), 1)){
        strcat(curr_command->arguments, token);
    }

    //Get input and output files
    while (saveptr && (!strcmp("<", token) || !strcmp(">", token))){
        
        //Get "input_file" (if any)
        if (!strcmp("<", token) && curr_command->input_file == NULL){ //if it begins with "<" it's an input file
            token = strtok_r(NULL, " ", &saveptr);
            if (strstr(token, "\n")) //check for newline characters, that means we have reached end of input
                newline_flag = 1;
            curr_command->input_file = calloc(strlen(token) + 1, sizeof(char));
            strcpy(curr_command->input_file, strtok(token, "\n"));
        }

        //Get "output_file" (if any)
        else if (!strcmp(">", token) && curr_command->output_file == NULL){ //if it begins with ">" it's an output file
            token = strtok_r(NULL, " ", &saveptr);
            if (strstr(token, "\n"))
                newline_flag = 1;
            curr_command->output_file = calloc(strlen(token) + 1, sizeof(char));
            strcpy(curr_command->output_file, strtok(token, "\n"));
        } 
        
        //If more than one of the same > or < found, error
        else {
            printf("Error, too many files.\n");  //DEBUGGING
            //print_command(curr_command);  //DEBUGGING
            return curr_command;  //exit function early
        }

        //If there was a newline found, terminate function early to avoid segfaults, since we are at end of command anyway
        if (newline_flag){
            print_command(curr_command);  //DEBUGGING
            return curr_command;
        }
        token = strtok_r(NULL, " ", &saveptr);
    }

    //Get if background or foreground process (defaults to foreground)
    if (!strcmp("&\n", token))    
        curr_command->background_flag = 1;  //background
    
    print_command(curr_command);  //DEBUGGING
    fflush(stdout);
    return curr_command; //return CommandLine structure
}

//Prints command_struct where members are not NULL. Used for DEBUGGING
void print_command(struct CommandLine *curr_command)
{
    if (curr_command->cmd != NULL)
        printf("Command: '%s'\n", curr_command->cmd);
    if (curr_command->arguments != NULL)
        printf("Arguments: '%s'\n", curr_command->arguments);
    if (curr_command->input_file != NULL)
        printf("Input file: '%s'\n", curr_command->input_file);
    if (curr_command->output_file != NULL)
        printf("Output file: '%s'\n", curr_command->output_file);
    if (curr_command->background_flag)
        printf("Background process\n");
    else
        printf("Foreground process\n");    
}