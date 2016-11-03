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

        /* cd manipulates current process, no fork. */
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
            cmd *command = parse_command(inbuf);
            run_command(command);
        }

        wait(NULL);
    }
    exit(EXIT_SUCCESS);
}

void run_command(cmd *command)
{
    if (command->type == CMD_EXEC)
    {
        int ret = execv(command->data.exec.argv[0], command->data.exec.argv);
        if (ret == -1)
        {
            perror("execv");
            exit(EXIT_FAILURE);
        }
    }
    else if (command->type == CMD_REDIR)
    {
        int fd = open(command->data.redir.fp, command->data.redir.mode);
        dup2(fd, command->data.redir.fd);
        fprintf(stderr, "Running redir");
        run_command(command->data.redir.cmd);
    }
    else if (command->type == CMD_PIPE)
    {
        int pipefd[2];
        if (pipe(pipefd) < 0)
        {
            perror("Error establishing pipe.");
            exit(EXIT_FAILURE);
        }

        int pid = fork();
        if (pid == 0)
        {
            dup2(pipefd[1], 1);
            close(pipefd[0]);
            close(pipefd[1]);

            /* stdout now writes to pipe write-end. */
            run_command(command->data.pipe.left);
        }

        pid = fork();
        if (pid == 0)
        {
            dup2(pipefd[0], 0);
            close(pipefd[0]);
            close(pipefd[1]);

            /* stdin now reads from pipe read-end. */
            run_command(command->data.pipe.right);
        }

        /* Close pipe in parent process. */
        close(pipefd[0]);
        close(pipefd[1]);

        /* Wait for both children to finish. */
        wait(NULL);
        wait(NULL);
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
