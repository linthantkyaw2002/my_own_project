#include "easyshell.h"

int g_signal_received = 0;

static void init_shell(t_shell *shell, char **envp)
{
    shell->env = init_env(envp);
    increment_shlvl(&shell->env);
    shell->tokens = NULL;
    shell->last_exit = 0;
    shell->visual_mode = 0;
    shell->skip_visual = 0;
}

static void handle_comments(char *input)
{
    int i;
    int in_single;
    int in_double;

    i = 0;
    in_single = 0;
    in_double = 0;
    while (input && input[i])
    {
        is_quote(input[i], &in_single, &in_double);
        if (input[i] == '#' && !in_single && !in_double)
        {
            if (i == 0 || ft_isspace(input[i - 1]))
            {
                input[i] = '\0';
                break ;
            }
        }
        i++;
    }
}

static char *get_input(void)
{
    char    *input;
    int     len;

    if (isatty(STDIN_FILENO))
        return (readline("easyshell$ "));
    input = read_byte_by_byte();
    if (input)
    {
        len = ft_strlen(input);
        if (len > 0 && input[len - 1] == '\n')
            input[len - 1] = '\0';
    }
    return (input);
}

/* MUST BE ABOVE shell_loop */
static void process_line(char *input, t_shell *shell, int execute)
{
    t_ast   *ast;
    t_token *tmp;

    g_signal_received = 0;
    shell->skip_visual = 0;
    handle_comments(input);
    shell->tokens = tokenize(input);
    if (shell->tokens)
    {
        if (check_syntax(shell->tokens))
        {
            tmp = shell->tokens;
            ast = parse_logic(&tmp, shell);
            if (ast)
            {
                /* 1. EXECUTE FIRST */
                if (execute && g_signal_received != SIGINT)
                    execute_ast(ast, shell);
                
                /* 2. VISUALIZE SECOND (Now it has the exit codes!) */
                if (shell->visual_mode && !shell->skip_visual)
                    draw_ast(ast, "", 0);
                
                free_ast(ast);
            }
        }
        else
            shell->last_exit = 2;
        free_tokens(shell->tokens);
        shell->tokens = NULL;
    }
}

/* MUST BE BELOW process_line */
static void shell_loop(t_shell *shell)
{
    char    *input;
    char    hook_buf[1024];

    while (1)
    {
        input = get_input();
        if (!input)
        {
            if (isatty(STDIN_FILENO))
                write(STDOUT_FILENO, "exit\n", 5);
            break ;
        }
        
        /* Only log and append history if running an interactive tty prompt */
        if (isatty(STDIN_FILENO) && *input)
        {
            add_history(input);
			snprintf(hook_buf, sizeof(hook_buf), "flow _hook \"%s\" > /dev/null 2>&1", input);
			system(hook_buf);
        }
        
        /* Notice the , 1 at the end here! */
        process_line(input, shell, 1); 
        
        free(input);
    }
}

int main(int argc, char **argv, char **envp)
{
    t_shell shell;

    (void)argc;
    (void)argv;
    init_shell(&shell, envp);
    setup_signals();

    /* Bind custom command tab-completion hook right into GNU Readline */
    rl_attempted_completion_function = easyshell_completion;

    /* CLI TOOL MODE: ./easyshell --ast "command string" */
    if (argc == 3 && ft_strcmp(argv[1], "--ast") == 0)
    {
        shell.visual_mode = 1;
        process_line(argv[2], &shell, 0); /* 0 = Do not execute, just visualize */
        free_env(shell.env);
        return (0);
    }

    /* STANDARD SHELL MODE */
    shell_loop(&shell);
    
    free_env(shell.env);
    rl_clear_history();
    return (shell.last_exit);
}