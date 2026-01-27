/*
Stage 1:
1- Display prompt (completed)
2- Read user input (completed)
3- Parse input line into tokens (completed)
4- Handle exit conditions
5- Continous prompting & processing
*/

#include <stdio.h>
#include <string.h>

int main()
{
    char input[512];
    char *token;

    printf("shell> ");

    if (fgets(input, sizeof(input), stdin) != NULL)
    {
        token = strtok(input, " \t|><&;");

        while (token != NULL)
        {
            printf(" %s \n", token);
            token = strtok(NULL, " \t|><&;");
        }
    }

    else
    {
        printf("Error reading input");
    }

    return 0;
}