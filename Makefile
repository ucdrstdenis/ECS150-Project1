CC      = gcc
CFLAGS 	= -m64 -Wall -Werror
HEADERS = noncanmode.h common.h history.h sshell.h
SOURCES = noncanmode.c common.c history.c sshell.c 
OBJECTS = $(SOURCES:.c=.o)
TARGET  = sshell

default: all

all: $(SOURCES) $(TARGET)

clean:
	rm -f $(OBJECTS)
	rm -f $(TARGET)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

