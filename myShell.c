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

    while(1)
    { 
        printf("shell> ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            if (feof(stdin)){
                //ctrl + d pressed
                printf("\n");
                break;
            }else if (ferror(stdin)){
                perror("Error reading input");
                break;
            }
        }
    
        input[strcspn(input, "\n")] = 0; 
        token = strtok(input, " \t\n|><&;");

        if(token == NULL){ //
            continue;
        }

        if(strcmp(token, "exit") == 0){ 
            printf("\n");
            break;
        }


        while (token != NULL) {
            printf("%s\n", token);
            token = strtok(NULL, " \t\n|><&;");
         }
    }

    return 0;
}