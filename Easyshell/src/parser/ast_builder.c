#include "easyshell.h"

static t_ast	*parse_term(t_token **tokens, t_shell *sh)
{
	t_ast	*node;

	if (!*tokens)
		return (NULL);
	if ((*tokens)->type == TOKEN_LPAREN)
	{
		*tokens = (*tokens)->next;
		node = new_ast_node(NODE_SUBSHELL);
		node->left = parse_logic(tokens, sh);
		if (*tokens && (*tokens)->type == TOKEN_RPAREN)
			*tokens = (*tokens)->next;
		return (node);
	}
	node = new_ast_node(NODE_PIPELINE);
	node->pipeline = parse_pipeline_tokens(tokens, sh);
	return (node);
}

t_ast	*new_ast_node(t_node_type type)
{
	t_ast	*node;

	node = malloc(sizeof(t_ast));
	if (!node)
		return (NULL);
	node->type = type;
	node->left = NULL;
	node->right = NULL;
	node->pipeline = NULL;
	return (node);
}

void	free_ast(t_ast *node)
{
	if (!node)
		return ;
	free_ast(node->left);
	free_ast(node->right);
	if (node->pipeline)
		free_cmds(node->pipeline);
	free(node);
}

t_ast	*parse_logic(t_token **tokens, t_shell *sh)
{
	t_ast	*left;
	t_ast	*node;
	int		type;

	left = parse_term(tokens, sh);
	while (*tokens && ((*tokens)->type == TOKEN_AND 
		|| (*tokens)->type == TOKEN_OR || (*tokens)->type == TOKEN_BG))
	{
		type = NODE_AND;
		if ((*tokens)->type == TOKEN_OR)
			type = NODE_OR;
		else if ((*tokens)->type == TOKEN_BG)
			type = NODE_BG;
		node = new_ast_node(type);
		*tokens = (*tokens)->next;
		node->left = left;
		if (type == NODE_BG && (!*tokens || (*tokens)->type == TOKEN_RPAREN))
			node->right = NULL;
		else
			node->right = parse_term(tokens, sh);
		left = node;
	}
	return (left);
}
