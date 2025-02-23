CC = gcc
CFLAGS = -Wall -Wextra -I./include
SRC_DIR = src
BUILD_DIR = build
PLUGINS_DIR = plugins

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TARGET = program

# 插件相关配置
PLUGIN_SRCS = $(wildcard $(PLUGINS_DIR)/*.c)
PLUGIN_OBJS = $(PLUGIN_SRCS:$(PLUGINS_DIR)/%.c=$(BUILD_DIR)/plugins/%.o)
PLUGIN_LIBS = $(PLUGIN_SRCS:$(PLUGINS_DIR)/%.c=$(BUILD_DIR)/plugins/lib%.so)

$(BUILD_DIR)/$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ -ldl

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/plugins/%.o: $(PLUGINS_DIR)/%.c
	@mkdir -p $(BUILD_DIR)/plugins
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(BUILD_DIR)/plugins/lib%.so: $(BUILD_DIR)/plugins/%.o
	$(CC) -shared -o $@ $<

plugins: $(PLUGIN_LIBS)

all: $(BUILD_DIR)/$(TARGET) plugins

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean plugins 