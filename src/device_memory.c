#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "device_memory.h"
#include "global_monitor.h"

// 创建设备内存
device_memory_t* device_memory_create(size_t size, struct global_monitor_t* monitor, 
                                     device_type_id_t device_type, int device_id) {
    if (size == 0) return NULL;
    
    device_memory_t* mem = (device_memory_t*)calloc(1, sizeof(device_memory_t));
    if (!mem) return NULL;
    
    mem->data = (uint8_t*)calloc(size, sizeof(uint8_t));
    if (!mem->data) {
        free(mem);
        return NULL;
    }
    
    mem->size = size;
    mem->monitor = monitor;
    mem->device_type = device_type;
    mem->device_id = device_id;
    
    return mem;
}

// 销毁设备内存
void device_memory_destroy(device_memory_t* mem) {
    if (!mem) return;
    
    if (mem->data) {
        free(mem->data);
        mem->data = NULL;
    }
    
    free(mem);
}

// 读取内存
int device_memory_read(device_memory_t* mem, uint32_t addr, uint32_t* value) {
    if (!mem || !value || addr >= mem->size) return -1;
    
    // 确保地址对齐
    if (addr % 4 != 0) {
        printf("Warning: Unaligned memory read at address 0x%08X\n", addr);
    }
    
    // 防止越界访问
    if (addr + 4 > mem->size) {
        printf("Error: Memory read out of bounds at address 0x%08X\n", addr);
        return -1;
    }
    
    // 读取32位值
    *value = *(uint32_t*)(mem->data + addr);
    
    return 0;
}

// 写入内存
int device_memory_write(device_memory_t* mem, uint32_t addr, uint32_t value) {
    if (!mem || addr >= mem->size) return -1;
    
    // 确保地址对齐
    if (addr % 4 != 0) {
        printf("Warning: Unaligned memory write at address 0x%08X\n", addr);
    }
    
    // 防止越界访问
    if (addr + 4 > mem->size) {
        printf("Error: Memory write out of bounds at address 0x%08X\n", addr);
        return -1;
    }
    
    // 写入32位值
    *(uint32_t*)(mem->data + addr) = value;
    
    // 通知全局监视器
    if (mem->monitor) {
        global_monitor_handle_address_change((global_monitor_t*)mem->monitor, 
                                           mem->device_type, mem->device_id, addr, value);
    }
    
    return 0;
}

// 读取字节
int device_memory_read_byte(device_memory_t* mem, uint32_t addr, uint8_t* value) {
    if (!mem || !value || addr >= mem->size) return -1;
    
    *value = mem->data[addr];
    return 0;
}

// 写入字节
int device_memory_write_byte(device_memory_t* mem, uint32_t addr, uint8_t value) {
    if (!mem || addr >= mem->size) return -1;
    
    mem->data[addr] = value;
    
    // 通知全局监视器（对于字节写入，我们需要读取完整的32位值）
    if (mem->monitor) {
        uint32_t aligned_addr = addr & ~0x3;  // 对齐到32位边界
        uint32_t full_value;
        
        if (device_memory_read(mem, aligned_addr, &full_value) == 0) {
            global_monitor_handle_address_change((global_monitor_t*)mem->monitor, 
                                               mem->device_type, mem->device_id, 
                                               aligned_addr, full_value);
        }
    }
    
    return 0;
}

// 批量读取内存
int device_memory_read_buffer(device_memory_t* mem, uint32_t addr, uint8_t* buffer, size_t length) {
    if (!mem || !buffer || addr >= mem->size || addr + length > mem->size) return -1;
    
    memcpy(buffer, mem->data + addr, length);
    return 0;
}

// 批量写入内存
int device_memory_write_buffer(device_memory_t* mem, uint32_t addr, const uint8_t* buffer, size_t length) {
    if (!mem || !buffer || addr >= mem->size || addr + length > mem->size) return -1;
    
    memcpy(mem->data + addr, buffer, length);
    
    // 通知全局监视器（对于每个对齐的32位地址）
    if (mem->monitor) {
        uint32_t start_addr = addr & ~0x3;  // 对齐到32位边界
        uint32_t end_addr = (addr + length + 3) & ~0x3;  // 对齐到32位边界
        
        for (uint32_t a = start_addr; a < end_addr; a += 4) {
            if (a < mem->size) {
                uint32_t value;
                if (device_memory_read(mem, a, &value) == 0) {
                    global_monitor_handle_address_change((global_monitor_t*)mem->monitor, 
                                                       mem->device_type, mem->device_id, a, value);
                }
            }
        }
    }
    
    return 0;
} 