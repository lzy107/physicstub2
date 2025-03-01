CC = gcc
CFLAGS = -Wall -Wextra -g -I./include
LDFLAGS = -lm -lpthread

# 源文件目录
SRC_DIR = src
CORE_DIR = $(SRC_DIR)/core
DEVICE_DIR = $(SRC_DIR)/device
MONITOR_DIR = $(SRC_DIR)/monitor
TEST_DIR = $(SRC_DIR)/test
PLUGIN_DIR = plugins

# 核心源文件
CORE_SRC = $(CORE_DIR)/main.c \
           $(CORE_DIR)/address_space.c

# 核心源文件（不包含main.c，用于测试）
CORE_TEST_SRC = $(CORE_DIR)/address_space.c

# 设备源文件
DEVICE_SRC = $(DEVICE_DIR)/device_types.c \
             $(DEVICE_DIR)/device_configs.c \
             $(DEVICE_DIR)/device_memory.c \
             $(DEVICE_DIR)/device_registry.c

# 监控源文件
MONITOR_SRC = $(MONITOR_DIR)/global_monitor.c \
              $(MONITOR_DIR)/action_manager.c \
              $(MONITOR_DIR)/device_rules.c \
              $(MONITOR_DIR)/device_rule_configs.c

# 测试源文件
TEST_SRC = $(TEST_DIR)/device_test.c \
           $(TEST_DIR)/test_main.c \
           $(TEST_DIR)/flash_test.c \
           $(TEST_DIR)/fpga_test.c \
           $(TEST_DIR)/temp_sensor_test.c \
           $(TEST_DIR)/device_wrapper.c

# Flash设备插件源文件
FLASH_SRC = $(PLUGIN_DIR)/flash/flash_device.c \
            $(PLUGIN_DIR)/flash/flash_configs.c \
            $(PLUGIN_DIR)/flash/flash_rule_configs.c

# FPGA设备插件源文件
FPGA_SRC = $(PLUGIN_DIR)/fpga/fpga_device.c \
           $(PLUGIN_DIR)/fpga/fpga_configs.c \
           $(PLUGIN_DIR)/fpga/fpga_rule_configs.c

# 温度传感器插件源文件
TEMP_SENSOR_SRC = $(PLUGIN_DIR)/temp_sensor/temp_sensor.c \
                  $(PLUGIN_DIR)/temp_sensor/temp_sensor_configs.c \
                  $(PLUGIN_DIR)/temp_sensor/temp_sensor_rule_configs.c

# 所有源文件
SRCS = $(CORE_SRC) $(DEVICE_SRC) $(MONITOR_SRC) $(FLASH_SRC) $(FPGA_SRC) $(TEMP_SENSOR_SRC) $(TEST_DIR)/device_test.c $(TEST_DIR)/device_wrapper.c

# 所有源文件（不包含main.c，用于测试）
TEST_SRCS = $(CORE_TEST_SRC) $(DEVICE_SRC) $(MONITOR_SRC) $(FLASH_SRC) $(FPGA_SRC) $(TEMP_SENSOR_SRC)

# 目标文件
OBJS = $(SRCS:.c=.o)

# 测试目标文件（不包含main.o）
TEST_OBJS = $(TEST_SRCS:.c=.o) $(TEST_SRC:.c=.o)

# 构建目录
BUILD_DIR = build
BIN_DIR = bin

# 可执行文件
PROGRAM = $(BUILD_DIR)/program
TEST_PROGRAM = $(BUILD_DIR)/test_program

# 默认目标
all: $(PROGRAM)

# 测试目标
test: $(TEST_PROGRAM)

# 主程序编译
$(PROGRAM): $(OBJS) | $(BUILD_DIR)
	$(CC) -o $@ $^ $(LDFLAGS)

# 测试程序编译
$(TEST_PROGRAM): $(TEST_OBJS) | $(BUILD_DIR)
	$(CC) -o $@ $^ $(LDFLAGS)

# 编译规则
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 清理
clean:
	rm -f $(OBJS) $(TEST_OBJS) $(PROGRAM) $(TEST_PROGRAM)
	find $(BUILD_DIR) -name "*.o" -type f -delete
	find . -name "*.o" -type f -delete
	@echo "所有目标文件(.o)和可执行文件已清理完毕"

# 运行主程序
run: $(PROGRAM)
	./$(PROGRAM)

# 运行测试程序
run_test: $(TEST_PROGRAM)
	./$(TEST_PROGRAM) --all

# 安装（可选）
install: $(PROGRAM)
	mkdir -p $(BIN_DIR)
	cp $(PROGRAM) $(BIN_DIR)/

.PHONY: all test clean run run_test install