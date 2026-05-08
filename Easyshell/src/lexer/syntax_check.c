#include "easyshell.h"

static int	check_syntax_error(t_token *t, t_token *p)
{
	if (t->type == TOKEN_WORD && ft_strcmp(t->value, ";") == 0)
	{
		ft_putendl_fd("minishell: syntax error near unexpected token `;'", 2);
		return (0);
	}
	if ((t->type == TOKEN_PIPE || t->type == TOKEN_AND
			|| t->type == TOKEN_OR || t->type == TOKEN_BG)
		&& (!p || p->type == TOKEN_PIPE || p->type == TOKEN_AND
			|| p->type == TOKEN_OR || p->type == TOKEN_BG))
	{
		ft_putstr_fd("minishell: syntax error near unexpected token `", 2);
		ft_putstr_fd(t->value, 2);
		ft_putendl_fd("'", 2);
		return (0);
	}
	return (1);
}

static int	check_syntax_redirect(t_token *t)
{
	if ((t->type == TOKEN_PIPE || t->type == TOKEN_AND
			|| t->type == TOKEN_OR) && !t->next)
	{
		ft_putstr_fd("minishell: syntax error near unexpected token `newline'\n", 2);
		return (0);
	}
	if (t->type >= TOKEN_REDIRECT_IN && t->type <= TOKEN_HEREDOC
		&& (!t->next || t->next->type != TOKEN_WORD))
	{
		ft_putstr_fd("minishell: syntax error near unexpected token `", 2);
		if (!t->next)
			ft_putstr_fd("newline", 2);
		else
			ft_putstr_fd(t->next->value, 2);
		ft_putendl_fd("'", 2);
		return (0);
	}
	return (1);
}

int	check_syntax(t_token *t)
{
	t_token	*p;

	p = NULL;
	while (t)
	{
		if (!check_syntax_error(t, p) || !check_syntax_redirect(t))
			return (0);
		p = t;
		t = t->next;
	}
	return (1);
}