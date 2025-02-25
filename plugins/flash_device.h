// flash_device.h
#ifndef FLASH_DEVICE_H
#define FLASH_DEVICE_H

#include <stdint.h>
#include "../include/device_types.h"
#include "../include/device_memory.h"
#include "../include/global_monitor.h"

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

// FLASH 设备实例结构
typedef struct {
    device_instance_t base;  // 基础设备实例
    device_memory_t* memory;      // 设备内存
    uint32_t status;              // 当前状态
    uint32_t control;             // 当前控制值
    uint32_t config;              // 当前配置
    uint32_t address;             // 当前地址
    uint32_t size;                // 设备大小
    pthread_mutex_t mutex;        // 互斥锁
} flash_device_t;

// 获取 FLASH 设备操作接口
device_ops_t* get_flash_device_ops(void);

// 注册 FLASH 设备类型
void register_flash_device_type(device_manager_t* dm);

#endif /* FLASH_DEVICE_H */