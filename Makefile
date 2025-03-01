CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lm -lpthread

# 临时目录
TEMP_DIR = temp_build
TEMP_INCLUDE = $(TEMP_DIR)/include

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

# 替换目标文件路径，使其放在临时目录中
TEMP_OBJS = $(patsubst %.c,$(TEMP_DIR)/%.o,$(SRCS))
TEMP_TEST_OBJS = $(patsubst %.c,$(TEMP_DIR)/%.o,$(TEST_SRCS)) $(patsubst %.c,$(TEMP_DIR)/%.o,$(TEST_SRC))

# 构建目录
BUILD_DIR = build
BIN_DIR = bin

# 可执行文件
PROGRAM = $(BUILD_DIR)/program
TEST_PROGRAM = $(BUILD_DIR)/test_program

# 头文件路径
INCLUDE_DIRS = include $(PLUGIN_DIR)/flash $(PLUGIN_DIR)/fpga $(PLUGIN_DIR)/temp_sensor

# 默认目标
all: prepare_temp $(PROGRAM)

# 测试目标
test: prepare_temp $(TEST_PROGRAM)

# 处理所有源文件和头文件，去除相对路径引用
process_files:
	@echo "处理所有源文件和头文件，删除相对路径引用..."
	@# 扫描所有C源文件
	@find $(SRC_DIR) $(PLUGIN_DIR) -name "*.c" | while read file; do \
		sed -i.bak -E 's|#include "[.][.]/+plugins/flash/|#include "flash/|g; \
		s|#include "[.][.]/+plugins/fpga/|#include "fpga/|g; \
		s|#include "[.][.]/+plugins/temp_sensor/|#include "temp_sensor/|g; \
		s|#include "[.][.]/+include/|#include "|g; \
		s|#include "[.][.]/+plugins/|#include "|g; \
		s|#include "[.][.]/[.][.]/+include/|#include "|g; \
		s|#include "[.][.]/[.][.]/+plugins/|#include "|g' $$file; \
		rm -f $$file.bak; \
	done
	@# 扫描所有头文件
	@find $(SRC_DIR) $(PLUGIN_DIR) include -name "*.h" | while read file; do \
		sed -i.bak -E 's|#include "[.][.]/+plugins/flash/|#include "flash/|g; \
		s|#include "[.][.]/+plugins/fpga/|#include "fpga/|g; \
		s|#include "[.][.]/+plugins/temp_sensor/|#include "temp_sensor/|g; \
		s|#include "[.][.]/+include/|#include "|g; \
		s|#include "[.][.]/+plugins/|#include "|g; \
		s|#include "[.][.]/[.][.]/+include/|#include "|g; \
		s|#include "[.][.]/[.][.]/+plugins/|#include "|g' $$file; \
		rm -f $$file.bak; \
	done
	@echo "文件处理完成"

# 准备临时目录和复制头文件
prepare_temp:
	@echo "准备临时构建环境..."
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(TEMP_DIR)
	@mkdir -p $(TEMP_INCLUDE)
	@# 为插件头文件创建子目录
	@mkdir -p $(TEMP_INCLUDE)/flash
	@mkdir -p $(TEMP_INCLUDE)/fpga
	@mkdir -p $(TEMP_INCLUDE)/temp_sensor
	@# 复制所有头文件到临时include目录，保持原有结构
	@# 复制include目录中的头文件
	@find include -name "*.h" -exec cp {} $(TEMP_INCLUDE)/ \;
	@# 复制插件头文件到相应的子目录
	@find $(PLUGIN_DIR)/flash -name "*.h" -exec cp {} $(TEMP_INCLUDE)/flash/ \;
	@find $(PLUGIN_DIR)/fpga -name "*.h" -exec cp {} $(TEMP_INCLUDE)/fpga/ \;
	@find $(PLUGIN_DIR)/temp_sensor -name "*.h" -exec cp {} $(TEMP_INCLUDE)/temp_sensor/ \;
	@# 为源文件创建临时目录结构
	@for src in $(SRCS) $(TEST_SRC); do \
		mkdir -p $(TEMP_DIR)/`dirname $$src`; \
	done
	@# 创建临时源文件，修改头文件包含方式
	@for src in $(SRCS) $(TEST_SRC); do \
		mkdir -p $(TEMP_DIR)/`dirname $$src`; \
		case $$src in \
			$(PLUGIN_DIR)/flash/*) \
				sed -E 's|#include "flash_device.h"|#include "flash/flash_device.h"|g; \
				       s|#include "[.][.]/[.][.]/include/|#include "|g; \
				       s|#include "[.][.]/[.][.]/plugins/flash/|#include "flash/|g; \
				       s|#include "[.][.]/[.][.]/plugins/fpga/|#include "fpga/|g; \
				       s|#include "[.][.]/[.][.]/plugins/temp_sensor/|#include "temp_sensor/|g; \
				       s|#include "[.][.]/include/|#include "|g; \
				       s|#include "[.][.]/plugins/flash/|#include "flash/|g; \
				       s|#include "[.][.]/plugins/fpga/|#include "fpga/|g; \
				       s|#include "[.][.]/plugins/temp_sensor/|#include "temp_sensor/|g' \
				       $$src > $(TEMP_DIR)/$$src ;; \
			$(PLUGIN_DIR)/fpga/*) \
				sed -E 's|#include "fpga_device.h"|#include "fpga/fpga_device.h"|g; \
				       s|#include "[.][.]/[.][.]/include/|#include "|g; \
				       s|#include "[.][.]/[.][.]/plugins/flash/|#include "flash/|g; \
				       s|#include "[.][.]/[.][.]/plugins/fpga/|#include "fpga/|g; \
				       s|#include "[.][.]/[.][.]/plugins/temp_sensor/|#include "temp_sensor/|g; \
				       s|#include "[.][.]/include/|#include "|g; \
				       s|#include "[.][.]/plugins/flash/|#include "flash/|g; \
				       s|#include "[.][.]/plugins/fpga/|#include "fpga/|g; \
				       s|#include "[.][.]/plugins/temp_sensor/|#include "temp_sensor/|g' \
				       $$src > $(TEMP_DIR)/$$src ;; \
			$(PLUGIN_DIR)/temp_sensor/*) \
				sed -E 's|#include "temp_sensor.h"|#include "temp_sensor/temp_sensor.h"|g; \
				       s|#include "[.][.]/[.][.]/include/|#include "|g; \
				       s|#include "[.][.]/[.][.]/plugins/flash/|#include "flash/|g; \
				       s|#include "[.][.]/[.][.]/plugins/fpga/|#include "fpga/|g; \
				       s|#include "[.][.]/[.][.]/plugins/temp_sensor/|#include "temp_sensor/|g; \
				       s|#include "[.][.]/include/|#include "|g; \
				       s|#include "[.][.]/plugins/flash/|#include "flash/|g; \
				       s|#include "[.][.]/plugins/fpga/|#include "fpga/|g; \
				       s|#include "[.][.]/plugins/temp_sensor/|#include "temp_sensor/|g' \
				       $$src > $(TEMP_DIR)/$$src ;; \
			*) \
				sed -E 's|#include "[.][.]/[.][.]/include/|#include "|g; \
				       s|#include "[.][.]/[.][.]/plugins/flash/|#include "flash/|g; \
				       s|#include "[.][.]/[.][.]/plugins/fpga/|#include "fpga/|g; \
				       s|#include "[.][.]/[.][.]/plugins/temp_sensor/|#include "temp_sensor/|g; \
				       s|#include "[.][.]/include/|#include "|g; \
				       s|#include "[.][.]/plugins/flash/|#include "flash/|g; \
				       s|#include "[.][.]/plugins/fpga/|#include "fpga/|g; \
				       s|#include "[.][.]/plugins/temp_sensor/|#include "temp_sensor/|g' \
				       $$src > $(TEMP_DIR)/$$src ;; \
		esac; \
	done
	@echo "临时构建环境准备完成"

# 添加临时目录到CFLAGS
override CFLAGS += -I$(TEMP_INCLUDE)

# 主程序编译
$(PROGRAM): $(TEMP_OBJS) | $(BUILD_DIR)
	$(CC) -o $@ $^ $(LDFLAGS)

# 测试程序编译
$(TEST_PROGRAM): $(TEMP_TEST_OBJS) | $(BUILD_DIR)
	$(CC) -o $@ $^ $(LDFLAGS)

# 编译规则 - 将源文件编译到临时目录中
$(TEMP_DIR)/%.o: $(TEMP_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理
clean:
	@echo "清理所有构建文件..."
	@rm -f $(PROGRAM) $(TEST_PROGRAM)
	@find $(BUILD_DIR) -name "*.o" -type f -delete
	@rm -rf $(TEMP_DIR)
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

.PHONY: all test clean run run_test install prepare_temp process_files