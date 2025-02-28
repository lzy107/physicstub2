#ifndef FPGA_DEVICE_H
#define FPGA_DEVICE_H

#include <stdint.h>
#include <pthread.h>
#include "device_types.h"
#include "device_memory.h"
#include "device_rules.h"

// 设备类型ID
#define DEVICE_TYPE_FPGA 2

// 寄存器地址定义
#define FPGA_STATUS_REG      0x00    // 状态寄存器 (R)
#define FPGA_CONFIG_REG      0x04    // 配置寄存器 (R/W)
#define FPGA_CONTROL_REG     0x08    // 控制寄存器 (R/W)
#define FPGA_IRQ_REG        0x0C    // 中断状态寄存器 (R/W)
#define FPGA_DATA_START     0x1000   // 数据区起始地址

// 状态寄存器位定义
#define STATUS_BUSY         (1 << 0)  // FPGA忙状态
#define STATUS_DONE        (1 << 1)   // 配置完成
#define STATUS_ERROR       (1 << 2)   // 错误状态
#define STATUS_READY       (1 << 3)   // 就绪状态

// 配置寄存器位定义
#define CONFIG_RESET       (1 << 0)   // 软复位
#define CONFIG_ENABLE      (1 << 1)   // 使能FPGA
#define CONFIG_IRQ_EN      (1 << 2)   // 中断使能
#define CONFIG_DMA_EN      (1 << 3)   // DMA使能

// 控制寄存器位定义
#define CTRL_START         (1 << 0)   // 启动操作
#define CTRL_STOP          (1 << 1)   // 停止操作
#define CTRL_PAUSE         (1 << 2)   // 暂停操作

// FPGA设备内存区域配置
#define FPGA_REG_REGION    0  // 寄存器区域索引
#define FPGA_DATA_REGION   1  // 数据区域索引
#define FPGA_REGION_COUNT  2  // 内存区域总数

// FPGA设备私有数据
typedef struct {
    pthread_mutex_t mutex;           // 互斥锁
    device_memory_t* memory;         // 设备内存
    device_rule_t device_rules[8];   // 设备规则
    int rule_count;                  // 规则计数
} fpga_dev_data_t;

// 获取FPGA设备操作接口
device_ops_t* get_fpga_device_ops(void);

// 向FPGA设备添加规则
int fpga_add_rule(device_instance_t* instance, uint32_t addr, 
                 uint32_t expected_value, uint32_t expected_mask, 
                 action_target_t* targets);

// 函数声明
int fpga_device_init(device_instance_t* instance);
void fpga_device_destroy(device_instance_t* instance);
int fpga_device_read(device_instance_t* instance, uint32_t addr, uint32_t* value);
int fpga_device_write(device_instance_t* instance, uint32_t addr, uint32_t value);
int fpga_device_read_buffer(device_instance_t* instance, uint32_t addr, uint8_t* buffer, size_t length);
int fpga_device_write_buffer(device_instance_t* instance, uint32_t addr, const uint8_t* buffer, size_t length);
int fpga_device_reset(device_instance_t* instance);
struct device_rule_manager* fpga_get_rule_manager(device_instance_t* instance);
int fpga_configure_memory(device_instance_t* instance, struct memory_region_config_t* configs, int config_count);

// 回调函数
void fpga_irq_callback(void* context, uint32_t addr, uint32_t value);
void fpga_control_callback(void* context, uint32_t addr, uint32_t value);
void fpga_config_callback(void* context, uint32_t addr, uint32_t value);

#endif // FPGA_DEVICE_H 