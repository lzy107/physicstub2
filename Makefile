CC = gcc
CFLAGS = -Wall -Wextra -I./include
SRC_DIR = src
BUILD_DIR = build
PLUGINS_DIR = plugins

# 源文件
SRCS = $(wildcard $(SRC_DIR)/*.c)
FLASH_SRCS = $(wildcard $(PLUGINS_DIR)/flash/*.c)
FPGA_SRCS = $(wildcard $(PLUGINS_DIR)/fpga/*.c)
TEMP_SENSOR_SRCS = $(wildcard $(PLUGINS_DIR)/temp_sensor/*.c)
COMMON_SRCS = $(wildcard $(PLUGINS_DIR)/common/*.c)

# 目标文件
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
FLASH_OBJS = $(FLASH_SRCS:$(PLUGINS_DIR)/flash/%.c=$(BUILD_DIR)/plugins/flash/%.o)
FPGA_OBJS = $(FPGA_SRCS:$(PLUGINS_DIR)/fpga/%.c=$(BUILD_DIR)/plugins/fpga/%.o)
TEMP_SENSOR_OBJS = $(TEMP_SENSOR_SRCS:$(PLUGINS_DIR)/temp_sensor/%.c=$(BUILD_DIR)/plugins/temp_sensor/%.o)
COMMON_OBJS = $(COMMON_SRCS:$(PLUGINS_DIR)/common/%.c=$(BUILD_DIR)/plugins/common/%.o)

# 所有目标文件
ALL_OBJS = $(OBJS) $(FLASH_OBJS) $(FPGA_OBJS) $(TEMP_SENSOR_OBJS) $(COMMON_OBJS)

TARGET = program

$(BUILD_DIR)/$(TARGET): $(ALL_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(ALL_OBJS) -o $@ -lpthread

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/plugins/flash/%.o: $(PLUGINS_DIR)/flash/%.c
	@mkdir -p $(BUILD_DIR)/plugins/flash
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/plugins/fpga/%.o: $(PLUGINS_DIR)/fpga/%.c
	@mkdir -p $(BUILD_DIR)/plugins/fpga
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/plugins/temp_sensor/%.o: $(PLUGINS_DIR)/temp_sensor/%.c
	@mkdir -p $(BUILD_DIR)/plugins/temp_sensor
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/plugins/common/%.o: $(PLUGINS_DIR)/common/%.c
	@mkdir -p $(BUILD_DIR)/plugins/common
	$(CC) $(CFLAGS) -c $< -o $@

all: $(BUILD_DIR)/$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean 