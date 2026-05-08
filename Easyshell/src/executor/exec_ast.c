#include "easyshell.h"

static void	exec_subshell(t_ast *node, t_shell *sh)
{
	pid_t	pid;
	int		status;

	pid = fork();
	if (pid == 0)
	{
		execute_ast(node->left, sh);
		exit(sh->last_exit);
	}
	waitpid(pid, &status, 0);
	if (WIFEXITED(status))
		sh->last_exit = WEXITSTATUS(status);
	else if (WIFSIGNALED(status))
		sh->last_exit = 128 + WTERMSIG(status);
}

void	execute_ast(t_ast *node, t_shell *sh)
{
	pid_t	pid;

	if (!node)
		return ;
	if (node->type == NODE_PIPELINE)
		execute_commands(node->pipeline, sh);
	else if (node->type == NODE_AND)
	{
		execute_ast(node->left, sh);
		if (sh->last_exit == 0)
			execute_ast(node->right, sh);
	}
	else if (node->type == NODE_OR)
	{
		execute_ast(node->left, sh);
		if (sh->last_exit != 0)
			execute_ast(node->right, sh);
	}
	else if (node->type == NODE_BG)
	{
		pid = fork();
		if (pid == 0)
		{
			execute_ast(node->left, sh);
			exit(sh->last_exit);
		}
		if (node->right)
			execute_ast(node->right, sh);
		else
			sh->last_exit = 0;
	}
	else if (node->type == NODE_SUBSHELL)
		exec_subshell(node, sh);
}