#include "easyshell.h"

static char	*get_heredoc_line(void)
{
	char	*line;
	int		len;

	if (isatty(STDIN_FILENO))
		return (readline("> "));
	line = read_byte_by_byte();
	if (line)
	{
		len = ft_strlen(line);
		if (len > 0 && line[len - 1] == '\n')
			line[len - 1] = '\0';
	}
	return (line);
}

static char	*handle_expansion(char *res, char *line, int *i, t_shell *shell)
{
	char	*var;
	char	*val;

	(*i)++;
	if (line[*i] == '?')
	{
		val = ft_itoa(shell->last_exit);
		res = append_str(res, val);
		free(val);
		(*i)++;
	}
	else
	{
		var = get_var_name(line, i);
		val = get_env_value(var, shell->env);
		free(var);
		if (val)
			res = append_str(res, val);
	}
	return (res);
}

static char	*expand_heredoc_line(char *line, t_shell *shell)
{
	char	*res;
	int		i;

	res = ft_strdup("");
	i = 0;
	while (line[i])
	{
		if (line[i] == '$' && line[i + 1]
			&& (ft_isalnum(line[i + 1]) || line[i + 1] == '_'
				|| line[i + 1] == '?'))
			res = handle_expansion(res, line, &i, shell);
		else
			res = append_char(res, line[i++]);
	}
	return (res);
}

static void	heredoc_child(char *delim, t_shell *sh, int fd, int quoted)
{
	char	*line;
	char	*expanded;

	signal(SIGINT, SIG_DFL);
	while (1)
	{
		line = get_heredoc_line();
		if (!line)
		{
			/* ADDED "at line 1 " to match bash exactly */
			ft_putstr_fd("minishell: warning: here-document at line 1 delimited by end-of-file (wanted `", 2);
			ft_putstr_fd(delim, 2);
			ft_putendl_fd("')", 2);
			break ;
		}
		if (ft_strcmp(line, delim) == 0)
		{
			free(line);
			break ;
		}
		if (quoted)
			expanded = ft_strdup(line);
		else
			expanded = expand_heredoc_line(line, sh);
		ft_putendl_fd(expanded, fd);
		free(expanded);
		free(line);
	}
	close(fd);
	exit(0);
}

int	read_heredoc(char *delim, t_shell *sh, int q)
{
	int			fd;
	int			st;
	char		*nm;
	char		*num;
	static int	i = 0;

	num = ft_itoa(i++);
	nm = ft_strjoin(".heredoc_tmp_", num);
	free(num);
	fd = open(nm, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd < 0)
		return (perror("minishell: heredoc"), free(nm), -1);
	signal(SIGINT, SIG_IGN);
	if (fork() == 0)
		heredoc_child(delim, sh, fd, q);
	wait(&st);
	setup_signals();
	close(fd);
	if (WIFSIGNALED(st) && WTERMSIG(st) == SIGINT)
		g_signal_received = SIGINT;
	if (g_signal_received == SIGINT && write(1, "\n", 1))
		sh->last_exit = 130;
	else
		fd = open(nm, O_RDONLY);
	unlink(nm);
	free(nm);
	return (fd);
}