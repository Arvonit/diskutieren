# Created from this StackOverflow post:
# https://stackoverflow.com/questions/30573481/how-to-write-a-makefile-with-separate-source-and-header-directories

CC := clang
CPPFLAGS := -Isrc -MMD -MP # Preprocessor flags; tell the preprocessor to look in src for headers
CFLAGS := -Wall -Werror -Wextra -Wno-unused-parameter -Wno-unused-variable -std=c99
LDLIBS := -ledit

SRC_DIR := src
BUILD_DIR := build
OBJ_DIR := build/obj

# Source files for client and server
CLIENT_SRC := client.c util.c
SERVER_SRC := server.c util.c
CLIENT_SRC := $(patsubst %.c, $(SRC_DIR)/%.c, $(CLIENT_SRC))
SERVER_SRC := $(patsubst %.c, $(SRC_DIR)/%.c, $(SERVER_SRC))

CLIENT_OBJ := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(CLIENT_SRC))
SERVER_OBJ := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SERVER_SRC))
DEP := $(CLIENT_OBJ:.o=.d) $(SERVER_OBJ:.o=.d) # Dependency files

CLIENT_EXE := $(BUILD_DIR)/irc
SERVER_EXE := $(BUILD_DIR)/ircd

.PHONY: all clean

all: client server

debug: CFLAGS += -g
debug: $(CLIENT_EXE) $(SERVER_EXE)

# $^ means target prerequisites
client: $(CLIENT_OBJ) | $(BUILD_DIR)
	$(CC) -o $(CLIENT_EXE) $^ $(LDLIBS)

server: $(SERVER_OBJ) | $(BUILD_DIR)
	$(CC) -o $(SERVER_EXE) $^ $(LDLIBS)

# Honestly not sure what `-c $<` does
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

$(BUILD_DIR):
	mkdir -p $@

$(OBJ_DIR):
	mkdir -p $@

clean:
	$(RM) -rv $(BUILD_DIR) $(OBJ_DIR)

-include $(DEP) # The dash silences errors when files don't exist (yet)
