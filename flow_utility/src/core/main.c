#include "flow.h"

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: flow <subcommand> [args...]\n");
        return 1;
    }

    /* 🕵️‍♂️ Hidden Automated Background Logger Hook */
    if (strcmp(argv[1], "_hook") == 0 && argc == 3)
    {
        cache_command_string(argv[2]);
        return (0);
    }

    /* ==================================================== */
    /* LIVE RECORDING SESSION CONTROLS                      */
    /* ==================================================== */
    if (strcmp(argv[1], "start") == 0)
    {
        /* Nested Call Guard */
        if (get_recording_state() == 1)
        {
            fprintf(stderr, "⚠️ Workflow recording is already active! Type commands normally.\n");
            return (0);
        }
        
        FILE *cache_fp = fopen("/tmp/.flow_record_cache", "w");
        if (cache_fp)
            fclose(cache_fp);

        printf("🎬 Workflow recording started. Type commands normally.\n");
        record_workspace_birth_path();
        
        set_recording_state(1);
        return (0);
    }
    /* ---------------------------------------------------- */
    /* INJECT THE MISSING STOP BLOCK HERE                   */
    /* ---------------------------------------------------- */
    else if (strcmp(argv[1], "stop") == 0)
    {
        if (get_recording_state() == 0)
        {
            fprintf(stderr, "⚠️ No workflow is currently active to stop.\n");
            return (1); // Exit with error
        }
        set_recording_state(0);
        printf("🛑 Workflow recording stopped.\n");
    }
    /* ---------------------------------------------------- */

    /* 2. Smart Overloaded Append */
    else if (strcmp(argv[1], "append") == 0) {
        if (argc == 3) {
            append_to_build("current", argv[2]);
        } else if (argc == 4) {
            append_to_build(argv[2], argv[3]);
        } else {
            fprintf(stderr, "Usage: flow append [buildname] \"<cmd>\"\n");
        }
    }
    
    /* 3. Smart Overloaded Insert */
    else if (strcmp(argv[1], "insert") == 0) {
        if (argc == 4) {
            insert_into_build("current", atoi(argv[2]), argv[3]);
        } else if (argc == 5) {
            insert_into_build(argv[2], atoi(argv[3]), argv[4]);
        } else {
            fprintf(stderr, "Usage: flow insert [buildname] <index> \"<cmd>\"\n");
        }
    }

    /* 4. Smart Overloaded Delist */
    else if (strcmp(argv[1], "delist") == 0) {
        if (argc == 3) {
            delist_from_build("current", argv[2]);
        } else if (argc == 4) {
            delist_from_build(argv[2], argv[3]);
        } else {
            fprintf(stderr, "Usage: flow delist [buildname] <index/indices/last>\n");
        }
    }

    /* 5. Build, Removal, Listing, and Replay Modules */
    else if (strcmp(argv[1], "build") == 0) {
        if (argc == 3) build_macro_file(argv[2]);
        else fprintf(stderr, "Usage: flow build <buildname>\n");
    }
    else if (strcmp(argv[1], "remove") == 0)
    {
        if (argc != 3)
        {
            fprintf(stderr, "Usage: flow remove <buildname> or flow remove all\n");
            return (1);
        }
        /* If the target parameter string equals "all", trigger bulk migration */
        if (strcmp(argv[2], "all") == 0)
        {
            return (remove_all_builds());
        }
        return (remove_build(argv[2]));
    }
    else if (strcmp(argv[1], "run") == 0) {
        if (argc == 2) run_workspace(NULL, 0);
        else if (argc == 3) {
            if (strcmp(argv[2], "-l") == 0) run_workspace(NULL, 1);
            else if (argv[2][0] >= '0' && argv[2][0] <= '9') run_workspace(argv[2], 0);
            else run_named_build(argv[2], NULL, 0);
        }
        else if (argc == 4) {
            if (argv[2][0] >= '0' && argv[2][0] <= '9' && strcmp(argv[3], "-l") == 0) run_workspace(argv[2], 1);
            else if (strcmp(argv[3], "-l") == 0) run_named_build(argv[2], NULL, 1);
            else run_named_build(argv[2], argv[3], 0);
        }
        else if (argc == 5 && strcmp(argv[4], "-l") == 0) {
            run_named_build(argv[2], argv[3], 1);
        }
    }
    else if (strcmp(argv[1], "list") == 0) {
        if (argc == 2) list_workspace();
        if (argc == 3 && strcmp(argv[2], "bin") == 0)
            return (list_bin());
        else if (argc == 3 && strcmp(argv[2], "build") == 0) list_builds();
        else if (argc == 3) list_named_build(argv[2]);
    }

    /* 6. Build Composition */
    else if (strcmp(argv[1], "join") == 0)
    {
        if (argc != 5)
        {
            fprintf(stderr, "Usage: flow join <build1> <build2> <new_build>\n");
            return (1);
        }
        return (join_builds(argv[2], argv[3], argv[4]));
    }
    /* 7. Build Duplication */
    else if (strcmp(argv[1], "copy") == 0)
    {
        if (argc != 4)
        {
            fprintf(stderr, "Usage: flow copy <old_build> <new_build>\n");
            return (1);
        }
        return (copy_build(argv[2], argv[3]));
    }

    /* 8. Trash Bin Management */
    else if (strcmp(argv[1], "bin") == 0)
    {
        if (argc == 3 && strcmp(argv[2], "clear") == 0)
        {
            clear_bin();
        }
        else if (argc == 4 && strcmp(argv[2], "remove") == 0)
        {
            return (remove_from_bin(argv[3]));
        }
        else if (argc == 4 && strcmp(argv[2], "restore") == 0)
        {
            return (restore_from_bin(argv[3]));
        }
        else
        {
            fprintf(stderr, "Usage: flow bin clear\n");
            fprintf(stderr, "       flow bin remove <index/indices/last>\n");
            fprintf(stderr, "       flow bin restore <index/indices/last>\n");
            return (1);
        }
    }

    else {
        fprintf(stderr, "flow: Unknown subcommand '%s'\n", argv[1]);
        return 1;
    }
    return 0;
}