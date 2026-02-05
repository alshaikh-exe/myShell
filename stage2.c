#include <stdio.h>
#include <string.h>
#include <unistd.h>    // fork
#include <sys/types.h> // pid_t
#include <sys/wait.h>  // wait

#define max_args 50

int parse_input(char *line, char *argv[])
{
    char *token;
    int argc = 0; // count for tokens

    if (line == NULL)
        return (0); /*incase line returns NUll,ex user ctrl+d*/

    line[strcspn(line, "\n")] = 0; /*removes the newline that fgets takes by accident*/

    if (strlen(line) == 0)
        return (0); /*incase line is empty*/

    token = strtok(line, " \t\n|><&;");

    while (token != NULL && argc < max_args - 1)
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
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("Fork Failed");
    }
    else if (pid == 0)
    {
        if (execvp(argv[0], argv) == -1)
        {
            fprintf(stderr, "shell: command not found %s\n", argv[0]);
            _exit(1);
        }
    }
    else
    {
        wait(NULL);
    }
}

int main()
{
    char input[512];
    char *argv[max_args];
    int argc;

    while (1)
    {
        printf("shell> ");

        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            printf("\n");
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
                printf("\n");
                break;
            }
            execCommand(argv);
        }

        for (int i = 0; i < argc; i++)
        {
            printf("%s\n", argv[i]);
        }
    }

    return 0;
}

