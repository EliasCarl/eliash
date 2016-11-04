#include "eliash.h"

int main(void)
{
    pid_t pid;
    static char inbuf[BUFLEN]; 
    while (1)
    {
        printf("$ ");

        if (fgets(inbuf, BUFLEN, stdin) == NULL)
            fatal("Error reading input");

        if (has_prefix(inbuf, "cd "))
        {
            /* cd manipulates current process, don't fork. */
            changedir(inbuf);
        }
        else
        {
            if ( (pid = ecfork()) == 0 )
                run_command(parse_command(inbuf)); // Doesn't return!

            wait(NULL);
        }
    }
}

void run_command(cmd *command)
{
    if (command->type == CMD_EXEC)
    {
        run_exec(&command->data.exec);
    }
    else if (command->type == CMD_REDIR)
    {
        run_redir(&command->data.redir);
    }
    else if (command->type == CMD_PIPE)
    {
        run_pipe(&command->data.pipe);
    }

    /*
     * It's important that we exit here. If we don't, this process
     * will return from run_command and go into its own read/eval
     * loop. This exit makes sure that when a redirection or pipe
     * has forked its children and done its work, it exits, just 
     * as if it was its own command. Pipes and redirections are
     * then transparent to the read eval loop and execute just as
     * single exec commands.
     */
    exit(EXIT_SUCCESS);
}

void run_exec(cmd_exec *execcmd)
{
    if (execv(execcmd->argv[0], execcmd->argv) == -1)
        fatal("execv");
}

void run_redir(cmd_redir *redircmd)
{
    int file = open(redircmd->fp, redircmd->mode);
    dup2(file, redircmd->fd);
    run_command(redircmd->cmd);
}

void run_pipe(cmd_pipe *pipecmd)
{
    pid_t pid;
    int pipefd[2];

    if (pipe(pipefd) < 0)
        fatal("Error establishing pipe.");

    if ( (pid = ecfork()) == 0 )
    {
        dup2(pipefd[1], 1);
        close(pipefd[0]);
        close(pipefd[1]);

        /* stdout now writes to pipe write-end. */
        run_command(pipecmd->left);
    }

    if ( (pid = ecfork()) == 0 )
    {
        dup2(pipefd[0], 0);
        close(pipefd[0]);
        close(pipefd[1]);

        /* stdin now reads from pipe read-end. */
        run_command(pipecmd->right);
    }

    /* Close pipe in parent process. */
    close(pipefd[0]);
    close(pipefd[1]);

    /* Wait for both children to finish. */
    wait(NULL);
    wait(NULL);
}

void changedir(char *inbuf)
{
    inbuf[strlen(inbuf) - 1] = '\0';
    if (chdir(inbuf + 3) < 0)
        fatal("Cannot cd");
}

int has_prefix(char *string, char *prefix)
{
    return !strncmp(prefix, string, strlen(prefix));
}

/* Error checked forking. */
pid_t ecfork()
{
    pid_t pid = fork();
    if (pid == -1) fatal("Forking error");
    if (pid == 0)  return 0;
    else           return pid;
}

void fatal(char *msg)
{
    perror(msg); 
    exit(EXIT_FAILURE);
}
