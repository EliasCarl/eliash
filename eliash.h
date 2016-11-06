#ifndef ELIASH_H
#define ELIASH_H

#define _GNU_SOURCE // execvpe

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define MAXARGS 10
#define BUFLEN 1024
#define CWDBUFLEN 256

/* 
 * The mode to use in case a new file is created in redirection.
 * See man 2 open for more information on these modes.
 */
#define CREATE_MODE S_IRWXU|S_IRWXG|S_IRWXO

/* Resolve circular dependency. */
typedef struct cmd cmd;

typedef enum {
    CMD_EXEC,
    CMD_REDIR,
    CMD_PIPE
} cmd_type;

/*
 * An execution command is the fundamental type. It is, as always,
 * the arguments to the command and nothing else. argv[0] is the
 * name of the program to execute. The full path of the program
 * must be used (i.e. /bin/ls) since the shell doesn't search PATH.
 */
typedef struct exec {
    char **argv;
} cmd_exec;

/*
 * A redirection is a command (see more the general cmd type below)
 * which should have its stdio file descriptors changed before being
 * executed. It needs a filepath to the file involved in the redir,
 * and the filedescriptor to open that file on. 
 */
typedef struct redir {
    cmd     *cmd;
    char    *fp;
    int      mode;
    int      fd;
} cmd_redir;

/*
 * Since pipes are pretty much handled by the kernel only the two
 * commands involved in the pipes are needed. See the implementation
 * of run_command in eliash.c to see how pipes are used by the shell.
 */
typedef struct pipe {
    cmd     *left;
    cmd     *right;
} cmd_pipe;

/*
 * This is the general command structure. Since a command is either
 * one of the three already described, a union helps to generalize.
 * The cmd_type indicates to the shell how to interpret the union.
 * This makes it possible to pass any of the three commands to 
 * run_command.
 */
typedef struct cmd {
    cmd_type type;
    union cmd_data {
        cmd_pipe  pipe;
        cmd_redir redir;
        cmd_exec  exec;
    } data;
} cmd;

void run_command(cmd *command);
void run_pipe(cmd_pipe *pipe);
void run_redir(cmd_redir *redircmd);
void run_exec(cmd_exec *execcmd);
void changedir(char *inbuf);

/* Parser functions. */
cmd* parse_exec(char *execstr);
cmd* build_exec(char *argv[], int argc);
cmd* build_pipe(cmd *left, cmd *right);
cmd* build_redir(cmd *cmd, char *fp, int mode, int fd);
cmd* parse_command(char *cmdstr);
char* locate_beginning(char *str, char *trimchars); 
char* locate_end(char *str, char *trimchars);
char* get_token_end(char *token, char *delimiters);
char* get_next_token(char *beginning, char *delimiters);

/* Helper/utility functions. */
pid_t ecfork(void);
char* ecgetcwd(char *buf, size_t size);
int has_prefix(char *string, char *prefix);
void del_trailing(char *str, char *trimchars);
char* del_leading(char *str, char *trimchars);
void fatal(char *msg);
#endif
