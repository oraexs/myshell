#include "env.h"
#include "history.h"
#include "rawmode.h"
#include <limits.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64

/*
 * Philosophy:
 * A shell is not a one-shot program.
 * It lives in a loop: prompt → read → interpret → execute → repeat.
 */
void shell_loop();

/*
 * Philosophy of read_input():
 * --------------------------
 * A shell must feel "alive" and responsive.
 * This function takes full control of the terminal:
 *  - Reads input character-by-character
 *  - Handles backspace manually
 *  - Handles arrow keys (history navigation)
 *  - Prints characters immediately
 *
 * This mimics how real shells like bash/zsh behave internally.
 */
void read_input(char* buffer) {
    int len = 0;
    buffer[0] = '\0';

    /* Reset history navigation state before reading new input */
    reset_history_index();

    /* Switch terminal to raw mode:
     * - Disable line buffering
     * - Disable echo by kernel
     * - Allow us to read every keystroke
     */
    enable_raw_mode();

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {

        /* ENTER key: command is complete */
        if (c == '\n') {
            buffer[len] = '\0';
            printf("\n");
            break;
        }

        /* BACKSPACE handling */
        else if (c == 127 || c == '\b') {
            if (len > 0) {
                len--;
                buffer[len] = '\0';

                /* Erase character visually */
                printf("\b \b");
                fflush(stdout);
            }
        }

        /* ESC sequence → arrow keys */
        else if (c == 27) {
            char seq[2];

            /* Arrow keys send ESC [ A/B/C/D */
            if (read(STDIN_FILENO, &seq[0], 1) == 1 &&
                read(STDIN_FILENO, &seq[1], 1) == 1) {

                if (seq[0] == '[') {

                    /* UP arrow → previous command */
                    if (seq[1] == 'A') {
                        const char* prevCmd = get_history_up();
                        if (prevCmd) {

                            /* Clear current line visually */
                            for (int i = 0; i < len; i++)
                                printf("\b \b");

                            /* Copy history command safely */
                            len = snprintf(buffer, MAX_INPUT, "%s", prevCmd);

                            /* Reprint command */
                            printf("%s", buffer);
                            fflush(stdout);
                        }
                    }

                    /* DOWN arrow → next command */
                    else if (seq[1] == 'B') {
                        const char* nextCmd = get_history_down();

                        for (int i = 0; i < len; i++)
                            printf("\b \b");

                        if (nextCmd) {
                            len = snprintf(buffer, MAX_INPUT, "%s", nextCmd);
                            printf("%s", buffer);
                        } else {
                            /* No command → empty line */
                            len = 0;
                            buffer[0] = '\0';
                        }
                        fflush(stdout);
                    }
                }
            }
        }

        /* Normal printable characters */
        else {
            if (len < MAX_INPUT - 1) {
                buffer[len++] = c;

                /* Manually echo character */
                write(STDOUT_FILENO, &c, 1);
            }
        }
    }

    /* Restore terminal to normal (canonical) mode */
    disable_raw_mode();
}

int main() {
    /*
     * Philosophy:
     * main() should stay boring.
     * All shell intelligence lives elsewhere.
     */
    shell_loop();
    return 0;
}

/*
 * Philosophy of shell_loop():
 * ---------------------------
 * This is the heart of the shell.
 * Responsibilities:
 *  - Display prompt
 *  - Read command
 *  - Handle built-ins internally
 *  - Fork for external commands
 *  - Measure execution time
 */
void shell_loop () {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];

    /* Initialize command history storage */
    shell_inithistory();

    while (1) {
        /* Display prompt */
        printf("myshell> ");
        fflush(stdout);

        /* Read user input (raw mode handling) */
        read_input(input);

        /* Ignore empty commands */
        if (strlen(input) == 0)
            continue;

        /* Save command to history */
        add_to_history(input);

        /* Built-in: exit */
        if (strcmp(input, "exit") == 0) {
            break;
        }

        /* Built-in: print environment variables */
        if (strcmp(input, "printenv") == 0) {
            print_shell_var();
            continue;
        }

        /* Built-in: variable assignment */
        if (is_valid_assignment(input)) {
            handle_valid_assignment(input);
            continue;
        }

        /*
         * Philosophy:
         * Variable expansion must happen BEFORE tokenization.
         * Example: echo $HOME
         */
        char *expanded_input = expand_variables(input);

        if (!expanded_input) {
            perror("Memory error during expansion\n");
            continue;
        }

        /* Tokenize input into argv[] */
        char *token = strtok(expanded_input, " ");
        int i = 0;
        while (token != NULL && i < MAX_ARGS - 1) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        /* Built-in: cd */
        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                perror("cd failed");
            } else if (chdir(args[1]) != 0) {
                perror("cd failed");
            }
            continue;
        }

        /* Built-in: pwd */
        if (strcmp(args[0], "pwd") == 0) {
            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            } else {
                perror("pwd failed");
            }
            continue;
        }

        /* Built-in: command history */
        if (strcmp(args[0], "print_history") == 0) {
            print_history();
            continue;
        }

        /*
         * External command execution
         * ---------------------------
         * fork() → child runs execvp()
         * parent waits and measures execution time
         */
        struct timeval start, end;
        gettimeofday(&start, NULL);

        pid_t pid = fork();

        if (pid == 0) {
            /* Child process: replace image */
            execvp(args[0], args);

            /* execvp returns only on failure */
            perror("execvp failed");
            exit(EXIT_FAILURE);
        }
        else if (pid > 0) {
            /* Parent process */
            int status;
            waitpid(pid, &status, 0);

            gettimeofday(&end, NULL);

            /* Execution time calculation */
            long seconds = end.tv_sec - start.tv_sec;
            long usec = end.tv_usec - start.tv_usec;
            double elapsed = seconds * 1000.0 + usec / 1000.0;

            printf("Exit status: %d\n", WEXITSTATUS(status));
            printf("Time taken: %.3f ms\n", elapsed);
        }
        else {
            perror("fork failed");
        }
    }
}
