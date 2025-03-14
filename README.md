# A Simple Unix Command Line Interpreter (Shell)
A simple custom Unix command line interpreter (CLI) supporting file system navigation, redirection, and parallel commands. The shell is at the heart of the command-line interface and is central to the Unix/C programming environment. 

## Running the program environment
A Makefile is provided that will compile the lsh.c file using gcc.

## Basic Shell
Two modes are available: interactive and batch. Interactive prints the prompt and executes each command the user types. Batch mode accepts a file, opens it, and reads commands from it line by line. A command can have any number of arguments. A new process is created for each new program that is being run, using fork(), execv(), and wait().

## Path
There is a global path array. Initially, it has only “/bin”. Using the built-in command, the user can rewrite the path.

## Build-in commands
**exit** lets the user exit the shell.
**cd** changes the directory, accepts only one arguments.
**path** rewrites the path array. The entire program uses dynamic memory allocation to
accommodate variable sizes of arrays.

## Redirection
The shell first checks if the “>” operator is present in the individual command portion of the line to see if redirection is used. It then calls parseRedirection() to parse the string. It is split at the “>” sign. If more than two tokens appear, that’s an error since we need only one sign. Empty tokens are also an error since we need both a command and a filename. It then splits the filename half of the command to see if there is more than one, which would be an error. The filename is stored and passed to parseCommand() for redirection.

## Parallel Commands
The shell checks if the “&” operator is present and then splits the string on it. Each string token is now a standalone command with a possible redirection. We then run a loop where we create a new process for each command with a fork(). Each command is then checked for redirection and executed in a normal manner. After that, we run a second loop to wait() for every process to complete. Initially, I did not use an exit() command after each process, and then had 3 processes running in the background, each consuming 17 Gb of my 8 Gb physical memory. Call exit() after each one has solved the issue.

## Program errors
All syntax errors are accounted for. The user may put any number of whitespace between any element of the command, and “>” and “&” operators do not require whitespace.

