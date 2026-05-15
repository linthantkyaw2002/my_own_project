#include "easyshell.h"

/* Helper to print the command arguments for Pipeline nodes */
static void	print_cmd_args(t_cmd *cmd)
{
	int	i;

	while (cmd)
	{
		i = 0;
		/* 1. Draw OPENING Bracket */
		if (cmd->exit_status == -1)
			printf("\033[1;30m[ \033[0m");
		else if (cmd->exit_status != 0)
			printf("\033[1;31m![ \033[0m");
		else
			printf("\033[1;32m[ \033[0m");

		/* 2. Draw Command Arguments */
		while (cmd->args && cmd->args[i])
		{
			if (cmd->exit_status == -1)
				printf("\033[1;30m%s \033[0m", cmd->args[i]);
			else if (cmd->exit_status != 0)
				printf("\033[1;31m%s \033[0m", cmd->args[i]);
			else
				printf("%s ", cmd->args[i]);
			i++;
		}

		/* --- NEW REDIRECTION BLOCK STARTS HERE --- */
		if (cmd->infile >= 0 || cmd->outfile >= 0)
		{
			printf(" \033[1;33m|"); /* Yellow separator */
			if (cmd->infile >= 0)
				printf(" <in");
			if (cmd->outfile >= 0)
				printf(" >out");
			printf("\033[0m");
		}
		/* --- NEW REDIRECTION BLOCK ENDS HERE --- */

		/* 3. Draw CLOSING Bracket */
		if (cmd->exit_status == -1)
			printf("\033[1;30m]\033[0m");
		else if (cmd->exit_status != 0)
			printf("\033[1;31m]\033[0m");
		else
			printf("\033[1;32m]\033[0m");
		
		/* 4. Draw Pipeline Arrows */
		if (cmd->next)
		{
			if (cmd->exit_status == -1)
				printf(" \033[1;30m->\033[0m "); 
			else if (cmd->exit_status != 0)
				printf(" \033[1;31m-!>\033[0m "); 
			else
				printf(" \033[1;35m->\033[0m ");
		}
		cmd = cmd->next;
	}
	printf("\n");
}

/* Recursive function to draw the tree */
void	draw_ast(t_ast *node, char *prefix, int is_left)
{
	char	*new_prefix;
	char	*branch;

	if (!node)
		return ;
	printf("%s", prefix);
	if (is_left)
		printf("├── ");
	else
		printf("└── ");

	if (node->type == NODE_PIPELINE)
		print_cmd_args(node->pipeline);
	else if (node->type == NODE_AND)
		printf("\033[1;33mAND (&&)\033[0m\n");
	else if (node->type == NODE_OR)
		printf("\033[1;33mOR (||)\033[0m\n");
	else if (node->type == NODE_BG)
		printf("\033[1;36mBACKGROUND (&)\033[0m\n");
	else if (node->type == NODE_SUBSHELL)
		printf("\033[1;35mSUBSHELL ( )\033[0m\n");

	/* Create the prefix for the next level down */
	branch = is_left ? "│   " : "    ";
	new_prefix = ft_strjoin(prefix, branch);

	/* Recurse left and right */
	draw_ast(node->left, new_prefix, 1);
	draw_ast(node->right, new_prefix, 0);
	free(new_prefix);
}