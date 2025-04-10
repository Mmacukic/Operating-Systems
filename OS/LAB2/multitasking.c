#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 64
#define DELIMITERS " \t\n"

void execute_command(char *command, char *args[]);
void parse_command(char *command, char *args[]);
void sigint_handler(int sig);
void sigterm_handler(int sig);

pid_t shell_pgid; // Store the process group ID of the shell

int main()
{
    char command[MAX_COMMAND_LENGTH];
    char *args[MAX_ARGS];
    char cwd[1024]; // buffer to store current working directory

    struct sigaction sigint_action, sigterm_action;
    sigint_action.sa_handler = sigint_handler;
    sigemptyset(&sigint_action.sa_mask);
    sigint_action.sa_flags = 0;
    sigaction(SIGINT, &sigint_action, NULL);

    sigterm_action.sa_handler = sigterm_handler;
    sigemptyset(&sigterm_action.sa_mask);
    sigterm_action.sa_flags = 0;
    sigaction(SIGTERM, &sigterm_action, NULL);

    shell_pgid = getpgrp(); // Get the process group ID of the shell

    while (1)
    {
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            printf("%s> ", cwd); // Display current working directory
        }
        else
        {
            perror("getcwd");
            break;
        }

        // Read user input
        char *status = fgets(command, sizeof(command), stdin);
        if (status == NULL)
        {
            break; // Exit on EOF
        }

        // Remove trailing newline
        command[strcspn(command, "\n")] = 0;

        // Parse and execute command
        parse_command(command, args);
        if (args[0] != NULL)
        {
            if (strcmp(args[0], "exit") == 0)
            {
                break;
            }
            else if (strcmp(args[0], "ps") == 0)
            {
                char ps_command[MAX_COMMAND_LENGTH];
                sprintf(ps_command, "/bin/ps -eo pid,comm,pgid | grep %d", shell_pgid);
                system(ps_command);
            }
            else if (strcmp(args[0], "cd") == 0)
            {
                if (args[1] == NULL)
                {
                    fprintf(stderr, "cd: missing argument\n");
                }
                else
                {
                    if (chdir(args[1]) != 0)
                    {
                        perror("cd");
                    }
                }
            }
            else if (strcmp(args[0], "kill") == 0)
            {
                if (args[1] == NULL)
                {
                    fprintf(stderr, "kill: missing argument\n");
                }
                else
                {
                    int pid;
                    char *signal = NULL;
                    // Check if SIGINT is explicitly mentioned in the command
                    if (strcmp(args[2], "SIGINT") == 0)
                    {
                        pid = atoi(args[1]);
                        signal = "SIGINT";
                    }
                    else
                    {
                        pid = atoi(args[1]);
                        signal = args[2];
                    }

                    // Get the process group ID of the specified PID
                    pid_t pgid = getpgid(pid);
                    if (pgid == -1)
                    {
                        perror("getpgid");
                        return;
                    }

                    // Send the signal to the process group
                    int signum = signame_to_signum(signal);
                    if (signum == -1)
                    {
                        fprintf(stderr, "kill: unknown signal: %s\n", signal);
                    }
                    else
                    {
                        if (kill(-pgid, signum) != 0)
                        {
                            perror("kill");
                        }
                    }
                }
            }

            else
            {
                // Check if the command should be run in the background
                int run_in_background = 0;
                int i;
                for (i = 0; args[i] != NULL; i++)
                {
                    if (strcmp(args[i], "&") == 0)
                    {
                        run_in_background = 1;
                        args[i] = NULL; // Remove '&' from arguments
                        break;
                    }
                }

                // Fork a child process to execute the command
                pid_t pid = fork();
                if (pid == 0)
                {
                    // Child process
                    if (run_in_background)
                    {
                        // Set the child process group ID to the shell's process group ID
                        setpgid(0, shell_pgid);
                        // Redirect input from /dev/null
                        int fd = open("/dev/null", O_RDONLY);
                        dup2(fd, STDIN_FILENO);
                        close(fd);
                        // Redirect output and error to /dev/null
                        fd = open("/dev/null", O_WRONLY);
                        dup2(fd, STDOUT_FILENO);
                        dup2(fd, STDERR_FILENO);
                        close(fd);
                    }
                    if (execvp(args[0], args) == -1)
                    {
                        perror("execvp");
                        exit(EXIT_FAILURE);
                    }
                }
                else if (pid < 0)
                {
                    // Fork failed
                    perror("fork");
                }
                else
                {
                    // Parent process
                    if (!run_in_background)
                    {
                        // Wait for the child process to finish if it's not running in the background
                        waitpid(pid, NULL, 0);
                    }
                    else
                    {
                        // Print the background process ID
                        printf("Background process started with PID %d\n", pid);
                    }
                }
            }
        }
    }

    return 0;
}

void parse_command(char *command, char *args[])
{
    int i = 0;
    args[i] = strtok(command, DELIMITERS);
    while (args[i] != NULL && i < MAX_ARGS - 1)
    {
        i++;
        args[i] = strtok(NULL, DELIMITERS);
    }
    args[i] = NULL; // Null-terminate the argument list
}

void sigint_handler(int sig)
{
    printf("Received SIGINT.\n");
}

void sigterm_handler(int sig)
{
    printf("Received SIGTERM.\n");
}
// Function to convert signal name to signal number
int signame_to_signum(const char *signame)
{
    if (strcmp(signame, "SIGINT") == 0)
    {
        return SIGINT; // Manual mapping for SIGINT
    }

    int signum = -1;
    if (signame != NULL)
    {
        for (int i = 1; i < NSIG; ++i)
        {
            if (strcmp(signame, strsignal(i)) == 0)
            {
                signum = i;
                break;
            }
        }
    }
    return signum;
}
