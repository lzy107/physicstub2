#ifndef DEVICE_MEMORY_H
#define DEVICE_MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include "device_types.h"

// 内存区域结构体 - 四元组结构
typedef struct {
    uint32_t base_addr;       // 基地址
    size_t unit_size;         // 单位大小（字节）
    size_t length;            // 区域长度（单位数量）
    uint8_t* data;            // 数据指针
    uint32_t device_type;     // 设备类型
    uint32_t device_id;       // 设备ID
} memory_region_t;

// 设备内存结构
struct device_memory {
    memory_region_t* regions;     // 内存区域数组
    int region_count;             // 区域数量
    void* monitor;                // 监视器指针（类型已改为void*）
    uint32_t device_type;         // 设备类型
    uint32_t device_id;           // 设备ID
};

// 创建设备内存
device_memory_t* device_memory_create(const memory_region_t* regions, int region_count, 
                                     void* monitor, uint32_t device_type, uint32_t device_id);

// 创建设备内存（从配置创建）
device_memory_t* device_memory_create_from_config(memory_region_config_t* configs, int config_count, 
                                                void* monitor, 
                                                device_type_id_t device_type, int device_id);

// 销毁设备内存
void device_memory_destroy(device_memory_t* mem);

// 读取内存（32位）
int device_memory_read(device_memory_t* mem, uint32_t addr, uint32_t* value);

// 写入内存（32位）
int device_memory_write(device_memory_t* mem, uint32_t addr, uint32_t value);

// 读取字节
int device_memory_read_byte(device_memory_t* mem, uint32_t addr, uint8_t* value);

// 写入字节
int device_memory_write_byte(device_memory_t* mem, uint32_t addr, uint8_t value);

// 批量读取内存
int device_memory_read_buffer(device_memory_t* mem, uint32_t addr, uint8_t* buffer, size_t length);

// 批量写入内存
int device_memory_write_buffer(device_memory_t* mem, uint32_t addr, const uint8_t* buffer, size_t length);

// 查找地址所在的内存区域
memory_region_t* device_memory_find_region(device_memory_t* mem, uint32_t addr);

#endif /* DEVICE_MEMORY_H */ 