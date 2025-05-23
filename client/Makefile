CC      := gcc
CFLAGS  := -Wall
LDLIBS  := -ldl -lpthread

.PHONY: all clean

all: client

client: client.c
	$(CC) $(CFLAGS) -o client client.c $(LDLIBS)

clean:
	rm -f client

