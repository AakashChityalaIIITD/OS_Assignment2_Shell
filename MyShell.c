#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <time.h>

#define MAXPWD 1000
#define MAXHISTORY 1000
int is_bg = 0;


typedef struct {
    pid_t pid;
    time_t start_time;
    char* command;
    double execution_time;
} CommandHistory;

CommandHistory history[MAXHISTORY];  // this array is initialized to store history of all executed commands and stores their relevant data like, pid, start_time, etc.
int history_count = 0;

// prints history of all the entered commands which are stored in history
void builtin_history() {
    printf("%-5s %-10s %-40s %-30s %s\n", "No.", "PID", "Command", "Start Time", "Execution Time");

    for (int i = 0; i < history_count; ++i) {
        char time_buf[26];
        ctime_r(&history[i].start_time, time_buf);
        time_buf[strcspn(time_buf, "\n")] = 0; // Remove newline character

        printf("%-5d %-10d %-40s %-30s %.6f sec\n", 
               i + 1, history[i].pid, history[i].command, time_buf, history[i].execution_time);
    }

}

// handler to catch SIGINT interruption (ctrl + C)
static void my_handler(int signum) {
    static int counter = 0;  
    if (signum == SIGINT) {
        builtin_history();
        char buff1[23] = "\nCaught SIGINT signal";
        write(STDOUT_FILENO, buff1, 23);
        exit(0);
    } 
}


// handler to catch SIGCHLD interruption (child termination either by ctrl + Z or something else)
static void sigchld_handler(int signum) {
    if (signum == SIGCHLD) 
        while (waitpid(-1, NULL, WNOHANG) > 0) ;
}

// prints current directory
void printCurrDir() {
    char pwd[MAXPWD];
    if (getcwd(pwd, sizeof(pwd)) != NULL) {
        char *last_dir = strrchr(pwd, '/'); // finds last occurence of '/'
        if (last_dir != NULL && *(last_dir + 1) != '\0') {
            printf("MySimpleShell %s: ", last_dir + 1);
        } else {
            printf("MySimpleShell %s: ", pwd);
        }
    } else {
        printf("Error occurred in getting the current directory!\n");
    }
}

// implements cd function by changing directory using chdir
void builtin_cd(char** args) {
    if (args[1] == NULL) {
        perror("cd: expected argument not given\n");
    }
    else if (chdir(args[1]) != 0) {
        perror("cd failed\n");
    }
}

// prints out the working directory
void builtin_pwd() {
    char cwd[MAXPWD];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd failed\n");
    }
}

void builtin_clear() {
    printf("\033[H\033[J");  // this is a terminal escape sequence which clears the screen
}


void add_to_history(char* command, double exec_time, pid_t pid, time_t start_time) {
    if (history_count < MAXHISTORY) {
        history[history_count].command = strdup(command);
        history[history_count].execution_time = exec_time;
        history[history_count].pid = pid; 
        history[history_count].start_time = start_time; 
        history_count++;
    } else {
        printf("History is full!\n");
    }
}


// checks to see whether the entered command is a built in function or not. If it is, then it runs using the above created functions, else it runs using the 'execute_single_command' function
int chk_builtin(char** parsed_command, char* input_copy) {
    clock_t clock_start = clock();
    time_t start_time;
    time(&start_time);
    int chk_builtin = 0;

    if (strcmp(parsed_command[0], "cd") == 0) {
        builtin_cd(parsed_command);
        chk_builtin = 1;
    } else if (strcmp(parsed_command[0], "pwd") == 0) {
        builtin_pwd();
        chk_builtin = 1;
    } else if (strcmp(parsed_command[0], "clear") == 0) {
        builtin_clear();
        chk_builtin = 1;
    } else if (strcmp(parsed_command[0], "history") == 0) {
        builtin_history();
        chk_builtin = 1;
    } else if (strcmp(parsed_command[0], "exit") == 0) {
        builtin_history();
        exit(0);
    }

    if (chk_builtin) {
        double exec_time = ((double)(clock() - clock_start)) / CLOCKS_PER_SEC;
        add_to_history(input_copy, exec_time, getpid(), start_time); 
    }

    return chk_builtin;
}


// This is used to parse or tokenize the command
void tokenize(char* input, char **args) {
    int i = 0;
    char *token = strtok(input, " ");
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;
}

// This function is executed when the entered command in not any of the built in commands
// We first use fork() to create a child process and then inside it we utlize the execvp() function override the current program to call that argument which we want to process
void execute_single_command(char** args, char* input_copy) {
    clock_t clock_start = clock();
    time_t start_time;
    time(&start_time);
    pid_t pid = fork();
    if (pid < 0) {
        printf("Error occurred while forking!\n");
    } else if (pid == 0) {
        if (execvp(args[0], args) < 0) {
            printf("Error: command not found!\n");
            exit(0);
        }
    } else {
        if (!is_bg) {
            wait(NULL); // waits for prcess to finish
            double exec_time = ((double)(clock() - clock_start)) / CLOCKS_PER_SEC;
            add_to_history(input_copy, exec_time, pid, start_time);
        } else {
            printf("Process moved to background with PID %d\n", pid);
            add_to_history(input_copy, 0.0, pid, start_time);
        }
    }
}

void take_input(char* input) {
    char *input_line = readline("");  // reads input from user in shell
    if (input_line) {
        strcpy(input, input_line);
        free(input_line);
    }
}

int chk_pipe(char* input) {
    return strchr(input, '|') != NULL;
}

// splits piped commands into individual commands
void split_pipes(char* input, char* commands[], int* num_commands) {
    char* token = strtok(input, "|");
    int i = 0;
    while (token != NULL) {
        commands[i++] = token;
        token = strtok(NULL, "|");
    }
    *num_commands = i;
}


void execute_piped_commands(char* commands[], int num_commands, char* input_copy) {
    clock_t clock_start = clock();
    time_t start_time;
    time(&start_time);
    int fds[2 * (num_commands - 1)];
    pid_t pids[MAXPWD];

    for (int i = 0; i < num_commands - 1; ++i) {
        if (pipe(fds + 2 * i) < 0) {
            perror("pipe failed\n");
            exit(1);
        }
    }

    for (int i = 0; i < num_commands; ++i) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork failed\n");
            exit(1);
        } else if (pids[i] == 0) {
            if (i > 0) {
                dup2(fds[(i - 1) * 2], 0); 
            }
            if (i < num_commands - 1) {
                dup2(fds[i * 2 + 1], 1);
            }

            for (int j = 0; j < 2 * (num_commands - 1); ++j) {
                close(fds[j]);
            }

            char *argv[MAXPWD];
            tokenize(commands[i], argv);
            if (execvp(argv[0], argv) < 0) {
                perror("exec failed\n");
                exit(1);
            }
        }
    }

    for (int i = 0; i < 2 * (num_commands - 1); ++i) {
        close(fds[i]);
    }

    if (!is_bg) {
        // wait for all children to complete
        for (int i = 0; i < num_commands; ++i) {
            waitpid(pids[i], NULL, 0);
        }
    } else {
        for (int i = 0; i < num_commands; ++i) {
            printf("Process moved to background with PID %d\n", pids[i]);
        }
    }

    double exec_time = ((double)(clock() - clock_start)) / CLOCKS_PER_SEC;
    add_to_history(input_copy, exec_time, pids[num_commands - 1], start_time);
}



int main() {
    struct sigaction sig;
    memset(&sig, 0, sizeof(sig));
    sig.sa_handler = my_handler;
    signal(SIGCHLD, sigchld_handler); // Below 2 lines handle the signal interruptions
    sigaction(SIGINT, &sig, NULL);  

    char input[MAXPWD];
    char input_copy[MAXPWD];
    char* args[MAXPWD];
    char* commands[MAXPWD];
    int num_commands;

    while (1) {
        printf("\033[0;32m");
        printCurrDir();
        printf("\033[0m"); 

        take_input(input);
        if (input[0] == '\0') continue;

        if (input[strlen(input)-1] == '&') {
            is_bg = 1;
            input[strlen(input) - 1] = '\0';
        }
        else is_bg = 0;

        strcpy(input_copy, input);

        if (chk_pipe(input)) {
            split_pipes(input, commands, &num_commands);
            execute_piped_commands(commands, num_commands, input_copy);
        } else {
            tokenize(input, args);
            if (!chk_builtin(args, input_copy)) {
                execute_single_command(args, input_copy);
            }
        }
    }
}
