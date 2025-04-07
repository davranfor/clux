CC = gcc
CFLAGS = -std=c11 -Wpedantic -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wconversion -Wshadow -Wcast-qual -Wnested-externs
CFLAGS += -Isrc/shared
LDFLAGS = -lclux
LDEXTRA = -lsqlite3
OBJDIR = obj
SRCDIR = src

SERVER_SRC = $(wildcard $(SRCDIR)/server/*.c) $(wildcard $(SRCDIR)/shared/*.c)
CLIENT_SRC = $(wildcard $(SRCDIR)/client/*.c) $(wildcard $(SRCDIR)/shared/*.c)

SERVER_OBJ = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SERVER_SRC))
CLIENT_OBJ = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(CLIENT_SRC))

SERVER_BIN = server
CLIENT_BIN = client

all: $(OBJDIR) $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDEXTRA)

$(CLIENT_BIN): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)/server $(OBJDIR)/client $(OBJDIR)/shared
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(OBJDIR)/server:
	@mkdir -p $(OBJDIR)/server

$(OBJDIR)/client:
	@mkdir -p $(OBJDIR)/client

$(OBJDIR)/shared:
	@mkdir -p $(OBJDIR)/shared

clean:
	rm -rf $(OBJDIR) $(SERVER_BIN) $(CLIENT_BIN)

.PHONY: all clean

