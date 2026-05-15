#include "easyshell.h"

static void handle_io(t_cmd *cmd, int prev_fd, int p_fd[2])
{
    if (cmd->infile >= 0)
        dup2(cmd->infile, STDIN_FILENO);
    else if (prev_fd != -1)
        dup2(prev_fd, STDIN_FILENO);
    if (cmd->outfile >= 0)
        dup2(cmd->outfile, STDOUT_FILENO);
    else if (cmd->next)
        dup2(p_fd[1], STDOUT_FILENO);
    if (prev_fd != -1)
        close(prev_fd);
    if (cmd->next)
    {
        close(p_fd[0]);
        close(p_fd[1]);
    }
    if (cmd->infile >= 0)
        close(cmd->infile);
    if (cmd->outfile >= 0)
        close(cmd->outfile);
}

static void run_child(t_cmd *cmd, t_shell *sh, int prev_fd, int p_fd[2])
{
    char    *path;
    char    **envp;
    int     j;

    if (cmd->error)
        exit(1);
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    handle_io(cmd, prev_fd, p_fd);
	if (cmd->args)
        expand_cmd_args(cmd, sh);
    if (!cmd->args || !cmd->args[0])
        exit(0);
    if (is_builtin(cmd->args, sh))
        exit(exe_builtin(cmd->args, sh));
    path = get_cmd_path(cmd->args[0], sh->env);
    if (path)
    {
        envp = env_to_array(sh->env);
        execve(path, cmd->args, envp);
        j = -1;
        while (envp && envp[++j])
            free(envp[j]);
        free(envp);
    }
    handle_exec_error(path, cmd->args[0]);
}

static void wait_pipeline(t_cmd *cmd, t_shell *sh)
{
    t_cmd   *curr;
    int     status;

    curr = cmd;
    while (curr)
    {
        /* Only wait if we successfully forked a child */
        if (curr->pid > 0)
        {
            waitpid(curr->pid, &status, 0);
            if (WIFEXITED(status))
                curr->exit_status = WEXITSTATUS(status);
            else if (WIFSIGNALED(status))
                curr->exit_status = 128 + WTERMSIG(status);
            
            /* Update the global shell exit status to the last command */
            sh->last_exit = curr->exit_status; 
        }
        curr = curr->next;
    }
}

static void fork_commands(t_cmd *cmd, t_shell *sh)
{
    int     p_fd[2];
    int     prev_fd;

    prev_fd = -1;
    setup_execution_signals();
    while (cmd)
    {
        if (cmd->next)
            pipe(p_fd);
        cmd->pid = fork();
        if (cmd->pid == 0)
            run_child(cmd, sh, prev_fd, p_fd);
        if (prev_fd != -1)
            close(prev_fd);
        if (cmd->next)
        {
            close(p_fd[1]);
            prev_fd = p_fd[0];
        }
        if (cmd->infile >= 0)
        {
            close(cmd->infile);
            cmd->infile = -1;
        }
        if (cmd->outfile >= 0)
        {
            close(cmd->outfile);
            cmd->outfile = -1;
        }
        cmd = cmd->next;
    }
    if (prev_fd != -1)
        close(prev_fd);
}

void    execute_commands(t_cmd *cmd, t_shell *sh)
{
    int     fd[2];
    t_cmd   *head;

    if (!cmd)
        return ;
    head = cmd;
    if (cmd->args)
        expand_cmd_args(cmd, sh);
    if (!cmd->next && cmd->error == 1)
    {
        sh->last_exit = 1;
        cmd->exit_status = 1;
        return ;
    }
    if (!cmd->next && is_builtin(cmd->args, sh))
    {
        fd[0] = dup(STDIN_FILENO);
        fd[1] = dup(STDOUT_FILENO); 
        handle_io(cmd, -1, NULL);
        sh->last_exit = exe_builtin(cmd->args, sh);
        dup2(fd[0], STDIN_FILENO);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        cmd->exit_status = sh->last_exit;
        return ;
    }
    
    fork_commands(cmd, sh);
    wait_pipeline(head, sh);
}