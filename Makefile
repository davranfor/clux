CC = gcc
CFLAGS = -std=c11 -Wpedantic -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wconversion -Wshadow -Wcast-qual -Wnested-externs
LDLIBS = -lm
SRCDIR = src
INCDIR = include
OBJDIR = obj
DESTLIB = /usr/local/lib
DESTINC = /usr/local/include/clux
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))

TARGET = demo

.PHONY: all debug release install uninstall clean

all: release

debug: CFLAGS += -DDEBUG -g
debug: $(TARGET)

release: CFLAGS += -DNDEBUG -O2
release: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDLIBS) -o $(TARGET)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) -I$(INCDIR) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJDIR):
	@mkdir -p $@

-include $(OBJECTS:.o=.d)

clean:
	rm -rf $(OBJDIR)
	rm -f $(TARGET)
	find . \( -name "*.o" -o -name "demo" \) -exec rm -f {} +

