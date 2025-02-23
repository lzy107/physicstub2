#ifndef DEVICE_MEMORY_H
#define DEVICE_MEMORY_H

#include <stdint.h>

// 设备内存区域描述符
typedef struct {
    uint32_t start_addr;     // 起始地址
    uint32_t unit_size;      // 单元字节数
    uint32_t unit_count;     // 单元个数
    void* mem_ptr;           // 内存指针
} device_mem_region_t;

// 设备内存配置
typedef struct {
    device_mem_region_t* regions;  // 内存区域数组
    uint32_t region_count;         // 区域个数
} device_mem_config_t;

// 初始化设备内存区域
int device_mem_init(device_mem_config_t* config);

// 释放设备内存区域
void device_mem_destroy(device_mem_config_t* config);

// 读取设备内存
int device_mem_read(const device_mem_config_t* config, uint32_t addr, void* value, uint32_t size);

// 写入设备内存
int device_mem_write(const device_mem_config_t* config, uint32_t addr, const void* value, uint32_t size);

// 查找地址所在的内存区域
device_mem_region_t* device_mem_find_region(const device_mem_config_t* config, uint32_t addr);

#endif // DEVICE_MEMORY_H 