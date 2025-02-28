// flash_device.h
#ifndef FLASH_DEVICE_H
#define FLASH_DEVICE_H

#include <stdint.h>
#include "../include/device_types.h"
#include "../include/device_memory.h"
#include "../include/global_monitor.h"
#include "../include/device_rules.h"

// FLASH 设备寄存器地址
#define FLASH_REG_STATUS    0x00  // 状态寄存器
#define FLASH_REG_CONTROL   0x04  // 控制寄存器
#define FLASH_REG_CONFIG    0x08  // 配置寄存器
#define FLASH_REG_ADDRESS   0x0C  // 地址寄存器
#define FLASH_REG_DATA      0x10  // 数据寄存器
#define FLASH_REG_SIZE      0x14  // 大小寄存器

// FLASH 状态寄存器位
#define FLASH_STATUS_BUSY   0x01  // 忙状态
#define FLASH_STATUS_ERROR  0x02  // 错误状态
#define FLASH_STATUS_READY  0x04  // 就绪状态
#define FLASH_STATUS_SRWD   0x08  // 状态寄存器写保护
#define FLASH_STATUS_WEL    0x10  // 写使能锁定

// FLASH 控制寄存器命令
#define FLASH_CTRL_READ     0x01  // 读取命令
#define FLASH_CTRL_WRITE    0x02  // 写入命令
#define FLASH_CTRL_ERASE    0x03  // 擦除命令

// FLASH 设备内存大小
#define FLASH_MEM_SIZE      (64 * 1024)  // 64KB
#define FLASH_DATA_START    0x1000       // 数据区起始地址
#define FLASH_TOTAL_SIZE    FLASH_MEM_SIZE // 总大小
#define FLASH_CTRL_REG      0x04         // 控制寄存器地址

// FLASH 设备区域定义
#define FLASH_REG_REGION    0  // 寄存器区域索引
#define FLASH_DATA_REGION   1  // 数据区域索引
#define FLASH_REGION_COUNT  2  // 内存区域总数

// Flash设备私有数据
typedef struct {
    pthread_mutex_t mutex;           // 互斥锁
    device_memory_t* memory;         // 设备内存
    device_rule_t device_rules[8];   // 设备规则
    int rule_count;                  // 规则计数
} flash_device_t;

// 获取 FLASH 设备操作接口
device_ops_t* get_flash_device_ops(void);

// 注册 FLASH 设备类型
void register_flash_device_type(device_manager_t* dm);

// 向Flash设备添加规则
int flash_device_add_rule(device_instance_t* instance, uint32_t addr, 
                         uint32_t expected_value, uint32_t expected_mask, 
                         action_target_t* targets);

// 函数声明
int flash_device_init(device_instance_t* instance);
void flash_device_destroy(device_instance_t* instance);
int flash_device_read(device_instance_t* instance, uint32_t addr, uint32_t* value);
int flash_device_write(device_instance_t* instance, uint32_t addr, uint32_t value);
int flash_device_read_buffer(device_instance_t* instance, uint32_t addr, uint8_t* buffer, size_t length);
int flash_device_write_buffer(device_instance_t* instance, uint32_t addr, const uint8_t* buffer, size_t length);
int flash_device_reset(device_instance_t* instance);
struct device_rule_manager* flash_get_rule_manager(device_instance_t* instance);
int flash_configure_memory(device_instance_t* instance, struct memory_region_config_t* configs, int config_count);

// 回调函数
void flash_erase_callback(void* context, uint32_t addr, uint32_t value);
void flash_read_callback(void* context, uint32_t addr, uint32_t value);
void flash_write_callback(void* context, uint32_t addr, uint32_t value);

#endif /* FLASH_DEVICE_H */