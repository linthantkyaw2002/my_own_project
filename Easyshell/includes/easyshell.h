#ifndef EASYSHELL_H
# define EASYSHELL_H

# include <stdio.h>
# include <errno.h>
# include <fcntl.h>
# include <limits.h>
# include <readline/history.h>
# include <readline/readline.h>
# include <signal.h>
# include <stdlib.h>
# include <string.h>
# include <sys/stat.h>
# include <sys/wait.h>
# include <unistd.h>
# include <dirent.h>
# include "libft.h"

extern int	g_signal_received;

/* ==========================================
   1. TYPE DEFINITIONS
   ========================================== */

typedef struct s_env
{
	char			*key;
	char			*value;
	struct s_env	*next;
}					t_env;

typedef enum s_token_type
{
	TOKEN_WORD,
	TOKEN_PIPE,
	TOKEN_REDIRECT_IN,
	TOKEN_REDIRECT_OUT,
	TOKEN_APPEND,
	TOKEN_HEREDOC,
	TOKEN_AMBIGUOUS,
	TOKEN_AND,
	TOKEN_OR,
	TOKEN_LPAREN,
	TOKEN_RPAREN,
	TOKEN_BG,
	TOKEN_EOF
}					t_token_type;

typedef struct s_token
{
	char			*value;
	t_token_type	type;
	struct s_token	*next;
}					t_token;

typedef struct s_shell
{
	t_env			*env;
	t_token			*tokens;
	int				last_exit;
	int				visual_mode;
	int				skip_visual;
}					t_shell;

typedef struct s_expand
{
	char			*str;
	int				i;
	int				in_single;
	int				in_double;
	t_shell			*shell;
}					t_expand;

typedef struct s_cmd
{
	char			**args;
	int				infile;
	int				outfile;
	int				error;
	pid_t			pid;         
	int				exit_status; 
	struct s_cmd	*next;
}					t_cmd;

typedef enum s_node_type
{
	NODE_PIPELINE,
	NODE_AND,
	NODE_OR,
	NODE_SUBSHELL,
	NODE_BG
}					t_node_type;

typedef struct s_ast
{
	t_node_type		type;
	struct s_ast	*left;
	struct s_ast	*right;
	t_cmd			*pipeline;
}					t_ast;


/* ==========================================
   2. GLOBAL FUNCTION PROTOTYPES
   ========================================== */

/* --- Core --- */
void	setup_signals(void);
void	setup_execution_signals(void);

/* --- Utils --- */
char	*read_byte_by_byte(void);
int		is_quote(char c, int *in_single, int *in_double);
char	*append_char(char *res, char c);
char	*append_str(char *res, const char *add);
char	*strip_quotes(char *str);
int     ft_isspace(char c);
int     ft_strcmp(const char *s1, const char *s2);
int     has_quotes(char *str);

/* --- Environment --- */
t_env	*init_env(char **envp);
void	free_env(t_env *env);
char	*get_var_name(char *str, int *i);
char	*get_env_value(char *key, t_env *env);
void	set_env_value(t_env **env, char *key, char *value);
void	increment_shlvl(t_env **env);

/* --- Lexer --- */
t_token	*tokenize(char *input);
int		check_syntax(t_token *t);
char	*expand_string(char *str, t_shell *shell);
void	expand_list(t_shell *shell);
char	*get_wildcard_matches(char *pattern);
void    add_back(t_token **list, t_token *new);
void    free_tokens(t_token *list);
void	expand_cmd_args(t_cmd *cmd, t_shell *sh);

/* --- Parser --- */
t_ast	*new_ast_node(t_node_type type);
void	free_ast(t_ast *node);
t_ast	*parse_logic(t_token **tokens, t_shell *sh);
t_cmd	*parse_pipeline_tokens(t_token **tokens, t_shell *shell);
int		is_cmd_end(t_token_type type);
t_cmd	*new_cmd(void);
int		count_args(t_token *tokens);
void	fill_args(t_cmd *cmd, t_token *tokens);
void	free_cmds(t_cmd *head);

/* --- Executor --- */
void	execute_ast(t_ast *node, t_shell *sh);
void	execute_commands(t_cmd *cmd, t_shell *sh);
char	**env_to_array(t_env *env);
char	*get_cmd_path(char *cmd, t_env *env);
void	handle_exec_error(char *path, char *cmd_name);
void	handle_redirection(t_cmd *cmd, t_token **tokens, t_shell *shell);
int		read_heredoc(char *delim, t_shell *sh, int q);

/* --- Builtins --- */
int		is_builtin(char **args, t_shell *sh);
int		exe_builtin(char **args, t_shell *sh);
int		mini_pwd(void);
int		mini_cd(char **args, t_env **env);
int		mini_env(t_env *env);
int		mini_unset(char **args, t_env **env);
int		mini_export(char **args, t_env **env);
int		mini_echo(char **args);
int		mini_exit(char **args, t_shell *sh);
int 	mini_visual(char **args, t_shell *sh);
int 	mini_ls(char **args);
int		mini_clear(t_shell *sh);

/* --- Renderer --- */
void	draw_ast(t_ast *node, char *prefix, int is_left);

/* --- Tab Completion --- */
char    **easyshell_completion(const char *text, int start, int end);
char    *command_generator(const char *text, int state);
#endif