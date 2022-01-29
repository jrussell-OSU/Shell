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

#define NUMBER_OF_STRINGS 600  //# of strings in 2d array for command input
#define STRING_SIZE 100  
#define ARG_SIZE 10
#define MAX_ARGS_SIZE 2049  //max length of all arguments
#define MAX_COMMAND_SIZE 2049  //max length of command
#define PATH_MAX 4096  //file_path size max

struct CommandLine {
    char * cmd;    
    char * arguments;
    char * input_file;
    char * output_file;
    int background_flag;
};

char * get_command(char command_line[]);
char ** tokenize_command(char command_line[]);
struct CommandLine *parse_command(char ** commands);
void print_command(struct CommandLine *curr_command);
void built_in_cmds(char * command, char * arguments);
char * expand$$(char * command);
void exit_cmd(char * command);
void cd_cmd(char * command, char * arguments);
void status_cmd(char * command);
void system_cmds(char * command, char * arguments);
//void child_sys_cmd(char * command, char * arguments);


int main()
{
    char command_line[MAX_COMMAND_SIZE];

    while(1){

        get_command(command_line); //Get command line input from user        

        char** commands = tokenize_command(command_line); //Divide up command input into tokens without whitespace

        if (commands[0][0] == '\n'){
            //printf("Blank line ignored.\n"); for DEBUGGING
            continue;
        }
        
        struct CommandLine *curr_command = parse_command(commands); //Parse command input, returns structure with parsed commands

        if (!curr_command->cmd)  //if command is NULL, get new one (fail-safe catch)
            continue; 

        //Run built in or sys commands based on command input
        if (!strncmp("cd", curr_command->cmd, 2) || !strncmp("exit", curr_command->cmd, 4) || !strncmp("status", curr_command->cmd, 5))
            built_in_cmds(curr_command->cmd, curr_command->arguments);  //check and execute any built-in commands (exit, cd, and status)
        else
            system_cmds(curr_command->cmd, curr_command->arguments);  //check and execute system call

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

//Go through user input command, tokenize to remove whitespace and save to array for later parsing
char ** tokenize_command(char command_line[])
{
    int i = 0;
    char *token; //holds delim'd tokens from strtok_r
    char *saveptr; //holds place for strtok_r
    
    //Allocate space for array of strings (array of pointers to strings) 
    char **token_arr = calloc(NUMBER_OF_STRINGS, sizeof(*token_arr));
    for (i = 0; i < NUMBER_OF_STRINGS; ++i)
        token_arr[i] = malloc(STRING_SIZE);

    //Break up input command line into tokens, then store in token_arr (array of strings)
    token = strtok_r(command_line, " ", &saveptr); //holds current delimn'd token from the command_line user input
    
    //Check for a blank input
    if (!(strcmp(token, "\n"))){
        //printf("newline first character\n");
        token_arr[0][0] = '\n';
        return token_arr;
    }

    i = 0;
    while (token && strcmp(token, "\n")){
        strcpy(token_arr[i++], strtok(token, "\n"));
        token = strtok_r(NULL, " ", &saveptr);
    }
    token_arr[i][0] = EOF;  //Puts stopper at end of array of strings

    //For debugging
    //for (i=0; token_arr[i][0] != EOF; ++i)
        //printf("'%s'\n", token_arr[i]);

    return token_arr;  
}

//Go through command line array, parse commands, and put in CommandLine struct members
struct CommandLine *parse_command(char ** commands)
{
    struct CommandLine *curr_command = malloc(sizeof(struct CommandLine)); //create (allocate memory for) CommandLine structure
    int i = 0;

    //Set all struct members to NULL so we can know what commands were received later
    curr_command->cmd = NULL;
    curr_command->arguments = NULL;
    curr_command->input_file = NULL;
    curr_command->output_file = NULL;
    curr_command->background_flag = 0;  //1 == background process, 0 == foreground

    //Check for # comments and blank lines
    if (!(strncmp("#", commands[0], 1))){
        //printf("Comment ignored.\n"); //DEBUGGING
        return curr_command;  //truncate function
    }
    
    //Allocate space for cmd member and copy user command into it
    curr_command->cmd = calloc(strlen(commands[i]) + 1, sizeof(char));
    strcpy(curr_command->cmd, commands[0]);
    //printf("Cmd: '%s'\n", curr_command->cmd);  //for DEBUGGING

    
    //Check for arguments, input and output, and background/foreground flag
    for (i = 1; commands[i] && commands[i][0] != EOF; ++i){
        //printf("Checking: '%s'\n", commands[i]);  //for DEBUGGING
        switch(commands[i][0]){
            case '-' :  //if it's an argument
                if (curr_command->arguments == NULL)
                    //curr_command->arguments = malloc(sizeof(curr_command->arguments))
                    curr_command->arguments = calloc(MAX_ARGS_SIZE, sizeof(char)); //need to use constant since we are looping to get args and cant keep reallocating
                strcat(curr_command->arguments, commands[i]);
                if (commands[i + 1][0] != EOF)  //if we aren't at the last line
                    strcat(curr_command->arguments, " "); //add " " between args or they won't process correctly
                //printf("Arg: '%s'\n", curr_command->arguments);  //for DEBUGGING
                break;
            case '>':  //if it's an output file
                curr_command->output_file = calloc(strlen(commands[++i]) + 1, sizeof(char));
                strcat(curr_command->output_file, commands[i]);
                //printf("Output file: '%s'\n", curr_command->output_file);  //for DEBUGGING        
                break;
            case '<': //if it's an input file
                curr_command->input_file = calloc(strlen(commands[++i]) + 1, sizeof(char));
                strcat(curr_command->input_file, commands[i]);
                //printf("Input file: '%s'\n", curr_command->input_file);  //for DEBUGGING 
                break;
            case '&': //if it's a background flag
                if (commands[i + 1][0] == EOF){  //if the '&' is the last command in the line, set flag. otherwise ignore
                    curr_command->background_flag = 1;
                    //printf("Background flag: %d\n", curr_command->background_flag);
                }
                break;
            default :
                printf("Unrecognized input.\n");
                break;
        }      
    }
    print_command(curr_command);  //for DEBUGGING
    return curr_command;
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
        exit_cmd(command); //checks for exit command, exits function if found
    }

    if (!strncmp("cd", command, 2)){
        cd_cmd(command, arguments); //checks for cd command, if found changes directory to home w/no arguments, or to a specified directory argument
    }

    if (!strncmp("status", command, 5)){
        status_cmd(command); //checks for exit command, exits function if found
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
    //int i, count;
    //i = count = 0;
    char cwd[PATH_MAX];
    //char *cwd = calloc(strlen(command) + strlen(arguments) + 1, sizeof(char));

    if (arguments){
        printf("File path specified, changing directory...\n");
        char *filepath = arguments + 1;  //remove '-' prefix
        chdir(filepath);  //cwd to new filepath
        printf("File path given: %s\n", filepath);  //for DEBUGGING
        getcwd(cwd, sizeof(cwd));
        printf("Current filepath: %s\n", cwd);  //for DEBUGGING
    } else {
        printf("No file path specified, changing directory to HOME.\n");
        chdir(getenv("HOME"));  //get file path of $HOME directory from env
        getcwd(cwd, sizeof(cwd));
        printf("Current filepath: %s\n", cwd);  //for DEBUGGING
    }
}

void status_cmd(char * command)
{
    if (strlen(command) >= 5 && !strncmp("status", command, 5)){
        printf("exit status %s\n", getenv("STATUS"));
    }
}

void system_cmds(char * command, char * arguments)
{
    //printf("args: '%s'\n", arguments);  //DEBUG
    int i = 0;
    char *token;
    char *saveptr;

    //Create the system call command exec() first argument
    char *sys_call = strtok(command, "\n");

    //Allocate space for an args array of strings
    char **args_array = calloc(NUMBER_OF_STRINGS, sizeof(*args_array));
    for (i = 0; i < NUMBER_OF_STRINGS; ++i)
        args_array[i] = malloc(ARG_SIZE);
    args_array[0] = sys_call;   //first must be file path for execv()
    i = 1;
    if (arguments){  //make sure there actually are arguments before trying to add to array
        token = strtok_r(arguments, " ", &saveptr);
        while (token && strcmp(token, "\n")){
            //printf("curr token: '%s'\n", token);
            strcpy(args_array[i++], strtok(token, "\n"));
            token = strtok_r(NULL, " ", &saveptr);
        }
    }
    args_array[i] = NULL;  //execvp requires array of strings to be null terminated
    //printf("array[0]: '%s' array[1] '%s'\n", args_array[0], args_array[1]);

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
            //execv(args_array[0], args_array);
            execvp(args_array[0], args_array);
            perror("execv");
            exit(EXIT_FAILURE);
            //SIGTERM;
            break;
        default:  //parent process
            wait(&wait_id);
            char status[20];
            sprintf(status, "%d", wait_id);
            setenv("STATUS", status, 1);
            break;
    }
}













