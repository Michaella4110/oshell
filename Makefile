CC = gcc
CFLAGS = -Wall -Wextra -Werror -Isrc/include
TARGET = oshell
MANDIR = man
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
MANDEST = $(PREFIX)/share/man/man1

# Source files with proper paths
SRC = src/main.c \
      src/core/builtins.c \
      src/core/errors.c \
      src/core/executor.c \
      src/core/expander.c \
      src/core/parser.c \
      src/core/state.c \
      src/core/utils.c \
      src/modes/batch.c \
      src/modes/determine.c \
      src/modes/interactive.c \
      src/modes/pipe.c

# Object files (same names but with .o extension)
OBJ = $(SRC:.c=.o)

# Man page files
MANPAGES = $(MANDIR)/alias.1 \
           $(MANDIR)/builtins.1 \
           $(MANDIR)/cd.1 \
           $(MANDIR)/env.1 \
           $(MANDIR)/exit.1 \
           $(MANDIR)/oshell.1 \
           $(MANDIR)/path.1 \
           $(MANDIR)/setenv.1 \
           $(MANDIR)/unsetenv.1

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

# Pattern rule to compile .c files to .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

fclean: clean
	rm -f $(TARGET)

re: fclean all

# Install target (optional)
install: $(TARGET) $(MANPAGES)
	mkdir -p $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/
	mkdir -p $(MANDEST)
	install -m 644 $(MANPAGES) $(MANDEST)/

# Uninstall target (optional)
uninstall:
	rm -f $(BINDIR)/$(TARGET)
	rm -f $(MANDEST)/alias.1 \
	      $(MANDEST)/builtins.1 \
	      $(MANDEST)/cd.1 \
	      $(MANDEST)/env.1 \
	      $(MANDEST)/exit.1 \
	      $(MANDEST)/oshell.1 \
	      $(MANDEST)/path.1 \
	      $(MANDEST)/setenv.1 \
	      $(MANDEST)/unsetenv.1

.PHONY: all clean fclean re install uninstall
