# ==========================================
# Target & Compilers
# ==========================================
NAME		= easyshell
CC			= cc
CFLAGS		= -Wall -Wextra -Werror -g3
RM			= rm -rf

# ==========================================
# Paths & Libraries
# ==========================================
INCLUDES	= -I./includes -I./libft
LIBFT_DIR	= ./libft
LIBFT		= $(LIBFT_DIR)/libft.a

# NOTE: Adjust readline paths if you are on macOS (Homebrew) vs Linux.
# Mac Homebrew typically needs: -I/opt/homebrew/opt/readline/include 
# and -L/opt/homebrew/opt/readline/lib
LDLIBS		= -lreadline
LDFLAGS		= 

# ==========================================
# Directories
# ==========================================
SRC_DIR		= src
OBJ_DIR		= obj

# ==========================================
# Source Files (Mapped to new architecture)
# ==========================================
CORE		= core/main.c core/signals.c
UTILS		= utils/io_utils.c utils/string_utils.c
ENV			= env/env_core.c env/env_ops.c
LEXER		= lexer/tokenizer.c lexer/syntax_check.c lexer/expander.c \
			  lexer/expander_list.c lexer/wildcard.c
PARSER		= parser/ast_builder.c parser/cmd_parser.c parser/cmd_utils.c
EXEC		= executor/exec_ast.c executor/exec_pipeline.c executor/exec_path.c \
			  executor/exec_redirects.c executor/exec_heredoc.c
BUILTINS	= builtins/builtin_hub.c builtins/builtin_dir.c \
			  builtins/builtin_env.c builtins/builtin_sys.c

SRCS		= $(addprefix $(SRC_DIR)/, $(CORE) $(UTILS) $(ENV) $(LEXER) $(PARSER) $(EXEC) $(BUILTINS))
OBJS		= $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# ==========================================
# Rules
# ==========================================

all: $(LIBFT) $(NAME)

$(NAME): $(OBJS) $(LIBFT)
	@echo "Linking $(NAME)..."
	@$(CC) $(CFLAGS) $(OBJS) $(LIBFT) $(LDFLAGS) $(LDLIBS) -o $(NAME)
	@echo "$(NAME) compiled successfully!"

# Create object directories dynamically if they don't exist
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(LIBFT):
	@echo "Compiling Libft..."
	@$(MAKE) -C $(LIBFT_DIR) > /dev/null

clean:
	@echo "Cleaning object files..."
	@$(RM) $(OBJ_DIR)
	@-$(MAKE) -C $(LIBFT_DIR) clean > /dev/null 2>&1 || true

fclean: clean
	@echo "Cleaning executable..."
	@$(RM) $(NAME)
	@-$(MAKE) -C $(LIBFT_DIR) fclean > /dev/null 2>&1 || true

re: fclean all

.PHONY: all clean fclean re