#include "easyshell.h"

int g_signal_received = 0;

static void	init_shell(t_shell *shell, char **envp)
{
	shell->env = init_env(envp);
	increment_shlvl(&shell->env);
	shell->tokens = NULL;
	shell->last_exit = 0;
}

static void	handle_comments(char *input)
{
	int	i;
	int	in_single;
	int	in_double;

	i = 0;
	in_single = 0;
	in_double = 0;
	while (input && input[i])
	{
		is_quote(input[i], &in_single, &in_double);
		if (input[i] == '#' && !in_single && !in_double)
		{
			if (i == 0 || ft_isspace(input[i - 1]))
			{
				input[i] = '\0';
				break ;
			}
		}
		i++;
	}
}

static void	process_line(char *input, t_shell *shell)
{
	t_ast	*ast;
	t_token	*tmp;

	g_signal_received = 0;
	handle_comments(input);
	shell->tokens = tokenize(input);
	if (shell->tokens)
	{
		if (check_syntax(shell->tokens))
		{
			expand_list(shell);
			tmp = shell->tokens;
			ast = parse_logic(&tmp, shell);
			if (ast)
			{
				if (g_signal_received != SIGINT)
					execute_ast(ast, shell);
				free_ast(ast);
			}
		}
		else
			shell->last_exit = 2;
		free_tokens(shell->tokens);
		shell->tokens = NULL;
	}
}

static char	*get_input(void)
{
	char	*input;
	int		len;

	if (isatty(STDIN_FILENO))
		return (readline("minishell$ "));
	input = read_byte_by_byte();
	if (input)
	{
		len = ft_strlen(input);
		if (len > 0 && input[len - 1] == '\n')
			input[len - 1] = '\0';
	}
	return (input);
}

static void	shell_loop(t_shell *shell)
{
	char	*input;

	while (1)
	{
		input = get_input();
		if (!input)
		{
			if (isatty(STDIN_FILENO))
				write(STDOUT_FILENO, "exit\n", 5);
			break ;
		}
		if (isatty(STDIN_FILENO) && *input)
			add_history(input);
		process_line(input, shell);
		free(input);
	}
}

int	main(int argc, char **argv, char **envp)
{
	t_shell	shell;

	(void)argc;
	(void)argv;
	init_shell(&shell, envp);
	setup_signals();
	shell_loop(&shell);
	free_env(shell.env);
	rl_clear_history();
	return (shell.last_exit);
}