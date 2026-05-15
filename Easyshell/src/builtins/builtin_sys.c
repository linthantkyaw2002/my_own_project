#include "easyshell.h"

static int	is_n_flag(char *str)
{
	int	i;

	if (!str || str[0] != '-' || str[1] != 'n')
		return (0);
	i = 1;
	while (str[i] == 'n')
		i++;
	if (str[i] != '\0')
		return (0);
	return (1);
}

static int	is_numeric(char *str)
{
	int	i;

	i = 0;
	if (str[i] == '-' || str[i] == '+')
		i++;
	if (!str[i])
		return (0);
	while (str[i])
	{
		if (!ft_isdigit(str[i]))
			return (0);
		i++;
	}
	return (1);
}

static void	clean_and_exit(t_shell *sh, int status)
{
	free_env(sh->env);
	rl_clear_history();
	exit(status);
}

int	mini_exit(char **args, t_shell *sh)
{
	int	status;

	if (isatty(STDIN_FILENO))
		ft_putendl_fd("exit", STDOUT_FILENO);
	if (!args[1])
		clean_and_exit(sh, sh->last_exit);
	if (!is_numeric(args[1]))
	{
		ft_putendl_fd("minishell: exit: numeric argument required", 2);
		clean_and_exit(sh, 2);
	}
	if (args[2])
	{
		ft_putendl_fd("minishell: exit: too many arguments", 2);
		return (1);
	}
	status = ft_atoi(args[1]);
	clean_and_exit(sh, status % 256);
	return (0);
}

int	mini_echo(char **args)
{
	int	i;
	int	n_flag;

	i = 1;
	n_flag = 0;
	while (args[i] && is_n_flag(args[i]))
	{
		n_flag = 1;
		i++;
	}
	while (args[i])
	{
		ft_putstr_fd(args[i], 1);
		if (args[i + 1])
			write(1, " ", 1);
		i++;
	}
	if (!n_flag)
		write(1, "\n", 1);
	return (0);
}

int	mini_clear(t_shell *sh)
{
	printf("\033[H\033[2J\033[3J");
	sh->skip_visual = 1;
	return (0);
}