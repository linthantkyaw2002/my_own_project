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


char	*expand_string(char *str, t_shell *shell)
{
	t_expand	exp;
	char		*res;
	char		*tmp;

	if (!str)
		return (NULL);
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
		if (ft_strchr(cmd->args[i], '$'))
		{
			expanded = expand_string(cmd->args[i], sh);
			free(cmd->args[i]);
			/* Wrap the expanded result in Cyan color codes! */
			cmd->args[i] = ft_strjoin("\033[1;36m", expanded);
			char *tmp = ft_strjoin(cmd->args[i], "\033[0m");
			free(cmd->args[i]);
			free(expanded);
			cmd->args[i] = tmp;
		}
		i++;
	}
}