#include "flow.h"

#define WRAPPER_FILE "/tmp/.flow_execution_wrapper.sh"

static void load_build_to_workspace(const char *src_path)
{
    FILE *src = fopen(src_path, "r");
    FILE *dst = fopen(CACHE_FILE, "w");
    if (!src || !dst)
    {
        if (src) fclose(src);
        if (dst) fclose(dst);
        return;
    }
    char ch;
    while ((ch = fgetc(src)) != EOF)
        fputc(ch, dst);
    fclose(src);
    fclose(dst);
}

static int parse_run_index(const char *token, int *out)
{
    char    *end;
    long    value;

    if (!token || token[0] == '\0')
        return (-1);
    value = strtol(token, &end, 10);
    if (end == token || *end != '\0')
        return (-1);
    if (value < 0)
        return (-1);
    *out = (int)value;
    return (0);
}

static int is_index_filtered(int current_idx, const char *indices_str)
{
    char    *dup;
    char    *token;
    int     selected;
    int     target_idx;

    if (!indices_str)
        return (1);
    dup = strdup(indices_str);
    if (!dup)
        return (0);
    selected = 0;
    token = strtok(dup, ",");
    while (token)
    {
        if (parse_run_index(token, &target_idx) == 0
            && target_idx == current_idx)
        {
            selected = 1;
            break;
        }
        token = strtok(NULL, ",");
    }
    free(dup);
    return (selected);
}

static void execute_file_pipeline(const char *filepath, const char *indices_str, const char *title, int is_local, int is_workspace)
{
    FILE *macro_fp = fopen(filepath, "r");
    if (!macro_fp) {
        fprintf(stderr, "flow run: Cannot access targeted macro footprint.\n");
        return;
    }
    FILE *wrapper_fp = fopen(WRAPPER_FILE, "w");
    if (!wrapper_fp) {
        fprintf(stderr, "flow run: Cannot create internal execution payload.\n");
        fclose(macro_fp);
        return;
    }
    /* ⚓ AUTO-CONTEXT ALIGNMENT FOR WORKSPACE [0] */
    if (!is_local && is_workspace)
    {
        FILE *path_fp = fopen("/tmp/.flow_recorded_path", "r");
        if (path_fp)
        {
            char birth_path[PATH_MAX];
            if (fgets(birth_path, sizeof(birth_path), path_fp))
            {
                size_t len = strlen(birth_path);
                if (len > 0 && birth_path[len - 1] == '\n')
                    birth_path[len - 1] = '\0';
                fprintf(wrapper_fp, "cd %s\n", birth_path);
            }
            fclose(path_fp);
        }
    }

    if (indices_str)
        printf("🚀 Running %s, showing output for steps: [%s]\n\n", title, indices_str);
    else
        printf("🚀 Running %s completely...\n\n", title);

    char line[1024];
    int current_idx = 0;
    while (fgets(line, sizeof(line), macro_fp))
    {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';

        /* ⚓ AUTO-CONTEXT ALIGNMENT FOR SAVED BUILDS [1] */
        if (strncmp(line, "# dir: ", 7) == 0)
        {
            if (!is_local)
                fprintf(wrapper_fp, "cd %s\n", line + 7);
            continue;
        }

        int print_output = is_index_filtered(current_idx, indices_str);

        if (print_output)
        {
            if (indices_str)
                fprintf(wrapper_fp, "echo \"[%d] Output:\"\n", current_idx);
            else
                fprintf(wrapper_fp, "echo \"Flow[%d] ▶️ : %s\"\n", current_idx, line);
            
            fprintf(wrapper_fp, "%s\n", line);
            
            if (indices_str)
                fprintf(wrapper_fp, "echo \"\"\n");
        }
        else
        {
            fprintf(wrapper_fp, "%s > /dev/null 2>&1\n", line);
        }
        current_idx++;
    }
    fclose(macro_fp);
    fclose(wrapper_fp);

    system("sh " WRAPPER_FILE);
    unlink(WRAPPER_FILE);
}

void run_workspace(const char *indices_str, int is_local)
{
    execute_file_pipeline(CACHE_FILE, indices_str, "current flow workspace [0]", is_local, 1);
}

void run_named_build(const char *name, const char *indices_str, int is_local)
{
    char *path = get_macro_file_path(name);
    if (!path || access(path, F_OK) == -1) {
        fprintf(stderr, "flow run: Named build '%s' does not exist.\n", name);
        return;
    }
    load_build_to_workspace(path);
    execute_file_pipeline(path, indices_str, name, is_local, 0);
}