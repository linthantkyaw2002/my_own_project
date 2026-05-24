#ifndef FLOW_H
#define FLOW_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>

#define STATE_FILE  "/tmp/.flow_active_state"
#define CACHE_FILE  "/tmp/.flow_record_cache"
#define MACRO_DIR   "/.flow_macros"

/* File and Disk Storage Subsystem */
char *get_macro_dir_path(void);
char *get_macro_file_path(const char *name);
int  get_recording_state(void);
void set_recording_state(int state);
void cache_command_string(const char *cmd);
void build_macro_file(const char *name);
void append_to_build(const char *name, const char *cmd);
void insert_into_build(const char *name, int index, const char *cmd);
void delist_from_build(const char *name, const char *index_str);
void list_workspace(void);
void list_builds(void);
void list_named_build(const char *name);
void record_workspace_birth_path(void);

 /* Subcommand Processing Functions */
void    run_workspace(const char *indices_str, int is_local);
void    run_named_build(const char *name, const char *indices_str, int is_local);

/* File System and Storage Management Subroutines */
int     join_builds(const char *b1, const char *b2, const char *newb);
int     copy_build(const char *oldb, const char *newb);

/* Trash Bin and Recovery Management Subroutines */
int     list_bin(void);
int     restore_from_bin(const char *indices_str);
int     remove_build(const char *name);
int     remove_all_builds(void);
int     clear_bin(void);
int     remove_from_bin(const char *indices_str);

#endif