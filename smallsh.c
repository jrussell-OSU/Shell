// Author: Jacob Russell
// Date: 01-24-22
// Class: CS344 Operating Systems
// Description: a small shell

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include "smallsh.h"

#define NUMBER_OF_STRINGS 100  //# of strings in 2d array for command input
#define STRING_SIZE 100  
#define ARG_SIZE 50
#define MAX_ARGS_SIZE 2049  //max length of all arguments
#define MAX_COMMAND_SIZE 2049  //max length of command
#define PATH_MAX 2049  //file_path size max

//Structure holds all elements of a user command
struct CommandLine {
    char * cmd;    
    char * cmd2;
    char * cmd3;
    char * arguments;
    char * input_file;
    char * output_file;
    int expansion_macro; //for $$
    int background_flag;
    int pid;
};

static volatile sig_atomic_t foreground_mode_flag;  //1 if foreground_only mode is on. else 0. 
static int status;  //holds exit status of most recent child

int main()
{
    status = -10;  //sets "status" to a default abritrary negative number
    char command_line[MAX_COMMAND_SIZE];  //will hold the entire user imput line
    foreground_mode_flag = 0;  //to track foreground_mode (ctrl-z)
    
    //set up custom signal handlers 
    signal(SIGINT, SIG_IGN);  
    setup_SIGTSTP();
    signal(SIGTSTP, handle_SIGTSTP);
    
    while(1){

        get_command(command_line); //Get command line input from user        

        char** commands = tokenize_command(command_line); //Divide up command input into tokens without whitespace

        
        if (commands[0][0] == '\n'){
            //printf("Blank line ignored.\n"); for DEBUGGING
            continue;
        }
        
        struct CommandLine *curr_command = parse_command(commands); //Parse command input, returns structure with parsed commands

        free(commands);

        if (!curr_command->cmd)  //if command is NULL, get new one (fail-safe catch)
            continue; 

        check_foreground_mode(curr_command);


        //Run built in or sys commands based on command input
        if (!strncmp("cd", curr_command->cmd, 2) || !strncmp("exit", curr_command->cmd, 4) || !strncmp("status", curr_command->cmd, 5))
            built_in_cmds(curr_command);  //check and execute any built-in commands (exit, cd, and status)
        else
            system_cmds(curr_command);  //check and execute system call
    
        //Free allocated memory
        free(curr_command->cmd);
        free(curr_command->cmd2);
        free(curr_command->cmd3);
        free(curr_command->arguments);
        free(curr_command->input_file);
        free(curr_command->output_file);
        free(curr_command);
        
        fflush(stdout);
    }
    return 0;
}

//Uses getline() to get user command_line input and store as char array
char * get_command(char command_line[])
{
    //Set up getline()
    char *curr_line = NULL;
    size_t buff_size = 0;
    //ssize_t num_char;
    curr_line = malloc(buff_size * sizeof(char));
    printf(": ");
    fflush(stdout);
    getline(&curr_line, &buff_size, stdin);

    strcpy(command_line, curr_line);
    
    free(curr_line);


    return command_line;
}

//Go through command_line[] from get_command(), tokenize to remove whitespace and save to array for later parsing
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

//Go through command line array from tokenize_command(), parse commands, and put in CommandLine struct members
struct CommandLine *parse_command(char ** commands)
{
    struct CommandLine *curr_command = malloc(sizeof(struct CommandLine)); //create (allocate memory for) CommandLine structure
    int i = 0;

    //Set all struct members to NULL so we can know what commands were received later
    curr_command->cmd = NULL;
    curr_command->cmd2 = NULL;
    curr_command->cmd3 = NULL;
    curr_command->arguments = NULL;
    curr_command->input_file = NULL;
    curr_command->output_file = NULL;
    curr_command->expansion_macro = 0;
    curr_command->background_flag = 0;  //1 == background process, 0 == foreground

    //Check for # comments and blank lines
    if (!(strncmp("#", commands[0], 1))){
        //printf("Comment ignored.\n"); //DEBUGGING
        printf("\n");
        fflush(stdout);
        return curr_command;  //truncate function
    }
    
    //Allocate space for cmd member and copy user command into it
    curr_command->cmd = calloc(strlen(commands[i]) + 1, sizeof(char));
    strcpy(curr_command->cmd, expand_macro(commands[i++]));  //expand macro before adding to struct
    
    //if echo command, just save everthing as secondary command and return early
    if (!strncmp(curr_command->cmd, "echo", 4)){
        for (i = 1; commands[i] && commands[i][0] != EOF; ++i){
            set_cmd2(curr_command, commands, i);
        }
        return curr_command;
    }

    //Check for arguments, input and output, and background/foreground flag
    for (i = 1; commands[i] && commands[i][0] != EOF; ++i){
        //printf("Checking: '%s'\n", commands[i]);  //for DEBUGGING
        switch(commands[i][0]){
            case '-' :  //if it's an argument
                set_arguments(curr_command, commands, i);
                break;
            case '>':  //if it's an output file
                curr_command->output_file = calloc(strlen(commands[++i]) + 1, sizeof(char));
                strcat(curr_command->output_file, commands[i]);
                break;
            case '<': //if it's an input file
                curr_command->input_file = calloc(strlen(commands[++i]) + 1, sizeof(char));
                strcat(curr_command->input_file, commands[i]);
                break;
            case '&': //if it's a background flag
                if (commands[i + 1][0] == EOF && !commands[i][1])  //IFF the '&' is the last command in the line
                    curr_command->background_flag = 1;
                else //if not end and, treat is a secondary command (e.g. as part of an echo command)
                    set_cmd3(curr_command, commands, i);
                break;
            case '$': //if possible expansion macro
                if (commands[i][1] == '$'){
                    curr_command->expansion_macro = getpid();
                    //char str_int[20];
                    //int my_pid = getpid();
                    //sprintf(str_int, "%d", my_pid);
                    //commands[i] = str_int;
                }
                break;
            default :
                if (!curr_command->arguments)
                    set_cmd2(curr_command, commands, i);
                else
                    set_cmd3(curr_command, commands, i);
                break;
        }      
    }
    //print_command(curr_command);  //for DEBUGGING
    
    return curr_command;
}

//If foreground only mode active, don't let any commands be background
void check_foreground_mode(struct CommandLine *curr_command)
{
    if (foreground_mode_flag == 1)
        curr_command->background_flag = 0;
}

//Go through command_line arguments struct and arrange information into array of strings for processing multiple arguments later on
void set_arguments(struct CommandLine *curr_command, char ** commands, int i)
{
    if (curr_command->arguments == NULL)
        curr_command->arguments = calloc(MAX_ARGS_SIZE, sizeof(char)); //need to use constant since we are looping to get args and cant keep reallocating
    strcat(curr_command->arguments, commands[i]);
    if (commands[i + 1][0] != EOF)  //if we aren't at the last line
        strcat(curr_command->arguments, " "); //add " " between args or they won't process correctly
}

//Gets any "secondary" commands. e.g. in command "echo hello world" the "hello world" is a secondary command
void set_cmd2(struct CommandLine *curr_command, char ** commands, int i)
{
    if (curr_command->cmd2 == NULL)
        curr_command->cmd2 = calloc(MAX_ARGS_SIZE, sizeof(char));
    if (curr_command->cmd && commands[i]){
    //Check for a directory after the command
        if (i > 1)  //add spaces back in
            strcat(curr_command->cmd2, " ");
        strcat(curr_command->cmd2, expand_macro(commands[i])); //expand $$macros (if any) then add to struct
    }
}

//Gets any tertiary commands that may come after an argument (e.g. in "test -f badfile" the "badfile" is a tertiary command)
void set_cmd3(struct CommandLine *curr_command, char ** commands, int i)
{
    if (curr_command->cmd3 == NULL)
        curr_command->cmd3 = calloc(MAX_ARGS_SIZE, sizeof(char));
    if (curr_command->cmd && commands[i]){
    //Check for a directory after the command
        if (i > 1)  //add spaces back in
            strcat(curr_command->cmd3, " ");
        strcat(curr_command->cmd3, expand_macro(commands[i])); //expand $$macros (if any) then add to struct
    }
}

//Prints command_struct where members are not NULL. Used for DEBUGGING
void print_command(struct CommandLine *curr_command)
{
    if (curr_command->cmd != NULL)
        printf("Command: '%s'\n", curr_command->cmd);
    if (curr_command->cmd2 != NULL)
        printf("Command2: '%s'\n", curr_command->cmd2);
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
    fflush(stdout);    
}

//Checks and executes three possible built in commands (cd, exit, and status)
void built_in_cmds(struct CommandLine *curr_command)
{    

    if (!strncmp("exit", curr_command->cmd, 4)){
        exit_cmd(curr_command); //checks for exit command, exits function if found
    }

    if (!strncmp("cd", curr_command->cmd, 2)){
        cd_cmd(curr_command); //checks for cd command, if found changes directory to home w/no arguments, or to a specified directory argument
    }

    if (!strncmp("status", curr_command->cmd, 5)){
        status_cmd(curr_command); //checks for exit command, exits function if found
    }

    fflush(stdout);

}

//If "$$" found in command, this will expand it into the process PID
char * expand_macro(char * command)
{
    int count, i, total$$;
    char str_itoa[100] = "";
    char full_pid[200] = "";
    char leftover[100] = "$";  //for adding back if odd number of $'s
    count = i = total$$ = 0;

    for (i = 0; i < (strlen(command)); ++i){
        if (command[i] == '$') 
            ++count;
    }

    if (count <= 1) // no $$ macro present, exit function early
        return command;

    strtok(command, "$$");  //remove $$'s from command (don't remove single $'s)

    total$$ = count / 2;
    for (i=0; i<total$$; ++i){
        sprintf(str_itoa, "%d", getpid()); 
        fflush(stdout);
        strcat(full_pid, str_itoa);
    }
    if (count % 2 == 1) {
        strcat(leftover, full_pid);  //if leftover $'s, add back to the line
        strcpy(full_pid, leftover);
    }
    if (strlen(command) == count)  //if the command is only $'s, with nothing else  
        strcpy(command, full_pid);
    else
        strcat(command, full_pid);




    return command;
}

//"exit" command will exit the shell
void exit_cmd(struct CommandLine *curr_command)
{
    if (strlen(curr_command->cmd) >= 4 && !strncmp("exit", curr_command->cmd, 4)){
        printf("Exiting shell...\n"); //for DEBUGGING
        fflush(stdout);
        exit(0);
    }
}

//"cd" command will change directory
void cd_cmd(struct CommandLine *curr_command)
{
    char cwd[PATH_MAX];
    //char *cwd = calloc(strlen(command) + strlen(arguments) + 1, sizeof(char));

    //If filepath specified, cd to that path. Otherwise, cd to HOME.
    if (curr_command->cmd2){
        //printf("File path specified, changing directory...\n");
        char *filepath = curr_command->cmd2;  
        chdir(filepath);  //cwd to new filepath
        //printf("File path given: %s\n", filepath);  //for DEBUGGING
        getcwd(cwd, sizeof(cwd));
        //printf("Current filepath: %s\n", cwd);  //for DEBUGGING
    } else {
        //printf("No file path specified, changing directory to HOME.\n");
        chdir(getenv("HOME"));  //get file path of $HOME directory from env
        getcwd(cwd, sizeof(cwd));
        //printf("Current filepath: %s\n", cwd);  //for DEBUGGING
    }
}

//"status" command prints the exit status of last child process
void status_cmd(struct CommandLine *curr_command)
{
    if (strlen(curr_command->cmd) >= 5 && !strncmp("status", curr_command->cmd, 5)){
        //printf("exit status %s\n", getenv("STATUS"));
        if (status == -10){
            printf("No status set yet.\n");
            fflush(stdout);
        } else {
            printf("exit status %d\n", status);
            fflush(stdout);
            status = -10;
        }
    }
}

//Takes command_line struct and returns an array of strings of arguments for the sys call
char ** sys_args(struct CommandLine *curr_command)
{
    int i = 0;
    char *token;
    char *saveptr;

    //Create the system call command exec() first argument
    char *sys_call = strtok(curr_command->cmd, "\n");

    //Allocate space for an args array of strings
    char **args_array = calloc(NUMBER_OF_STRINGS, sizeof(*args_array));
    for (i = 0; i < NUMBER_OF_STRINGS; ++i)
        args_array[i] = malloc(ARG_SIZE);
    args_array[0] = sys_call;   //first must be file path for execv()
    i = 1;
    
    if (curr_command->cmd2) //check for directory input first
        strcpy(args_array[i++], strtok(curr_command->cmd2, "\n"));
    
    if (curr_command->arguments){  //make sure there actually are arguments before trying to add to array
        token = strtok_r(curr_command->arguments, " ", &saveptr);
        while (token && strcmp(token, "\n")){
            //printf("curr token: '%s'\n", token);
            strcpy(args_array[i++], strtok(token, "\n"));
            token = strtok_r(NULL, " ", &saveptr);
        }
    }

    if (curr_command->cmd3) //check for directory input first
        strcpy(args_array[i++], strtok(curr_command->cmd3, "\n"));

    if (curr_command->expansion_macro){
        char str_int[20];
        int my_pid = getpid();
        sprintf(str_int, "%d", my_pid);
        strcpy(args_array[i++], str_int);
    } 

    //if (curr_command->background_flag)
        //args_array[i++] = "&";
    args_array[i] = NULL;  //execvp requires array of strings to be null terminated
    return args_array;
}

//Handles file redirection based on '<' and '>' (input & output files) given by user
void file_redirect(struct CommandLine *curr_command)
{
    //Much of code below taken from class explorations
    
    //Redirect stdin to input file
    //fflush(stdin);
    if (curr_command->input_file){
        char *fileInPath = curr_command->input_file;
        int inFD = open(fileInPath, O_RDONLY);
        fflush(stdin);
        if (inFD == -1) { 
            //perror("Cannot open %s for input", fileInPath); 
            printf("Cannot open %s for input\n", fileInPath);
            fflush(stdout);
            exit(1);
        }
        fflush(stdout);
        fflush(stdin);

        int result = dup2(inFD, 0);  //redirect stdin to input_file
        fflush(stdout);

        if (result == -1) {
            perror("input dup2()");
            fflush(stdout);
            exit(2);
        }
        fflush(stdout);
        fflush(stdin);
    }
    

    //Redirect stdout to output file
    if (curr_command->output_file){
        char *fileOutPath = curr_command->output_file;
        //printf("Opening file %s.\n", newFilePath);
        int outFD = open(fileOutPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (outFD == -1) { 
            //perror("output open()"); 
            printf("Cannot open %s for output\n", fileOutPath);
            fflush(stdout);
            exit(1);
        }
        //fflush(stdout);

       
        int result = dup2(outFD, 1); //redirect stdout to output_file
        if (result == -1) {
            perror("output dup2()"); 
            fflush(stdout);
            exit(2); 

        fflush(stdout);
        }
    }


}

//Handles response to SIGTSTP signal (ctrl-z command). Enters and exits foreground_mode
void handle_SIGTSTP(int signo)
{
    char* foreground_on = "Entering foreground-only mode (& is now ignored)\n";
    char* foreground_off = "Exiting foreground-only mode\n";

    if (foreground_mode_flag == 0){
        foreground_mode_flag = 1;
        //code partly from class explorations
        // We are using write rather than printf
        write(STDOUT_FILENO, foreground_on, 49);
    } else {
        foreground_mode_flag = 0;
        write(STDOUT_FILENO, foreground_off, 30);
    }  
}

//Setup SIGTSTP to use custom handler (see handle_SIGTSTP())
void setup_SIGTSTP()
{
    //much of code below taken from class explorations

	// Initialize struct to be empty
	struct sigaction SIGTSTP_action = {{0}};

	// Fill out the SIGINT_action struct
	// Register signal handler
	SIGTSTP_action.sa_handler = handle_SIGTSTP;
	// Block all catchable signals while handler is running
	sigfillset(&SIGTSTP_action.sa_mask);
	// No flags set
	SIGTSTP_action.sa_flags = 0;

	// Install our signal handler
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

}

//Checks array of all background process PIDs to see if any have termed
void check_background_processes(int PIDs_array[])
{
    //check array of background processes                 
    for(int j = 0; PIDs_array[j]; j++){
        pid_t childPID = PIDs_array[j];
        //printf("Checking background process: %d\n", childPID);
        fflush(stdout);
        int childExitStatus;
        
        pid_t wait_result = waitpid(childPID, &childExitStatus, WNOHANG);
        if (WIFSIGNALED(childExitStatus)){
                int exit_status = WTERMSIG(childExitStatus);
                printf("background child %d was terminated by signal %d\n", childPID, exit_status);
                fflush(stdout);    
        } else if (wait_result > 0){
            printf("Exit status of background child %d is %d\n", childPID, childExitStatus);
            fflush(stdout);
            }
        }
}

//Executes sys calls made by user in commandline (e.g. ls, kill, ps, sleep)
void system_cmds(struct CommandLine *curr_command)
{
    static int PIDs_array[300];  //stores PIDs of background processes
    static int i = 0;  //stores PIDs_array index

    char **args_array = sys_args(curr_command);  //get array of strings of arguments to use for system calls below

    //Execute the system call from a child process (Reference: some of code below taken from class explorations)
    pid_t childPID = -5;  
    int childExitStatus;
    //pid_t wait_result;

    //set up foreground and background children signals
    if (!curr_command->background_flag){
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_IGN);        
    } else {
        signal(SIGINT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN); 
    }
    childPID = fork();
    switch (childPID){
        case -1:  //error
            perror("child fork() failed!");
            fflush(stdout);
            exit(1);
            break;
        case 0:  //child process
            if (!curr_command->background_flag){

                file_redirect(curr_command); //file redirection (if any)
                execvp(args_array[0], args_array);
                perror(args_array[0]);
                exit(EXIT_FAILURE);
            } else {

                int devNull = open("/dev/null", O_WRONLY);
                dup2(devNull, 1); //don't print output unless file redirection (below) is specified (i.e. don't print background processes to the terminal, only to specified files)
                
                file_redirect(curr_command); //file redirection (if any)
                execvp(args_array[0], args_array);
                perror(args_array[0]);
                exit(EXIT_FAILURE);                
            }            
            break;
        default:  //parent process

            setup_SIGTSTP();
            signal(SIGTSTP, handle_SIGTSTP);
            signal(SIGINT, SIG_IGN);
            if (!curr_command->background_flag){ //if foreground process
                waitpid(childPID, &childExitStatus, 0);
                /*
                if (WIFEXITED(childExitStatus)){
                    int exit_status = WEXITSTATUS(childExitStatus);
                    printf("Child exited with status %d\n", exit_status);
                    exit_status = WIF
                }
                */
                if (WIFSIGNALED(childExitStatus)){
                    int exit_status = WTERMSIG(childExitStatus);
                    printf("terminated by signal %d\n", exit_status);
                    fflush(stdout);

                }
                if (childExitStatus > 0)
                    status = 1;  //setenv("STATUS", "1", 1);
                else
                    status = 0;  //setenv("STATUS", "0", 1);

            } else {  //if background process
                printf("Background process %d started.\n", childPID);
                fflush(stdout);
                waitpid(childPID, &childExitStatus, WNOHANG);
                if (WIFSIGNALED(childExitStatus)){
                        int exit_status = WTERMSIG(childExitStatus);
                        printf("background child %d was terminated by signal %d\n", childPID, exit_status);
                        fflush(stdout);
                } else 
                    PIDs_array[i++] = childPID;
                               
             

                if (childExitStatus > 0)
                    status = 1;  //setenv("STATUS", "1", 1);
                else
                    status = 0;  //setenv("STATUS", "0", 1); 
            }                        
    }
    check_background_processes(PIDs_array); //look for terminated background children
    

    free(args_array);
}





