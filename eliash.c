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

cmd* parse_command(char *cmd_str)
{
    /* TODO: Remove trailing whitespace. */
    /* TODO: Generalize delimiters. */
    /* Only exec commands work for now. */
    int argc = 0;
    char *cmd_str_ptr = cmd_str; // Current position during traversal.
    char *cmd_str_end = cmd_str + strlen(cmd_str);
    char *argv[MAXARGS];

    /* Remove trailing newline. */
    *(cmd_str_end - 1) = '\0';

    /* Chop up cmd_string by delimiters (only ' ' for now). */
    while (cmd_str_ptr < cmd_str_end)
    {
        char *next_token = strchr(cmd_str_ptr, ' ');

        if (next_token == NULL)
        {
            /* This is the last token. */
            argv[argc] = cmd_str_ptr;
            argc++;

            /* Manually set next token to indicate end of tokens. */
            next_token = cmd_str_end;
        }
        else
        {
            argv[argc] = cmd_str_ptr;
            argc++;

            *next_token = '\0';
            next_token++;
        }
        /* Set the "current pointer" to next token for upcoming iteration. */
        cmd_str_ptr = next_token;
    }
    return build_exec(argv, argc);
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
