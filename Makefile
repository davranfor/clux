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

ifeq ($(shell uname -s),Darwin)
	LINKER = -dynamiclib -install_name $(DESTLIB)/$(TARGET)
	TARGET = libclux.dylib
else
	LINKER = -shared
	TARGET = libclux.so
endif

.PHONY: all debug release install uninstall clean

all: release

debug: CFLAGS += -DDEBUG -g
debug: $(TARGET)

release: CFLAGS += -DNDEBUG -O2
release: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDLIBS) $(LINKER) -o $(TARGET)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) -I$(INCDIR) $(CFLAGS) -MMD -MP -fPIC -c $< -o $@

$(OBJDIR):
	@mkdir -p $@

-include $(OBJECTS:.o=.d)

install:
	mkdir -p $(DESTLIB) $(DESTINC)
	mv $(TARGET) $(DESTLIB)
	cp $(INCDIR)/*.h $(DESTINC)

uninstall:
	rm -f $(DESTLIB)/$(TARGET)
	rm -rf $(DESTINC)

clean:
	rm -rf $(OBJDIR)
	rm -f $(TARGET)

