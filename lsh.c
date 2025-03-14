/**
 * @author: Maksym Turkot
 * @version: 3/15/24
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

char error_message[30] = "An error has occurred\n"; // Error message.
char **path; // Path variable.

/**
 * Main function running the shell.
 *
 * @return status.
 */
int main(int, char**);

/**
 * Interactive mode where file was not passed.
 */
_Noreturn void interactive();

/**
 * Run the shell from the batch file.
 */
void batch(char*);

/**
 * Parse the command if parallel operator was detected.
 */
void parseParallel(char*);

/**
 * Parse the command if redirection operator was detected.
 */
void parseRedirect(char*);

/**
 * Parse the command and arguments.
 */
void parseCommand(char*, char*);

/**
 * Selects appropriate behavior: built-in command or program.
 */
void selectCommand(int, char*[]);

/**
 * Runs the process. Uses fork(), wait(), and execv() system calls.
 */
void runProgram(char*[]);

/**
 * Exits the shell. Built-in command.
 */
void builtInExit(int);

/**
 * Changes the directory to a specified destination.
 */
void builtInCd(int, char*[]);

/**
 * Replaces current path with specified list of destinations.
 */
void builtInPath(int, char*[]);

/**
 * Main function running the shell.
 *
 * @param argc argument count
 * @param argv argument values
 * @return status.
 */
int main(int argc, char *argv[]) {
    // Reallocate space and handle a potential leak.
    if ((path = (char **)realloc(path, (1) * sizeof(char *))) == NULL) { // NOLINT
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

    path[0] = "/bin"; // Set up path.

    // Check if filename passed and pick mode.
    if (argc == 1) {
        interactive();
    } else if (argc == 2) { // One file passed.
        batch(argv[1]);
    } else { // More than one file passed.
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
    return 0;
}

/**
 * Interactive mode where file was not passed.
 * Runs the shell prompt loop and reads the line from stdin.
 */
_Noreturn void interactive() {
    size_t len = 0;

    // The lsh loop
    while (1) {
        char *line = NULL;
        printf("lsh> ");

        getline(&line, &len, stdin);
        line[strcspn(line, "\n")] = 0;

        // Check if parallel or redirection is used.
        if (strstr(line, "&") != NULL) {
            parseParallel(line);
        } else if (strstr(line, ">") != NULL) {
            parseRedirect(line);
        } else {
            parseCommand(line, NULL);
        }
        free(line);
    }
}

/**
 * Run the shell from the batch file.
 * Read file line by line.
 *
 * @param filename of the batch file.
 */
void batch(char* filename) {
    FILE *file = fopen(filename, "r");
    char *line = NULL;
    size_t len = 0;

    // If lsh could not open file.
    if (file == NULL) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

    // Read each line.
    while (getline(&line, &len, file) != -1) {
        line[strcspn(line, "\n")] = 0; // Remove trailing \n.

        // Check if parallel or redirection is used.
        if (strstr(line, "&") != NULL) {
            parseParallel(line);
        } else if (strstr(line, ">") != NULL) {
            parseRedirect(line);
        } else {
            parseCommand(line, NULL);
        }
    }
    free(line);
    fclose(file);
    builtInExit(1);
}

/**
 * Parse the command if parallel operator was detected.
 * Tokenize string based on "&" and run each command in parallel.
 *
 * @param line the command line passed.
 */
void parseParallel(char *line) {
    char *token, *remainder;
    remainder = strdup(line);

    // Split string on "&" sign.
    while ((token = strsep(&remainder, "&")) != NULL) {

        // Skip empty tokens.
        if (*token == '\0') {
            continue;
        }

        int rc = fork(); // Start new process.

        // Check if in child/parent process
        if (rc < 0) {
            write(STDERR_FILENO, error_message, strlen(error_message));
        } else if (rc == 0) { // In child process.
            // Check if redirection is used.
            if (strstr(line, ">") != NULL) {
                parseRedirect(token);
            } else {
                parseCommand(token, NULL);
            }
            exit(EXIT_SUCCESS); // Exit the child process when done.
        } else {
            continue; // Move on to the next process.
        }
    }
    while (wait(NULL) > 0) {} // Wait for all child processes to finish.
}

/**
 * Parse the command if redirection operator was detected.
 * Tokenize string based on ">" and check for valid syntax.
 * Store the redirection filename and execute the command.
 *
 * @param line that was passed.
 */
void parseRedirect(char *line) {
    char *token, *redRem, *nameRem;
    char **redTokens = malloc(0);
    char **nameTokens = malloc(0);
    int redTokensSize = 0;
    int nameTokensSize = 0;
    redRem = strdup(line);

    // Split string on ">" sign.
    while ((token = strsep(&redRem, ">")) != NULL) {

        // Empty tokens indicate no command or no filename.
        if (*token == '\0') {
            write(STDERR_FILENO, error_message, strlen(error_message));
            return;
        }

        // Reallocate space and handle a potential leak.
        if ((redTokens = (char **)realloc(redTokens, (++redTokensSize) * sizeof(char *))) == NULL) { // NOLINT
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
        redTokens[redTokensSize - 1] = strdup(token);
    }

    // Check if there were 2 redirects, no command, or no filename.
    if (redTokensSize != 2) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return;
    }

    nameRem = strdup(redTokens[1]); // Half of line after the ">".

    // Split filename half of redirect, if possible.
    while ((token = strsep(&nameRem, " \t")) != NULL) {

        // Skip empty tokens.
        if (*token == '\0') {
            continue;
        }

        // Reallocate space and handle a potential leak.
        if ((nameTokens = (char **)realloc(nameTokens, (++nameTokensSize) * sizeof(char *))) == NULL) { // NOLINT
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }

        nameTokens[nameTokensSize - 1] = strdup(token);
    }

    // Check if there was no filename, or more than one filename.
    if (nameTokensSize != 1) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return;
    }

    parseCommand(redTokens[0], nameTokens[0]); // Pass the command and filename.

    free(redTokens);
}

/**
 * Parse the command and arguments.
 * Redirect output to a file if necessary.
 *
 * @param line of the command.
 * @param filename to redirect to, NULL otherwise.
 */
void parseCommand(char *line, char *filename) {
    int stdout_fd;
    char *token, *remainder;
    char **tokens = malloc(0);
    int size = 0;
    FILE *file;

    remainder = strdup(line);

    // Count the number of tokens & check for redirect
    while ((token = strsep(&remainder, " \t")) != NULL) {

        // Skip empty tokens.
        if (*token == '\0') {
            continue;
        }

        // Reallocate space and handle a potential leak.
        if ((tokens = (char **)realloc(tokens, (++size) * sizeof(char *))) == NULL) { // NOLINT
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }

        tokens[size - 1] = strdup(token);
    }

    // Check if line was empty
    if (*tokens == NULL) {
        return;
    }

    // If redirection detected, redirect output to a file.
    if (filename != NULL) {
        stdout_fd = dup(STDOUT_FILENO);
        file = freopen(filename, "w", stdout);
    }

    selectCommand(size, tokens);

    // If redirection detected, close file and redirect back to stdout.
    if (filename != NULL) {
        fclose(file);
        dup2(stdout_fd, STDOUT_FILENO);
        close(stdout_fd);
    }
}

/**
 * Selects appropriate behavior: built-in command or program.
 *
 * @param cnt number of tokens.
 * @param tokens array of tokens.
 */
void selectCommand(int size, char *tokens[]) {
    // Check for built-in commands
    if (strcmp(tokens[0], "exit") == 0) {
        builtInExit(size);
    } else if (strcmp(tokens[0], "cd") == 0) {
        builtInCd(size, tokens);
    } else if (strcmp(tokens[0], "path") == 0) {
        builtInPath(size, tokens);
    } else { // run program
        // Reallocate space and handle a potential leak.
        if ((tokens = (char **)realloc(tokens, (++size) * sizeof(char *))) == NULL) { // NOLINT
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
        tokens[size - 1] = NULL; // Add a trailing NULL for execv.
        runProgram(tokens);
    }
}

/**
 * Runs the process. Uses fork(), wait(), and execv() system calls.
 *
 * @param args program name and its arguments with NULL, passed to execv().
 */
void runProgram(char *args[]) {
    char *command = args[0];

    // Check every entry in path.
    for (int i = 0; path[i] != NULL;  i++) {

        char *executable = strdup(path[i]);
        strcat(executable, "/");
        strcat(executable, command); // build a program path.

        // Check if program path is executable.
        if (access(executable, X_OK) == 0) {
            int rc = fork();

            // Check if in child/parent process.
            if (rc < 0) {
                write(STDERR_FILENO, error_message, strlen(error_message));
            } else if (rc == 0) { // In child.
                execv(executable, args);
            } else {
                wait(NULL);
            }
            free(executable);
            return;
        }
    }
    write(STDERR_FILENO, error_message, strlen(error_message)); // no access path
}

/**
 * Exits the shell. Built-in command.
 */
void builtInExit(int size) {
    // Check if other arguments were passed to exit.
    if (size == 1) {
        exit(0);
    } else {
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
}

/**
 * Changes the directory to a specified destination.
 * Exactly one destination must be provided. Built-in command.
 *
 * @param cnt number of elements in the tokens array.
 * @param tokens array of command tokens.
 */
void builtInCd(int cnt, char *tokens[]) {
    // Check if there is one command and one argument
    if (cnt == 2) {
        // Check if chdir failed.
        if (chdir(tokens[1]) == -1) {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
    } else { // Wrong argument count
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
}

/**
 * Replaces current path with specified list of destinations.
 * Can take 0 ore more arguments.
 *
 * @param size number of command tokens.
 * @param tokens array of command tokens.
 */
void builtInPath(int size, char *tokens[]) {
    // Reallocate space and handle a potential leak.
    if ((path = (char **)realloc(path, (size) * sizeof(char *))) == NULL) { // NOLINT
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

    // Save new values to path (including NULL at the end).
    for (int i = 0; i < size; i++) {
        path[i] = tokens[i + 1];
    }
}
