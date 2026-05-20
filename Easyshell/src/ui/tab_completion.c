#include "easyshell.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

/*
** The Chooser: Intercepts Readline's autocomplete event.
*/
char **easyshell_completion(const char *text, int start, int end)
{
    char **matches = NULL;
    (void)end;

    if (start == 0)
    {
        /* ⚡ PATH-OVERRIDE EXCEPTION
        ** If the user explicitly types a path character, skip path/builtin loops
        ** and hand rendering control right back to native filename completion!
        */
        if (text[0] == '.' || text[0] == '/' || text[0] == '~')
            return NULL;

        matches = rl_completion_matches(text, command_generator);
        rl_attempted_completion_over = 1; /* Suppress filename fallback for standard commands */
    }
    return matches;
}

/*
** The Generator: Returns matches one-by-one iteratively to Readline.
*/
char *command_generator(const char *text, int state)
{
    static int     builtin_idx;
    static int     path_idx;
    static char    **path_dirs;
    static DIR     *dir_stream;
    char           *match;
    int            len;

    // Unified Phase 2 internal builtin checklist
    static char *builtins[] = {
        "echo", "cd", "export", "unset", "exit", 
        "visual", "clear", "explain", NULL
    };

    // State 0 indicates a fresh TAB stroke; reset lookups
    if (!state)
    {
        builtin_idx = 0;
        path_idx = 0;
        dir_stream = NULL;
        
        // Grab environment string from system or your local t_env map
        char *path_env = getenv("PATH");
        if (path_env)
            path_dirs = ft_split(path_env, ':'); // Reuses your existing split utility
        else
            path_dirs = NULL;
    }

    len = strlen(text);

    // 1. Scan Builtin Arrays First
    while (builtins[builtin_idx] != NULL)
    {
        char *b_name = builtins[builtin_idx++];
        if (strncmp(b_name, text, len) == 0)
            return (strdup(b_name)); // Readline takes ownership and frees this
    }

    // 2. Scan Executable Binaries inside $PATH Directories
    if (path_dirs == NULL)
        return (NULL);

    while (path_dirs[path_idx] != NULL)
    {
        if (dir_stream == NULL)
        {
            dir_stream = opendir(path_dirs[path_idx]);
            if (dir_stream == NULL)
            {
                path_idx++;
                continue;
            }
        }

        struct dirent *dir_entry;
        while ((dir_entry = readdir(dir_stream)) != NULL)
        {
            if (dir_entry->d_name[0] == '.') // Ignore hidden items
                continue;

            if (strncmp(dir_entry->d_name, text, len) == 0)
            {
                match = strdup(dir_entry->d_name);
                return (match); // Found binary candidate match
            }
        }

        // Close stream and advance to the next index in the $PATH matrix
        closedir(dir_stream);
        dir_stream = NULL;
        path_idx++;
    }

    // Garbage collection for path split allocations once search completes
    if (path_dirs)
    {
        int i = 0;
        while (path_dirs[i])
            free(path_dirs[i++]);
        free(path_dirs);
        path_dirs = NULL;
    }

    return (NULL); // End of match stream
}