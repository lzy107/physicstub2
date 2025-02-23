#include <stdlib.h>
#include <string.h>
#include "device_memory.h"

int device_mem_init(device_mem_config_t* config) {
    if (!config || !config->regions || config->region_count == 0) {
        return -1;
    }
    
    for (uint32_t i = 0; i < config->region_count; i++) {
        device_mem_region_t* region = &config->regions[i];
        if (region->unit_size == 0 || region->unit_count == 0) {
            return -1;
        }
        
        // 分配内存
        size_t total_size = region->unit_size * region->unit_count;
        region->mem_ptr = calloc(1, total_size);
        if (!region->mem_ptr) {
            // 清理已分配的内存
            for (uint32_t j = 0; j < i; j++) {
                free(config->regions[j].mem_ptr);
                config->regions[j].mem_ptr = NULL;
            }
            return -1;
        }
    }
    
    return 0;
}

void device_mem_destroy(device_mem_config_t* config) {
    if (!config || !config->regions) {
        return;
    }
    
    for (uint32_t i = 0; i < config->region_count; i++) {
        if (config->regions[i].mem_ptr) {
            free(config->regions[i].mem_ptr);
            config->regions[i].mem_ptr = NULL;
        }
    }
}

device_mem_region_t* device_mem_find_region(const device_mem_config_t* config, uint32_t addr) {
    if (!config || !config->regions) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < config->region_count; i++) {
        device_mem_region_t* region = &config->regions[i];
        uint32_t end_addr = region->start_addr + (region->unit_size * region->unit_count);
        
        if (addr >= region->start_addr && addr < end_addr) {
            return region;
        }
    }
    
    return NULL;
}

int device_mem_read(const device_mem_config_t* config, uint32_t addr, void* value, uint32_t size) {
    if (!config || !value || size == 0) {
        return -1;
    }
    
    device_mem_region_t* region = device_mem_find_region(config, addr);
    if (!region || !region->mem_ptr) {
        return -1;
    }
    
    // 计算偏移量和单元索引
    uint32_t offset = addr - region->start_addr;
    uint32_t unit_index = offset / region->unit_size;
    
    if (unit_index >= region->unit_count || size > region->unit_size) {
        return -1;
    }
    
    // 读取数据
    uint8_t* src = (uint8_t*)region->mem_ptr + (unit_index * region->unit_size);
    memcpy(value, src, size);
    
    return 0;
}

int device_mem_write(const device_mem_config_t* config, uint32_t addr, const void* value, uint32_t size) {
    if (!config || !value || size == 0) {
        return -1;
    }
    
    device_mem_region_t* region = device_mem_find_region(config, addr);
    if (!region || !region->mem_ptr) {
        return -1;
    }
    
    // 计算偏移量和单元索引
    uint32_t offset = addr - region->start_addr;
    uint32_t unit_index = offset / region->unit_size;
    
    if (unit_index >= region->unit_count || size > region->unit_size) {
        return -1;
    }
    
    // 写入数据
    uint8_t* dst = (uint8_t*)region->mem_ptr + (unit_index * region->unit_size);
    memcpy(dst, value, size);
    
    return 0;
} 