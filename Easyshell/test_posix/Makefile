# test_posix/Makefile
# Place this file at:  Easyshell/test_posix/Makefile
# Build and run from that directory:
#   make        # build
#   make run    # build + run
#   make clean  # remove artefacts

CC      = cc
CFLAGS  = -Wall -Wextra -g -I../includes -I../libft

LIBS    = ../libft/libft.a -lreadline

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

$(TARGET): $(SRC) ../libft/libft.a
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: run clean
