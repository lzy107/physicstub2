// flash_device.h
#ifndef FLASH_DEVICE_H
#define FLASH_DEVICE_H

#include <stdint.h>
#include <pthread.h>
#include "device_types.h"
#include "device_memory.h"

// FLASH设备特有参数
#define FLASH_PAGE_SIZE       256     // 页大小(字节)
#define FLASH_BLOCK_SIZE     4096     // 块大小(字节)
#define FLASH_TOTAL_SIZE    65536     // 总容量(字节)

// FLASH状态寄存器地址
#define FLASH_STATUS_REG    0x00      // 状态寄存器
#define FLASH_CTRL_REG      0x01      // 控制寄存器
#define FLASH_DATA_START    0x100     // 数据区起始地址

// 状态寄存器位定义
#define STATUS_BUSY         (1 << 0)  // 忙状态
#define STATUS_WEL         (1 << 1)   // 写使能锁存器
#define STATUS_BP0         (1 << 2)   // 块保护位0
#define STATUS_BP1         (1 << 3)   // 块保护位1
#define STATUS_BP2         (1 << 4)   // 块保护位2
#define STATUS_TB          (1 << 5)   // 顶部/底部保护
#define STATUS_SRWD        (1 << 7)   // 状态寄存器写禁止

// FLASH设备内存区域配置
#define FLASH_REG_REGION    0  // 寄存器区域索引
#define FLASH_DATA_REGION   1  // 数据区域索引
#define FLASH_REGION_COUNT  2  // 内存区域总数

// FLASH设备私有数据结构
typedef struct {
    device_mem_config_t mem_config;   // 内存配置
    pthread_mutex_t mutex;            // 操作互斥锁
} flash_dev_data_t;

// 插件导出函数
const char* get_plugin_name(void);
device_ops_t* get_device_ops(void);

// 获取FLASH设备操作接口
device_ops_t* get_flash_device_ops(void);

#endif