#include "easyshell.h"
#include "../includes/colors.h"

/* The new toggle command */
int	mini_visual(char **args, t_shell *sh)
{
	if (!args[1])
	{
		printf("Visual mode is %s\n", sh->visual_mode ? "ON" : "OFF");
		return (0);
	}
	if (ft_strcmp(args[1], "on") == 0)
	{
		sh->visual_mode = 1;
		printf("🎨 Visual Engine: %sON%s\n", BOLD, RESET);
	}
	else if (ft_strcmp(args[1], "off") == 0)
	{
		sh->visual_mode = 0;
		printf("🎨 Visual Engine: OFF\n");
	}
	else
	{
		ft_putendl_fd("easyshell: visual: invalid argument (use 'on' or 'off')", 2);
		return (1);
	}
	return (0);
}

/* Recursive helper to draw directories */
static void	list_dir(char *target, int show_all, int recursive)
{
	DIR				*dir;
	struct dirent	*entry;
	struct stat		file_stat;
	char			*full_path;
	char			*tmp;

	dir = opendir(target);
	if (!dir)
		return (perror("easyshell: ls"));
	
	if (recursive)
		printf("\n\033[1;35m%s:\033[0m\n", target); /* Purple header for sub-dirs */

	/* Pass 1: Print current directory */
	while ((entry = readdir(dir)) != NULL)
	{
		if (!show_all && entry->d_name[0] == '.')
			continue ;
		tmp = ft_strjoin(target, "/");
		full_path = ft_strjoin(tmp, entry->d_name);
		free(tmp);
		if (stat(full_path, &file_stat) == 0)
		{
			if (S_ISDIR(file_stat.st_mode))
				printf("📁 %s%s%s  ", C_DIR, entry->d_name, RESET);
			else if (file_stat.st_mode & S_IXUSR)
				printf("⚙️ %s%s%s  ", C_EXEC, entry->d_name, RESET);
			else
				printf("📄 %s%s%s  ", C_FILE, entry->d_name, RESET);
		}
		free(full_path);
	}
	printf("\n");

	/* Pass 2: If recursive, dive into subdirectories */
	if (recursive)
	{
		rewinddir(dir); /* Reset folder reader to the top */
		while ((entry = readdir(dir)) != NULL)
		{
			/* CRITICAL: Skip . and .. to avoid infinite loops! */
			if (ft_strcmp(entry->d_name, ".") == 0 || ft_strcmp(entry->d_name, "..") == 0)
				continue ;
			if (!show_all && entry->d_name[0] == '.')
				continue ;

			tmp = ft_strjoin(target, "/");
			full_path = ft_strjoin(tmp, entry->d_name);
			free(tmp);
			
			if (stat(full_path, &file_stat) == 0 && S_ISDIR(file_stat.st_mode))
				list_dir(full_path, show_all, recursive); /* Recurse! */
			free(full_path);
		}
	}
	closedir(dir);
}

/* The smart ls renderer */
int	mini_ls(char **args)
{
	char	*target = ".";
	int		show_all = 0;
	int		recursive = 0;
	int		i = 1;
	int		j;

	/* Parse arguments dynamically so it handles -R, -a, or -aR */
	while (args[i])
	{
		if (args[i][0] == '-')
		{
			j = 1;
			while (args[i][j])
			{
				if (args[i][j] == 'a') show_all = 1;
				if (args[i][j] == 'R') recursive = 1;
				j++;
			}
		}
		else
			target = args[i];
		i++;
	}
	list_dir(target, show_all, recursive);
	return (0);
}