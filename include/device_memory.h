#ifndef DEVICE_MEMORY_H
#define DEVICE_MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include "device_types.h"

// 前向声明
struct global_monitor_t;

// 设备内存结构
typedef struct {
    uint8_t* data;                // 内存数据
    size_t size;                  // 内存大小
    struct global_monitor_t* monitor;    // 全局监视器
    device_type_id_t device_type; // 设备类型
    int device_id;                // 设备ID
} device_memory_t;

// 创建设备内存
device_memory_t* device_memory_create(size_t size, struct global_monitor_t* monitor, 
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

#endif /* DEVICE_MEMORY_H */ 