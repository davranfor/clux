CC := gcc
CFLAGS := -std=c11 -Wpedantic -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wconversion -Wshadow -Wcast-qual -Wnested-externs
LDFLAGS = -lclux -lsqlite3 -lcrypto

ifdef CLUX_ALLOW_INSECURE_TOKEN
CFLAGS += -DALLOW_INSECURE_TOKEN
endif

ifeq ($(shell uname -s),Darwin)
OPENSSL_PREFIX = $(shell brew --prefix openssl@3)
CFLAGS += -I$(OPENSSL_PREFIX)/include
LDFLAGS += -L$(OPENSSL_PREFIX)/lib
endif

SRC_DIR := src
OBJ_DIR := obj
DEP_DIR := $(OBJ_DIR)/.deps
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))
DEP := $(patsubst $(SRC_DIR)/%.c,$(DEP_DIR)/%.d,$(SRC))
TARGET := server

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR) $(DEP_DIR)
	$(CC) $(CFLAGS) -MMD -MP -MF $(DEP_DIR)/$*.d -c -o $@ $<

$(OBJ_DIR) $(DEP_DIR):
	@mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

-include $(DEP)

.PHONY: all clean

