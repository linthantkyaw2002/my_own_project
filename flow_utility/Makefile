NAME     = flow
CC       = cc
CFLAGS   = -Wall -Wextra -Werror -I./includes -g

SRC      = src/core/main.c \
           src/core/engine.c \
           src/storage/file_io.c

OBJ      = $(SRC:.c=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

# Installs the tool straight into your global path parameters
install: all
	cp $(NAME) /usr/local/bin/
	chmod 755 /usr/local/bin/$(NAME)
	@echo "🚀 'flow' is now installed globally as a cross-shell utility!"

.PHONY: all clean fclean re install