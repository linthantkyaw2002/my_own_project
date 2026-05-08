#include "easyshell.h"

static t_token	*create_token(char *value, t_token_type type)
{
	t_token	*new;

	new = malloc(sizeof(t_token));
	if (!new)
		return (NULL);
	new->value = value;
	new->type = type;
	new->next = NULL;
	return (new);
}

static int	get_operator(char *s, t_token **list)
{
	if (ft_strncmp(s, ">>", 2) == 0 || ft_strncmp(s, "<<", 2) == 0
		|| ft_strncmp(s, "&&", 2) == 0 || ft_strncmp(s, "||", 2) == 0)
	{
		if (ft_strncmp(s, ">>", 2) == 0)
			add_back(list, create_token(ft_strdup(">>"), TOKEN_APPEND));
		else if (ft_strncmp(s, "<<", 2) == 0)
			add_back(list, create_token(ft_strdup("<<"), TOKEN_HEREDOC));
		else if (ft_strncmp(s, "&&", 2) == 0)
			add_back(list, create_token(ft_strdup("&&"), TOKEN_AND));
		else
			add_back(list, create_token(ft_strdup("||"), TOKEN_OR));
		return (2);
	}
	if (*s == '>' || *s == '<' || *s == '|' || *s == '(' || *s == ')'
		|| *s == '&' || *s == ';')
	{
		if (*s == '>')
			add_back(list, create_token(ft_strdup(">"), TOKEN_REDIRECT_OUT));
		else if (*s == '<')
			add_back(list, create_token(ft_strdup("<"), TOKEN_REDIRECT_IN));
		else if (*s == '|')
			add_back(list, create_token(ft_strdup("|"), TOKEN_PIPE));
		else if (*s == '(')
			add_back(list, create_token(ft_strdup("("), TOKEN_LPAREN));
		else if (*s == ')')
			add_back(list, create_token(ft_strdup(")"), TOKEN_RPAREN));
		else if (*s == '&')
			add_back(list, create_token(ft_strdup("&"), TOKEN_BG));
		else if (*s == ';')
			add_back(list, create_token(ft_strdup(";"), TOKEN_WORD));
		return (1);
	}
	return (0);
}

static int	get_word(char *s, t_token **list)
{
	int		i;
	char	quote;

	i = 0;
	/* UPDATED: Added &() to ft_strchr so words stop at logic operators */
	while (s[i] && !ft_isspace(s[i]) && !ft_strchr("|&<>()", s[i]))
	{
		if (s[i] == '\'' || s[i] == '\"')
		{
			quote = s[i++];
			while (s[i] && s[i] != quote)
				i++;
			if (!s[i])
			{
				ft_putendl_fd("minishell: syntax error: unclosed quote", 2);
				free_tokens(*list);
				*list = NULL;
				return (-1);
			}
			i++;
		}
		else
			i++;
	}
	add_back(list, create_token(ft_substr(s, 0, i), TOKEN_WORD));
	return (i);
}

t_token	*tokenize(char *input)
{
	t_token	*list;
	int		i;
	int		len;

	list = NULL;
	i = 0;
	while (input[i])
	{
		while (input[i] && ft_isspace(input[i]))
			i++;
		if (!input[i])
			break ;
		/* UPDATED: Added &() so the lexer routes to get_operator correctly */
		if (ft_strchr("|&<>()", input[i]))
			len = get_operator(&input[i], &list);
		else
			len = get_word(&input[i], &list);
		if (len == -1)
			return (NULL);
		i += len;
	}
	return (list);
}

void	add_back(t_token **list, t_token *new)
{
	t_token	*tmp;

	if (!list || !new)
		return ;
	if (!*list)
	{
		*list = new;
		return ;
	}
	tmp = *list;
	while (tmp->next)
		tmp = tmp->next;
	tmp->next = new;
}

void	free_tokens(t_token *list)
{
	t_token	*tmp;

	while (list)
	{
		tmp = list->next;
		if (list->value)
			free(list->value);
		free(list);
		list = tmp;
	}
}