/*
Stage 4:
1- Implement internal cd command
2- Handle cd with no arguments (change to HOME directory)
3- Handle cd with one argument (change to specific path)
4- Use perror for system errors
5- Handle error for too many arguments (more than 1 parameter)
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>    // fork
#include <sys/types.h> // pid_t
#include <sys/wait.h>  // wait
#include <stdlib.h>    // exit

#define MAX_ARGS 50

// -- Stage 2 Implementation

int parse_input(char *line, char *argv[])
{
    char *token;
    int argc = 0; // count for tokens

    if (line == NULL)
        return 0; /*incase line returns NUll,ex user ctrl+d*/

    line[strcspn(line, "\n")] = 0; /*removes the newline that fgets takes by accident*/

    if (strlen(line) == 0)
        return 0; /*incase line is empty*/

    token = strtok(line, " \t\n|><&;");

    while (token != NULL && argc < MAX_ARGS - 1)
    {
        argv[argc] = token;
        argc++;
        token = strtok(NULL, " \t\n|><&;"); /*get next token*/
    }
    /*
        ex:ls -la /home
        token 0 = "ls", store in argv[0].

        token 1 = "-la", store in argv[1].

        token 2 = "/home", store in argv[2].
    */

    argv[argc] = NULL; // NULL-terminate argv
    return (argc);     // return number of tokens
}

void execCommand(char *argv[])
{
    pid_t pid = fork(); // process cloned
    if (pid < 0)
    {
        perror("Fork Failed");
    }
    else if (pid == 0)
    { // -- Child Process --
        // replace new child process with program passed by the user
        execvp(argv[0], argv);
        // perror("execvp");
        fprintf(stderr, "%s command not found\n", argv[0]);
        exit(1); // kill the child
    }
    else
    {
        // parent process : wait for the child to finish
        wait(NULL);
    }
}

void getpath(char **args, int arg_count)
{
    char *path = getenv("PATH");
    if (path == NULL)
    {
        printf("PATH not found.\n");
    }
    else if (arg_count == 1)
    {
        printf("Current PATH:~%s\n", path);
    }
    else
    {
        printf("Error: getpath takes no parameters.\n");
    }
}

void setpath(char **args, int arg_count)
{
    if (arg_count == 1)
    {
        printf("Error: too few arguments to setpath.\n");
    }
    else if (arg_count == 2)
    {
        if (setenv("PATH", args[1], 1) == -1)
        {
            perror("setenv");
        }
    }
    else
    {
        printf("Error: too many arguments passed.\n");
    }
}

void cleanup(char *originalPath)
{
    if (originalPath != NULL)
    {
        setenv("PATH", originalPath, 1);
        printf("Restored Path: %s\n", getenv("PATH"));
        free(originalPath);
    }
}

// void printDir(){
//     char cwd[1024];
//     getcwd(cwd, sizeof(cwd));
//     printf("Dir: %s\n", cwd);
// }

int main(void)
{
    char input[512];
    char *argv[MAX_ARGS];
    int argc;
    char *home = getenv("HOME");
    char *originalPath = strdup(getenv("PATH"));

    // change at the start of your shell its current direcotry to the HOME directory
    if (home != NULL)
    {
        chdir(home);
    }

    //printDir();
    // char s[100];
    // printf("%s\n", getcwd(s,100));
    while (1)
    {
        printf("shell> ");

        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            cleanup(originalPath);
            break;
        }

        argc = parse_input(input, argv);

        if (argc == 0)
        {
            continue;
        }
        if (argc > 0)
        {
            if (strcmp(argv[0], "exit") == 0)
            {
                cleanup(originalPath);
                break;
            } // Handle getpath and setpath
            else if (strcmp(argv[0], "getpath") == 0)
            {
                getpath(argv, argc);
                continue;
            } else if (strcmp(argv[0], "cd")==0){
                chdir(argv[1]);
                if (argc>=2){
                    printf("Error: too many arguments to change directory.\n");
                    continue;
                }
            }
            else if (strcmp(argv[0], "setpath") == 0)
            {
                setpath(argv, argc);
                continue;
            } 
            
            execCommand(argv);
        }

        // for (int i = 0; i < argc; i++)
        // {
        //     printf("%s\n", argv[i]);
        // }
    }

    return 0;
}
