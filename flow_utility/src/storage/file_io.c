#include "flow.h"
#include <time.h>
#include <utime.h>

/* 📁 Resolves the permanent user storage workspace path layout */
char *get_macro_dir_path(void)
{
    static char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s%s", getenv("HOME"), MACRO_DIR);
    return path;
}

static int confirm_build_overwrite(const char *name)
{
    char answer[16];

    if (!isatty(STDIN_FILENO))
    {
        fprintf(stderr,
            "flow: Saved build '%s' already exists. Refusing to overwrite without confirmation.\n",
            name);
        return (0);
    }
    printf("⚠️ Saved build '%s' already exists. Overwrite it? [y/N] ", name);
    fflush(stdout);
    if (!fgets(answer, sizeof(answer), stdin))
        return (0);
    if (answer[0] == 'y' || answer[0] == 'Y')
        return (1);
    printf("flow: overwrite cancelled. Existing build '%s' was kept.\n", name);
    return (0);
}

char *get_macro_file_path(const char *name)
{
    static char path[PATH_MAX];
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

    char path[PATH_MAX];
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

    char path[PATH_MAX];
    char tmp_path[PATH_MAX];

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
        if (strncmp(line, "# dir: ", 7) == 0)
        {
            fprintf(new_fp, "%s", line);
            continue;
        }
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

static int parse_delist_index(const char *token, int total_lines, int *out)
{
    char    *end;
    long    value;

    if (!token || token[0] == '\0')
        return (-1);
    if (strcmp(token, "last") == 0)
    {
        if (total_lines <= 0)
            return (-1);
        *out = total_lines - 1;
        return (0);
    }
    value = strtol(token, &end, 10);
    if (end == token || *end != '\0')
        return (-1);
    if (value < 0 || value >= total_lines)
        return (-1);
    *out = (int)value;
    return (0);
}

static int validate_delist_query(const char *index_str, int total_lines)
{
    char    *dup;
    char    *token;
    int     idx;

    if (!index_str || index_str[0] == '\0')
        return (-1);
    if (index_str[0] == ',')
        return (-1);
    if (index_str[strlen(index_str) - 1] == ',')
        return (-1);
    if (strstr(index_str, ",,"))
        return (-1);
    dup = strdup(index_str);
    if (!dup)
        return (-1);
    token = strtok(dup, ",");
    while (token)
    {
        if (parse_delist_index(token, total_lines, &idx) == -1)
        {
            free(dup);
            return (-1);
        }
        token = strtok(NULL, ",");
    }
    free(dup);
    return (0);
}

static int is_marked_for_deletion(int current_idx, int total_lines, const char *index_str)
{
    char    *dup;
    char    *token;
    int     target_idx;
    int     match;

    dup = strdup(index_str);
    if (!dup)
        return (0);
    match = 0;
    token = strtok(dup, ",");
    while (token)
    {
        if (parse_delist_index(token, total_lines, &target_idx) == 0
            && target_idx == current_idx)
        {
            match = 1;
            break;
        }
        token = strtok(NULL, ",");
    }
    free(dup);
    return (match);
}

void delist_from_build(const char *name, const char *index_str)
{
    char path[PATH_MAX];
    char tmp_path[PATH_MAX];
    FILE *fp;

    if (strcmp(name, "current") == 0 || strcmp(name, "0") == 0)
        snprintf(path, sizeof(path), "%s", CACHE_FILE);
    else
        snprintf(path, sizeof(path), "%s", get_macro_file_path(name));

    if (strcmp(name, "current") != 0 && strcmp(name, "0") != 0 && access(path, F_OK) == -1) 
    {
        fprintf(stderr, "flow delist: Target build '%s' does not exist.\n", name);
        return;
    }

    fp = fopen(path, "r");
    if (!fp) return;
    char line[1024];
    int total_lines = 0;

    while (fgets(line, sizeof(line), fp))
    {
        if (strncmp(line, "# dir: ", 7) == 0)
            continue;
        total_lines++;
    }   
    fclose(fp);

    if (total_lines == 0) 
    {
    fprintf(stderr, "flow delist: Target '%s' contains no commands.\n", name);
    return;
    }

    if (validate_delist_query(index_str, total_lines) == -1)
    {
        fprintf(stderr, "flow delist: invalid index syntax or out of bounds '%s'\n", index_str);
        fprintf(stderr, "Usage: flow delist [buildname] <index/indices/last>\n");
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
        if (strncmp(line, "# dir: ", 7) == 0)
        {
            fprintf(new_fp, "%s", line);
            continue;
        }
        if (is_marked_for_deletion(current_idx, total_lines, index_str))
        {
            items_deleted++;
        }
        else
        {
            fprintf(new_fp, "%s", line);
        }
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
static int is_real_command_line(const char *line)
{
    int i;

    if (!line || strncmp(line, "# dir: ", 7) == 0)
        return (0);
    i = 0;
    while (line[i] == ' ' || line[i] == '\t' || line[i] == '\n')
        i++;
    return (line[i] != '\0');
}

static int count_command_lines(const char *path)
{
    FILE    *fp;
    char    line[1024];
    int     count;

    fp = fopen(path, "r");
    if (!fp)
        return (0);
    count = 0;
    while (fgets(line, sizeof(line), fp))
    {
        if (is_real_command_line(line))
            count++;
    }
    fclose(fp);
    return (count);
}

void build_macro_file(const char *name)
{
    char dest[PATH_MAX];
    char *resolved;
    FILE *src_fp;
    FILE *dst_fp;
    FILE *path_fp;
    char birth_path[PATH_MAX];
    char ch;

    if (!name || name[0] == '\0')
    {
        fprintf(stderr, "flow build: build name cannot be empty.\n");
        return;
    }
    resolved = get_macro_file_path(name);
    if (!resolved)
        return;
    snprintf(dest, sizeof(dest), "%s", resolved);
    mkdir(get_macro_dir_path(), 0755);
    if (count_command_lines(CACHE_FILE) == 0)
    {
        fprintf(stderr, "flow build: Current workspace [0] is empty. Nothing to build.\n");
        return;
    }
    if (access(dest, F_OK) == 0 && !confirm_build_overwrite(name))
        return;
    src_fp = fopen(CACHE_FILE, "r");
    if (!src_fp)
    {
        fprintf(stderr, "flow build: Current workspace [0] is empty. Nothing to build.\n");
        return;
    }
    dst_fp = fopen(dest, "w");
    if (!dst_fp)
    {
        fclose(src_fp);
        perror("flow build failed");
        return;
    }
    path_fp = fopen("/tmp/.flow_recorded_path", "r");
    if (path_fp)
    {
        if (fgets(birth_path, sizeof(birth_path), path_fp))
        {
            size_t len = strlen(birth_path);

            if (len > 0 && birth_path[len - 1] == '\n')
                birth_path[len - 1] = '\0';
            fprintf(dst_fp, "# dir: %s\n", birth_path);
        }
        fclose(path_fp);
    }
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
        if (strncmp(line, "# dir: ", 7) == 0)
            continue;
        printf("[%d] %s", idx++, line);
    }

    if (idx == 0)
        printf("(empty)\n");

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
    char old_path[PATH_MAX];
    char bin_dir[PATH_MAX];
    char new_path[PATH_MAX * 2];
    char *home = getenv("HOME");
    char *resolved;

    if (!home)
        return (1);
    if (!name || name[0] == '\0')
    {
        fprintf(stderr, "flow remove: build name cannot be empty.\n");
        return (1);
    }
    mkdir(get_macro_dir_path(), 0755);
    snprintf(bin_dir, sizeof(bin_dir), "%s/.flow_macros/.bin", home);
    mkdir(bin_dir, 0755);
    resolved = get_macro_file_path(name);
    if (!resolved)
        return (1);
    snprintf(old_path, sizeof(old_path), "%s", resolved);
    snprintf(new_path, sizeof(new_path), "%s/%s", bin_dir, name);
    if (access(old_path, F_OK) == -1)
    {
        fprintf(stderr, "flow remove: Saved build '%s' does not exist.\n", name);
        return (1);
    }
    if (rename(old_path, new_path) != 0)
    {
        perror("flow remove: failed to archive profile to bin");
        return (1);
    }
    utime(new_path, NULL);
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
    char macro_dir[PATH_MAX];
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
    char cwd[PATH_MAX];
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

typedef struct s_bin_item
{
    char    *name;
    time_t  deleted_at;
}   t_bin_item;

static void free_bin_items(t_bin_item *items, int total)
{
    int i;

    i = 0;
    while (i < total)
    {
        free(items[i].name);
        i++;
    }
}

static void sort_bin_items_newest_first(t_bin_item *items, int total)
{
    int         i;
    int         j;
    t_bin_item tmp;

    i = 0;
    while (i < total)
    {
        j = i + 1;
        while (j < total)
        {
            if (items[j].deleted_at > items[i].deleted_at
                || (items[j].deleted_at == items[i].deleted_at
                    && strcmp(items[j].name, items[i].name) < 0))
            {
                tmp = items[i];
                items[i] = items[j];
                items[j] = tmp;
            }
            j++;
        }
        i++;
    }
}

static int load_bin_items(t_bin_item *items, int *total, const char *bin_dir)
{
    DIR             *dir;
    struct dirent   *entry;
    struct stat      st;
    char             file_path[PATH_MAX * 2];

    *total = 0;
    dir = opendir(bin_dir);
    if (!dir)
        return (-1);
    while ((entry = readdir(dir)) != NULL && *total < 256)
    {
        if (entry->d_name[0] == '.')
            continue;
        snprintf(file_path, sizeof(file_path), "%s/%s", bin_dir, entry->d_name);
        if (stat(file_path, &st) == -1)
            continue;
        items[*total].name = strdup(entry->d_name);
        if (!items[*total].name)
            continue;
        items[*total].deleted_at = st.st_mtime;
        (*total)++;
    }
    closedir(dir);
    sort_bin_items_newest_first(items, *total);
    return (0);
}

static int parse_bin_index(const char *token, int total, int *out)
{
    char    *end;
    long    value;

    if (!token || token[0] == '\0')
        return (-1);
    if (strcmp(token, "last") == 0)
    {
        if (total <= 0)
            return (-1);
        *out = 0;
        return (0);
    }
    value = strtol(token, &end, 10);
    if (end == token || *end != '\0')
        return (-1);
    if (value < 0 || value >= total)
        return (-1);
    *out = (int)value;
    return (0);
}

static int validate_bin_query(const char *indices_str, int total)
{
    char    *copy;
    char    *token;
    int     idx;

    if (!indices_str || indices_str[0] == '\0')
        return (-1);
    if (indices_str[0] == ',' || indices_str[strlen(indices_str) - 1] == ',')
        return (-1);
    if (strstr(indices_str, ",,"))
        return (-1);
    copy = strdup(indices_str);
    if (!copy)
        return (-1);
    token = strtok(copy, ",");
    while (token)
    {
        if (parse_bin_index(token, total, &idx) == -1)
        {
            free(copy);
            return (-1);
        }
        token = strtok(NULL, ",");
    }
    free(copy);
    return (0);
}

int list_bin(void)
{
    char        bin_dir[PATH_MAX];
    char        *home = getenv("HOME");
    t_bin_item  items[256];
    int         total;
    int         i;

    if (!home)
        return (1);
    snprintf(bin_dir, sizeof(bin_dir), "%s/.flow_macros/.bin", home);
    if (load_bin_items(items, &total, bin_dir) == -1 || total == 0)
    {
        printf("♻️ Recycling Bin is currently empty.\n");
        return (0);
    }
    printf("♻️ Archived Garbage Tracks in Recycling Bin (newest first):\n");
    i = 0;
    while (i < total)
    {
        printf("[%d] %s\n", i, items[i].name);
        i++;
    }
    free_bin_items(items, total);
    return (0);
}

int restore_from_bin(const char *indices_str)
{
    char        bin_dir[PATH_MAX];
    char        macro_dir[PATH_MAX];
    char        old_file[PATH_MAX * 2];
    char        new_file[PATH_MAX * 2];
    char        *home = getenv("HOME");
    char        *copy;
    char        *token;
    t_bin_item  items[256];
    int         total;
    int         idx;

    if (!home)
        return (1);
    snprintf(bin_dir, sizeof(bin_dir), "%s/.flow_macros/.bin", home);
    snprintf(macro_dir, sizeof(macro_dir), "%s/.flow_macros", home);
    if (load_bin_items(items, &total, bin_dir) == -1 || total == 0)
    {
        printf("♻️ Recycling Bin is empty.\n");
        return (0);
    }
    if (validate_bin_query(indices_str, total) == -1)
    {
        fprintf(stderr, "flow bin restore: invalid index syntax or out of bounds '%s'\n", indices_str);
        fprintf(stderr, "Usage: flow bin restore <index/indices/last>\n");
        free_bin_items(items, total);
        return (1);
    }
    mkdir(macro_dir, 0755);
    copy = strdup(indices_str);
    if (!copy)
    {
        free_bin_items(items, total);
        return (1);
    }
    token = strtok(copy, ",");
    while (token)
    {
        parse_bin_index(token, total, &idx);
        snprintf(old_file, sizeof(old_file), "%s/%s", bin_dir, items[idx].name);
        snprintf(new_file, sizeof(new_file), "%s/%s", macro_dir, items[idx].name);
        if (access(new_file, F_OK) == 0)
            fprintf(stderr, "flow bin restore: live build '%s' already exists. Remove or rename it first.\n", items[idx].name);
        else if (rename(old_file, new_file) == 0)
            printf("⏪ Restored macro '%s' back to live library successfully!\n", items[idx].name);
        else
            fprintf(stderr, "flow bin restore: failed moving '%s'\n", items[idx].name);
        token = strtok(NULL, ",");
    }
    free(copy);
    free_bin_items(items, total);
    return (0);
}

int clear_bin(void)
{
    char        bin_dir[PATH_MAX];
    char        file_path[PATH_MAX * 2];
    char        *home = getenv("HOME");
    t_bin_item  items[256];
    int         total;
    int         i;
    int         count;

    if (!home)
        return (1);
    snprintf(bin_dir, sizeof(bin_dir), "%s/.flow_macros/.bin", home);
    if (load_bin_items(items, &total, bin_dir) == -1 || total == 0)
    {
        printf("♻️ Recycling Bin is already empty. Nothing to clear.\n");
        return (0);
    }
    i = 0;
    count = 0;
    while (i < total)
    {
        snprintf(file_path, sizeof(file_path), "%s/%s", bin_dir, items[i].name);
        if (unlink(file_path) == 0)
            count++;
        i++;
    }
    free_bin_items(items, total);
    printf("💥 Permanent Purge Success! Vaporized %d archived profile(s) from disk.\n", count);
    return (0);
}

int remove_from_bin(const char *indices_str)
{
    char        bin_dir[PATH_MAX];
    char        target_path[PATH_MAX * 2];
    char        *home = getenv("HOME");
    char        *copy;
    char        *token;
    t_bin_item  items[256];
    int         total;
    int         idx;

    if (!home)
        return (1);
    snprintf(bin_dir, sizeof(bin_dir), "%s/.flow_macros/.bin", home);
    if (load_bin_items(items, &total, bin_dir) == -1 || total == 0)
    {
        printf("♻️ Recycling Bin is empty.\n");
        return (0);
    }
    if (validate_bin_query(indices_str, total) == -1)
    {
        fprintf(stderr, "flow bin remove: invalid index syntax or out of bounds '%s'\n", indices_str);
        fprintf(stderr, "Usage: flow bin remove <index/indices/last>\n");
        free_bin_items(items, total);
        return (1);
    }
    copy = strdup(indices_str);
    if (!copy)
    {
        free_bin_items(items, total);
        return (1);
    }
    token = strtok(copy, ",");
    while (token)
    {
        parse_bin_index(token, total, &idx);
        snprintf(target_path, sizeof(target_path), "%s/%s", bin_dir, items[idx].name);
        if (unlink(target_path) == 0)
            printf("🔥 Permanently erased tracking asset: '%s'\n", items[idx].name);
        else
            fprintf(stderr, "flow bin remove: failed to erase '%s'\n", items[idx].name);
        token = strtok(NULL, ",");
    }
    free(copy);
    free_bin_items(items, total);
    return (0);
}

/**
 * @brief Combines two archived macro builds line-by-line into a fresh named profile.
 * Preserves all original internal "# dir:" path tags untouched for midflow alignment.
 */
int join_builds(const char *b1, const char *b2, const char *newb)
{
    char path1[PATH_MAX];
    char path2[PATH_MAX];
    char path_new[PATH_MAX];
    char line[1024];
    char *resolved;
    FILE *f1;
    FILE *f2;
    FILE *fn;

    if (!b1 || !b2 || !newb || newb[0] == '\0')
    {
        fprintf(stderr, "flow join: invalid build name.\n");
        return (1);
    }
    if (strcmp(newb, b1) == 0 || strcmp(newb, b2) == 0)
    {
        fprintf(stderr,
            "flow join: destination build cannot be the same as source build.\n");
        return (1);
    }
    resolved = get_macro_file_path(b1);
    if (!resolved)
        return (1);
    snprintf(path1, sizeof(path1), "%s", resolved);
    resolved = get_macro_file_path(b2);
    if (!resolved)
        return (1);
    snprintf(path2, sizeof(path2), "%s", resolved);
    resolved = get_macro_file_path(newb);
    if (!resolved)
        return (1);
    snprintf(path_new, sizeof(path_new), "%s", resolved);
    if (access(path1, F_OK) == -1)
    {
        fprintf(stderr, "flow join: source build '%s' does not exist.\n", b1);
        return (1);
    }
    if (access(path2, F_OK) == -1)
    {
        fprintf(stderr, "flow join: source build '%s' does not exist.\n", b2);
        return (1);
    }
    if (access(path_new, F_OK) == 0 && !confirm_build_overwrite(newb))
        return (1);
    f1 = fopen(path1, "r");
    if (!f1)
    {
        perror("flow join: source build 1 not found");
        return (1);
    }
    f2 = fopen(path2, "r");
    if (!f2)
    {
        fclose(f1);
        perror("flow join: source build 2 not found");
        return (1);
    }
    fn = fopen(path_new, "w");
    if (!fn)
    {
        fclose(f1);
        fclose(f2);
        perror("flow join: failed to build target");
        return (1);
    }
    while (fgets(line, sizeof(line), f1))
        fprintf(fn, "%s", line);
    while (fgets(line, sizeof(line), f2))
        fprintf(fn, "%s", line);
    fclose(f1);
    fclose(f2);
    fclose(fn);
    printf("🔀 Workflows '%s' and '%s' joined successfully into '%s'\n",
        b1, b2, newb);
    return (0);
}

/**
 * @brief Duplicates a permanent macro file inside ~/.flow_macros/ completely,
 * ensuring all internal tracking headers remain safe.
 */
int copy_build(const char *oldb, const char *newb)
{
    char path_old[PATH_MAX];
    char path_new[PATH_MAX];
    char line[1024];
    char *resolved;
    FILE *f_old;
    FILE *f_new;

    if (!oldb || !newb || newb[0] == '\0')
    {
        fprintf(stderr, "flow copy: invalid build name.\n");
        return (1);
    }
    if (strcmp(oldb, newb) == 0)
    {
        fprintf(stderr,
            "flow copy: destination build cannot be the same as source build.\n");
        return (1);
    }
    resolved = get_macro_file_path(oldb);
    if (!resolved)
        return (1);
    snprintf(path_old, sizeof(path_old), "%s", resolved);
    resolved = get_macro_file_path(newb);
    if (!resolved)
        return (1);
    snprintf(path_new, sizeof(path_new), "%s", resolved);
    if (access(path_old, F_OK) == -1)
    {
        fprintf(stderr, "flow copy: source build '%s' does not exist.\n", oldb);
        return (1);
    }
    if (access(path_new, F_OK) == 0 && !confirm_build_overwrite(newb))
        return (1);
    f_old = fopen(path_old, "r");
    if (!f_old)
    {
        perror("flow copy: source profile not found");
        return (1);
    }
    f_new = fopen(path_new, "w");
    if (!f_new)
    {
        fclose(f_old);
        perror("flow copy: failed to initialize destination");
        return (1);
    }
    while (fgets(line, sizeof(line), f_old))
        fprintf(f_new, "%s", line);
    fclose(f_old);
    fclose(f_new);
    printf("📋 Workflow '%s' copied successfully to '%s'\n", oldb, newb);
    return (0);
}