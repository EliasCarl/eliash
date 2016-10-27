/* Simple implemetation of a shell. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXARGS 10
#define BUFLEN 100

/* These are the types the parser will translate commands into. */

/* Resolve circular dependency in structs. */
typedef struct cmd cmd;

typedef enum {
    CMD_EXEC,
    CMD_REDIR,
    CMD_PIPE
} cmd_type;

typedef struct exec {
    char *argv[MAXARGS];
} cmd_exec;

typedef struct redir {
    cmd     *cmd;
    char    *fp;
    int      mode;
    int      fd;
} cmd_redir;

typedef struct pipe {
    cmd     *left;
    cmd     *right;
} cmd_pipe;

typedef struct cmd {
    cmd_type type;
    union cmd_data {
        cmd_pipe  pipe;
        cmd_redir redir;
        cmd_exec  exec;
    } data;
} cmd;

int has_prefix(char *string, char *prefix);
void run_command(cmd *command);
cmd* parse_command(char *command_str);
cmd* build_exec(char *argv[], int argc);
cmd* build_pipe(cmd *left, cmd *right);
char* locate_beginning(char *str, char *trimchars); 
char* locate_end(char *str, char *trimchars);
char* get_token_end(char *token, char *delimiters);
char* get_next_token(char *beginning, char *delimiters);

/* Execution begins here. Go into infinite input loop. */

int main(void)
{
    static char inbuf[BUFLEN]; 
    while (1)
    {
        printf("$ ");

        if (fgets(inbuf, BUFLEN, stdin) == NULL)
        {
            fprintf(stderr, "Error reading input\n");
            exit(EXIT_FAILURE);
        }

        /* cd is manipulates current process, no fork. */
        if (has_prefix(inbuf, "cd ") == 0)
        {
            *(inbuf + strlen(inbuf) - 1) = '\0';
            if (chdir(inbuf + 3) < 0)
            {
                perror("Cannot cd");
            }
            continue;
        }

        /* Other commands are forked before execed. */ 
        int pid, status;
        if ((pid = fork()) == 0)
            run_command(parse_command(inbuf));
        else
            wait(&status);
    }
    exit(EXIT_SUCCESS);
}

void run_command(cmd *command)
{
    switch (command->type)
    {
        case CMD_EXEC:
            execv(command->data.exec.argv[0], command->data.exec.argv);
            break;
        case CMD_REDIR:
            fprintf(stderr, "Redirection not implemented.");
            break;
        case CMD_PIPE:
            fprintf(stderr, "Pipes not implemented.");
            break;
    }
}

/* Only exec commands work for now. */
cmd* parse_command(char *cmd_str)
{
    /* TODO: Clean this mess up. */
    char whitespace[] = " \t\r\n\v";
    char *cmd_end;
    cmd_end = locate_end(cmd_str, whitespace);
    cmd_str = locate_beginning(cmd_str, whitespace);
    *cmd_end = '\0';

    int argc = 0;
    char *argv[MAXARGS];

    char *token = cmd_str;
    char *token_end;
    while (token)
    {
        /* Null-terminate this token. */
        token_end = get_token_end(token, whitespace); 
        *token_end = '\0';

        /* Add this token to the args. */
        argv[argc] = token;
        argc++;

        /* Search for next token after the end of this one. */
        token = get_next_token(token_end + 1, whitespace);
    }
    return build_exec(argv, argc);
}

char* get_token_end(char *token, char *delimiters)
{
    char *token_ptr = token;
    while (!strchr(delimiters, *token_ptr) && *token_ptr != '\0')
        token_ptr++;

    return token_ptr;
}

/* Skip delimiters and return next token. */
char* get_next_token(char *beginning, char *delimiters)
{
    char *cmd_end = beginning + strlen(beginning);
    char *token = beginning;
    int delimiter_found = 0;

    /* Search for first non-delimiter occuring after a delimiter.
     * If we reach end of the whole command, this was the last token. */
    while (token < cmd_end)
    {
        if (delimiter_found && !strchr(delimiters, *token))
        {
            /* A non-delimiter after a delimiter was found. */
            return token;   
        }

        if (!delimiter_found && strchr(delimiters, *token))
        {
            delimiter_found = 1;
        }

        token++;
    }

    /* Return NULL to indicate there is no next token. */
    return NULL;
}

/* Type builders. */

cmd* build_exec(char *argv[], int argc)
{
    cmd *command = malloc(sizeof(cmd));
    command->type = CMD_EXEC;

    /* Copy parsed args into the exec struct. */
    for (int i = 0; i < argc; i++)
    {
        command->data.exec.argv[i] = argv[i];
    }

    return command;
}

cmd* build_pipe(cmd *left, cmd *right)
{
    cmd *command = malloc(sizeof(cmd));
    command->type = CMD_PIPE;
    command->data.pipe.left = left;
    command->data.pipe.right = right;
    return command;
}

/* Some helper functions. */

int has_prefix(char *string, char *prefix)
{
    return strncmp(prefix, string, strlen(prefix));
}

/* Return ptr to first non-trim character from end. */
char* locate_end(char *str, char *trimchars) 
{
    char *str_end = str + strlen(str);
    while (strchr(trimchars, *str_end) != NULL)
       str_end--;
    return str_end + 1;
}

/* Return ptr to first non-trim character. */
char* locate_beginning(char *str, char *trimchars) 
{
    while (strchr(trimchars, *str) != NULL)
       str++;
    return str;
}
