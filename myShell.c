/*
FINAL STAGE!!!
 - Modify Input Handling Loop to Support History Invocation.
 - Implement alias to alias mapping.
 - Integrate History Invocations in Aliases.
 - Add Recursive History Updates .
 - Implement Cycle Detection.
 - Develop Parameter Concatenation Logic.
 FIXES:
 
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>    // fork
#include <sys/types.h> // pid_t
#include <sys/wait.h>  // wait
#include <stdlib.h>    // exit
#include <errno.h>     //strerror and errno
#include <ctype.h>     // isdigit

#define MAX_ARGS 50
#define HIST_SIZE 20
#define MAX_LINE 512
#define ALIAS_SIZE 10
#define MAX_ALIAS_NAME 100
#define MAX_ALIAS_COMMND 512

typedef struct Alias
{
    char name[MAX_ALIAS_NAME];
    char command[MAX_ALIAS_COMMND];
} Alias;

Alias aliases[ALIAS_SIZE];
int aliaseCount = 0;

char history[HIST_SIZE][MAX_LINE];

int hist_count = 0; // number of commands entered
int hist_next = 0;  // circular pointer



void print_history(char **argv, int argc);
void save_history();
void save_aliases();

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

void getpath(char **args, int argc)
{
    char *path = getenv("PATH");
    if (path == NULL)
    {
        printf("PATH not found.\n");
    }
    else if (argc == 1)
    {
        printf("Current PATH:~%s\n", path);
    }
    else
    {
        printf("Error: getpath takes no parameters.\n");
    }
}

void setpath(char **args, int argc)
{
    if (argc == 1)
    {
        printf("Error: too few arguments to setpath.\n");
    }
    else if (argc == 2)
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

void changeDir(char **argv, int argc)
{
    char *home = getenv("HOME");
    if (argc == 1)
    {
        if (home != NULL && chdir(home) != 0)
        {
            perror("cd");
        }
    }
    else if (argc == 2)
    {
        if (chdir(argv[1]) != 0)
        {
            fprintf(stderr, "cd: %s: %s\n", argv[1], strerror(errno));
        }
    }
    else
    {
        printf("ERROR: too many arguments to change dir.\n");
    }
}

// Stage 5
int is_history_command(char *line)
{
    if (line[0] == '!')
        return 1;
    return 0;
}

void clearHistory()
{

    for (int i = 0; i < HIST_SIZE; i++)
    {
        history[i][0] = '\0'; // empty string
    }
    hist_count = 0;
    hist_next = 0;
}

void add_history(char *line)
{
    if (line[0] == '!' || line[0] == '\0')
        return;

    strcpy(history[hist_next], line);

    hist_next = (hist_next + 1) % HIST_SIZE; // count = (count+1) % 20
    hist_count++;
}

void print_history(char **argv, int argc)
{
    if (argc == 1)
    {

        if (hist_count == 0)
        {
            printf("History is empty\n");
            return;
        }

        int start = hist_count > HIST_SIZE ? hist_count - HIST_SIZE : 0;

        for (int i = start; i < hist_count; i++)
        {
            int index = i % HIST_SIZE;
            printf("%d %s \n", i - start + 1, history[index]);
        }
    }
    else
    {
        fprintf(stderr, "myshell: history command takes no extra parameter.\n");
        return;
    }
}

int history_exists(int cmd_no)
{
    if (hist_count <= 0)
        return 0;
    if (cmd_no < 1 || cmd_no > hist_count)
        return 0;

    // if history is full (more than HIST_SIZE commands ever entered),
    if (hist_count > HIST_SIZE)
    {
        int oldest_available = hist_count - HIST_SIZE + 1;
        if (cmd_no < oldest_available)
            return 0;
    }
    return 1;
}

/*
 * copies the command line for history command number cmd_no into out (null-terminated).
 * returns 1 on success 0 on failure.
 */
int get_history_command(int cmd_no, char *out, size_t outsz)
{
    if (!history_exists(cmd_no))
        return 0;

    int index = (cmd_no - 1) % HIST_SIZE;
    strncpy(out, history[index], outsz - 1);
    out[outsz - 1] = '\0';

    out[strcspn(out, "\n")] = '\0';
    return 1;
}

int resolve_history_invocation(const char *line, char *out, size_t outsz)
{
    // skip leading whitespace
    while (*line == ' ' || *line == '\t')
        line++;

    if (*line != '!')
        return 0; // not a history invocation

    if (hist_count == 0)
    {
        fprintf(stderr, "Error: history is empty.\n");
        return 0;
    }

    // extract first token (up to whitespace)
    char token[MAX_LINE];
    size_t i = 0;
    while (line[i] != '\0' && line[i] != ' ' && line[i] != '\t' && line[i] != '\n' && i < sizeof(token) - 1)
    {
        token[i] = line[i];
        i++;
    }
    token[i] = '\0';

    // the "rest" becomes extra parameters
    const char *rest = line + i;
    while (*rest == ' ' || *rest == '\t')
        rest++;

    int target_no = -1;

    // case 1: !!
    if (strcmp(token, "!!") == 0)
    {
        target_no = hist_count; // last command entered into history
    }

    // case 2: !-n
    else if (strncmp(token, "!-", 2) == 0)
    {
        const char *p = token + 2;
        if (*p == '\0')
        {
            fprintf(stderr, "Error: invalid history invocation '%s'. Use !-<number>.\n", token);
            return 0;
        }
        for (const char *q = p; *q; q++)
        {
            if (!isdigit((unsigned char)*q))
            {
                fprintf(stderr, "Error: invalid history invocation '%s'. Use !-<number>.\n", token);
                return 0;
            }
        }

        int n = atoi(p);

        // per test note: !-0 should execute the last command in history
        if (n == 0)
        {
            target_no = hist_count;
        }
        // {
        //     fprintf(stderr, "myshell: Event not found: '%s'. Use !-<number>.\n", token);
        //     return 0;
        // }
        else
        {
            target_no = (hist_count + 1) - n; // "current command number" is hist_count+1
        }
}
    // case 3: !n
    else
    {
        const char *p = token + 1;
        if (*p == '\0')
        {
            fprintf(stderr, "Error: invalid history invocation '!'. Use !!, !<number>, or !-<number>.\n");
            return 0;
        }
        for (const char *q = p; *q; q++)
        {
            if (!isdigit((unsigned char)*q))
            {
                fprintf(stderr, "Error: invalid history invocation '%s'. Use !<number>.\n", token);
                return 0;
            }
        }

        target_no = atoi(p);
        int start = hist_count > HIST_SIZE ? hist_count - HIST_SIZE : 0;
        target_no = target_no + start;
    }

    

    if (!history_exists(target_no))
    {
        // differentiate “out of range” vs “too old / overwritten”
        if (target_no < 1 || target_no > hist_count)
        {
            fprintf(stderr, "Error: command \"%d\" doesn't exist (only %d commands enterd).\n",target_no,hist_count);
        }
        else
        {
            fprintf(stderr, "Error: command is not in the last %d history entries.\n", HIST_SIZE);
        }
        return 0;
    }

    char base[MAX_LINE];
    if (!get_history_command(target_no, base, sizeof(base)))
    {
        fprintf(stderr, "Error: failed to retrieve command %d from history.\n", target_no);
        return 0;
    }
    if (*rest == '\0')
    {
        strncpy(out, base, outsz - 1);
        out[outsz - 1] = '\0';
    }
    else
    {
        // base + space + rest
        snprintf(out, outsz, "%s %s", base, rest);
    }

    // ensure  command is not itself a history invocation
    while (*out == ' ' || *out == '\t')
        out++;
    if (out[0] == '!')
    {
        fprintf(stderr, "Error: history invocation cannot resolve to another history invocation.\n");
        return 0;
    }

    return 1;
}

int findAlias(const char *name)
{
    for (int i = 0; i < aliaseCount; i++)
    {
        if (strcmp(aliases[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

void printAliases()
{
    if (aliaseCount == 0)
    {
        printf("No aliases are set yet!\n");
        return;
    }

    for (int i = 0; i < aliaseCount; i++)
    {
        printf("Alias: \"%s\" => \"%s\"\n", aliases[i].name, aliases[i].command);
    }
}

void addAlias(const char *name, const char *command)
{
    int idx = findAlias(name);
    if (idx != -1)
    {
        char oldCommand[MAX_ALIAS_COMMND];
        strcpy(oldCommand, aliases[idx].command);
        strncpy(aliases[idx].command, command, MAX_ALIAS_COMMND - 1);
        aliases[idx].command[MAX_ALIAS_COMMND - 1] = '\0';
        printf("Alias %s is overriden, cmd : \"%s\" => \"%s\" \n", name, oldCommand, command);
        return;
    }

    if (aliaseCount >= ALIAS_SIZE)
    {
        printf("Cannot add alias: max aliases reached!\n");
        return;
    }

    strncpy(aliases[aliaseCount].name, name, MAX_ALIAS_NAME - 1);
    aliases[aliaseCount].name[MAX_ALIAS_NAME - 1] = '\0';

    strncpy(aliases[aliaseCount].command, command, MAX_ALIAS_COMMND - 1);
    aliases[aliaseCount].command[MAX_ALIAS_COMMND - 1] = '\0';

    aliaseCount++;
    printf("Alias \"%s\" has been successfully added!\n", name);
}

void removeAlias(const char *name)
{
    int idx = findAlias(name);

    if (idx == -1)
    {
        printf("Alias \"%s\" not found!\n", name);
        return;
    }

    for (int i = idx; i < aliaseCount - 1; i++)
    {
        aliases[i] = aliases[i + 1];
    }

    aliaseCount--;
    printf("Alias \"%s\" has been removed\n", name);
}

void combineCommand(char *cmd, char **argv, int argc)
{
    cmd[0] = '\0';
    // skip name alias and name
    for (int i = 2; i < argc; i++)
    {
        strcat(cmd, argv[i]);
        if (i < argc - 1)
        {
            strcat(cmd, " ");
        }
    }
}

void commands(char **argv, int argc, char *originalPath)
{
    if (strcmp(argv[0], "exit") == 0)
    {
        save_history();
        save_aliases();
        cleanup(originalPath);
        exit(0);
    } // Handle getpath and setpath
    else if (strcmp(argv[0], "history") == 0)
    {
        print_history(argv, argc);
    }
    else if (strcmp(argv[0], "getpath") == 0)
    {
        getpath(argv, argc);
    }
    else if (strcmp(argv[0], "setpath") == 0)
    {
        setpath(argv, argc);
    }
    else if (strcmp(argv[0], "cd") == 0)
    {
        changeDir(argv, argc);
    }
    else if (strcmp(argv[0], "clearhistory") == 0)
    {
        clearHistory();
    }
    else if (strcmp(argv[0], "alias") == 0)
    {
        if (argc == 1)
        {
            printAliases();
            return;
        }
        if (argc < 3)
        {
            printf("Error: alias requires a name and a command. Usage: alias <name> <command>\n");
            return;
        }

        char command[MAX_ALIAS_COMMND];
        combineCommand(command, argv, argc);
        addAlias(argv[1], command);
    }
    else if (strcmp(argv[0], "unalias") == 0)
    {
        if (argc != 2)
        {
            printf("Error: unalias requires exactly one argument. Usage: unalias <name>\n");
            return;
        }
        removeAlias(argv[1]);
    }
    else
    {
        execCommand(argv);
    }
}
// Stage 6 R1: Locate .hist_list in HOME Directory.
void get_history_path(char *path)
{ // The path finding function.
    char *home = getenv("HOME");
    if (home == NULL)
    {
        path[0] = '\0';
        return;
    }
    snprintf(path, MAX_LINE, "%s/.hist_list", home);
}

void load_history()
{
    char path[MAX_LINE];
    get_history_path(path);
    if (path[0] == '\0')
        return;

    // Stage 6 R2: Load history from .hist_list on startup.
    FILE *file = fopen(path, "r"); // Read mode

    // Stage 6 R5: Handle missing/failed files.
    if (file == NULL)
    {
        return;
    }
    // Stage 6 R4: Handle 512-character limit.
    char line[MAX_LINE + 20];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        line[strcspn(line, "\n")] = '\0'; // Remove newline.

        int num;
        char cmd[MAX_LINE];
        if (sscanf(line, "%d %511[^\n]", &num, cmd) == 2)
        {                     // Parse number and command.
            add_history(cmd); // Add to history.
        }
    }
    fclose(file);
}

void save_history()
{
    char path[MAX_LINE];
    get_history_path(path);
    if (path[0] == '\0')
        return;

    FILE *file = fopen(path, "w"); // write mode
    if (file == NULL)
    {
        fprintf(stderr, "Error: could not open history file for writing.\n");
        return;
    }
    int start = hist_count > HIST_SIZE ? hist_count - HIST_SIZE : 0;

    for (int i = start; i < hist_count; i++)
    {
        int index = i % HIST_SIZE;
        fprintf(file, "%d %s\n", i - start + 1, history[index]);
    }
    fclose(file);
}

int substituteCommand(char *input)
{
    char temp[MAX_LINE];
    strcpy(temp, input);
    char *first = strtok(temp, " \t\n");
    if (first == NULL)
    {
        return 0;
    }

    int idx = findAlias(first);
    if (idx != -1)
    {
        char newLine[MAX_LINE];
        strcpy(newLine, aliases[idx].command);
        char *command = input + strlen(first);
        strcat(newLine, command);
        strcpy(input, newLine);
        return 1;
    }
    return 0;
}

//Stage 8 
void get_aliases_path(char* path)
{
    char* home = getenv("HOME");

    if (home == NULL)
    { 
        *path = '\0';
        return;
    }

    snprintf(path, MAX_LINE, "%s/.aliases",home);
}

void save_aliases()
{
    char path[MAX_LINE];
    get_aliases_path(path);

    if(*path == '\0')
    {
        return;
    }

    FILE* file = fopen(path, "w");
    if (file == NULL)
    {
        fprintf(stderr,"myshell: error: file not found");
        return;
    }

    for(int idx = 0; idx < aliaseCount; idx++)
    {
        fprintf(file,"%s %s\n", aliases[idx].name, aliases[idx].command);
    }

    fclose(file);
}

void load_aliases() 
{
    char path[MAX_LINE];
    get_aliases_path(path);
    if(*path == '\0')
    {
        return;
    }

    FILE* file = fopen(path, "r");
    if (file == NULL) 
    {
        return;
    }

    char line[MAX_LINE];
    while(fgets(line,sizeof(line),file) != NULL)
    {
        char name[MAX_ALIAS_NAME];
        char cmd[MAX_ALIAS_COMMND];
       if (sscanf(line, "%s %[^\n]", name, cmd) == 2)
       {
         addAlias(name, cmd);
       }
            
    }

    fclose(file);
    return;
}

// Stage 9

int expand_command(char *input){
    int expansions = 0;
    int max_expansions = 5;
    int changed = 1;

    //R1: Modify Input Handling Loop.
    while(changed && expansions < max_expansions) {
        changed = 0;

        char *start = input;
        while(*start == ' ' || *start == '\t') {
            start++;
        }
        if(*start == '\0') break;

        char temp[MAX_LINE];
        strcpy(temp, start);
        char *first = strtok(temp, " \t\n");
        if(first == NULL) break;

        //R3: Integrate History Invocations in Aliases.
         if(first[0] == '!'){
            char resolved[MAX_LINE];
            if(resolve_history_invocation(start, resolved, sizeof(resolved))) {
                strcpy(input, resolved);
                changed = 1;
                expansions++;
                continue;
             }
             else{
                return 0;
             }
            }
             //R2: Implement alias to alias mapping.
              int idx = findAlias(first);
              if(idx != -1) { 
             char newLine[MAX_LINE];             
             strcpy(newLine, aliases[idx].command);                          
             char *rest = start + strlen(first);             
             strcat (newLine, rest);              
             strcpy(input, newLine);             
             changed = 1;             
             expansions++;             
             continue;         
            }     
            break;
        }          
        if(expansions >= max_expansions) {         
        fprintf(stderr, "Error:  Recursive alias or cycle detected.\n");         
        return 0;     }     
        return 1; }       


    





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

    //  char s[100];
    //  printf("%s\n", getcwd(s,100));

    load_history();
    load_aliases();

    while (1)
    {
        printf("shell> ");

        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            save_history();
            save_aliases();
            cleanup(originalPath);
            break;
        }

        char original_line[MAX_LINE];         
        strcpy(original_line, input);
        original_line[strcspn(original_line, "\n")] = '\0';          
        if(!expand_command(input)){             
            continue;          
        }           
        argc = parse_input(input, argv);           
        if(argc ==0){             
            continue;          
        }          
        if(original_line[0] != '!' && original_line[0] != '\n'){             
            add_history(original_line);}           
            commands(argv, argc, originalPath);

        }
}