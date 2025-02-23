CC = gcc
CFLAGS = -Wall -Wextra -I./include
SRC_DIR = src
BUILD_DIR = build
PLUGINS_DIR = plugins

SRCS = $(wildcard $(SRC_DIR)/*.c)
PLUGIN_SRCS = $(wildcard $(PLUGINS_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o) $(PLUGIN_SRCS:$(PLUGINS_DIR)/%.c=$(BUILD_DIR)/plugins/%.o)
TARGET = program

$(BUILD_DIR)/$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJS) -o $@ -lpthread

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/plugins/%.o: $(PLUGINS_DIR)/%.c
	@mkdir -p $(BUILD_DIR)/plugins
	$(CC) $(CFLAGS) -c $< -o $@

all: $(BUILD_DIR)/$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean 