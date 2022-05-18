CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic
CC=gcc
NAME=proj2
SOURCES=$(NAME).c

$(NAME): $(SOURCES)
	$(CC) $(CFLAGS)  $(SOURCES) -o $(NAME) -pthread

clean:
	rm $(NAME) && rm $(NAME).out

clear:
	rm $(NAME) && rm $(NAME).out


