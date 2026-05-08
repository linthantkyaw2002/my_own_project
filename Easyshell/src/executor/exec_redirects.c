#include "easyshell.h"

static void	handle_file(t_cmd *cmd, t_token *op, char *file)
{
	if (op->type == TOKEN_REDIRECT_IN)
	{
		if (cmd->infile >= 0)
			close(cmd->infile);
		cmd->infile = open(file, O_RDONLY);
		if (cmd->infile == -1)
			cmd->error = 1;
	}
	else
	{
		if (cmd->outfile >= 0)
			close(cmd->outfile);
		if (op->type == TOKEN_REDIRECT_OUT)
			cmd->outfile = open(file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
		else
			cmd->outfile = open(file, O_CREAT | O_WRONLY | O_APPEND, 0644);
		if (cmd->outfile == -1)
			cmd->error = 1;
	}
	if (cmd->error)
		perror(file);
}

static void	dispatch_redir(t_cmd *cmd, t_token *op, char *clean, t_shell *sh)
{
	if (op->type == TOKEN_HEREDOC)
	{
		cmd->infile = read_heredoc(clean, sh, has_quotes(op->next->value));
		if (cmd->infile == -1 && g_signal_received == SIGINT)
			cmd->error = 1;
	}
	else
		handle_file(cmd, op, clean);
}

void	handle_redirection(t_cmd *cmd, t_token **tokens, t_shell *shell)
{
	t_token	*op;
	char	*clean;

	op = *tokens;
	if (!op || !op->next)
		return ;
	if (op->next->type == TOKEN_AMBIGUOUS)
	{
		ft_putstr_fd("minishell: ", 2);
		ft_putstr_fd(op->next->value, 2);
		ft_putendl_fd(": ambiguous redirect", 2);
		cmd->error = 1;
	}
	clean = strip_quotes(op->next->value);
	if (!cmd->error)
		dispatch_redir(cmd, op, clean, shell);
	free(clean);
	*tokens = op->next;
}
