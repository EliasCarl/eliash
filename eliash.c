#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "eliash.h"

/* Begin read-eval loop. */
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
            inbuf[strlen(inbuf) - 1] = '\0';
            if (chdir(inbuf + 3) < 0)
            {
                perror("Cannot cd");
            }
            continue;
        }

        int pid = fork();
        if (pid == -1)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0)
        {
            cmd *command = parse_tokens(inbuf);
            run_command(command);
        }
        else
            wait(NULL);
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

            int pipefd[2];
            pipe(pipfd);

            /* leftcmd and rightcmd must be exec cmds for now.
             * Since the parser can't handle anything else. */

            int pid = fork();
            if (pid == 0)
            {
                /* Fork a new process for leftcmd. */
                close(pipefd[0]);  // Close read end of pipe.
                fclose(stdout);    // Free stdout.
                fdopen(pipefd[1]); // Open write end on stdout.
                
                /* stdout now writes to pipe. */
                run_command(command->data.pipe.left);
            }

            break;
    }
}


/* 
 * Parse and chop tokens in cmdstr _in place_. This means that every
 * token separated by some delimiters (whitespace) gets chopped into
 * their own null-terminated strings and placed into argv. 
 *
 * TODO: Simple implementation of _one_ pipe.
 */
char whitespace[] = " \t\r\n\v";
cmd* parse_tokens(char *cmdstr)
{
    char *argv[MAXARGS];
    int argc = 0;

    /* Trim leading and trailing whitespace. */
    cmdstr = trimcmd(cmdstr, whitespace);

    char *token = cmdstr;
    while (token)
    {
        /* Chop this token. */
        char *token_end = get_token_end(token, whitespace); 
        *token_end = '\0';

        /* Point current arg to this token. */
        argv[argc] = token;
        argc++;

        /* Start search of next token at token_end + 1.*/
        token = get_next_token(token_end + 1, whitespace);
    }
    return build_exec(argv, argc);
}

char* get_token_end(char *token, char *delimiters)
{
    while (!strchr(delimiters, *token) && *token != '\0')
        token++;
    return token;
}

char* get_next_token(char *token, char *delimiters)
{
    char *cmd_end = token + strlen(token);
    int delimiter_found = 0;
    while (token < cmd_end)
    {
        if (delimiter_found && !strchr(delimiters, *token))
            return token;   

        if (!delimiter_found && strchr(delimiters, *token))
            delimiter_found = 1;

        token++;
    }
    return NULL; // No next token exists.
}

cmd* build_exec(char *argv[], int argc)
{
    cmd *command = malloc(sizeof(cmd));
    command->type = CMD_EXEC;

    /* Copy parsed args into the exec struct. */
    for (int i = 0; i < argc; i++)
        command->data.exec.argv[i] = argv[i];

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

/* Utility functions. */

int has_prefix(char *string, char *prefix)
{
    return strncmp(prefix, string, strlen(prefix));
}

char* trimcmd(char *cmd, char *trimchars)
{
    del_trailing(cmd, trimchars);
    return del_leading(cmd, trimchars);
}

void del_trailing(char *str, char *trimchars) 
{
    char *strend = str + strlen(str);
    while (strchr(trimchars, *strend))
        strend--;
    *(strend + 1) = '\0';
}

char* del_leading(char *str, char *trimchars) 
{
    while (strchr(trimchars, *str))
        str++;
    return str;
}
