#include "easyshell.h"

int	match_pattern(char *pattern, char *str)
{
	if (*pattern == '\0')
		return (*str == '\0');
	if (*pattern == '*')
	{
		if (*(pattern + 1) == '\0')
			return (1);
		if (*str != '\0' && match_pattern(pattern, str + 1))
			return (1);
		return (match_pattern(pattern + 1, str));
	}
	if (*pattern == *str)
		return (match_pattern(pattern + 1, str + 1));
	return (0);
}

/* Reads current directory and returns a space-separated string of matches */
char	*get_wildcard_matches(char *pattern)
{
	DIR				*dir;
	struct dirent	*entry;
	char			*res;
	char			*tmp;

	res = ft_strdup("");
	dir = opendir(".");
	if (!dir || !res)
		return (NULL);
	entry = readdir(dir);
	while (entry)
	{
		if (entry->d_name[0] != '.' && match_pattern(pattern, entry->d_name))
		{
			tmp = append_str(res, entry->d_name);
			res = append_str(tmp, " ");
		}
		entry = readdir(dir);
	}
	closedir(dir);
	if (res[0] == '\0')
		return (free(res), ft_strdup(pattern));
	return (res);
}
