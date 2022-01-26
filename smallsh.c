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
void built_in_cmds(char * command, char * arguments);
char * expand$$(char * command);
void exit_cmd(char * command);
void cd_cmd(char * command, char * arguments);
void system_cmds(char * command);

extern int built_in_cmd_flag = 0;  //if a built_in command was executed
extern int system_cmd_flag = 0;  //if a system call was executed

int main()
{
    char command_line[3000];
    int built_in_cmd_flag;
    int system_cmd_flag;

    while(1){
        built_in_cmd_flag = 0;
        system_cmd_flag = 0;

        get_command(command_line); //Get command line input from user        

        struct CommandLine *curr_command = parse_command(command_line); //Parse command input, returns structure with parsed commands

        if (!curr_command->cmd)  //if command is NULL, get new one
            continue; 
        
        built_in_cmds(curr_command->cmd, curr_command->arguments);  //check and execute any built-in commands (exit, cd, and status)

        if (!built_in_cmd_flag)
            system_cmds(curr_command->cmd);


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

    printf(": ");
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
    if (strstr(token, "\n"))
        newline_flag = 1;

    curr_command->cmd = calloc(strlen(token) + 1, sizeof(char));
    //printf("token last character: '%c'\n", token[strlen(token)]);
    strcpy(curr_command->cmd, strtok(token, "\n"));

    //end function since line end found
    if (newline_flag){
        //print_command(curr_command);
        return curr_command;
    }

    //Get "arguments" (if any)
    curr_command->arguments = calloc(1000, sizeof(char));  //allocate mem for arguments
    while (!strncmp("-", (token = strtok_r(NULL, " ", &saveptr)), 1)){
        if (strstr(token, "\n"))
            newline_flag = 1;
        strcat(curr_command->arguments, strtok(token, "\n"));
        if (newline_flag){
            //print_command(curr_command);
            return curr_command;
        }
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
            //printf("Error, too many files.\n");  //DEBUGGING
            //print_command(curr_command);  //DEBUGGING
            return curr_command;  //exit function early
        }

        //If there was a newline found, terminate function early to avoid segfaults, since we are at end of command anyway
        if (newline_flag){
            //print_command(curr_command);  //DEBUGGING
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

void built_in_cmds(char * command, char * arguments)
{    
    expand$$(command);  //expand $$ macro in command to getpid()
    //printf("Expanded command: %s\n", command); //for DEBUGGING

    if (!strncmp("exit", command, 4)){
        built_in_cmd_flag = 1;
        exit_cmd(command); //checks for exit command, exits function if found
    }

    if (!strncmp("cd", command, 2)){
        built_in_cmd_flag = 1;
        cd_cmd(command, arguments); //checks for cd command, if found changes directory to home w/no arguments, or to a specified directory argument
    }
}

char * expand$$(char * command)
{
    int count, i, total$$, itoa_len, rem$;
    char str_itoa[100];
    count = i = total$$ = itoa_len = rem$ = 0;
    //char * temp;

    for (i = 0; i < (strlen(command)); ++i){
        if (command[i] == '$') 
            ++count;
    }

    if (count <= 1) // no $$ macro present, exit function early
        return command;

    strtok(command, "$");  //remove $'s from command
    
    if ((rem$ = count % 2) == 1)
        strcat(command, "$");  //get if any $'s leftover

    total$$ = count / 2;
    for (i=0; i<total$$; ++i){
        itoa_len = sprintf(str_itoa, "%d", getpid());    
        strcat(command, str_itoa);
    }

    return command;
}

void exit_cmd(char * command)
{
    if (strlen(command) >= 4 && !strncmp("exit", command, 4)){
        printf("Exiting shell..."); //for DEBUGGING
        exit(0);
    }
}

void cd_cmd(char * command, char * arguments)
{
    int i, count;
    i = count = 0;
    char cwd[500];

    //Get number of arguments (first make sure arguments !null to prevent segfault)
    for (i=0;arguments && i<strlen(arguments); ++i){
        if (arguments[i] == '-')
            ++count;
    }

    switch(count){
        case 0 :  //no arguments, cwd to HOME directory
            printf("Filepath not specified, changing to HOME directory.\n"); //for debugging
            chdir(getenv("HOME"));  //get file path of $HOME directory from env
            getcwd(cwd, sizeof(cwd));
            printf("Current filepath: %s\n", cwd);  //for DEBUGGING
            break;
        case 1 :  //one argument, cwd to specified file path
            printf("File path specified, changing directory...\n");
            chdir(strtok(arguments, "-"));  //cwd to new filepath
            printf("File path given: %s\n", strtok(arguments, "-"));  //for DEBUGGING
            getcwd(cwd, sizeof(cwd));
            printf("Current filepath: %s\n", cwd);  //for DEBUGGING
            break;
        default : //too many arguments, do nothing
            printf("Too many arguments! Only one filepath allowed for cd command.\n");
            break;
    }
}

void system_cmds(char * command)
{
    

    char *newargv[] = { "/bin/ls", "-al", NULL };

    int wait_id = 0;


    //Execute the system call from a child process
    pid_t spawnpid = -5;  //part of code below taken from class explorations
    spawnpid = fork();
    switch (spawnpid){
        case -1:  //error
            perror("fork() failed!");
            exit(1);
            break;
        case 0:  //child process
            printf("Child process now...\n");  //for DEBUGGING
            if (!strncmp("ls", command, 4))
                execv(newargv[0], newargv);
            SIGTERM;
            break;
        default:  //parent process
            wait(&wait_id);
            printf("Child process is terminated. Wait id: %d\n", wait_id); //for DEBUGGING
            break;
    }


}







