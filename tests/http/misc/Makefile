CC = gcc
CFLAGS = -std=c11 -Wpedantic -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wconversion -Wshadow -Wcast-qual -Wnested-externs
CFLAGS += -Isrc/shared
LDFLAGS = -lclux -lsqlite3 -lcrypto
OBJDIR = obj
SRCDIR = src

ifeq ($(shell uname -s),Darwin)
    OPENSSL_PREFIX = $(shell brew --prefix openssl@3)
    CFLAGS += -I$(OPENSSL_PREFIX)/include
    LDFLAGS += -L$(OPENSSL_PREFIX)/lib
endif

SERVER_SRC = $(wildcard $(SRCDIR)/server/*.c) $(wildcard $(SRCDIR)/shared/*.c)
CLIENT_SRC = $(wildcard $(SRCDIR)/client/*.c) $(wildcard $(SRCDIR)/shared/*.c)

SERVER_OBJ = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SERVER_SRC))
CLIENT_OBJ = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(CLIENT_SRC))

SERVER_BIN = server
CLIENT_BIN = client

all: $(OBJDIR) $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

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

