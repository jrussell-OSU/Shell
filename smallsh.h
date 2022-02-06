char * get_command(char command_line[]);
char ** tokenize_command(char command_line[]);
struct CommandLine *parse_command(char ** commands);
void print_command(struct CommandLine *curr_command);
void built_in_cmds(struct CommandLine *curr_command);
char * expand_macro(char * command);
void exit_cmd(struct CommandLine *curr_command);
void cd_cmd(struct CommandLine *curr_command);
void status_cmd(struct CommandLine *curr_command);
void system_cmds(struct CommandLine *curr_command);
char ** sys_args(struct CommandLine *curr_command);
void file_redirect(struct CommandLine *curr_command);
void set_arguments(struct CommandLine *curr_command, char ** commands, int i);
void set_cmd2(struct CommandLine *curr_command, char ** commands, int i);
void handle_SIGTSTP(int sig);
void setup_SIGTSTP();
void handle_SIGCHLD(int sig);
void setup_SIGCHLD();
void check_background_processes(int PIDs_array[]);
void handle_SIGINT(int signo);
void setup_SIGINT();
void check_foreground_mode(struct CommandLine *curr_command);
void set_cmd3(struct CommandLine *curr_command, char ** commands, int i);