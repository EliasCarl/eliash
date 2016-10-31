/*
 * The functions to parse a command sent to the shell. A command is
 * parsed into on of three types: CMD_EXEC, CMD_REDIR and CMD_PIPE.
 * The redirection and pipe command structs are just compounds of
 * the exec command, indicating to the shell that some shuffling
 * of file descriptors should take place before calling execv().
 * These three types are "unioned" into a more general type (cmd)
 * which will be executed by the shell.
 *
 * See eliash.h for a more detailed description of these types.
 *
 * The parser takes the given command string and tokenizes it 
 * _in place_. That is, where a delimiter of some sort is found,
 * the parser terminates the string at that place. This is similar
 * to what strtok() does, and strok should probably be used instead.
 * But since I'm just doing this for learning purposes I'll just
 * keep my implementation for the moment.
 *
 * TODO: Implement checking for MAXARGS.
 */

#include "eliash.h"
static char whitespace[] = " \t\r\n\v";

/*
 * Search for the exec commands and construct them with whatever
 * redirection and piping can be found. When an exec command has
 * been isolated, it is parsed and built by parse_exec.
 */
cmd* parse_command(char *cmdstr)
{
    char *cmdbeginning = cmdstr;
    while (*cmdstr)
    {
        if (*cmdstr == '|')
        {
            (*cmdstr) = '\0';
            return build_pipe(parse_exec(cmdbeginning), parse_command(cmdstr + 1));
        }
        cmdstr++;
    }
    return parse_exec(cmdbeginning);
}

cmd* parse_exec(char *execstr)
{
    /* Trim leading and trailing whitespace. */
    execstr = trimcmd(execstr, whitespace);

    char *argv[MAXARGS];
    int argc = 0;

    char *token = execstr;
    while (token)
    {
        /* Chop this token. */
        char *token_end = get_token_end(token, whitespace); 
        *token_end = '\0';

        /* Point current arg to this token. */
        argv[argc] = token;
        argc++;

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
    if (!strchr(delimiters, *token))
        return token; // Already on a token.

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
    int i;
    for (i = 0; i < argc; i++)
        command->data.exec.argv[i] = argv[i];

    argv[++i] = NULL; // execv needs null terminated argv.
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
