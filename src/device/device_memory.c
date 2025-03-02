#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include "device_memory.h"
#include "device_rule_configs.h"
#include "action_manager.h"

// 声明外部全局变量
extern device_manager_t* g_device_manager;
extern action_manager_t* g_action_manager;

// 查找地址所在的内存区域
memory_region_t* device_memory_find_region(device_memory_t* mem, uint32_t addr) {
    if (!mem || !mem->regions || mem->region_count <= 0) {
        printf("错误: device_memory_find_region - 无效的内存对象或内存区域数组，mem=%p, regions=%p, region_count=%d\n", 
              (void*)mem, mem ? (void*)mem->regions : NULL, mem ? mem->region_count : 0);
        return NULL;
    }
    
    printf("DEBUG: device_memory_find_region - 开始查找地址 0x%08X，内存区域数量: %d\n", addr, mem->region_count);
    
    for (int i = 0; i < mem->region_count; i++) {
        memory_region_t* region = &mem->regions[i];
        uint32_t region_end = region->base_addr + (region->unit_size * region->length);
        
        printf("DEBUG: 区域 #%d: 基址=0x%08X, 结束地址=0x%08X, 数据指针=%p\n", 
               i, region->base_addr, region_end - 1, (void*)region->data);
        
        // 检查地址是否在当前区域范围内
        if (addr >= region->base_addr && addr < region_end) {
            printf("DEBUG: 找到地址 0x%08X 所在区域 #%d\n", addr, i);
            return region;
        }
    }
    
    // 未找到匹配区域，打印所有可用的内存区域信息
    printf("错误：未能找到地址 0x%08X 所在的内存区域\n", addr);
    printf("可用的内存区域如下：\n");
    for (int i = 0; i < mem->region_count; i++) {
        memory_region_t* region = &mem->regions[i];
        uint32_t region_end = region->base_addr + (region->unit_size * region->length);
        printf("区域 %d: 基地址=0x%08X, 结束地址=0x%08X, 单元大小=%zu字节, 长度=%zu单元, 设备类型=%d, 设备ID=%d\n",
               i, region->base_addr, region_end - 1, region->unit_size, region->length, 
               region->device_type, region->device_id);
    }
    
    return NULL;  // 未找到匹配的区域
}

// 创建设备内存（从配置创建）
device_memory_t* device_memory_create_from_config(memory_region_config_t* configs, int config_count, 
                                                void* monitor, 
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
                                     void* monitor, uint32_t device_type, uint32_t device_id) {
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
    
   {
        gettimeofday(&tv, NULL);
        printf("[%ld.%06ld] device_memory_write - 无监视器\n", tv.tv_sec, (long)tv.tv_usec);
        fflush(stdout);
        
        // 直接检查设备规则表
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
                
                printf("[%ld.%06ld] device_memory_write - 检查规则 %d: 触发地址=0x%08X, 当前地址=0x%08X\n", 
                      tv.tv_sec, (long)tv.tv_usec, i, rule->trigger.trigger_addr, addr);
                fflush(stdout);
                
                // 检查是否满足触发条件
                if (rule->trigger.trigger_addr == addr) {
                    uint32_t masked_value = value & rule->trigger.expected_mask;
                    uint32_t expected_masked = rule->trigger.expected_value & rule->trigger.expected_mask;
                    
                    printf("[%ld.%06ld] device_memory_write - 检查规则触发条件: 地址=0x%08X, 值=0x%08X, 掩码=0x%08X, 期望值=0x%08X\n", 
                          tv.tv_sec, (long)tv.tv_usec, rule->trigger.trigger_addr, value, rule->trigger.expected_mask, rule->trigger.expected_value);
                    printf("[%ld.%06ld] device_memory_write - 掩码后的值: 0x%08X, 期望的掩码后的值: 0x%08X\n", 
                          tv.tv_sec, (long)tv.tv_usec, masked_value, expected_masked);
                    fflush(stdout);
                    
                    if (masked_value == expected_masked) {
                        printf("[%ld.%06ld] device_memory_write - 规则 \"%s\" 触发条件满足，执行处理动作\n", 
                              tv.tv_sec, (long)tv.tv_usec, rule->name);
                        fflush(stdout);
                        
                        // 打印规则的目标动作信息
                        printf("[%ld.%06ld] device_memory_write - 规则目标动作数量: %d\n", 
                              tv.tv_sec, (long)tv.tv_usec, rule->targets.count);
                        for (int j = 0; j < rule->targets.count; j++) {
                            action_target_t* target = &rule->targets.targets[j];
                            printf("[%ld.%06ld] device_memory_write - 目标动作[%d]: 类型=%d, 设备类型=%d, 设备ID=%d, 地址=0x%08X, 值=0x%08X\n", 
                                  tv.tv_sec, (long)tv.tv_usec, j, target->type, target->device_type, target->device_id, 
                                  target->target_addr, target->target_value);
                            fflush(stdout);
                        }
                        
                        // 创建临时规则
                        action_rule_t temp_rule;
                        memset(&temp_rule, 0, sizeof(temp_rule));
                        temp_rule.rule_id = 0xFFFF;  // 临时ID
                        temp_rule.name = rule->name;
                        temp_rule.trigger = rule->trigger;
                        temp_rule.priority = rule->priority;
                        temp_rule.targets = rule->targets;
                        
                        // 获取设备管理器和动作管理器
                        device_manager_t* dm = device_manager_get_instance();
                        action_manager_t* am = action_manager_get_instance();
                        
                        printf("[%ld.%06ld] device_memory_write - 获取管理器: dm=%p, am=%p\n", 
                               tv.tv_sec, (long)tv.tv_usec, (void*)dm, (void*)am);
                        
                        // 修改目标动作的设备类型和ID
                        for (int j = 0; j < temp_rule.targets.count; j++) {
                            action_target_t* target = &temp_rule.targets.targets[j];
                            // 如果目标动作没有指定设备类型和ID，则使用当前内存对象的设备类型和ID
                            if (target->device_type == 0) {
                                target->device_type = mem->device_type;
                            }
                            if (target->device_id == 0) {
                                target->device_id = mem->device_id;
                            }
                            printf("[%ld.%06ld] device_memory_write - 更新目标动作[%d]: 类型=%d, 设备类型=%d, 设备ID=%d, 地址=0x%08X, 值=0x%08X\n", 
                                   tv.tv_sec, (long)tv.tv_usec, j, target->type, target->device_type, 
                                   target->device_id, target->target_addr, target->target_value);
                        }
                        
                        if (dm && am) {
                            // 如果找到了设备管理器和动作管理器，则使用动作管理器执行规则
                            printf("[%ld.%06ld] device_memory_write - 找到设备管理器和动作管理器，执行规则\n", 
                                   tv.tv_sec, (long)tv.tv_usec);
                            int result = action_manager_execute_rule(am, &temp_rule, dm);
                            printf("[%ld.%06ld] device_memory_write - 规则执行结果: %d\n", 
                                   tv.tv_sec, (long)tv.tv_usec, result);
                        } else if (dm) {
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
    
    return 0;
} 