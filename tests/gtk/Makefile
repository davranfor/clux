CC = gcc
CFLAGS += -std=c11 -pedantic -Wall -Wextra -W -Wmissing-prototypes -Wstrict-prototypes -Wconversion -Wshadow -Wcast-qual -Wnested-externs
CFLAGS += `pkg-config --cflags gtk4`
LDFLAGS = `pkg-config --libs gtk4` -lclux
OBJECTS = main.o utils.o

all: demo

main.o: utils.h
utils.o: utils.h

demo: $(OBJECTS) 
	$(CC) $(CFLAGS) $(OBJECTS) -o demo $(LDFLAGS)	

clean:
	rm -f *.o demo

