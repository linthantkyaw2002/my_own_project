#include "easyshell.h"
#include "../includes/colors.h"
#include <pwd.h>    /* For getpwuid (Owner name) */
#include <grp.h>    /* For getgrgid (Group name) */
#include <time.h>   /* For localtime & strftime (Timestamp) */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>

/* The new toggle command */
int mini_visual(char **args, t_shell *sh)
{
    if (!args[1])
    {
        printf("Visual mode is %s\n", sh->visual_mode ? "ON" : "OFF");
        return (0);
    }
    if (ft_strcmp(args[1], "on") == 0)
    {
        sh->visual_mode = 1;
        printf("🎨 Visual Engine: %sON%s\n", BOLD, RESET);
    }
    else if (ft_strcmp(args[1], "off") == 0)
    {
        sh->visual_mode = 0;
        printf("🎨 Visual Engine: OFF\n");
    }
    else
    {
        ft_putendl_fd("easyshell: visual: invalid argument (use 'on' or 'off')", 2);
        return (1);
    }
    return (0);
}

/* Helper to print standard POSIX permissions vertically for long-list mode */
static void print_permissions(mode_t mode)
{
    printf("%c", S_ISDIR(mode) ? 'd' : '-');
    printf("%c", (mode & S_IRUSR) ? 'r' : '-');
    printf("%c", (mode & S_IWUSR) ? 'w' : '-');
    printf("%c", (mode & S_IXUSR) ? 'x' : '-');
    printf("%c", (mode & S_IRGRP) ? 'r' : '-');
    printf("%c", (mode & S_IWGRP) ? 'w' : '-');
    printf("%c", (mode & S_IXGRP) ? 'x' : '-');
    printf("%c", (mode & S_IROTH) ? 'r' : '-');
    printf("%c", (mode & S_IWOTH) ? 'w' : '-');
    printf("%c", (mode & S_IXOTH) ? 'x' : '-');
}

/* Recursive helper to draw directories - Full Metadata Long-List Support */
static void list_dir(char *target, int show_all, int recursive, int long_list)
{
    DIR             *dir;
    struct dirent   *entry;
    struct stat     file_stat;
    char            *full_path;
    char            *tmp;
    
    /* Metadata extraction tools */
    struct passwd   *pwd_entry;
    struct group    *grp_entry;
    char            time_buf[64];

    dir = opendir(target);
    if (!dir)
        return (perror("easyshell: ls"));
    
    if (recursive)
        printf("\n\033[1;35m%s:\033[0m\n", target); /* Purple header for sub-dirs */

    /* Pass 1: Print current directory entries with full specifications */
    while ((entry = readdir(dir)) != NULL)
    {
        if (!show_all && entry->d_name[0] == '.')
            continue ;
        tmp = ft_strjoin(target, "/");
        full_path = ft_strjoin(tmp, entry->d_name);
        free(tmp);
        
        if (stat(full_path, &file_stat) == 0)
        {
            if (long_list)
            {
                /* 1. Print Permissions */
                print_permissions(file_stat.st_mode);
                
                /* 2. Print Number of Links */
                printf(" %2hu", file_stat.st_nlink);
                
                /* 3. Resolve and Print Owner Name */
                pwd_entry = getpwuid(file_stat.st_uid);
                if (pwd_entry)
                    printf(" %-8s", pwd_entry->pw_name);
                else
                    printf(" %-8d", file_stat.st_uid);
                
                /* 4. Resolve and Print Group Name */
                grp_entry = getgrgid(file_stat.st_gid);
                if (grp_entry)
                    printf(" %-8s", grp_entry->gr_name);
                else
                    printf(" %-8d", file_stat.st_gid);
                
                /* 5. Print File Size */
                printf(" %8lld", (long long)file_stat.st_size);
                
                /* 6. Format and Print Modification Time (e.g., May 16 21:42) */
                strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", localtime(&file_stat.st_mtime));
                printf(" %s ", time_buf);
                
                /* 7. Inject Visual Icon and Filename Component */
                if (S_ISDIR(file_stat.st_mode))
                    printf("📁 %s%s%s\n", C_DIR, entry->d_name, RESET);
                else if (file_stat.st_mode & S_IXUSR)
                    printf("⚙️ %s%s%s\n", C_EXEC, entry->d_name, RESET);
                else
                    printf("📄 %s%s%s\n", C_FILE, entry->d_name, RESET);
            }
            else
            {
                /* Default Horizontal Grid Layout for clean 'ls' strings */
                if (S_ISDIR(file_stat.st_mode))
                    printf("📁 %s%s%s  ", C_DIR, entry->d_name, RESET);
                else if (file_stat.st_mode & S_IXUSR)
                    printf("⚙️ %s%s%s  ", C_EXEC, entry->d_name, RESET);
                else
                    printf("📄 %s%s%s  ", C_FILE, entry->d_name, RESET);
            }
        }
        free(full_path);
    }
    
    if (!long_list)
        printf("\n");

    /* Pass 2: If recursive, dive into subdirectories */
    if (recursive)
    {
        rewinddir(dir);
        while ((entry = readdir(dir)) != NULL)
        {
            if (ft_strcmp(entry->d_name, ".") == 0 || ft_strcmp(entry->d_name, "..") == 0)
                continue ;
            if (!show_all && entry->d_name[0] == '.')
                continue ;

            tmp = ft_strjoin(target, "/");
            full_path = ft_strjoin(tmp, entry->d_name);
            free(tmp);
            
            if (stat(full_path, &file_stat) == 0 && S_ISDIR(file_stat.st_mode))
                list_dir(full_path, show_all, recursive, long_list);
            free(full_path);
        }
    }
    closedir(dir);
}

/* The smart ls renderer */
int mini_ls(char **args)
{
    char    *target = ".";
    int     show_all = 0;
    int     recursive = 0;
    int     long_list = 0;
    int     i = 1;
    int     j;

    /* Parse arguments dynamically so it handles -R, -a, -la, or -lh variants */
    while (args[i])
    {
        if (args[i][0] == '-')
        {
            j = 1;
            while (args[i][j])
            {
                if (args[i][j] == 'a') show_all = 1;
                if (args[i][j] == 'R') recursive = 1;
                if (args[i][j] == 'l') long_list = 1;
                j++;
            }
        }
        else
            target = args[i];
        i++;
    }
    list_dir(target, show_all, recursive, long_list);
    return (0);
}