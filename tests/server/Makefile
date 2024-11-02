CC := gcc
CFLAGS := -std=c11 -Wpedantic -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wconversion -Wshadow -Wcast-qual -Wnested-externs
SOURCE := $(wildcard *.c)
HEADER := $(wildcard *.h)
TARGET := demo 

all: $(TARGET)

$(TARGET): $(SOURCE) $(HEADER)
	$(CC) $(CFLAGS) -o $@ $(SOURCE) -lclux

clean:
	rm -f $(TARGET)

.PHONY: all clean

