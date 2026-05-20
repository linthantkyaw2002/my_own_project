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

static char *expand_tilde(char *arg, t_shell *sh)
{
    char *home;
    char *expanded;

    // Trigger only if unquoted word starts with '~' and is standalone or followed by '/'
    if (arg && arg[0] == '~' && (arg[1] == '\0' || arg[1] == '/'))
    {
        // Fetch HOME from your environment structure map
        home = get_env_value("HOME", sh->env); 
        if (!home)
            return (strdup(arg)); // Fallback to literal if HOME is completely missing
        
        if (arg[1] == '\0')
            return (strdup(home));
            
        // Handle path combinations like ~/src
        expanded = ft_strjoin(home, arg + 1);
        return (expanded);
    }
    return (strdup(arg));
}

/* Bridge function to expand the final command arguments right before exec */
void    expand_cmd_args(t_cmd *cmd, t_shell *sh)
{
    int     i;
    int     j;
    char    *expanded;
    int     had_quotes;
    char    *tilde_resolved;
    char    **new_args;

    if (!cmd || !cmd->args)
        return ;
    
    i = 0;
    while (cmd->args[i])
        i++;
    new_args = malloc(sizeof(char *) * (i + 1));
    if (!new_args)
        return ;

    i = 0;
    j = 0;
    while (cmd->args[i])
    {
        /* 1. Track quote properties on raw user input tokens */
        had_quotes = has_quotes(cmd->args[i]);
        
        /* 2. Run POSIX Tilde expansion pass cleanly first */
        tilde_resolved = expand_tilde(cmd->args[i], sh);
        
        /* 3. FIXED: Feed 'tilde_resolved' directly into expand_string 
        ** to preserve the resolved $HOME path properties! */
        expanded = expand_string(tilde_resolved, sh);
        
        /* Free raw parameter entries and our temporary tilde buffer */
        free(cmd->args[i]);
        free(tilde_resolved);
        
        /* 4. FIXED: Safe compact dropout block without leaking paths */
        if (expanded && expanded[0] == '\0' && !had_quotes)
        {
            free(expanded);
            i++;
            continue ;
        }
        
        new_args[j++] = expanded;
        i++;
    }
    new_args[j] = NULL;
    
    free(cmd->args);
    cmd->args = new_args;
}