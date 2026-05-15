#include "easyshell.h"

int	is_builtin(char **args, t_shell *sh)
{
	if (!args || !args[0])
		return (0);
	if (ft_strcmp(args[0], "echo") == 0 || ft_strcmp(args[0], "cd") == 0
		|| ft_strcmp(args[0], "pwd") == 0 || ft_strcmp(args[0], "export") == 0
		|| ft_strcmp(args[0], "unset") == 0 || ft_strcmp(args[0], "env") == 0
		|| ft_strcmp(args[0], "exit") == 0 || ft_strcmp(args[0], ".") == 0
		|| ft_strcmp(args[0], "visual") == 0 || ft_strcmp(args[0], "clear") == 0)
		return (1);
	if (ft_strcmp(args[0], "ls") == 0 && sh->visual_mode == 1)
		return (1);
	return (0);
}

int	exe_builtin(char **args, t_shell *sh)
{
	if (ft_strcmp(args[0], "echo") == 0)
		return (mini_echo(args));
	else if (ft_strcmp(args[0], "cd") == 0)
		return (mini_cd(args, &sh->env));
	else if (ft_strcmp(args[0], "pwd") == 0)
		return (mini_pwd());
	else if (ft_strcmp(args[0], "export") == 0)
		return (mini_export(args, &sh->env));
	else if (ft_strcmp(args[0], "unset") == 0)
		return (mini_unset(args, &sh->env));
	else if (ft_strcmp(args[0], "env") == 0)
		return (mini_env(sh->env));
	else if (ft_strcmp(args[0], "exit") == 0)
		return (mini_exit(args, sh));
	else if (ft_strcmp(args[0], "visual") == 0)
		return (mini_visual(args, sh));
	else if (ft_strcmp(args[0], "clear") == 0)
		return (mini_clear(sh));
	else if (ft_strcmp(args[0], "ls") == 0)
		return (mini_ls(args)); /* Routes to our new renderer */
	else if (ft_strcmp(args[0], ".") == 0)
	{
		ft_putendl_fd("easyshell: .: filename argument required", 2);
		return (2);
	}
	return (0);
}