# test_posix/Makefile
# Place this file at:  Easyshell/test_posix/Makefile

CC      = cc

# ==============================================================================
# DYNAMIC READLINE DETECTOR FOR MACOS (POSIX COMPLIANCE)
# ==============================================================================
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S), Darwin)
    # Try to locate the Homebrew prefix dynamically via the brew command
    BREW_PREFIX := $(shell which brew >/dev/null 2>&1 && brew --prefix || echo "")
    
    ifneq ($(BREW_PREFIX),)
        READLINE_INC = -I$(BREW_PREFIX)/opt/readline/include
        READLINE_LIB = -L$(BREW_PREFIX)/opt/readline/lib
    else
        # Hard fallbacks if the user's terminal environment variable path isn't exposed to make
        ifneq ($(wildcard /opt/homebrew/opt/readline),)
            READLINE_INC = -I/opt/homebrew/opt/readline/include
            READLINE_LIB = -L/opt/homebrew/opt/readline/lib
        else ifneq ($(wildcard /usr/local/opt/readline),)
            READLINE_INC = -I/usr/local/opt/readline/include
            READLINE_LIB = -L/usr/local/opt/readline/lib
        endif
    endif
endif

# ==============================================================================
# COMPILER CONFIGURATIONS
# ==============================================================================
CFLAGS  = -Wall -Wextra -g -I../includes -I../libft $(READLINE_INC)
LIBS    = ../libft/libft.a $(READLINE_LIB) -lreadline

SRC     = test_posix_exit_codes.c \
          ../src/builtins/builtin_sys.c \
          ../src/builtins/builtin_dir.c \
          ../src/builtins/builtin_env.c \
          ../src/builtins/builtin_hub.c \
          ../src/builtins/builtin_vls.c \
          ../src/env/env_core.c \
          ../src/env/env_ops.c \
          ../src/executor/exec_path.c \
          ../src/executor/exec_pipeline.c \
          ../src/executor/exec_ast.c \
          ../src/executor/exec_redirects.c \
          ../src/executor/exec_heredoc.c \
          ../src/parser/ast_builder.c \
          ../src/parser/cmd_parser.c \
          ../src/parser/cmd_utils.c \
          ../src/lexer/expander.c \
          ../src/lexer/expander_list.c \
          ../src/lexer/wildcard.c \
          ../src/core/signals.c \
          ../src/utils/string_utils.c \
          ../src/utils/io_utils.c

TARGET  = test_posix

# ==============================================================================
# RULES
# ==============================================================================
$(TARGET): $(SRC) ../libft/libft.a
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: run clean