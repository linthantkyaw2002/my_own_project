#include "easyshell.h"

static int	allocate_and_fill(t_cmd *cmd, t_token *tokens)
{
	int	count;

	count = count_args(tokens);
	cmd->args = malloc(sizeof(char *) * (count + 1));
	if (!cmd->args)
		return (0);
	fill_args(cmd, tokens);
	return (1);
}

static void	advance_token(t_token **tokens, t_cmd *cmd, t_shell *shell)
{
	if (!*tokens)
		return ;
	if ((*tokens)->type >= TOKEN_REDIRECT_IN
		&& (*tokens)->type <= TOKEN_HEREDOC)
		handle_redirection(cmd, tokens, shell);
	if (*tokens)
		*tokens = (*tokens)->next;
}

t_cmd	*parse_pipeline_tokens(t_token **tokens, t_shell *shell)
{
	t_cmd	*head;
	t_cmd	*curr;

	head = new_cmd();
	if (!head)
		return (NULL);
	curr = head;
	while (*tokens && (*tokens)->type != TOKEN_AND 
		&& (*tokens)->type != TOKEN_OR && (*tokens)->type != TOKEN_RPAREN
		&& (*tokens)->type != TOKEN_BG)
	{
		if (!allocate_and_fill(curr, *tokens))
			return (free_cmds(head), NULL);
		while (*tokens && !is_cmd_end((*tokens)->type))
			advance_token(tokens, curr, shell);
		if (*tokens && (*tokens)->type == TOKEN_PIPE)
		{
			curr->next = new_cmd();
			if (!curr->next)
				return (free_cmds(head), NULL);
			curr = curr->next;
			*tokens = (*tokens)->next;
		}
	}
	return (head);
}
