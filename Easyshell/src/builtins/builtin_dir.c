#include "easyshell.h"

static void	update_pwd(t_env **env)
{
	char	cwd[1024];
	char	*oldpwd;

	oldpwd = get_env_value("PWD", *env);
	if (oldpwd)
		set_env_value(env, "OLDPWD", oldpwd);
	if (getcwd(cwd, sizeof(cwd)))
		set_env_value(env, "PWD", cwd);
}

int	mini_cd(char **args, t_env **env)
{
	char	*path;

	/* POSIX: More than 1 target directory operand must trigger an error */
	if (args[1] && args[2])
	{
		ft_putendl_fd("easyshell: cd: too many arguments", 2);
		return (1);
	}
	/* POSIX: 'cd -' must navigate to $OLDPWD and print the path to stdout */
	if (args[1] && ft_strcmp(args[1], "-") == 0)
	{
		path = get_env_value("OLDPWD", *env);
		if (!path)
		{
			ft_putendl_fd("easyshell: cd: OLDPWD not set", 2);
			return (1);
		}
		if (chdir(path) != 0)
		{
			perror("easyshell: cd");
			return (1);
		}
		ft_putendl_fd(path, 1); /* Mandated by POSIX */
		update_pwd(env);
		return (0);
	}
	if (!args[1])
	{
		path = get_env_value("HOME", *env);
		if (!path)
		{
			ft_putendl_fd("easyshell: cd: HOME not set", 2);
			return (1);
		}
	}
	else
		path = args[1];
	if (chdir(path) != 0)
	{
		perror("easyshell: cd");
		return (1);
	}
	update_pwd(env);
	return (0);
}

int	mini_pwd(void)
{
	char	cwd[1024];

	if (getcwd(cwd, sizeof(cwd)))
	{
		ft_putendl_fd(cwd, 1);
		return (0);
	}
	return (1);
}