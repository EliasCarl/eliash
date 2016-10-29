#define MAXARGS 10
#define BUFLEN 100

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

void run_command(cmd *command);

/* Parser functions. */
cmd* parse_tokens(char *cmdstr);
cmd* build_exec(char *argv[], int argc);
cmd* build_pipe(cmd *left, cmd *right);
cmd* parse_input(char *cmdstr);
char* locate_beginning(char *str, char *trimchars); 
char* locate_end(char *str, char *trimchars);
char* get_token_end(char *token, char *delimiters);
char* get_next_token(char *beginning, char *delimiters);

/* Helper/utility functions. */
int has_prefix(char *string, char *prefix);
void del_trailing(char *str, char *trimchars);
char* del_leading(char *str, char *trimchars);
char* trimcmd(char *cmd, char *trimchars);
