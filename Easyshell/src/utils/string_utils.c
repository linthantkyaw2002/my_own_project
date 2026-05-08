#include "easyshell.h"

int	is_quote(char c, int *in_single, int *in_double)
{
	if (c == '\'' && !(*in_double))
	{
		*in_single = !(*in_single);
		return (1);
	}
	if (c == '"' && !(*in_single))
	{
		*in_double = !(*in_double);
		return (1);
	}
	return (0);
}

char	*append_char(char *res, char c)
{
	char	*new;
	int		len;

	len = 0;
	if (res)
		len = ft_strlen(res);
	new = malloc(len + 2);
	if (!new)
	{
		free(res);
		return (NULL);
	}
	if (res)
		ft_memcpy(new, res, len);
	new[len] = c;
	new[len + 1] = '\0';
	free(res);
	return (new);
}

char	*append_str(char *res, const char *add)
{
	char	*new;
	int		len1;
	int		len2;

	len1 = 0;
	if (res)
		len1 = ft_strlen(res);
	len2 = 0;
	if (add)
		len2 = ft_strlen(add);
	new = malloc(len1 + len2 + 1);
	if (!new)
	{
		free(res);
		return (NULL);
	}
	if (res)
		ft_memcpy(new, res, len1);
	if (add)
		ft_memcpy(new + len1, add, len2);
	new[len1 + len2] = '\0';
	free(res);
	return (new);
}

char	*strip_quotes(char *str)
{
	char	*res;
	int		i;
	int		j;

	if (!str)
		return (NULL);
	res = malloc(sizeof(char) * (ft_strlen(str) + 1));
	if (!res)
		return (NULL);
	i = 0;
	j = 0;
	while (str[i])
	{
		if (str[i] != '\'' && str[i] != '\"')
			res[j++] = str[i];
		i++;
	}
	res[j] = '\0';
	return (res);
}

int	ft_isspace(char c)
{
	return (c == ' ' || c == '\t' || c == '\n'
		|| c == '\v' || c == '\f' || c == '\r');
}

int	ft_strcmp(const char *s1, const char *s2)
{
	while (*s1 && (*s1 == *s2))
	{
		s1++;
		s2++;
	}
	return ((unsigned char)*s1 - (unsigned char)*s2);
}

int	has_quotes(char *str)
{
	int	i;

	i = 0;
	if (!str)
		return (0);
	while (str[i])
	{
		if (str[i] == '\'' || str[i] == '\"')
			return (1);
		i++;
	}
	return (0);
}