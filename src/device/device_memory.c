#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include "device_memory.h"
#include "global_monitor.h"
#include "device_rule_configs.h"
#include "action_manager.h"

// 声明外部全局变量
extern device_manager_t* g_device_manager;
extern action_manager_t* g_action_manager;

// 查找地址所在的内存区域
memory_region_t* device_memory_find_region(device_memory_t* mem, uint32_t addr) {
    if (!mem || !mem->regions || mem->region_count <= 0) return NULL;
    
    for (int i = 0; i < mem->region_count; i++) {
        memory_region_t* region = &mem->regions[i];
        uint32_t region_end = region->base_addr + (region->unit_size * region->length);
        
        // 检查地址是否在当前区域范围内
        if (addr >= region->base_addr && addr < region_end) {
            return region;
        }
    }
    
    return NULL;  // 未找到匹配的区域
}

// 创建设备内存（从配置创建）
device_memory_t* device_memory_create_from_config(memory_region_config_t* configs, int config_count, 
                                                struct global_monitor_t* monitor, 
                                                device_type_id_t device_type, int device_id) {
    if (!configs || config_count <= 0) return NULL;
    
    device_memory_t* mem = (device_memory_t*)calloc(1, sizeof(device_memory_t));
    if (!mem) return NULL;
    
    // 分配内存区域数组
    mem->regions = (memory_region_t*)calloc(config_count, sizeof(memory_region_t));
    if (!mem->regions) {
        free(mem);
        return NULL;
    }
    
    // 复制区域信息并分配每个区域的内存
    for (int i = 0; i < config_count; i++) {
        mem->regions[i].base_addr = configs[i].base_addr;
        mem->regions[i].unit_size = configs[i].unit_size;
        mem->regions[i].length = configs[i].length;
        mem->regions[i].device_type = device_type;
        mem->regions[i].device_id = device_id;
        
        // 计算区域总大小并分配内存
        size_t region_size = configs[i].unit_size * configs[i].length;
        mem->regions[i].data = (uint8_t*)calloc(region_size, sizeof(uint8_t));
        
        if (!mem->regions[i].data) {
            // 清理已分配的内存
            for (int j = 0; j < i; j++) {
                free(mem->regions[j].data);
            }
            free(mem->regions);
            free(mem);
            return NULL;
        }
    }
    
    mem->region_count = config_count;
    mem->monitor = monitor;
    mem->device_type = device_type;
    mem->device_id = device_id;
    
    return mem;
}

// 创建设备内存
device_memory_t* device_memory_create(const memory_region_t* regions, int region_count, 
                                     struct global_monitor_t* monitor, uint32_t device_type, uint32_t device_id) {
    if (!regions || region_count <= 0) {
        return NULL;
    }
    
    // 分配设备内存结构
    device_memory_t* memory = (device_memory_t*)calloc(1, sizeof(device_memory_t));
    if (!memory) {
        return NULL;
    }
    
    // 分配区域数组
    memory->regions = (memory_region_t*)calloc(region_count, sizeof(memory_region_t));
    if (!memory->regions) {
        free(memory);
        return NULL;
    }
    
    memory->region_count = region_count;
    memory->monitor = monitor;
    memory->device_type = device_type;
    memory->device_id = device_id;
    
    // 复制并初始化每个区域
    for (int i = 0; i < region_count; i++) {
        memory->regions[i].base_addr = regions[i].base_addr;
        memory->regions[i].unit_size = regions[i].unit_size;
        memory->regions[i].length = regions[i].length;
        memory->regions[i].device_type = device_type;
        memory->regions[i].device_id = device_id;
        
        // 分配数据内存
        size_t data_size = regions[i].unit_size * regions[i].length;
        memory->regions[i].data = (uint8_t*)calloc(1, data_size);
        if (!memory->regions[i].data) {
            // 释放已分配的内存
            for (int j = 0; j < i; j++) {
                free(memory->regions[j].data);
            }
            free(memory->regions);
            free(memory);
            return NULL;
        }
    }
    
    return memory;
}

// 销毁设备内存
void device_memory_destroy(device_memory_t* mem) {
    if (!mem) return;
    
    if (mem->regions) {
        // 释放每个区域的内存
        for (int i = 0; i < mem->region_count; i++) {
            if (mem->regions[i].data) {
                free(mem->regions[i].data);
                mem->regions[i].data = NULL;
            }
        }
        
        free(mem->regions);
        mem->regions = NULL;
    }
    
    free(mem);
}

// 读取内存
int device_memory_read(device_memory_t* mem, uint32_t addr, uint32_t* value) {
    if (!mem || !value) return -1;
    
    // 查找地址所在的区域
    memory_region_t* region = device_memory_find_region(mem, addr);
    if (!region) {
        printf("Error: Memory read at invalid address 0x%08X\n", addr);
        return -1;
    }
    
    // 计算区域内的偏移量
    uint32_t offset = addr - region->base_addr;
    
    // 检查对齐
    if (offset % 4 != 0) {
        printf("Warning: Unaligned memory read at address 0x%08X\n", addr);
    }
    
    // 防止越界访问
    if (offset + 4 > region->unit_size * region->length) {
        printf("Error: Memory read out of bounds at address 0x%08X\n", addr);
        return -1;
    }
    
    // 读取32位值
    *value = *(uint32_t*)(region->data + offset);
    
    // 调试输出
    printf("DEBUG: device_memory_read - 地址: 0x%08X, 区域基址: 0x%08X, 偏移: %u, 值: 0x%08X\n", 
           addr, region->base_addr, offset, *value);
    
    // 记录访问
    if (mem->monitor) {
        memory_access_record(mem->monitor, addr, *value, 0, region->device_type, region->device_id);
    }
    
    return 0;
}

// 写入内存
int device_memory_write(device_memory_t* mem, uint32_t addr, uint32_t value) {
    // 获取当前时间戳
    struct timeval tv;
    gettimeofday(&tv, NULL);
    printf("[%ld.%06ld] device_memory_write - 开始执行，地址: 0x%08X, 值: 0x%08X\n", 
           tv.tv_sec, (long)tv.tv_usec, addr, value);
    fflush(stdout);
    
    if (!mem) {
        printf("[%ld.%06ld] device_memory_write - 内存对象为NULL\n", tv.tv_sec, (long)tv.tv_usec);
        fflush(stdout);
        return -1;
    }
    
    // 查找地址所在的区域
    printf("[%ld.%06ld] device_memory_write - 查找地址所在区域\n", tv.tv_sec, (long)tv.tv_usec);
    fflush(stdout);
    
    memory_region_t* region = device_memory_find_region(mem, addr);
    gettimeofday(&tv, NULL);
    
    if (!region) {
        printf("[%ld.%06ld] device_memory_write - 错误: 内存写入无效地址 0x%08X\n", 
               tv.tv_sec, (long)tv.tv_usec, addr);
        fflush(stdout);
        return -1;
    }
    
    printf("[%ld.%06ld] device_memory_write - 找到区域，基址: 0x%08X, 长度: %u, 设备类型: %d, 设备ID: %d\n", 
           tv.tv_sec, (long)tv.tv_usec, region->base_addr, (unsigned int)region->length, region->device_type, region->device_id);
    fflush(stdout);
    
    // 计算区域内的偏移量
    uint32_t offset = addr - region->base_addr;
    
    // 检查对齐
    if (offset % 4 != 0) {
        printf("[%ld.%06ld] device_memory_write - 警告: 内存写入地址 0x%08X 未对齐\n", 
               tv.tv_sec, (long)tv.tv_usec, addr);
        fflush(stdout);
    }
    
    // 防止越界访问
    if (offset + 4 > region->unit_size * region->length) {
        printf("[%ld.%06ld] device_memory_write - 错误: 内存写入地址 0x%08X 越界\n", 
               tv.tv_sec, (long)tv.tv_usec, addr);
        fflush(stdout);
        return -1;
    }
    
    printf("[%ld.%06ld] device_memory_write - 准备写入数据到内存地址 0x%p\n", 
           tv.tv_sec, (long)tv.tv_usec, (void*)(region->data + offset));
    fflush(stdout);
    
    // 写入32位值
    uint32_t* write_addr = (uint32_t*)(region->data + offset);
    *write_addr = value;
    
    gettimeofday(&tv, NULL);
    printf("[%ld.%06ld] device_memory_write - 数据已写入，地址: 0x%08X, 区域基址: 0x%08X, 偏移: %u, 值: 0x%08X\n", 
           tv.tv_sec, (long)tv.tv_usec, addr, region->base_addr, offset, value);
    fflush(stdout);
    
    // 记录访问
    if (mem->monitor) {
        gettimeofday(&tv, NULL);
        printf("[%ld.%06ld] device_memory_write - 准备记录内存访问\n", tv.tv_sec, (long)tv.tv_usec);
        fflush(stdout);
        
        struct timeval start_time, end_time;
        gettimeofday(&start_time, NULL);
        
        // 调用memory_access_record
        printf("[%ld.%06ld] device_memory_write - 调用 memory_access_record...\n", 
              start_time.tv_sec, (long)start_time.tv_usec);
        fflush(stdout);
        
        memory_access_record(mem->monitor, addr, value, 1, region->device_type, region->device_id);
        
        gettimeofday(&end_time, NULL);
        double elapsed = (end_time.tv_sec - start_time.tv_sec) + 
                       (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
        
        printf("[%ld.%06ld] device_memory_write - 内存访问记录处理耗时 %.6f 秒\n", 
              end_time.tv_sec, (long)end_time.tv_usec, elapsed);
        fflush(stdout);
    } else {
        gettimeofday(&tv, NULL);
        printf("[%ld.%06ld] device_memory_write - 无监视器，跳过记录访问\n", tv.tv_sec, (long)tv.tv_usec);
        fflush(stdout);
        
        // 即使没有监视器，也直接检查设备规则表
        gettimeofday(&tv, NULL);
        printf("[%ld.%06ld] device_memory_write - 直接检查设备规则表\n", tv.tv_sec, (long)tv.tv_usec);
        fflush(stdout);
        
        // 获取设备规则表并检查
        int rule_count = 0;
        const rule_table_entry_t* rules = get_device_rules(region->device_type, &rule_count);
        
        if (rules && rule_count > 0) {
            printf("[%ld.%06ld] device_memory_write - 找到 %d 条规则，开始检查\n", 
                  tv.tv_sec, (long)tv.tv_usec, rule_count);
            fflush(stdout);
            
            // 检查每条规则
            for (int i = 0; i < rule_count; i++) {
                const rule_table_entry_t* rule = &rules[i];
                
                // 检查是否满足触发条件
                if (rule->trigger.trigger_addr == addr) {
                    uint32_t masked_value = value & rule->trigger.expected_mask;
                    uint32_t expected_masked = rule->trigger.expected_value & rule->trigger.expected_mask;
                    
                    if (masked_value == expected_masked) {
                        printf("[%ld.%06ld] device_memory_write - 规则 \"%s\" 触发条件满足，执行处理动作\n", 
                              tv.tv_sec, (long)tv.tv_usec, rule->name);
                        fflush(stdout);
                        
                        // 创建临时规则
                        action_rule_t temp_rule;
                        temp_rule.rule_id = 0;  // 临时ID
                        temp_rule.name = rule->name;
                        temp_rule.trigger = rule->trigger;
                        temp_rule.priority = rule->priority;
                        memcpy(&temp_rule.targets, &rule->targets, sizeof(action_target_array_t));
                        
                        // 尝试获取全局管理器
                        extern device_manager_t* device_manager_get_instance(void);
                        
                        device_manager_t* dm = device_manager_get_instance();
                        
                        // 由于没有全局的action_manager_get_instance函数，我们只能使用设备管理器
                        if (dm) {
                            printf("[%ld.%06ld] device_memory_write - 找到设备管理器，但无法获取动作管理器，无法执行规则\n", 
                                  tv.tv_sec, (long)tv.tv_usec);
                            fflush(stdout);
                            
                            // 这里我们可以直接执行动作，而不是通过动作管理器
                            // 但这需要实现一个简化版的执行动作函数
                            for (int j = 0; j < temp_rule.targets.count; j++) {
                                action_target_t* target = &temp_rule.targets.targets[j];
                                if (target->type == ACTION_TYPE_WRITE) {
                                    // 获取目标设备
                                    device_instance_t* target_device = 
                                        device_get(dm, target->device_type, target->device_id);
                                    
                                    if (target_device) {
                                        // 执行写入操作
                                        uint32_t write_value = target->target_value;
                                        printf("[%ld.%06ld] device_memory_write - 执行写入操作: 设备类型=%d, 设备ID=%d, 地址=0x%08X, 值=0x%08X\n", 
                                              tv.tv_sec, (long)tv.tv_usec, target->device_type, target->device_id, 
                                              target->target_addr, write_value);
                                        fflush(stdout);
                                        
                                        // 调用设备写入函数
                                        // 获取设备类型
                                        device_type_t* device_type = &dm->types[target->device_type];
                                        if (device_type && device_type->ops.write) {
                                            int result = device_type->ops.write(target_device, 
                                                                             target->target_addr, 
                                                                             write_value);
                                            printf("[%ld.%06ld] device_memory_write - 写入结果: %d\n", 
                                                  tv.tv_sec, (long)tv.tv_usec, result);
                                            fflush(stdout);
                                        } else {
                                            printf("[%ld.%06ld] device_memory_write - 设备不支持写入操作\n", 
                                                  tv.tv_sec, (long)tv.tv_usec);
                                            fflush(stdout);
                                        }
                                    } else {
                                        printf("[%ld.%06ld] device_memory_write - 未找到目标设备\n", 
                                              tv.tv_sec, (long)tv.tv_usec);
                                        fflush(stdout);
                                    }
                                } else {
                                    printf("[%ld.%06ld] device_memory_write - 不支持的动作类型: %d\n", 
                                          tv.tv_sec, (long)tv.tv_usec, target->type);
                                    fflush(stdout);
                                }
                            }
                        } else {
                            printf("[%ld.%06ld] device_memory_write - 无法获取设备管理器，无法执行规则\n", 
                                  tv.tv_sec, (long)tv.tv_usec);
                            fflush(stdout);
                        }
                    }
                }
            }
        } else {
            printf("[%ld.%06ld] device_memory_write - 未找到设备类型 %d 的规则\n", 
                  tv.tv_sec, (long)tv.tv_usec, region->device_type);
            fflush(stdout);
        }
    }
    
    gettimeofday(&tv, NULL);
    printf("[%ld.%06ld] device_memory_write - 写入操作完成\n", tv.tv_sec, (long)tv.tv_usec);
    fflush(stdout);
    return 0;
}

// 读取字节
int device_memory_read_byte(device_memory_t* mem, uint32_t addr, uint8_t* value) {
    if (!mem || !value) return -1;
    
    // 查找地址所在的区域
    memory_region_t* region = device_memory_find_region(mem, addr);
    if (!region) {
        printf("Error: Byte read at invalid address 0x%08X\n", addr);
        return -1;
    }
    
    // 计算区域内的偏移量
    uint32_t offset = addr - region->base_addr;
    
    // 防止越界访问
    if (offset >= region->unit_size * region->length) {
        printf("Error: Byte read out of bounds at address 0x%08X\n", addr);
        return -1;
    }
    
    *value = region->data[offset];
    return 0;
}

// 写入字节
int device_memory_write_byte(device_memory_t* mem, uint32_t addr, uint8_t value) {
    if (!mem) return -1;
    
    // 查找地址所在的区域
    memory_region_t* region = device_memory_find_region(mem, addr);
    if (!region) {
        printf("Error: Byte write at invalid address 0x%08X\n", addr);
        return -1;
    }
    
    // 计算区域内的偏移量
    uint32_t offset = addr - region->base_addr;
    
    // 防止越界访问
    if (offset >= region->unit_size * region->length) {
        printf("Error: Byte write out of bounds at address 0x%08X\n", addr);
        return -1;
    }
    
    region->data[offset] = value;
    
    // 通知全局监视器（对于字节写入，我们需要处理包含此字节的32位值）
    if (mem->monitor) {
        uint32_t aligned_addr = addr & ~0x3;  // 对齐到32位边界
        
        // 使用新的函数处理字节写入导致的地址变化
        global_monitor_handle_address_range_changes(
            (global_monitor_t*)mem->monitor,
            region->device_type,
            region->device_id,
            aligned_addr,
            aligned_addr + 4,  // 只处理一个32位地址
            region->data,
            region->unit_size * region->length
        );
    }
    
    return 0;
}

// 批量读取内存
int device_memory_read_buffer(device_memory_t* mem, uint32_t addr, uint8_t* buffer, size_t length) {
    if (!mem || !buffer || length == 0) return -1;
    
    // 查找地址所在的区域
    memory_region_t* region = device_memory_find_region(mem, addr);
    if (!region) {
        printf("Error: Buffer read at invalid address 0x%08X\n", addr);
        return -1;
    }
    
    // 计算区域内的偏移量
    uint32_t offset = addr - region->base_addr;
    
    // 防止越界访问
    if (offset + length > region->unit_size * region->length) {
        printf("Error: Buffer read out of bounds at address 0x%08X, length %zu\n", addr, length);
        return -1;
    }
    
    memcpy(buffer, region->data + offset, length);
    return 0;
}

// 批量写入内存
int device_memory_write_buffer(device_memory_t* mem, uint32_t addr, const uint8_t* buffer, size_t length) {
    if (!mem || !buffer || length == 0) return -1;
    
    // 查找地址所在的区域
    memory_region_t* region = device_memory_find_region(mem, addr);
    if (!region) {
        printf("Error: Buffer write at invalid address 0x%08X\n", addr);
        return -1;
    }
    
    // 计算区域内的偏移量
    uint32_t offset = addr - region->base_addr;
    
    // 防止越界访问
    if (offset + length > region->unit_size * region->length) {
        printf("Error: Buffer write out of bounds at address 0x%08X, length %zu\n", addr, length);
        return -1;
    }
    
    memcpy(region->data + offset, buffer, length);
    
    // 通知全局监视器（对于每个对齐的32位地址）
    if (mem->monitor) {
        uint32_t start_addr = addr & ~0x3;  // 对齐到32位边界
        uint32_t end_addr = (addr + length + 3) & ~0x3;  // 对齐到32位边界
        
        // 使用新的函数处理地址范围内的监视点变化
        global_monitor_handle_address_range_changes(
            (global_monitor_t*)mem->monitor,
            region->device_type,
            region->device_id,
            start_addr,
            end_addr,
            region->data,
            region->unit_size * region->length
        );
    }
    
    return 0;
} 