NAME = my_tar

CC = gcc
CFLAGS = -Wall -Wextra -Werror

SRC = my_tar.c

all:
	$(CC) $(CFLAGS) $(SRC) -o $(NAME)

clean:
	rm -f $(NAME)

fclean: clean

re: fclean all