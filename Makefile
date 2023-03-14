CC = clang
CFLAGS = -Wall -Wextra -Werror -Wpedantic -pthread
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
BINEXEC = httpserver

all: $(BINEXEC)

$(BINEXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

tidy:
	rm -f $(OBJ)

clean: tidy
	rm -f $(BINEXEC)

format:
	clang-format -i -style=file *.[ch]

debug: CFLAGS += -g -DDEBUG_ENABLED
debug: all
