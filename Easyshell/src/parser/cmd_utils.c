#include "easyshell.h"

int	is_cmd_end(t_token_type type)
{
	return (type == TOKEN_PIPE || type == TOKEN_AND
		|| type == TOKEN_OR || type == TOKEN_RPAREN || type == TOKEN_BG);
}

t_cmd	*new_cmd(void)
{
	t_cmd	*cmd;

	cmd = malloc(sizeof(t_cmd));
	if (!cmd)
		return (NULL);
	cmd->args = NULL;
	cmd->infile = -2;
	cmd->outfile = -2;
	cmd->error = 0;
	cmd->pid = -1;         /* Initialize to -1 */
	cmd->exit_status = 0;  /* Initialize to 0 (Success) */
	cmd->next = NULL;
	return (cmd);
}

int	count_args(t_token *tokens)
{
	int	count;

	count = 0;
	while (tokens && !is_cmd_end(tokens->type))
	{
		if (tokens->type >= TOKEN_REDIRECT_IN && tokens->type <= TOKEN_HEREDOC)
		{
			tokens = tokens->next;
			if (tokens)
				tokens = tokens->next;
			continue ;
		}
		if (tokens->type == TOKEN_WORD)
			count++;
		tokens = tokens->next;
	}
	return (count);
}

void	fill_args(t_cmd *cmd, t_token *tokens)
{
	int	i;

	i = 0;
	while (tokens && !is_cmd_end(tokens->type))
	{
		if (tokens->type >= TOKEN_REDIRECT_IN && tokens->type <= TOKEN_HEREDOC)
		{
			tokens = tokens->next;
			if (tokens)
				tokens = tokens->next;
			continue ;
		}
		if (tokens->type == TOKEN_WORD)
			cmd->args[i++] = ft_strdup(tokens->value);
		tokens = tokens->next;
	}
	cmd->args[i] = NULL;
}

void	free_cmds(t_cmd *head)
{
	t_cmd	*tmp;
	int		idx;

	while (head)
	{
		tmp = head->next;
		if (head->args)
		{
			idx = 0;
			while (head->args[idx])
				free(head->args[idx++]);
			free(head->args);
		}
		/* NEW: Close the file descriptors so the parent shell doesn't leak them! */
		if (head->infile >= 0)
			close(head->infile);
		if (head->outfile >= 0)
			close(head->outfile);
		free(head);
		head = tmp;
	}
}
