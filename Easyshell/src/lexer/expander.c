#include "easyshell.h"

static char	*handle_exit(char *res, int *i, int last_exit)
{
	char	*val;
	char	*tmp;

	val = ft_itoa(last_exit);
	if (!val)
		return (NULL);
	tmp = append_str(res, val);
	free(val);
	if (!tmp)
		return (NULL);
	(*i)++;
	return (tmp);
}

static char	*handle_dollar(char *res, t_expand *exp)
{
	char	*var;
	char	*val;

	if (!exp->str[exp->i + 1] || (!ft_isalpha(exp->str[exp->i + 1])
			&& exp->str[exp->i + 1] != '_' && exp->str[exp->i + 1] != '?'))
	{
		exp->i++;
		return (append_char(res, '$'));
	}
	exp->i++;
	if (exp->str[exp->i] == '?')
		return (handle_exit(res, &exp->i, exp->shell->last_exit));
	var = get_var_name(exp->str, &exp->i);
	if (!var)
		return (NULL);
	val = get_env_value(var, exp->shell->env);
	free(var);
	if (!val)
		val = "";
	return (append_str(res, val));
}

static char	*process_expand_char(char *res, t_expand *exp)
{
	if (is_quote(exp->str[exp->i], &exp->in_single, &exp->in_double))
	{
		exp->i++;
		return (res);
	}
	if (exp->str[exp->i] == '$' && !exp->in_single)
		return (handle_dollar(res, exp));
	return (append_char(res, exp->str[exp->i++]));
}

/*
** expand_tilde: POSIX tilde expansion.
**
** Rules implemented:
**   ~          → $HOME
**   ~/foo      → $HOME/foo
**   ~word      → treated literally (user tilde not supported, matches bash)
**
** Expansion only happens when:
**   - the token starts with '~'
**   - the '~' is NOT inside any quotes (checked by the caller via has_quotes
**     on the leading character — if the whole token has no quotes we expand,
**     matching the POSIX requirement that quoted tildes are not expanded)
**
** Returns a newly malloc'd string; caller must free.
** Returns NULL only on allocation failure.
*/
static char	*expand_tilde(char *str, t_shell *shell)
{
	char	*home;
	char	*rest;
	char	*tmp;
	char	*result;

	if (str[0] != '~')
		return (ft_strdup(str));
	/* Only expand bare ~ or ~/... — not ~username */
	if (str[1] != '\0' && str[1] != '/')
		return (ft_strdup(str));
	home = get_env_value("HOME", shell->env);
	if (!home)
		return (ft_strdup(str));   /* HOME unset: leave literal, per POSIX */
	rest = str + 1;                /* skip the '~'; rest is "" or "/foo" */
	tmp = ft_strdup(home);
	if (!tmp)
		return (NULL);
	result = append_str(tmp, rest);
	return (result);
}

char	*expand_string(char *str, t_shell *shell)
{
	t_expand	exp;
	char		*res;
	char		*tmp;

	if (!str)
		return (NULL);
	/* Tilde expansion: only when unquoted and leading char is ~ */
	if (str[0] == '~' && !has_quotes(str))
	{
		tmp = expand_tilde(str, shell);
		if (!tmp)
			return (NULL);
		/* Now run dollar expansion on the tilde-expanded result */
		res = expand_string(tmp, shell);
		free(tmp);
		return (res);
	}
	exp.str = str;
	exp.i = 0;
	exp.in_single = 0;
	exp.in_double = 0;
	exp.shell = shell;
	res = ft_strdup("");
	if (!res)
		return (NULL);
	while (exp.str[exp.i])
	{
		tmp = process_expand_char(res, &exp);
		if (!tmp)
		{
			return (NULL);
		}
		res = tmp;
	}
	return (res);
}

/* Bridge function to expand the final command arguments right before exec */
void	expand_cmd_args(t_cmd *cmd, t_shell *sh)
{
	int		i;
	char	*expanded;

	i = 0;
	while (cmd->args && cmd->args[i])
	{
		if (ft_strchr(cmd->args[i], '$') || cmd->args[i][0] == '~')
		{
			expanded = expand_string(cmd->args[i], sh);
			free(cmd->args[i]);
			cmd->args[i] = expanded;
		}
		i++;
	}
}
