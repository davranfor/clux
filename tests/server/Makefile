CC := gcc
CFLAGS := -std=c11 -Wpedantic -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wconversion -Wshadow -Wcast-qual -Wnested-externs
LDFLAGS := -lclux

SOURCE := $(wildcard *.c)
HEADER := $(wildcard *.h)
TARGET := demo 

all: $(TARGET)

$(TARGET): $(SOURCE) $(HEADER)
	$(CC) $(CFLAGS) -o $@ $(SOURCE) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean

