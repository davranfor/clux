CC = gcc
CFLAGS = -std=c11 -Wpedantic -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wconversion -Wshadow -Wcast-qual -Wnested-externs

all: demo 

demo: main.c config.h server.c server.h buffer.c buffer.h 
	$(CC) $(CFLAGS) main.c server.c buffer.c -O2 -o demo -lclux

clean:
	rm -f demo

