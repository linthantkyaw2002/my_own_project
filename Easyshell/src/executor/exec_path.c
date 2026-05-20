#include "easyshell.h"

static int	env_list_size(t_env *env)
{
	int	i;

	i = 0;
	while (env)
	{
		i++;
		env = env->next;
	}
	return (i);
}

static void	free_split(char **split)
{
	int	i;

	i = 0;
	if (!split)
		return ;
	while (split[i])
		free(split[i++]);
	free(split);
}

static char	*find_valid_path(char **paths, char *cmd)
{
	char	*part;
	char	*full;
	int		i;

	i = -1;
	while (paths && paths[++i])
	{
		part = ft_strjoin(paths[i], "/");
		full = ft_strjoin(part, cmd);
		free(part);
		if (access(full, F_OK | X_OK) == 0)
			return (full);
		free(full);
	}
	return (NULL);
}

static void	print_error_exit(char *cmd, char *msg, int code)
{
	ft_putstr_fd("minishell: ", 2);
	ft_putstr_fd(cmd, 2);
	ft_putstr_fd(": ", 2);
	ft_putendl_fd(msg, 2);
	exit(code);
}
	
char	**env_to_array(t_env *env)
{
	char	**arr;
	int		i;
	char	*tmp;

	arr = malloc(sizeof(char *) * (env_list_size(env) + 1));
	if (!arr)
		return (NULL);
	i = 0;
	while (env)
	{
		if (env->value)
		{
			tmp = ft_strjoin(env->key, "=");
			arr[i] = ft_strjoin(tmp, env->value);
			free(tmp);
		}
		else
			arr[i] = ft_strdup(env->key);
		i++;
		env = env->next;
	}
	arr[i] = NULL;
	return (arr);
}

char	*get_cmd_path(char *cmd, t_env *env)
{
	char	**paths;
	char	*part;
	char	*full;

	if (!cmd || cmd[0] == '\0')
		return (NULL);
	if (ft_strchr(cmd, '/'))
		return (ft_strdup(cmd));
	part = get_env_value("PATH", env);
	if (!part)
		return (NULL);
	paths = ft_split(part, ':');
	full = find_valid_path(paths, cmd);
	free_split(paths);
	return (full);
}

void	handle_exec_error(char *path, char *cmd_name)
{
	struct stat	path_stat;

	if (!path)
		print_error_exit(cmd_name, "command not found", 127);
	if (ft_strchr(cmd_name, '/') && access(path, F_OK) != 0)
		print_error_exit(cmd_name, "No such file or directory", 127);
	stat(path, &path_stat);
	if (S_ISDIR(path_stat.st_mode))
		print_error_exit(cmd_name, "is a directory", 126);
	if (access(path, X_OK) != 0)
		print_error_exit(cmd_name, "Permission denied", 126);
	if (errno == ENOEXEC)
	{
		if (ft_strchr(cmd_name, '/'))
			exit(0);
		print_error_exit(cmd_name, "No such file or directory", 2);
	}
	if (errno == ENOENT)
		print_error_exit(cmd_name, "No such file or directory", 127);
	print_error_exit(cmd_name, "command not found", 127);
}
