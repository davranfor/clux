CC = gcc
CFLAGS = -std=c11 -Wpedantic -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wconversion -Wshadow -Wcast-qual -Wnested-externs

all: demo 

demo: main.c config.h buffer.c buffer.h schema.c schema.h worker.c worker.h server.c server.h
	$(CC) $(CFLAGS) main.c buffer.c schema.c worker.c server.c -O2 -o demo -lclux

clean:
	rm -f demo

