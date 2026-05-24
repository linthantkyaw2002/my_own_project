#include "flow.h"
#include <limits.h>

static int parse_main_index(const char *str, int *out)
{
    char    *end;
    long    value;

    if (!str || str[0] == '\0')
        return (-1);
    value = strtol(str, &end, 10);
    if (end == str || *end != '\0')
        return (-1);
    if (value < 0 || value > INT_MAX)
        return (-1);
    *out = (int)value;
    return (0);
}

static int is_index_query(const char *str)
{
    int i;
    int expect_digit;

    if (!str || str[0] == '\0')
        return (0);
    i = 0;
    expect_digit = 1;
    while (str[i])
    {
        if (str[i] >= '0' && str[i] <= '9')
            expect_digit = 0;
        else if (str[i] == ',' && !expect_digit)
            expect_digit = 1;
        else
            return (0);
        i++;
    }
    return (!expect_digit);
}

static int usage_run(void)
{
    fprintf(stderr, "Usage: flow run [index/indices] [-l]\n");
    fprintf(stderr, "       flow run <buildname> [index/indices] [-l]\n");
    return (1);
}

static int usage_list(void)
{
    fprintf(stderr, "Usage: flow list\n");
    fprintf(stderr, "       flow list build\n");
    fprintf(stderr, "       flow list bin\n");
    fprintf(stderr, "       flow list <buildname>\n");
    return (1);
}

static int handle_run(int argc, char **argv)
{
    if (argc == 2)
    {
        run_workspace(NULL, 0);
        return (0);
    }
    if (argc == 3)
    {
        if (strcmp(argv[2], "-l") == 0)
            run_workspace(NULL, 1);
        else if (is_index_query(argv[2]))
            run_workspace(argv[2], 0);
        else
            run_named_build(argv[2], NULL, 0);
        return (0);
    }
    if (argc == 4)
    {
        if (strcmp(argv[3], "-l") == 0 && is_index_query(argv[2]))
            run_workspace(argv[2], 1);
        else if (strcmp(argv[3], "-l") == 0)
            run_named_build(argv[2], NULL, 1);
        else if (is_index_query(argv[3]))
            run_named_build(argv[2], argv[3], 0);
        else
            return (usage_run());
        return (0);
    }
    if (argc == 5 && strcmp(argv[4], "-l") == 0 && is_index_query(argv[3]))
    {
        run_named_build(argv[2], argv[3], 1);
        return (0);
    }
    return (usage_run());
}

static int handle_list(int argc, char **argv)
{
    if (argc == 2)
    {
        list_workspace();
        return (0);
    }
    if (argc == 3 && strcmp(argv[2], "bin") == 0)
        return (list_bin());
    if (argc == 3 && strcmp(argv[2], "build") == 0)
    {
        list_builds();
        return (0);
    }
    if (argc == 3)
    {
        list_named_build(argv[2]);
        return (0);
    }
    return (usage_list());
}

static int show_version(void)
{
    printf("Flow CLI Tool\n");
    printf("Version: 0.1.0-beta\n");
    printf("License: Beta / Proprietary\n");
    printf("Copyright (c) 2026 Lin Thant Kyaw. All rights reserved.\n");
    return (0);
}

int main(int argc, char **argv)
{
    int index;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: flow <subcommand> [args...]\n");
        return (1);
    }
    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0)
    return (show_version());
    if (strcmp(argv[1], "about") == 0)
    return (show_version());
    if (strcmp(argv[1], "_hook") == 0)
    {
        if (argc != 3)
            return (1);
        cache_command_string(argv[2]);
        return (0);
    }
    if (strcmp(argv[1], "start") == 0)
    {
        FILE *cache_fp;

        if (argc != 2)
        {
            fprintf(stderr, "Usage: flow start\n");
            return (1);
        }
        if (get_recording_state() == 1)
        {
            fprintf(stderr, "⚠️ Workflow recording is already active! Type commands normally.\n");
            return (0);
        }
        cache_fp = fopen(CACHE_FILE, "w");
        if (cache_fp)
            fclose(cache_fp);
        printf("🎬 Workflow recording started. Type commands normally.\n");
        record_workspace_birth_path();
        set_recording_state(1);
        return (0);
    }
    else if (strcmp(argv[1], "stop") == 0)
    {
        if (argc != 2)
        {
            fprintf(stderr, "Usage: flow stop\n");
            return (1);
        }
        if (get_recording_state() == 0)
        {
            fprintf(stderr, "⚠️ No workflow is currently active to stop.\n");
            return (1);
        }
        set_recording_state(0);
        printf("🛑 Workflow recording stopped.\n");
        return (0);
    }
    else if (strcmp(argv[1], "append") == 0)
    {
        if (argc == 3)
            append_to_build("current", argv[2]);
        else if (argc == 4)
            append_to_build(argv[2], argv[3]);
        else
        {
            fprintf(stderr, "Usage: flow append [buildname] \"<cmd>\"\n");
            return (1);
        }
    }
    else if (strcmp(argv[1], "insert") == 0)
    {
        if (argc == 4)
        {
            if (parse_main_index(argv[2], &index) == -1)
            {
                fprintf(stderr, "flow insert: invalid index '%s'\n", argv[2]);
                fprintf(stderr, "Usage: flow insert [buildname] <index> \"<cmd>\"\n");
                return (1);
            }
            insert_into_build("current", index, argv[3]);
        }
        else if (argc == 5)
        {
            if (parse_main_index(argv[3], &index) == -1)
            {
                fprintf(stderr, "flow insert: invalid index '%s'\n", argv[3]);
                fprintf(stderr, "Usage: flow insert [buildname] <index> \"<cmd>\"\n");
                return (1);
            }
            insert_into_build(argv[2], index, argv[4]);
        }
        else
        {
            fprintf(stderr, "Usage: flow insert [buildname] <index> \"<cmd>\"\n");
            return (1);
        }
    }
    else if (strcmp(argv[1], "delist") == 0)
    {
        if (argc == 3)
            delist_from_build("current", argv[2]);
        else if (argc == 4)
            delist_from_build(argv[2], argv[3]);
        else
        {
            fprintf(stderr, "Usage: flow delist [buildname] <index/indices/last>\n");
            return (1);
        }
    }
    else if (strcmp(argv[1], "build") == 0)
    {
        if (argc == 3)
            build_macro_file(argv[2]);
        else
        {
            fprintf(stderr, "Usage: flow build <buildname>\n");
            return (1);
        }
    }
    else if (strcmp(argv[1], "remove") == 0)
    {
        if (argc != 3)
        {
            fprintf(stderr, "Usage: flow remove <buildname> or flow remove all\n");
            return (1);
        }
        if (strcmp(argv[2], "all") == 0)
            return (remove_all_builds());
        return (remove_build(argv[2]));
    }
    else if (strcmp(argv[1], "run") == 0)
        return (handle_run(argc, argv));
    else if (strcmp(argv[1], "list") == 0)
        return (handle_list(argc, argv));
    else if (strcmp(argv[1], "join") == 0)
    {
        if (argc != 5)
        {
            fprintf(stderr, "Usage: flow join <build1> <build2> <new_build>\n");
            return (1);
        }
        return (join_builds(argv[2], argv[3], argv[4]));
    }
    else if (strcmp(argv[1], "copy") == 0)
    {
        if (argc != 4)
        {
            fprintf(stderr, "Usage: flow copy <old_build> <new_build>\n");
            return (1);
        }
        return (copy_build(argv[2], argv[3]));
    }
    else if (strcmp(argv[1], "bin") == 0)
    {
        if (argc == 3 && strcmp(argv[2], "clear") == 0)
            return (clear_bin());
        else if (argc == 4 && strcmp(argv[2], "remove") == 0)
            return (remove_from_bin(argv[3]));
        else if (argc == 4 && strcmp(argv[2], "restore") == 0)
            return (restore_from_bin(argv[3]));
        else
        {
            fprintf(stderr, "Usage: flow bin clear\n");
            fprintf(stderr, "       flow bin remove <index/indices/last>\n");
            fprintf(stderr, "       flow bin restore <index/indices/last>\n");
            return (1);
        }
    }
    else
    {
        fprintf(stderr, "flow: Unknown subcommand '%s'\n", argv[1]);
        return (1);
    }
    return (0);
}