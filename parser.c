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
 * TODO: Cleaner solution for redirection.
 * TODO: Generalize redirection to work with execs and files.
 */

#include "eliash.h"
static char whitespace[] = " \t\r\n\v";

cmd* parse_command(char *cmdstr)
{
    char *cmdbeginning = cmdstr;
    char *modifier;
    
    if ((modifier = strchr(cmdstr, '|')))
    {
        (*modifier) = '\0';
        return build_pipe(parse_command(cmdbeginning), parse_command(modifier + 1));
    }

    if ((modifier = strchr(cmdstr, '>')))
    {
        (*modifier) = '\0';
        char *fp = modifier + 1;
        fp = get_next_token(fp, whitespace);
        *(get_token_end(fp, whitespace)) = '\0';
        return build_redir(parse_command(cmdbeginning), fp, O_CREAT, 1);
    }

    if ((modifier = strchr(cmdstr, '<')))
    {
        (*modifier) = '\0';
        char *fp = modifier + 1;
        fp = get_next_token(fp, whitespace);
        *(get_token_end(fp, whitespace)) = '\0';
        return build_redir(parse_command(cmdbeginning), fp, O_RDONLY, 0);
    }

    return parse_exec(cmdbeginning);
}

cmd* parse_exec(char *execstr)
{
    del_trailing(execstr, whitespace);
    execstr = del_leading(execstr, whitespace);

    char **argv = malloc(sizeof(char *) * MAXARGS);
    char *token_end;
    int argc = 0;

    char *token = execstr;
    while (token)
    {
        token_end = get_token_end(token, whitespace); 
        *token_end = '\0';
        argv[argc++] = token;
        token = get_next_token(token_end + 1, whitespace);
    }
    argv[argc] = NULL; // execvpe expects null terminated list.
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

/* Constructors */

cmd* build_exec(char **argv, int argc)
{
    cmd *command = malloc(sizeof(cmd));
    command->type = CMD_EXEC;
    command->data.exec.argv = argv;
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

cmd* build_redir(cmd *exec, char *fp, int mode, int fd)
{
    cmd *command = malloc(sizeof(cmd));
    char *path = malloc(sizeof(char) * strlen(fp) + 1);
    strcpy(path, fp);
    command->type = CMD_REDIR;
    command->data.redir.cmd = exec;
    command->data.redir.fp = path;
    command->data.redir.mode = mode;
    command->data.redir.fd = fd;
    return command;
}

/* Utility functions. */

char* trim_leading(char *cmd, char *trimchars)
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
