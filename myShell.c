// Imports / Libraries 
#include <stdio.h>
#include <string.h>
#include <unistd.h>    
#include <sys/types.h> 
#include <sys/wait.h> 
#include <stdlib.h>    
#include <errno.h>   
#include <ctype.h>   

// Constants / Macros

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

int hist_count = 0; 
int hist_next = 0;  

void print_history(char **argv, int argc);
void save_history();
void save_aliases();


/* parses a raw input line into argv tokens for later execution. */
int parse_input(char *line, char *argv[])
{
    char *token;
    int argc = 0; 

    if (line == NULL)
        return 0;

    line[strcspn(line, "\n")] = 0;

    if (strlen(line) == 0)
        return 0; 

    token = strtok(line, " \t\n|><&;");

    while (token != NULL && argc < MAX_ARGS - 1)
    {
        argv[argc] = token;
        argc++;
        token = strtok(NULL, " \t\n|><&;"); 
    }

    argv[argc] = NULL; 
    return (argc);     
}
/* forks and executes an external program using execvp then waits for it. */
void execCommand(char *argv[])
{
    pid_t pid = fork(); 
    if (pid < 0)
    {
        perror("Fork Failed");
    }
    else if (pid == 0)
    { 
        // replace new child process with program passed by the user
        execvp(argv[0], argv);
     
        fprintf(stderr, "%s command not found\n", argv[0]);
        exit(1); 
    }
    else
    {

        wait(NULL);
    }
}
/* prints the current PATH value or reports invalid usage. */
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
/* changes the PATH environment variable after checking the argument count. */
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
/* restores the original PATH and releases saved memory before exit. */
void cleanup(char *originalPath)
{
    if (originalPath != NULL)
    {
        setenv("PATH", originalPath, 1);
        printf("Restored Path: %s\n", getenv("PATH"));
        free(originalPath);
    }
}
/* changes the current directory to HOME or to the directory given by the user. */
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

/* returns 1 when the input line starts with a history invocation. */
int is_history_command(char *line)
{
    if (line[0] == '!')
        return 1;
    return 0;
}
/* clears all stored history entries and resets the circular buffer state. */
void clearHistory()
{

    for (int i = 0; i < HIST_SIZE; i++)
    {
        history[i][0] = '\0'; 
    }
    hist_count = 0;
    hist_next = 0;
}
/* adds a command line to the circular history buffer unless it is empty or a history shortcut. */
void add_history(char *line)
{
    if (line[0] == '!' || line[0] == '\0')
        return;/* Store commands in a circular buffer so the oldest entry is overwritten after 20 commands. */

    strcpy(history[hist_next], line);

    hist_next = (hist_next + 1) % HIST_SIZE; 
    hist_count++;
}
/* prints the stored history entries in ascending order. */
void print_history(char **argv, int argc)
{
    if (argc == 1)
    {

        if (hist_count == 0)
        {
            printf("History is empty\n");
            return;
        }
        /* Start from the oldest available entry so history prints in correct chronological order. */
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
/* checks whether a history command number is still available in the buffer. */
int history_exists(int cmd_no)
{
    if (hist_count <= 0)
        return 0;
    if (cmd_no < 1 || cmd_no > hist_count)
        return 0;

    /* Reject numbers that are outside the active history window. */
    if (hist_count > HIST_SIZE)
    {
        int oldest_available = hist_count - HIST_SIZE + 1;
        if (cmd_no < oldest_available)
            return 0;
    }
    return 1;
}
/* copies a stored history command into the output buffer if it exists. 
 copies the command line for history command number cmd_no into out (null-terminated).
 returns 1 on success 0 on failure.
 */
int get_history_command(int cmd_no, char *out, size_t outsz)
{
    if (!history_exists(cmd_no))
        return 0;
    /* map the history command number to the circular-buffer slot. */
    int index = (cmd_no - 1) % HIST_SIZE;
    strncpy(out, history[index], outsz - 1);
    out[outsz - 1] = '\0';

    out[strcspn(out, "\n")] = '\0';
    return 1;
}
/* resolves !!, !n, and !-n into the original command line to execute. */
int resolve_history_invocation(const char *line, char *out, size_t outsz)
{

    while (*line == ' ' || *line == '\t')
        line++;

    if (*line != '!')
        return 0; 

    if (hist_count == 0)
    {
        fprintf(stderr, "Error: history is empty.\n");
        return 0;
    }


    char token[MAX_LINE];
    size_t i = 0;
    while (line[i] != '\0' && line[i] != ' ' && line[i] != '\t' && line[i] != '\n' && i < sizeof(token) - 1)
    {
        token[i] = line[i];
        i++;
    }
    token[i] = '\0';


    const char *rest = line + i;
    while (*rest == ' ' || *rest == '\t')
        rest++;

    int target_no = -1;

  
    if (strcmp(token, "!!") == 0) /* handle !! as the most recent command. */
    {
        target_no = hist_count; 
    }

    /* handle !-n as a relative lookup from the current history count. */
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


        if (n == 0)
        {
            target_no = hist_count;
        }
       /* handle !n as an absolute history number. */
        else
        {
            target_no = (hist_count + 1) - n; 
        }
    }
 
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

        if (target_no < 1 || target_no > hist_count)
        {
            fprintf(stderr, "Error: command \"%d\" doesn't exist (only %d commands entered).\n", target_no, hist_count);
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
    if (*rest == '\0')/* append any extra arguments after the resolved history command. */
    {
        strncpy(out, base, outsz - 1);
        out[outsz - 1] = '\0';
    }
    else
    {
  
        snprintf(out, outsz, "%s %s", base, rest);/* rebuild the command line using the stored command plus the new tail arguments. */
    }

    return 1;
}
/* searches the alias table and returns its index or -1 if it does not exist. */
int findAlias(const char *name)
{   /* search the alias table for a matching name. */
    for (int i = 0; i < aliaseCount; i++)
    {
        if (strcmp(aliases[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}
/* prints all currently defined aliases. */
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
/* adds a new alias or replaces an existing alias with the same name. */
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
/* removes an alias and shifts the remaining entries to keep the array packed. */
void removeAlias(const char *name)
{
    int idx = findAlias(name);

    if (idx == -1)
    {
        printf("Alias \"%s\" not found!\n", name);
        return;
    }
    /* shift aliases left to keep the array compact after removal. */
    for (int i = idx; i < aliaseCount - 1; i++)
    {
        aliases[i] = aliases[i + 1];
    }

    aliaseCount--;
    printf("Alias \"%s\" has been removed\n", name);
}
/* combines alias command arguments into one command string. */
void combineCommand(char *cmd, char **argv, int argc)
{
    cmd[0] = '\0';

    for (int i = 2; i < argc; i++)/* join the alias command tokens into one space-separated string. */
    {
        strcat(cmd, argv[i]);
        if (i < argc - 1)
        {
            strcat(cmd, " ");
        }
    }
}
/* dispatches built-in commands or forwards unknown commands to execution. */
void commands(char **argv, int argc, char *originalPath)
{
    if (strcmp(argv[0], "exit") == 0)
    {
        save_history();
        save_aliases();
        cleanup(originalPath);
        exit(0);
    } 
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
/* builds the full file path for the persistent history file in the user's home directory. */
void get_history_path(char *path)
{ 
    char *home = getenv("HOME");
    if (home == NULL)
    {
        path[0] = '\0';
        return;
    }
    snprintf(path, MAX_LINE, "%s/.hist_list", home);
}
/* loads command history from the history file if it exists. */
void load_history()
{
    char path[MAX_LINE];
    get_history_path(path);
    if (path[0] == '\0')
        return;


    FILE *file = fopen(path, "r"); 

 
    if (file == NULL)
    {
        return;
    }

    char line[MAX_LINE + 20];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        line[strcspn(line, "\n")] = '\0'; 

        int num;
        char cmd[MAX_LINE];
        if (sscanf(line, "%d %511[^\n]", &num, cmd) == 2)/* ignore bad lines instead of crashing on bad file contents. */
        {                
            add_history(cmd);
        }
    }
    fclose(file);
}
/* saves the current in-memory history entries to the history file. */
void save_history()
{
    char path[MAX_LINE];
    get_history_path(path);
    if (path[0] == '\0')
        return;

    FILE *file = fopen(path, "w"); 
    if (file == NULL)
    {
        fprintf(stderr, "Error: could not open history file for writing.\n");
        return;
    }
    int start = hist_count > HIST_SIZE ? hist_count - HIST_SIZE : 0;

    for (int i = start; i < hist_count; i++)/* overwrite the previous file so the saved state matches the current shell state. */
    {
        int index = i % HIST_SIZE;
        fprintf(file, "%d %s\n", i - start + 1, history[index]);
    }
    fclose(file);
}

/* builds the full file path for the persistent alias file in the user's home directory. */
void get_aliases_path(char *path)
{
    char *home = getenv("HOME");

    if (home == NULL)
    {
        *path = '\0';
        return;
    }

    snprintf(path, MAX_LINE, "%s/.aliases", home);
}
/* saves all currently defined aliases to the alias file. */
void save_aliases()
{
    char path[MAX_LINE];
    get_aliases_path(path);

    if (*path == '\0')
    {
        return;
    }

    FILE *file = fopen(path, "w");
    if (file == NULL)
    {
        fprintf(stderr, "myshell: error: file not found");
        return;
    }

    for (int idx = 0; idx < aliaseCount; idx++)
    {
        fprintf(file, "%s %s\n", aliases[idx].name, aliases[idx].command);
    }

    fclose(file);
}
/* loads alias entries from the alias file if it exists. */
void load_aliases()
{
    char path[MAX_LINE];
    get_aliases_path(path);
    if (*path == '\0')
    {
        return;
    }

    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
        return;
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), file) != NULL)
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

/* replaces aliases and history shortcuts until the command no longer changes. */
int expand_command(char *input)
{
    int expansions = 0;
    int max_expansions = 5;
    int changed = 1;


    while (changed && expansions < max_expansions)/* keep resolving aliases and history references until no more substitutions apply. */
    {
        changed = 0;

        char *start = input;
        while (*start == ' ' || *start == '\t')
        {
            start++;
        }
        if (*start == '\0')
            break;

        char temp[MAX_LINE];
        strcpy(temp, start);
        char *first = strtok(temp, " \t\n");
        if (first == NULL)
            break;

    
        if (first[0] == '!')
        {
            char resolved[MAX_LINE];
            if (resolve_history_invocation(start, resolved, sizeof(resolved)))
            {
                strcpy(input, resolved);
                changed = 1;
                expansions++;
                continue;
            }
            else
            {
                return 0;
            }
        }
 
        int idx = findAlias(first);/* resolve history shortcuts before checking aliases, since aliases may expand to history commands. */
        if (idx != -1)
        {
            char newLine[MAX_LINE];
            strcpy(newLine, aliases[idx].command);
            char *rest = start + strlen(first);
            strcat(newLine, rest);
            strcpy(input, newLine);
            changed = 1;
            expansions++;
            continue;
        }
        break;
    }
    if (expansions >= max_expansions)/* Stop after a safe number of substitutions to avoid alias cycles. */
    {
        fprintf(stderr, "Error:  Recursive alias or cycle detected.\n");
        return 0;
    }
    return 1;
}

int main(void)
{
    char input[512];
    char *argv[MAX_ARGS];
    int argc;
    char *home = getenv("HOME");
    char *originalPath = strdup(getenv("PATH"));


    if (home != NULL)
    {
        chdir(home);
    }

    load_history();
    load_aliases();
/* main shell loop: read commands, expand aliases/history, parse, and execute. */
    while (1)
    {
        printf("shell> ");

        if (fgets(input, sizeof(input), stdin) == NULL)/* save state and clean up if the user closes input with Ctrl-D. */
        {
            save_history();
            save_aliases();
            cleanup(originalPath);
            break;
        }

        char original_line[MAX_LINE];
        strcpy(original_line, input);
        original_line[strcspn(original_line, "\n")] = '\0';
        if (!expand_command(input))
        {
            continue;
        }
        argc = parse_input(input, argv);
        if (argc == 0)
        {
            continue;
        }
        if (original_line[0] != '!' && original_line[0] != '\n')
        {
            add_history(original_line);
        }
        commands(argv, argc, originalPath);
    }
}