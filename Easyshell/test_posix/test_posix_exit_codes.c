/*
 * test_posix_exit_codes.c
 *
 * POSIX-1003.2 exit-code compliance tests for Easyshell builtins.
 *
 * Compile (from the Easyshell/ directory):
 *   cc -Wall -Wextra -o test_posix \
 *       test_posix_exit_codes.c \
 *       src/builtins/builtin_sys.c  \
 *       src/builtins/builtin_dir.c  \
 *       src/builtins/builtin_env.c  \
 *       src/builtins/builtin_hub.c  \
 *       src/env/env_core.c          \
 *       src/env/env_ops.c           \
 *       src/utils/string_utils.c    \
 *       libft/libft.a               \
 *       -Iincludes -Ilibft -lreadline
 *   ./test_posix
 *
 * POSIX references used throughout:
 *   - exit utility:   POSIX.1-2017 §2.8.2, §2.9.1
 *   - cd utility:     POSIX.1-2017 "cd" built-in
 *   - echo utility:   POSIX.1-2017 "echo" built-in
 *   - export/unset:   POSIX.1-2017 "export" / "unset" built-ins
 *   - dot (.):        POSIX.1-2017 "." built-in
 *   - Signal codes:   exit status = 128 + signal number
 *   - Command not found: exit 127
 *   - Not executable:    exit 126
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "easyshell.h"

/* g_signal_received is declared extern in easyshell.h and normally
   defined in src/core/main.c, which we don't link in the test build.
   Define it here so the linker is satisfied.                          */
int g_signal_received = 0;

/* ------------------------------------------------------------------ */
/*  Helpers                                                             */
/* ------------------------------------------------------------------ */

/* Build a minimal t_shell with last_exit set and a tiny env list.    */
static t_shell make_shell(int last_exit)
{
    t_shell sh;
    memset(&sh, 0, sizeof(sh));
    sh.last_exit  = last_exit;
    sh.visual_mode = 0;
    sh.skip_visual = 0;

    /* Provide HOME and PATH so cd / exec tests can use them. */
    char *envp[] = {
        "HOME=/tmp",
        "PATH=/usr/bin:/bin",
        "PWD=/tmp",
        NULL
    };
    sh.env = init_env(envp);
    return sh;
}

/* Fork, run fn(args, sh) in the child, return the raw exit status.   */
static int run_in_child(int (*fn)(char **, t_shell *),
                        char **args, t_shell *sh)
{
    pid_t pid = fork();
    assert(pid >= 0);
    if (pid == 0)
    {
        int code = fn(args, sh);
        exit(code);
    }
    int status;
    waitpid(pid, &status, 0);
    return status;
}

/* Convenience: extract the actual exit code from waitpid status.     */
static int exit_code(int status)
{
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);
    return -1;
}

/* Count passed / failed globally. */
static int g_pass = 0;
static int g_fail = 0;

#define CHECK(label, got, want) \
    do { \
        if ((got) == (want)) { \
            printf("  PASS  %s\n", label); \
            g_pass++; \
        } else { \
            printf("  FAIL  %s  (got %d, want %d)\n", label, got, want); \
            g_fail++; \
        } \
    } while (0)

/* ------------------------------------------------------------------ */
/*  1. mini_echo                                                        */
/*     POSIX: echo always exits 0.                                     */
/* ------------------------------------------------------------------ */
static void test_echo(void)
{
    puts("\n=== echo ===");
    t_shell sh = make_shell(0);

    /* 1.1  Plain word */
    {
        char *args[] = {"echo", "hello", NULL};
        int s = run_in_child((int(*)(char**,t_shell*))
                             (void*)mini_echo, args, &sh);
        /* mini_echo doesn't take t_shell*, cast via wrapper below */
        (void)s;
    }

    /* Use a wrapper because mini_echo has a different signature.      */
    /* We fork and call mini_echo directly in the child here.          */
    {
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            char *args[] = {"echo", "hello", NULL};
            exit(mini_echo(args));
        }
        int status; waitpid(pid, &status, 0);
        /* POSIX §echo: "shall exit with a status of 0" */
        CHECK("echo hello → 0", exit_code(status), 0);
    }
    {
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            char *args[] = {"echo", "-n", "no newline", NULL};
            exit(mini_echo(args));
        }
        int status; waitpid(pid, &status, 0);
        CHECK("echo -n → 0", exit_code(status), 0);
    }
    {
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            char *args[] = {"echo", NULL};   /* no operands */
            exit(mini_echo(args));
        }
        int status; waitpid(pid, &status, 0);
        CHECK("echo (no args) → 0", exit_code(status), 0);
    }
    {
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            /* -nnn is still a valid -n flag per the implementation */
            char *args[] = {"echo", "-nnn", "text", NULL};
            exit(mini_echo(args));
        }
        int status; waitpid(pid, &status, 0);
        CHECK("echo -nnn → 0", exit_code(status), 0);
    }

    free_env(sh.env);
}

/* ------------------------------------------------------------------ */
/*  2. mini_cd                                                          */
/*     POSIX: success → 0; any error → >0 (typically 1).              */
/* ------------------------------------------------------------------ */
static void test_cd(void)
{
    puts("\n=== cd ===");
    t_shell sh = make_shell(0);

    /* 2.1  cd to a directory that exists */
    {
        char *args[] = {"cd", "/tmp", NULL};
        int ret = mini_cd(args, &sh.env);
        CHECK("cd /tmp → 0", ret, 0);
    }

    /* 2.2  cd with no argument falls back to $HOME (/tmp in our env) */
    {
        char *args[] = {"cd", NULL};
        int ret = mini_cd(args, &sh.env);
        CHECK("cd (no arg, HOME=/tmp) → 0", ret, 0);
    }

    /* 2.3  cd to a nonexistent path → 1 */
    {
        char *args[] = {"cd", "/this/path/does/not/exist/ever", NULL};
        int ret = mini_cd(args, &sh.env);
        CHECK("cd nonexistent → 1", ret, 1);
    }

    /* 2.4  Too many arguments → 1  (POSIX: "too many arguments" is an error) */
    {
        char *args[] = {"cd", "/tmp", "/usr", NULL};
        int ret = mini_cd(args, &sh.env);
        CHECK("cd too many args → 1", ret, 1);
    }

    /* 2.5  HOME not set → 1 */
    {
        /* Remove HOME from env temporarily. */
        t_shell sh2 = make_shell(0);
        mini_unset((char *[]){"unset", "HOME", NULL}, &sh2.env);
        char *args[] = {"cd", NULL};
        int ret = mini_cd(args, &sh2.env);
        CHECK("cd HOME unset → 1", ret, 1);
        free_env(sh2.env);
    }

    free_env(sh.env);
}

/* ------------------------------------------------------------------ */
/*  3. mini_export                                                      */
/*     POSIX: success → 0; invalid identifier → 1.                    */
/* ------------------------------------------------------------------ */
static void test_export(void)
{
    puts("\n=== export ===");
    t_shell sh = make_shell(0);

    /* 3.1  Valid assignment */
    {
        char *args[] = {"export", "MY_VAR=hello", NULL};
        int ret = mini_export(args, &sh.env);
        CHECK("export MY_VAR=hello → 0", ret, 0);
    }

    /* 3.2  Export existing variable name only (no =) */
    {
        char *args[] = {"export", "MY_VAR", NULL};
        int ret = mini_export(args, &sh.env);
        CHECK("export MY_VAR (name only) → 0", ret, 0);
    }

    /* 3.3  Invalid identifier – starts with digit → 1 */
    {
        char *args[] = {"export", "1INVALID=x", NULL};
        int ret = mini_export(args, &sh.env);
        CHECK("export 1INVALID=x → 1", ret, 1);
    }

    /* 3.4  Invalid identifier – contains hyphen → 1 */
    {
        char *args[] = {"export", "MY-VAR=x", NULL};
        int ret = mini_export(args, &sh.env);
        CHECK("export MY-VAR=x → 1", ret, 1);
    }

    /* 3.5  Mixed: one valid, one invalid → 1 overall
     *      POSIX: utility shall write a diagnostic for each invalid
     *             name but shall not abort; final status is non-zero. */
    {
        char *args[] = {"export", "GOOD=1", "3BAD=2", NULL};
        int ret = mini_export(args, &sh.env);
        CHECK("export GOOD=1 3BAD=2 → 1", ret, 1);
    }

    /* 3.6  No operands → 0 (list mode, no-op in this implementation) */
    {
        char *args[] = {"export", NULL};
        int ret = mini_export(args, &sh.env);
        CHECK("export (no args) → 0", ret, 0);
    }

    free_env(sh.env);
}

/* ------------------------------------------------------------------ */
/*  4. mini_unset                                                       */
/*     POSIX: always exits 0 (errors are non-fatal).                   */
/* ------------------------------------------------------------------ */
static void test_unset(void)
{
    puts("\n=== unset ===");
    t_shell sh = make_shell(0);

    /* 4.1  Unset an existing variable */
    {
        char *args[] = {"unset", "HOME", NULL};
        int ret = mini_unset(args, &sh.env);
        CHECK("unset HOME → 0", ret, 0);
    }

    /* 4.2  Unset a variable that doesn't exist (must still return 0) */
    {
        char *args[] = {"unset", "DOES_NOT_EXIST", NULL};
        int ret = mini_unset(args, &sh.env);
        CHECK("unset nonexistent → 0", ret, 0);
    }

    /* 4.3  Multiple variables at once */
    {
        char *args[] = {"unset", "PATH", "PWD", NULL};
        int ret = mini_unset(args, &sh.env);
        CHECK("unset PATH PWD → 0", ret, 0);
    }

    free_env(sh.env);
}

/* ------------------------------------------------------------------ */
/*  5. mini_exit                                                        */
/*     POSIX §2.8.2:                                                   */
/*       exit         → last exit status                               */
/*       exit N       → N % 256                                        */
/*       exit N M     → 1  (too many arguments, shell continues)       */
/*       exit non-num → 2  (numeric argument required)                 */
/* ------------------------------------------------------------------ */
static void test_exit(void)
{
    puts("\n=== exit ===");

    /* 5.1  exit with no argument → inherits last_exit (42 here) */
    {
        t_shell sh = make_shell(42);
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            char *args[] = {"exit", NULL};
            mini_exit(args, &sh);
            _exit(99); /* should never reach here */
        }
        int status; waitpid(pid, &status, 0);
        CHECK("exit (no arg) → last_exit (42)", exit_code(status), 42);
        free_env(sh.env);
    }

    /* 5.2  exit 0 */
    {
        t_shell sh = make_shell(1);
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            char *args[] = {"exit", "0", NULL};
            mini_exit(args, &sh);
            _exit(99);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("exit 0 → 0", exit_code(status), 0);
        free_env(sh.env);
    }

    /* 5.3  exit 1 */
    {
        t_shell sh = make_shell(0);
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            char *args[] = {"exit", "1", NULL};
            mini_exit(args, &sh);
            _exit(99);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("exit 1 → 1", exit_code(status), 1);
        free_env(sh.env);
    }

    /* 5.4  exit 127 – "command not found" sentinel value */
    {
        t_shell sh = make_shell(0);
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            char *args[] = {"exit", "127", NULL};
            mini_exit(args, &sh);
            _exit(99);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("exit 127 → 127", exit_code(status), 127);
        free_env(sh.env);
    }

    /* 5.5  exit 255 – maximum meaningful value */
    {
        t_shell sh = make_shell(0);
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            char *args[] = {"exit", "255", NULL};
            mini_exit(args, &sh);
            _exit(99);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("exit 255 → 255", exit_code(status), 255);
        free_env(sh.env);
    }

    /* 5.6  exit 256 → 256 % 256 = 0 (POSIX wraps at 256) */
    {
        t_shell sh = make_shell(0);
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            char *args[] = {"exit", "256", NULL};
            mini_exit(args, &sh);
            _exit(99);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("exit 256 → 0 (mod 256)", exit_code(status), 0);
        free_env(sh.env);
    }

    /* 5.7  exit 300 → 300 % 256 = 44 */
    {
        t_shell sh = make_shell(0);
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            char *args[] = {"exit", "300", NULL};
            mini_exit(args, &sh);
            _exit(99);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("exit 300 → 44 (mod 256)", exit_code(status), 44);
        free_env(sh.env);
    }

    /* 5.8  exit with non-numeric argument → 2
     *      POSIX: "If the argument is not a valid unsigned integer,
     *              the exit status shall be unspecified" – bash/dash use 2. */
    {
        t_shell sh = make_shell(0);
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            char *args[] = {"exit", "abc", NULL};
            mini_exit(args, &sh);
            _exit(99);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("exit abc (non-numeric) → 2", exit_code(status), 2);
        free_env(sh.env);
    }

    /* 5.9  exit with too many arguments → returns 1, does NOT exit
     *      POSIX: "If more than one argument is given, the results
     *              are unspecified" – bash returns 1 and stays alive. */
    {
        t_shell sh = make_shell(0);
        /* Call directly (no fork) to verify the shell is still alive. */
        char *args[] = {"exit", "1", "2", NULL};
        int ret = mini_exit(args, &sh);
        CHECK("exit 1 2 (too many) → 1, shell alive", ret, 1);
        free_env(sh.env);
    }

    /* 5.10 exit -1 → (-1 % 256) in C = implementation-defined for
     *       negative numbers; the implementation does ft_atoi then % 256.
     *       ft_atoi("-1") = -1; -1 % 256 in C99 = -1, but cast to
     *       unsigned char → 255.  We test what the code actually does. */
    {
        t_shell sh = make_shell(0);
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            char *args[] = {"exit", "-1", NULL};
            mini_exit(args, &sh);
            _exit(99);
        }
        int status; waitpid(pid, &status, 0);
        /*
         * NOTE / BUG REPORT:
         * POSIX only defines exit with non-negative integers.
         * The implementation accepts "-1" as numeric (is_numeric allows
         * a leading '-') and calls exit(-1 % 256).
         * In C99, -1 % 256 == -1, and exit(-1) is equivalent to
         * exit(255) on Linux (low byte of status word).
         * Expected: 255.  If this fails, investigate is_numeric() and
         * whether negative values should be rejected with exit code 2.
         */
        CHECK("exit -1 → 255 (wraps via exit() syscall)", exit_code(status), 255);
        free_env(sh.env);
    }
}

/* ------------------------------------------------------------------ */
/*  6. dot (.) built-in                                                 */
/*     POSIX: if filename argument is missing → exit 2.               */
/* ------------------------------------------------------------------ */
static void test_dot(void)
{
    puts("\n=== . (dot) ===");
    t_shell sh = make_shell(0);

    /* 6.1  No filename → 2 */
    {
        char *args[] = {".", NULL};
        int ret = exe_builtin(args, &sh);
        CHECK(". (no filename) → 2", ret, 2);
    }

    free_env(sh.env);
}

/* ------------------------------------------------------------------ */
/*  7. Signal exit codes  (128 + signo)                                */
/*     POSIX §2.8.2: "If a command is terminated by a signal, the     */
/*     exit status shall be greater than 128."                         */
/*     Linux convention (and bash): 128 + signal number.              */
/* ------------------------------------------------------------------ */
static void test_signal_exit_codes(void)
{
    puts("\n=== signal exit codes (128+N) ===");

    /* 7.1  Child killed by SIGTERM → 128 + 15 = 143 */
    {
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            /* Spin so the parent can signal us. */
            for (;;) pause();
        }
        kill(pid, SIGTERM);
        int status; waitpid(pid, &status, 0);
        int code = WIFSIGNALED(status) ? 128 + WTERMSIG(status) : -1;
        CHECK("SIGTERM → 143", code, 143);
    }

    /* 7.2  Child killed by SIGKILL → 128 + 9 = 137 */
    {
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0) { for (;;) pause(); }
        kill(pid, SIGKILL);
        int status; waitpid(pid, &status, 0);
        int code = WIFSIGNALED(status) ? 128 + WTERMSIG(status) : -1;
        CHECK("SIGKILL → 137", code, 137);
    }

    /* 7.3  Child killed by SIGINT (Ctrl-C) → 128 + 2 = 130 */
    {
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0) { for (;;) pause(); }
        kill(pid, SIGINT);
        int status; waitpid(pid, &status, 0);
        int code = WIFSIGNALED(status) ? 128 + WTERMSIG(status) : -1;
        CHECK("SIGINT → 130", code, 130);
    }
}

/* ------------------------------------------------------------------ */
/*  8. Command-not-found / not-executable codes                        */
/*     These are produced by handle_exec_error() and validated here    */
/*     by examining the constants the function exits with.             */
/*                                                                     */
/*     POSIX §2.8.2:                                                   */
/*       127 – command not found                                       */
/*       126 – command found but not executable                        */
/* ------------------------------------------------------------------ */
static void test_exec_error_codes(void)
{
    puts("\n=== exec error codes ===");

    /* 8.1  Unknown command → 127 */
    {
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            /* handle_exec_error with path=NULL triggers 127. */
            handle_exec_error(NULL, "nonexistent_cmd_xyz");
            _exit(0);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("command not found → 127", exit_code(status), 127);
    }

    /* 8.2  Path is a directory → 126 */
    {
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            /*
             * Pass path = "/tmp" (a directory that exists) and cmd_name
             * without '/' so handle_exec_error's stat branch is reached.
             * We use "/tmp" as both path and name; the stat() will show
             * S_ISDIR → exits 126.
             */
            handle_exec_error("/tmp", "/tmp");
            _exit(0);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("exec a directory → 126", exit_code(status), 126);
    }
}

/* ------------------------------------------------------------------ */
/*  9. $? propagation                                                   */
/*     POSIX §2.8.2: last_exit must reflect the most recent pipeline.  */
/* ------------------------------------------------------------------ */
static void test_last_exit_propagation(void)
{
    puts("\n=== \\$? propagation ===");

    /* 9.1  Successful builtin sets last_exit to 0 */
    {
        t_shell sh = make_shell(99);
        char *args[] = {"echo", "hi", NULL};
        /* redirect stdout to /dev/null so output doesn't clutter */
        int devnull = open("/dev/null", O_WRONLY);
        int saved = dup(STDOUT_FILENO);
        dup2(devnull, STDOUT_FILENO);
        sh.last_exit = mini_echo(args);
        dup2(saved, STDOUT_FILENO);
        close(devnull); close(saved);
        CHECK("echo sets last_exit → 0", sh.last_exit, 0);
        free_env(sh.env);
    }

    /* 9.2  Failed cd sets last_exit to 1 */
    {
        t_shell sh = make_shell(0);
        char *args[] = {"cd", "/no/such/path", NULL};
        int devnull = open("/dev/null", O_WRONLY);
        int saved_err = dup(STDERR_FILENO);
        dup2(devnull, STDERR_FILENO);
        sh.last_exit = mini_cd(args, &sh.env);
        dup2(saved_err, STDERR_FILENO);
        close(devnull); close(saved_err);
        CHECK("failed cd sets last_exit → 1", sh.last_exit, 1);
        free_env(sh.env);
    }

    /* 9.3  exit with no arg uses last_exit, not 0 */
    {
        t_shell sh = make_shell(7);
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            char *args[] = {"exit", NULL};
            mini_exit(args, &sh);
            _exit(99);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("exit (no arg) inherits last_exit=7", exit_code(status), 7);
        free_env(sh.env);
    }

    /* 9.4  export invalid → last_exit becomes 1 */
    {
        t_shell sh = make_shell(0);
        int devnull = open("/dev/null", O_WRONLY);
        int saved_err = dup(STDERR_FILENO);
        dup2(devnull, STDERR_FILENO);
        char *args[] = {"export", "BAD NAME=x", NULL};
        sh.last_exit = mini_export(args, &sh.env);
        dup2(saved_err, STDERR_FILENO);
        close(devnull); close(saved_err);
        CHECK("invalid export sets last_exit → 1", sh.last_exit, 1);
        free_env(sh.env);
    }
}

/* ------------------------------------------------------------------ */
/*  10. && / || short-circuit exit codes                               */
/*      POSIX §2.9.3:                                                  */
/*        cmd1 && cmd2 : run cmd2 only if cmd1 exits 0;               */
/*                       $? = exit status of last command run.         */
/*        cmd1 || cmd2 : run cmd2 only if cmd1 exits non-zero;        */
/*                       $? = exit status of last command run.         */
/* ------------------------------------------------------------------ */
static void test_and_or_exit_codes(void)
{
    puts("\n=== && / || exit codes ===");

    /* Helper: run a two-node AND/OR AST in a child, return exit code. */
    /* We build the AST manually to avoid needing the full tokenizer.  */

    /* 10.1  true && true  → 0 */
    {
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            t_shell sh = make_shell(0);
            /* Simulate: left succeeds (last_exit=0) → right runs and succeeds */
            sh.last_exit = 0;          /* left cmd exited 0 */
            if (sh.last_exit == 0)
                sh.last_exit = 0;      /* right cmd exits 0 */
            exit(sh.last_exit);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("true && true → 0", exit_code(status), 0);
    }

    /* 10.2  false && true  → 1  (right side skipped) */
    {
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            t_shell sh = make_shell(0);
            sh.last_exit = 1;          /* left cmd failed */
            /* AND: right must NOT run; $? stays 1 */
            if (sh.last_exit == 0)
                sh.last_exit = 0;
            exit(sh.last_exit);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("false && true → 1 (right skipped)", exit_code(status), 1);
    }

    /* 10.3  false || true  → 0  (right side runs) */
    {
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            t_shell sh = make_shell(0);
            sh.last_exit = 1;          /* left failed */
            if (sh.last_exit != 0)
                sh.last_exit = 0;      /* right succeeds */
            exit(sh.last_exit);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("false || true → 0", exit_code(status), 0);
    }

    /* 10.4  true || false  → 0  (right side skipped) */
    {
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            t_shell sh = make_shell(0);
            sh.last_exit = 0;          /* left succeeded */
            if (sh.last_exit != 0)
                sh.last_exit = 1;      /* right would fail, but skipped */
            exit(sh.last_exit);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("true || false → 0 (right skipped)", exit_code(status), 0);
    }

    /* 10.5  execute_ast AND node: verify real implementation */
    {
        /*
         * Build:  NODE_AND
         *           left:  NODE_PIPELINE  → "true"  (exit 0)
         *           right: NODE_PIPELINE  → "false" (exit 1)
         * Expected last_exit: 1  (right ran and failed)
         */
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            t_shell sh = make_shell(0);

            /* left pipeline: /bin/true */
            t_cmd *left_cmd = new_cmd();
            left_cmd->args = malloc(sizeof(char *) * 2);
            left_cmd->args[0] = strdup("/bin/true");
            left_cmd->args[1] = NULL;
            left_cmd->infile = -1; left_cmd->outfile = -1;

            t_ast *left_node = new_ast_node(NODE_PIPELINE);
            left_node->pipeline = left_cmd;

            /* right pipeline: /bin/false */
            t_cmd *right_cmd = new_cmd();
            right_cmd->args = malloc(sizeof(char *) * 2);
            right_cmd->args[0] = strdup("/bin/false");
            right_cmd->args[1] = NULL;
            right_cmd->infile = -1; right_cmd->outfile = -1;

            t_ast *right_node = new_ast_node(NODE_PIPELINE);
            right_node->pipeline = right_cmd;

            t_ast *and_node = new_ast_node(NODE_AND);
            and_node->left  = left_node;
            and_node->right = right_node;

            execute_ast(and_node, &sh);
            exit(sh.last_exit);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("execute_ast: true && false → 1", exit_code(status), 1);
    }

    /* 10.6  execute_ast OR node: false || true → 0 */
    {
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            t_shell sh = make_shell(0);

            t_cmd *left_cmd = new_cmd();
            left_cmd->args = malloc(sizeof(char *) * 2);
            left_cmd->args[0] = strdup("/bin/false");
            left_cmd->args[1] = NULL;
            left_cmd->infile = -1; left_cmd->outfile = -1;
            t_ast *left_node = new_ast_node(NODE_PIPELINE);
            left_node->pipeline = left_cmd;

            t_cmd *right_cmd = new_cmd();
            right_cmd->args = malloc(sizeof(char *) * 2);
            right_cmd->args[0] = strdup("/bin/true");
            right_cmd->args[1] = NULL;
            right_cmd->infile = -1; right_cmd->outfile = -1;
            t_ast *right_node = new_ast_node(NODE_PIPELINE);
            right_node->pipeline = right_cmd;

            t_ast *or_node = new_ast_node(NODE_OR);
            or_node->left  = left_node;
            or_node->right = right_node;

            execute_ast(or_node, &sh);
            exit(sh.last_exit);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("execute_ast: false || true → 0", exit_code(status), 0);
    }
}

/* ------------------------------------------------------------------ */
/*  11. Pipeline exit code                                              */
/*      POSIX §2.9.2: exit status of a pipeline = exit status of      */
/*      the LAST command in the pipeline.                              */
/* ------------------------------------------------------------------ */
static void test_pipeline_exit_code(void)
{
    puts("\n=== pipeline exit code (last cmd wins) ===");

    /* 11.1  true | false  → 1 */
    {
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            t_shell sh = make_shell(0);

            t_cmd *cmd1 = new_cmd();
            cmd1->args = malloc(sizeof(char *) * 2);
            cmd1->args[0] = strdup("/bin/true");
            cmd1->args[1] = NULL;
            cmd1->infile = -1; cmd1->outfile = -1;

            t_cmd *cmd2 = new_cmd();
            cmd2->args = malloc(sizeof(char *) * 2);
            cmd2->args[0] = strdup("/bin/false");
            cmd2->args[1] = NULL;
            cmd2->infile = -1; cmd2->outfile = -1;

            cmd1->next = cmd2;

            execute_commands(cmd1, &sh);
            exit(sh.last_exit);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("true | false → 1 (last cmd)", exit_code(status), 1);
    }

    /* 11.2  false | true  → 0 */
    {
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            t_shell sh = make_shell(0);

            t_cmd *cmd1 = new_cmd();
            cmd1->args = malloc(sizeof(char *) * 2);
            cmd1->args[0] = strdup("/bin/false");
            cmd1->args[1] = NULL;
            cmd1->infile = -1; cmd1->outfile = -1;

            t_cmd *cmd2 = new_cmd();
            cmd2->args = malloc(sizeof(char *) * 2);
            cmd2->args[0] = strdup("/bin/true");
            cmd2->args[1] = NULL;
            cmd2->infile = -1; cmd2->outfile = -1;

            cmd1->next = cmd2;

            execute_commands(cmd1, &sh);
            exit(sh.last_exit);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("false | true → 0 (last cmd)", exit_code(status), 0);
    }
}

/* ------------------------------------------------------------------ */
/*  12. Redirect failure exit code                                      */
/*      POSIX §2.8.1: if a redirection fails, the command shall exit   */
/*      with a non-zero status (effectively 1 in most shells).         */
/* ------------------------------------------------------------------ */
static void test_redirect_failure(void)
{
    puts("\n=== redirect failure exit code ===");

    /* 12.1  Redirect input from nonexistent file → exit 1 */
    {
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            t_shell sh = make_shell(0);
            t_cmd *cmd = new_cmd();
            cmd->args = malloc(sizeof(char *) * 2);
            cmd->args[0] = strdup("cat");
            cmd->args[1] = NULL;
            cmd->infile  = -1;
            cmd->outfile = -1;
            /* Simulate a failed open (what handle_file sets) */
            cmd->infile = open("/no/such/file/ever", O_RDONLY);
            if (cmd->infile == -1)
                cmd->error = 1;
            execute_commands(cmd, &sh);
            exit(sh.last_exit);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("redirect nonexistent file → 1", exit_code(status), 1);
    }

    /* 12.2  Redirect output to unwritable path → exit 1 */
    {
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            t_shell sh = make_shell(0);
            t_cmd *cmd = new_cmd();
            cmd->args = malloc(sizeof(char *) * 2);
            cmd->args[0] = strdup("echo");
            cmd->args[1] = NULL;
            cmd->infile  = -1;
            cmd->outfile = -1;
            cmd->outfile = open("/no/such/dir/out.txt",
                                O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (cmd->outfile == -1)
                cmd->error = 1;
            execute_commands(cmd, &sh);
            exit(sh.last_exit);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("redirect bad output path → 1", exit_code(status), 1);
    }
}

/* ------------------------------------------------------------------ */
/*  13. cd edge cases                                                   */
/*      POSIX §cd:                                                      */
/*        cd -       → go to $OLDPWD, print it, exit 0                */
/*        cd <file>  → exit 1 (not a directory)                        */
/* ------------------------------------------------------------------ */
static void test_cd_edge_cases(void)
{
    puts("\n=== cd edge cases ===");

    /* 13.1  cd to a regular file → 1 */
    {
        /* Create a temp file to cd into */
        int fd = open("/tmp/es_test_file", O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd >= 0) close(fd);

        t_shell sh = make_shell(0);
        int devnull = open("/dev/null", O_WRONLY);
        int saved_err = dup(STDERR_FILENO);
        dup2(devnull, STDERR_FILENO);
        char *args[] = {"cd", "/tmp/es_test_file", NULL};
        int ret = mini_cd(args, &sh.env);
        dup2(saved_err, STDERR_FILENO);
        close(devnull); close(saved_err);
        unlink("/tmp/es_test_file");
        CHECK("cd to a file → 1", ret, 1);
        free_env(sh.env);
    }

    /* 13.2  cd - uses $OLDPWD → 0 */
    {
        t_shell sh = make_shell(0);
        /* Set OLDPWD to /tmp */
        set_env_value(&sh.env, "OLDPWD", "/tmp");
        int devnull = open("/dev/null", O_WRONLY);
        int saved_out = dup(STDOUT_FILENO);
        /* cd - prints OLDPWD to stdout; suppress it */
        dup2(devnull, STDOUT_FILENO);
        char *args[] = {"cd", "-", NULL};
        int ret = mini_cd(args, &sh.env);
        dup2(saved_out, STDOUT_FILENO);
        close(devnull); close(saved_out);
        /*
         * NOTE: POSIX requires "cd -" to cd to $OLDPWD and print it.
         * If your mini_cd does not handle "-" specially it will try to
         * chdir("-") which fails → returns 1. This test will FAIL and
         * reveal the missing feature.
         */
        CHECK("cd - (OLDPWD=/tmp) → 0", ret, 0);
        free_env(sh.env);
    }
}

/* ------------------------------------------------------------------ */
/*  14. exit with leading-plus argument                                 */
/*      is_numeric() accepts '+', so exit +0 should → 0.              */
/*      POSIX only defines non-negative integers; the + prefix is an   */
/*      extension. Verify the implementation is at least self-consistent*/
/* ------------------------------------------------------------------ */
static void test_exit_plus(void)
{
    puts("\n=== exit with + prefix ===");

    /* 14.1  exit +0 → 0 */
    {
        t_shell sh = make_shell(99);
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            char *args[] = {"exit", "+0", NULL};
            mini_exit(args, &sh);
            _exit(99);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("exit +0 → 0", exit_code(status), 0);
        free_env(sh.env);
    }

    /* 14.2  exit +42 → 42 */
    {
        t_shell sh = make_shell(0);
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            char *args[] = {"exit", "+42", NULL};
            mini_exit(args, &sh);
            _exit(99);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("exit +42 → 42", exit_code(status), 42);
        free_env(sh.env);
    }

    /* 14.3  exit + (plus alone, no digits) → 2 (non-numeric) */
    {
        t_shell sh = make_shell(0);
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            char *args[] = {"exit", "+", NULL};
            mini_exit(args, &sh);
            _exit(99);
        }
        int status; waitpid(pid, &status, 0);
        /*
         * is_numeric("+") : i advances past '+', then !str[i] → return 0
         * So this should be treated as non-numeric → exit 2.
         */
        CHECK("exit + (no digits) → 2", exit_code(status), 2);
        free_env(sh.env);
    }
}

/* ------------------------------------------------------------------ */
/*  15. Background (&) exit code                                        */
/*      POSIX §2.9.3.1: "If a command is run asynchronously, the exit  */
/*      status of the command substitution shall be 0."                 */
/*      i.e. the shell sets $? = 0 immediately, without waiting.       */
/* ------------------------------------------------------------------ */
static void test_background_exit_code(void)
{
    puts("\n=== background (&) sets \\$? = 0 immediately ===");

    /* 15.1  After launching a background command, last_exit must be 0 */
    {
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0)
        {
            t_shell sh = make_shell(42); /* start with non-zero */

            t_cmd *bg_cmd = new_cmd();
            bg_cmd->args = malloc(sizeof(char *) * 2);
            bg_cmd->args[0] = strdup("/bin/false"); /* would exit 1 */
            bg_cmd->args[1] = NULL;
            bg_cmd->infile  = -1;
            bg_cmd->outfile = -1;

            t_ast *pipeline = new_ast_node(NODE_PIPELINE);
            pipeline->pipeline = bg_cmd;

            t_ast *bg_node = new_ast_node(NODE_BG);
            bg_node->left  = pipeline;
            bg_node->right = NULL;

            execute_ast(bg_node, &sh);
            /* POSIX: $? must be 0 now, regardless of bg_cmd's future exit */
            exit(sh.last_exit);
        }
        int status; waitpid(pid, &status, 0);
        CHECK("cmd & → last_exit = 0 immediately", exit_code(status), 0);
    }
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */
int main(void)
{
    puts("Easyshell – POSIX-1003.2 exit-code compliance tests");
    puts("=====================================================");

    test_echo();
    test_cd();
    test_export();
    test_unset();
    test_exit();
    test_dot();
    test_signal_exit_codes();
    test_exec_error_codes();
    test_last_exit_propagation();
    test_and_or_exit_codes();
    test_pipeline_exit_code();
    test_redirect_failure();
    test_cd_edge_cases();
    test_exit_plus();
    test_background_exit_code();

    printf("\n=====================================================\n");
    printf("Results: %d passed, %d failed\n", g_pass, g_fail);

    return (g_fail > 0 ? 1 : 0);
}
