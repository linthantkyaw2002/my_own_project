#include "flow.h"

/* 📁 Resolves the permanent user storage workspace path layout */
char *get_macro_dir_path(void)
{
    static char path[512];
    snprintf(path, sizeof(path), "%s%s", getenv("HOME"), MACRO_DIR);
    return path;
}

char *get_macro_file_path(const char *name)
{
    static char path[512];
    snprintf(path, sizeof(path), "%s/%s", get_macro_dir_path(), name);
    return path;
}

int get_recording_state(void)
{
    FILE *fp = fopen("/tmp/.flow_active_state", "r");
    int state = 0;

    if (fp)
    {
        fscanf(fp, "%d", &state);
        fclose(fp);
    }
    return (state);
}

void set_recording_state(int state)
{
    FILE *fp = fopen(STATE_FILE, "w");
    if (fp)
    {
        fprintf(fp, "%d\n", state);
        fclose(fp);
    }
}

void cache_command_string(const char *cmd)
{
    if (!get_recording_state()) return;
    if (strncmp(cmd, "flow", 4) == 0 && (cmd[4] == '\0' || cmd[4] == ' ')) return;

    FILE *fp = fopen(CACHE_FILE, "a");
    if (fp)
    {
        fprintf(fp, "%s\n", cmd);
        fclose(fp);
    }
}

void append_to_build(const char *name, const char *cmd)
{
    /* 🛡️ INFINITE LOOP GUARDRAIL */
    if (strncmp(cmd, "flow", 4) == 0 && (cmd[4] == '\0' || cmd[4] == ' '))
        return; // Silently drop to prevent recursion

    char path[512];
    if (strcmp(name, "current") == 0 || strcmp(name, "0") == 0)
        snprintf(path, sizeof(path), "%s", CACHE_FILE);
    else
        snprintf(path, sizeof(path), "%s", get_macro_file_path(name));

    FILE *fp = fopen(path, "a");
    if (fp)
    {
        fprintf(fp, "%s\n", cmd);
        fclose(fp);
        printf("➕ Appended step to target '%s'.\n", name);
    }
}

void insert_into_build(const char *name, int index, const char *cmd)
{
    /* 🛡️ INFINITE LOOP GUARDRAIL */
    if (strncmp(cmd, "flow", 4) == 0 && (cmd[4] == '\0' || cmd[4] == ' ')) {
        fprintf(stderr, "flow insert: Cannot inject 'flow' commands. Infinite loop protection active.\n");
        return;
    }

    char path[512];
    char tmp_path[512];

    if (strcmp(name, "current") == 0 || strcmp(name, "0") == 0)
        snprintf(path, sizeof(path), "%s", CACHE_FILE);
    else
        snprintf(path, sizeof(path), "%s", get_macro_file_path(name));

    if (strcmp(name, "current") != 0 && strcmp(name, "0") != 0 && access(path, F_OK) == -1) {
        fprintf(stderr, "flow insert: Named build '%s' does not exist.\n", name);
        return;
    }
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);

    FILE *old_fp = fopen(path, "r");
    FILE *new_fp = fopen(tmp_path, "w");
    if (!old_fp || !new_fp) {
        if (old_fp) fclose(old_fp);
        if (new_fp) fclose(new_fp);
        return;
    }

    char line[1024];
    int current_idx = 0, inserted = 0;
    while (fgets(line, sizeof(line), old_fp))
    {
        if (current_idx == index) {
            fprintf(new_fp, "%s\n", cmd);
            inserted = 1;
        }
        fprintf(new_fp, "%s", line);
        current_idx++;
    }
    if (!inserted && current_idx == index) {
        fprintf(new_fp, "%s\n", cmd);
        inserted = 1;
    }
    fclose(old_fp); fclose(new_fp);

    if (inserted) {
        rename(tmp_path, path);
        printf("💉 Successfully injected step at index [%d] inside target '%s'.\n", index, name);
    } else {
        unlink(tmp_path);
        fprintf(stderr, "flow insert: Index %d out of bounds.\n", index);
    }
}

static int is_marked_for_deletion(int current_idx, int total_lines, const char *index_str)
{
    if (strcmp(index_str, "last") == 0)
        return (current_idx == total_lines - 1);

    char *dup = strdup(index_str);
    if (!dup) return 0;

    int match = 0;
    char *token = strtok(dup, ",");
    while (token)
    {
        if (atoi(token) == current_idx) {
            match = 1;
            break;
        }
        token = strtok(NULL, ",");
    }
    free(dup);
    return match;
}

void delist_from_build(const char *name, const char *index_str)
{
    char path[512];
    char tmp_path[512];
    FILE *fp;

    if (strcmp(name, "current") == 0 || strcmp(name, "0") == 0)
        snprintf(path, sizeof(path), "%s", CACHE_FILE);
    else
        snprintf(path, sizeof(path), "%s", get_macro_file_path(name));

    if (strcmp(name, "current") != 0 && strcmp(name, "0") != 0 && access(path, F_OK) == -1) {
        fprintf(stderr, "flow delist: Target build '%s' does not exist.\n", name);
        return;
    }

    fp = fopen(path, "r");
    if (!fp) return;
    char line[1024];
    int total_lines = 0;
    while (fgets(line, sizeof(line), fp))
        total_lines++;
    fclose(fp);

    if (total_lines == 0) {
        fprintf(stderr, "flow delist: Target '%s' contains no commands.\n", name);
        return;
    }

    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);
    FILE *old_fp = fopen(path, "r");
    FILE *new_fp = fopen(tmp_path, "w");
    if (!old_fp || !new_fp) {
        if (old_fp) fclose(old_fp);
        if (new_fp) fclose(new_fp);
        return;
    }

    int current_idx = 0;
    int items_deleted = 0;
    while (fgets(line, sizeof(line), old_fp))
    {
        if (is_marked_for_deletion(current_idx, total_lines, index_str))
            items_deleted++;
        else
            fprintf(new_fp, "%s", line);
        current_idx++;
    }
    fclose(old_fp);
    fclose(new_fp);

    if (items_deleted > 0) {
        rename(tmp_path, path);
        printf("✂️ Successfully removed %d step(s) from target '%s'. Remainder re-indexed.\n", items_deleted, name);
    } else {
        unlink(tmp_path);
        fprintf(stderr, "flow delist: No valid commands found matching deletion query '%s'.\n", index_str);
    }
}

void build_macro_file(const char *name)
{
    char *dest = get_macro_file_path(name);
    if (!dest) return;

    // Fast backup step to build target parent storage spaces
    mkdir(get_macro_dir_path(), 0755);

    FILE *src_fp = fopen(CACHE_FILE, "r");
    if (!src_fp)
    {
        fprintf(stderr, "flow: Current workspace [0] is empty. Nothing to build.\n");
        return;
    }
    FILE *dst_fp = fopen(dest, "w");
    if (!dst_fp)
    {
        fclose(src_fp);
        perror("flow build failed");
        return;
    }

    FILE *path_fp = fopen("/tmp/.flow_recorded_path", "r");
    if (path_fp)
    {
        char birth_path[512];
        if (fgets(birth_path, sizeof(birth_path), path_fp))
        {
            size_t len = strlen(birth_path);
            if (len > 0 && birth_path[len - 1] == '\n')
                birth_path[len - 1] = '\0';
            fprintf(dst_fp, "# dir: %s\n", birth_path);
        }
        fclose(path_fp);
    }

    char ch;
    while ((ch = fgetc(src_fp)) != EOF)
        fputc(ch, dst_fp);

    fclose(src_fp);
    fclose(dst_fp);
    printf("💾 Current flow compiled successfully to named build: '%s'\n", name);
}

void list_workspace(void)
{
    FILE *fp = fopen(CACHE_FILE, "r");
    if (!fp)
    {
        printf("📝 Current Flow Workspace [0] is empty.\n");
        return;
    }
    printf("📝 Current Flow Workspace [0]:\n");
    char line[1024];
    int idx = 0;
    while (fgets(line, sizeof(line), fp))
    {
        printf("[%d] %s", idx++, line);
    }
    fclose(fp);
}

void list_builds(void)
{
    DIR *dir = opendir(get_macro_dir_path());
    if (!dir)
    {
        printf("📦 Saved Macro Builds [0]:\n(No custom builds saved yet)\n");
        return;
    }
    struct dirent *entry;
    int count = 0;
    printf("📦 Saved Macro Builds:\n");
    while ((entry = readdir(dir)))
    {
        if (entry->d_name[0] != '.')
        {
            printf("[•] %s\n", entry->d_name);
            count++;
        }
    }
    closedir(dir);
    if (count == 0) printf("(No custom builds saved yet)\n");
}

void list_named_build(const char *name)
{
    char *path = get_macro_file_path(name);
    FILE *fp = fopen(path, "r");
    if (!fp)
    {
        fprintf(stderr, "flow list: Saved build '%s' does not exist.\n", name);
        return;
    }
    printf("📋 Saved Build '%s':\n", name);
    char line[1024];
    int idx = 0;
    while (fgets(line, sizeof(line), fp))
    {
        if (line[0] == '#') continue; // Hide metadata paths from the interface listings
        printf("[%d] %s", idx++, line);
    }
    fclose(fp);
}

/**
 * @brief Overrides permanent deletion. Migrates a macro into the hidden
 * storage trash directory (~/.flow_macros/.bin/) for safety.
 */
int remove_build(const char *name)
{
    char old_path[512];
    char bin_dir[512];
    char new_path[512];
    char *home = getenv("HOME");
    char *resolved;

    if (!home) return (1);
    
    /* Ensure the hidden trash bin directory exists on host machine */
    snprintf(bin_dir, sizeof(bin_dir), "%s/.flow_macros/.bin", home);
    mkdir(bin_dir, 0755);

    resolved = get_macro_file_path(name);
    if (!resolved) return (1);
    strncpy(old_path, resolved, sizeof(old_path));

    snprintf(new_path, sizeof(new_path), "%s/%s", bin_dir, name);

    if (rename(old_path, new_path) != 0)
    {
        perror("flow remove: failed to archive profile to bin");
        return (1);
    }
    printf("🗑️ Macro '%s' moved to Recycling Bin. Restore using 'flow bin restore <index/indices/last>'.\n", name);
    return (0);
}

/**
 * @brief Traverses the live macro path directory and programmatically 
 * routes all active files straight into the recycling trash bin.
 */
int remove_all_builds(void)
{
    DIR *dir;
    struct dirent *entry;
    char macro_dir[512];
    char *home = getenv("HOME");
    int count = 0;

    if (!home) return (1);
    snprintf(macro_dir, sizeof(macro_dir), "%s/.flow_macros", home);

    dir = opendir(macro_dir);
    if (!dir)
    {
        printf("📦 Macro library directory is already empty.\n");
        return (0);
    }

    /* Scan entries inside the directory */
    while ((entry = readdir(dir)) != NULL)
    {
        /* Skip dot references '.', '..', and your hidden '.bin' trash folder itself */
        if (entry->d_name[0] == '.')
            continue;

        /* Dynamically hand the macro string over to your safe rename wrapper */
        if (remove_build(entry->d_name) == 0)
            count++;
    }
    closedir(dir);

    if (count == 0)
        printf("📦 No active macro profiles found to clean.\n");
    else
        printf("🧹 Success! Cleaned up %d macro profiles into the Recycling Bin.\n", count);

    return (0);
}

void record_workspace_birth_path(void)
{
    char cwd[512];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        FILE *fp = fopen("/tmp/.flow_recorded_path", "w");
        if (fp)
        {
            fprintf(fp, "%s\n", cwd);
            fclose(fp);
        }
    }
}

/**
 * @brief Sequentially loops through the hidden trash subsystem and prints 
 * available profiles matching a clean indexed layout starting at [0].
 */
int list_bin(void)
{
    DIR *dir;
    struct dirent *entry;
    char bin_dir[512];
    char *home = getenv("HOME");
    int idx = 0;

    if (!home) return (1);
    snprintf(bin_dir, sizeof(bin_dir), "%s/.flow_macros/.bin", home);
    
    dir = opendir(bin_dir);
    if (!dir)
    {
        printf("♻️ Recycling Bin is currently empty.\n");
        return (0);
    }
    
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        if (idx == 0)
            printf("♻️ Archived Garbage Tracks in Recycling Bin:\n");
        printf("[%d] %s\n", idx++, entry->d_name);
    }
    
    if (idx == 0)
        printf("♻️ Recycling Bin is currently empty.\n");
        
    closedir(dir);
    return (0);
}

/**
 * @brief Parses comma-delimited tokens and handles cross-directory moves 
 * to bring profiles safely back into active workflow visibility arrays.
 */
int restore_from_bin(const char *indices_str)
{
    DIR *dir;
    struct dirent *entry;
    char bin_dir[512];
    char macro_dir[512];
    char *home = getenv("HOME");
    char *filenames[256];
    char *str_copy;
    char *token;
    int total_files = 0;

    if (!home) return (1);
    snprintf(bin_dir, sizeof(bin_dir), "%s/.flow_macros/.bin", home);
    snprintf(macro_dir, sizeof(macro_dir), "%s/.flow_macros", home);

    dir = opendir(bin_dir);
    if (!dir)
    {
        fprintf(stderr, "flow restore: trash tracking framework empty or missing.\n");
        return (1);
    }

    /* Snapshot current file structure state onto stack frames to align index mappings */
    while ((entry = readdir(dir)) != NULL && total_files < 256)
    {
        if (entry->d_name[0] == '.')
            continue;
        filenames[total_files] = strdup(entry->d_name);
        total_files++;
    }
    closedir(dir);

    if (total_files == 0)
    {
        printf("♻️ Recycling Bin is empty.\n");
        return (0);
    }

    /* Tokenize multi-index arguments step-by-step using string extraction bounds */
    str_copy = strdup(indices_str);
    token = strtok(str_copy, ",");
   while (token != NULL)
    {
        int target_idx = -1;
        
        // Safely handle the "last" keyword context
        if (strcmp(token, "last") == 0)
            target_idx = total_files - 1;
        else
            target_idx = atoi(token);

        if (target_idx >= 0 && target_idx < total_files)
        {
            char old_file[512];
            char new_file[512];

            snprintf(old_file, sizeof(old_file), "%s/%s", bin_dir, filenames[target_idx]);
            snprintf(new_file, sizeof(new_file), "%s/%s", macro_dir, filenames[target_idx]);

            if (rename(old_file, new_file) == 0)
                printf("⏪ Restored macro '%s' back to live library views successfully!\n", filenames[target_idx]);
            else
                fprintf(stderr, "flow restore: transmission deadlock moving '%s'\n", filenames[target_idx]);
        }
        else
        {
            fprintf(stderr, "flow restore: user input index [%d] out of bounds.\n", target_idx);
        }
        token = strtok(NULL, ",");
    }

    /* Release dynamic memory segments cleanly */
    for (int i = 0; i < total_files; i++)
        free(filenames[i]);
    free(str_copy);
    return (0);
}

/**
 * @brief Loops through the hidden trash subsystem and hard-unlinks every single
 * file, fully emptying the recycling bin.
 */
int clear_bin(void)
{
    DIR *dir;
    struct dirent *entry;
    char bin_dir[512];
    char file_path[512];
    char *home = getenv("HOME");
    int count = 0;

    if (!home) return (1);
    snprintf(bin_dir, sizeof(bin_dir), "%s/.flow_macros/.bin", home);

    dir = opendir(bin_dir);
    if (!dir)
    {
        printf("♻️ Recycling Bin is already empty.\n");
        return (0);
    }

    while ((entry = readdir(dir)) != NULL)
    {
        /* Shield directory tracking layers (. and ..) from processing attempts */
        if (entry->d_name[0] == '.')
            continue;

        snprintf(file_path, sizeof(file_path), "%s/%s", bin_dir, entry->d_name);
        if (unlink(file_path) == 0)
            count++;
    }
    closedir(dir);

    printf("💥 Permanent Purge Success! Vaporized %d archived profiles from disk.\n", count);
    return (0);
}

/**
 * @brief Parses comma-delimited strings to snapshot trash files onto stack memory
 * and selectively unlinks individual targeted files out of the recycling bin.
 */
int remove_from_bin(const char *indices_str)
{
    DIR *dir;
    struct dirent *entry;
    char bin_dir[512];
    char *home = getenv("HOME");
    char *filenames[256];
    char *str_copy;
    char *token;
    int total_files = 0;

    if (!home) return (1);
    snprintf(bin_dir, sizeof(bin_dir), "%s/.flow_macros/.bin", home);

    dir = opendir(bin_dir);
    if (!dir)
    {
        printf("♻️ Recycling Bin is empty.\n");
        return (0);
    }

    /* Index and map current files available in trash onto local stack frames */
    while ((entry = readdir(dir)) != NULL && total_files < 256)
    {
        if (entry->d_name[0] == '.')
            continue;
        filenames[total_files] = strdup(entry->d_name);
        total_files++;
    }
    closedir(dir);

    if (total_files == 0)
    {
        printf("♻️ Recycling Bin is empty.\n");
        return (0);
    }

    /* Tokenize target arguments step-by-step using dynamic string splitting limits */
    str_copy = strdup(indices_str);
    token = strtok(str_copy, ",");
    while (token != NULL)
    {
        int target_idx = -1;
        
        // Safely handle the "last" keyword context
        if (strcmp(token, "last") == 0)
            target_idx = total_files - 1;
        else
            target_idx = atoi(token);

        if (target_idx >= 0 && target_idx < total_files)
        {
            char target_path[512];
            snprintf(target_path, sizeof(target_path), "%s/%s", bin_dir, filenames[target_idx]);

            if (unlink(target_path) == 0)
                printf("🔥 Permanently erased tracking asset: '%s'\n", filenames[target_idx]);
            else
                fprintf(stderr, "flow bin remove: write deadlock on filename '%s'\n", filenames[target_idx]);
        }
        else
        {
            fprintf(stderr, "flow bin remove: parameter string item index [%d] out of bounds.\n", target_idx);
        }
        token = strtok(NULL, ",");
    }

    /* Free snapshot string tracking slices cleanly */
    for (int i = 0; i < total_files; i++)
        free(filenames[i]);
    free(str_copy);
    return (0);
}

/**
 * @brief Combines two archived macro builds line-by-line into a fresh named profile.
 * Preserves all original internal "# dir:" path tags untouched for midflow alignment.
 */
int join_builds(const char *b1, const char *b2, const char *newb)
{
    char path1[512];
    char path2[512];
    char path_new[512];
    char line[1024];
    char *resolved;

    /* Securely pull separate absolute path copies to bypass static buffer overwrites */
    resolved = get_macro_file_path(b1);
    if (!resolved) return (1);
    strncpy(path1, resolved, sizeof(path1));

    resolved = get_macro_file_path(b2);
    if (!resolved) return (1);
    strncpy(path2, resolved, sizeof(path2));

    resolved = get_macro_file_path(newb);
    if (!resolved) return (1);
    strncpy(path_new, resolved, sizeof(path_new));

    FILE *f1 = fopen(path1, "r");
    if (!f1) { perror("flow join: source build 1 not found"); return (1); }

    FILE *f2 = fopen(path2, "r");
    if (!f2) { fclose(f1); perror("flow join: source build 2 not found"); return (1); }

    FILE *fn = fopen(path_new, "w");
    if (!fn) { fclose(f1); fclose(f2); perror("flow join: failed to build target"); return (1); }

    /* Stream the complete contents of macro 1 into the new layout */
    while (fgets(line, sizeof(line), f1))
        fprintf(fn, "%s", line);

    /* Append the complete contents of macro 2 directly below it */
    while (fgets(line, sizeof(line), f2))
        fprintf(fn, "%s", line);

    fclose(f1);
    fclose(f2);
    fclose(fn);
    printf("🔀 Workflows '%s' and '%s' joined successfully into '%s'\n", b1, b2, newb);
    return (0);
}

/**
 * @brief Duplicates a permanent macro file inside ~/.flow_macros/ completely,
 * ensuring all internal tracking headers remain safe.
 */
int copy_build(const char *oldb, const char *newb)
{
    char path_old[512];
    char path_new[512];
    char line[1024];
    char *resolved;

    /* Isolate paths onto the local stack frames immediately */
    resolved = get_macro_file_path(oldb);
    if (!resolved) return (1);
    strncpy(path_old, resolved, sizeof(path_old));

    resolved = get_macro_file_path(newb);
    if (!resolved) return (1);
    strncpy(path_new, resolved, sizeof(path_new));

    FILE *f_old = fopen(path_old, "r");
    if (!f_old) { perror("flow copy: source profile not found"); return (1); }

    FILE *f_new = fopen(path_new, "w");
    if (!f_new) { fclose(f_old); perror("flow copy: failed to initialize destination"); return (1); }

    while (fgets(line, sizeof(line), f_old))
        fprintf(f_new, "%s", line);

    fclose(f_old);
    fclose(f_new);
    printf("📋 Workflow '%s' copied successfully to '%s'\n", oldb, newb);
    return (0);
}