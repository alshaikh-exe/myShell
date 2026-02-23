/*
Stage 6: Persistent history
1- Locate .hist_list in HOME Directory.
2- Load history from .hist_list on startup.
3- Parse each line (number + command) and initialie history structure.
4- Handle 512-character limit.
5- Handle missing/failed files.
6- Save history to .hist_list on exit.
7- Integrate loading/saving into start and exit.
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

char history[HIST_SIZE][MAX_LINE];

int hist_count = 0; // number of commands entered
int hist_next = 0; // circular pointer
void print_history();

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

void commands(char **argv, int argc, char *originalPath)
{
    if (strcmp(argv[0], "exit") == 0)
    {
        cleanup(originalPath);
        exit(0);
    } // Handle getpath and setpath
    else if (strcmp(argv[0], "history") == 0)
 {
        print_history();
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
    else
    {
        execCommand(argv);
    }
}

int is_history_command(char *line)
{
    if (line[0] == '!')
        return 1;
    return 0;
}

void add_history(char *line)
{
    if (line[0] == '!' || line[0] == '\0')
        return;

    strcpy(history[hist_next], line);

    hist_next = (hist_next + 1) % HIST_SIZE; // count = (count+1) % 20
    hist_count++;
}

void print_history()
{

    if (hist_count == 0)
    {
        printf("History is empty");
        return;
    }

    int start = hist_count > HIST_SIZE ? hist_count - HIST_SIZE : 0;

    for (int i = start; i < hist_count; i++)
    {
        int index = i % HIST_SIZE;
        printf("%d %s \n", i + 1, history[index]);
    }
}
/////////////////////////////////////////////// Stage 6 and 7 Implementation

int history_exists(int cmd_no)
{
    if (hist_count <= 0) return 0;
    if (cmd_no < 1 || cmd_no > hist_count) return 0;

    // if history is full (more than HIST_SIZE commands ever entered),
    if (hist_count > HIST_SIZE)
    {
        int oldest_available = hist_count - HIST_SIZE + 1;
        if (cmd_no < oldest_available) return 0;
    }
    return 1;
}

/*
 * copies the command line for history command number cmd_no into out (null-terminated).
 * returns 1 on success 0 on failure.
 */
int get_history_command(int cmd_no, char *out, size_t outsz)
{
    if (!history_exists(cmd_no)) return 0;

    int index = (cmd_no - 1) % HIST_SIZE;
    strncpy(out, history[index], outsz - 1);
    out[outsz - 1] = '\0';


    out[strcspn(out, "\n")] = '\0';
    return 1;
}


int resolve_history_invocation(const char *line, char *out, size_t outsz)
{
    // skip leading whitespace
    while (*line == ' ' || *line == '\t') line++;

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
    while (*rest == ' ' || *rest == '\t') rest++;

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
            target_no = hist_count;
        else
            target_no = (hist_count + 1) - n; // "current command number" is hist_count+1
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
    }

    if (!history_exists(target_no))
    {
        // differentiate “out of range” vs “too old / overwritten”
        if (target_no < 1 || target_no > hist_count)
        {
            fprintf(stderr, "Error: no such command in history: %d.\n", target_no);
        }
        else
        {
            fprintf(stderr, "Error: command %d is not in the last %d history entries.\n", target_no, HIST_SIZE);
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
    while (*out == ' ' || *out == '\t') out++;
    if (out[0] == '!')
    {
        fprintf(stderr, "Error: history invocation cannot resolve to another history invocation.\n");
        return 0;
    }

    return 1;
}
/// /////////////////////////////
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
    while (1)
    {
        printf("shell> ");

        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            cleanup(originalPath);
            break;
        }
////////////////////////////////////////////////////////////////////////////////////
        char original_line[MAX_LINE]; // command unmodified by strtok
        strcpy(original_line, input);

        int is_history = is_history_command(original_line); // check raw line

        // if history invocation, resolve it BEFORE parse_input()
        if (is_history)
        {
            char resolved[MAX_LINE];

            if (!resolve_history_invocation(original_line, resolved, sizeof(resolved)))
            {
                continue;
            }

            // parse and execute the resolved command (do NOT store in history)
            char exec_line[MAX_LINE];
            strncpy(exec_line, resolved, sizeof(exec_line) - 1);
            exec_line[sizeof(exec_line) - 1] = '\0';

            argc = parse_input(exec_line, argv);
            if (argc == 0) continue;

            commands(argv, argc, originalPath);
            continue;
        }

        // normal (non-history) command: parse input as before
        argc = parse_input(input, argv);
///////////////////////////////////////////////////////////////////////////////////
        if (argc == 0)
        {
            continue;
        }

        if (!is_history && strcmp(argv[0], "history") != 0)
            add_history(original_line);

        commands(argv, argc, originalPath);
    }

    return 0;
}