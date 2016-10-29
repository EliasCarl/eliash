/* All parser functions. */

#include "eliash.h"

cmd* parse_input(char *cmdstr)
{
    char *pc; // Ptr to pipe char.
    if (pc = strchr(cmdstr, '|'))
    {
        /* 
         * There is a pipe character. For now assume there is only
         * _one_ pipe. Chop the cmdstr into the cmd before and after
         * the pipe (|) character, parse these and put into a pipecmd.
         *
         * TODO: What if pipe char is in beginning or end of cmdstr?
         */

        /* Chop the string where the pipe was. */
        *pc = '\0';

        char *right  = pc + 1;
        char *left = cmdstr;
        return build_pipe(parse_tokens(left), parse_tokens(right));
    }
    return parse_tokens(cmdstr);
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
    /* Trim leading and trailing whitespace. */
    cmdstr = trimcmd(cmdstr, whitespace);

    char *argv[MAXARGS];
    int argc = 0;

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
