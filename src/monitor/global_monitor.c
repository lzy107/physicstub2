// src/global_monitor.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global_monitor.h"
#include "device_registry.h"
#include "device_rules.h"
#include "action_manager.h"
// 添加设备头文件引用
#include "../plugins/flash/flash_device.h"
#include "../plugins/temp_sensor/temp_sensor.h"
#include "../plugins/fpga/fpga_device.h"

global_monitor_t* global_monitor_create(action_manager_t* am, device_manager_t* dm) {
    global_monitor_t* gm = (global_monitor_t*)calloc(1, sizeof(global_monitor_t));
    if (!gm) return NULL;
    
    pthread_mutex_init(&gm->mutex, NULL);
    gm->am = am;
    gm->dm = dm;
    gm->watch_points = NULL;
    gm->watch_count = 0;
    
    return gm;
}

void global_monitor_destroy(global_monitor_t* gm) {
    if (!gm) return;
    
    pthread_mutex_lock(&gm->mutex);
    
    if (gm->watch_points) {
        free(gm->watch_points);
        gm->watch_points = NULL;
    }
    
    pthread_mutex_unlock(&gm->mutex);
    pthread_mutex_destroy(&gm->mutex);
    free(gm);
}

int global_monitor_add_watch(global_monitor_t* gm, device_type_id_t device_type, 
                            int device_id, uint32_t addr) {
    if (!gm) return -1;
    
    pthread_mutex_lock(&gm->mutex);
    
    // 检查是否已存在相同的监视点
    for (int i = 0; i < gm->watch_count; i++) {
        watch_point_t* wp = &gm->watch_points[i];
        if (wp->device_type == device_type && wp->device_id == device_id && wp->addr == addr) {
            pthread_mutex_unlock(&gm->mutex);
            return 0; // 已存在，无需添加
        }
    }
    
    // 分配新的监视点数组
    watch_point_t* new_watches = (watch_point_t*)realloc(gm->watch_points, 
        (gm->watch_count + 1) * sizeof(watch_point_t));
        
    if (!new_watches) {
        pthread_mutex_unlock(&gm->mutex);
        return -1;
    }
    
    gm->watch_points = new_watches;
    
    // 初始化新的监视点
    watch_point_t* wp = &gm->watch_points[gm->watch_count];
    wp->device_type = device_type;
    wp->device_id = device_id;
    wp->addr = addr;
    wp->last_value = 0;
    
    // 尝试读取当前值
    device_instance_t* instance = device_get(gm->dm, device_type, device_id);
    if (instance) {
        device_ops_t* ops = &gm->dm->types[device_type].ops;
        if (ops && ops->read) {
            ops->read(instance, addr, &wp->last_value);
        }
    }
    
    gm->watch_count++;
    
    pthread_mutex_unlock(&gm->mutex);
    return 0;
}

void global_monitor_remove_watch(global_monitor_t* gm, device_type_id_t device_type, 
                                int device_id, uint32_t addr) {
    if (!gm) return;
    
    pthread_mutex_lock(&gm->mutex);
    
    for (int i = 0; i < gm->watch_count; i++) {
        watch_point_t* wp = &gm->watch_points[i];
        if (wp->device_type == device_type && wp->device_id == device_id && wp->addr == addr) {
            // 移动后面的监视点
            if (i < gm->watch_count - 1) {
                memmove(&gm->watch_points[i], &gm->watch_points[i + 1], 
                    (gm->watch_count - i - 1) * sizeof(watch_point_t));
            }
            gm->watch_count--;
            break;
        }
    }
    
    pthread_mutex_unlock(&gm->mutex);
}

void global_monitor_handle_address_change(global_monitor_t* gm, device_type_id_t device_type, 
                                         int device_id, uint32_t addr, uint32_t value) {
    if (!gm || !gm->am) return;
    
    // 创建一个临时缓冲区来存储当前值
    uint8_t buffer[4];
    *(uint32_t*)buffer = value;
    
    // 将单地址变化处理转发到范围变化处理函数
    global_monitor_handle_address_range_changes(gm, device_type, device_id, addr, addr + 4, buffer, 4);
}

// 检查值是否匹配规则条件
static inline int check_rule_match(uint32_t value, uint32_t expected_value, uint32_t expected_mask) {
    return (value & expected_mask) == (expected_value & expected_mask);
}

// 执行动作目标
static void execute_action_target(action_target_t* target, device_manager_t* dm) {
    if (!target || !dm) return;
    
    switch (target->type) {
        case ACTION_TYPE_WRITE: {
            // 执行写操作
            device_instance_t* target_dev = device_get(dm, target->device_type, target->device_id);
            if (target_dev) {
                device_type_t* dev_type = &dm->types[target->device_type];
                if (dev_type->ops.write) {
                    dev_type->ops.write(target_dev, target->target_addr, target->target_value);
                }
            }
            break;
        }
        case ACTION_TYPE_CALLBACK:
            // 执行回调函数
            if (target->callback) {
                target->callback(target->callback_data ? 
                               target->callback_data : &target->target_value);
            }
            break;
        default:
            // 其他类型暂不处理
            break;
    }
}

// 处理设备规则
static int handle_device_rules(device_rule_manager_t* rule_manager, uint32_t start_addr, uint32_t end_addr, 
                             uint8_t* memory_data, size_t memory_size, device_manager_t* dm) {
    if (!rule_manager || !rule_manager->rules || !memory_data || !dm) return 0;
    
    int rules_triggered = 0;
    pthread_mutex_lock(rule_manager->mutex);
    
    // 遍历所有规则
    for (int i = 0; i < rule_manager->rule_count; i++) {
        device_rule_t* rule = &rule_manager->rules[i];
        if (!rule->active) continue;
        
        // 检查地址是否在范围内
        if (rule->addr < start_addr || rule->addr >= end_addr) {
            continue;
        }
        
        // 读取当前值
        uint32_t value = *(uint32_t*)(memory_data + (rule->addr - start_addr));
        
        // 检查条件是否满足
        if ((value & rule->expected_mask) == (rule->expected_value & rule->expected_mask)) {
            // 执行所有目标动作
            for (int j = 0; j < rule->targets->count; j++) {
                action_target_t* target = &rule->targets->targets[j];
                execute_action_target(target, dm);
            }
            rules_triggered++;
        }
    }
    
    pthread_mutex_unlock(rule_manager->mutex);
    return rules_triggered;
}

// 处理地址范围内的监视点变化
void global_monitor_handle_address_range_changes(global_monitor_t* gm, device_type_id_t device_type,
                                               int device_id, uint32_t start_addr, uint32_t end_addr,
                                               uint8_t* memory_data, size_t memory_size) {
    // 参数验证
    if (!gm || !gm->am || !memory_data || start_addr >= end_addr || start_addr >= memory_size) {
        return;
    }
    
    // 防止越界访问
    if (end_addr > memory_size) {
        end_addr = memory_size;
    }
    
    // 获取设备实例，用于检查设备特定规则
    device_instance_t* instance = device_get(gm->dm, device_type, device_id);
    
    // 处理设备特定规则 - 使用设备提供的钩子函数
    if (instance) {
        device_type_t* dev_type = &gm->dm->types[device_type];
        if (dev_type->ops.get_rule_manager) {
            device_rule_manager_t* rule_manager = dev_type->ops.get_rule_manager(instance);
            if (rule_manager) {
                handle_device_rules(rule_manager, start_addr, end_addr, memory_data, memory_size, gm->dm);
            }
        }
    }
    
}

// 添加监视点并设置规则
int global_monitor_setup_watch_rule(global_monitor_t* gm, device_type_id_t device_type, 
                                   int device_id, uint32_t addr, uint32_t expected_value, 
                                   uint32_t expected_mask, const action_target_array_t* targets) {
    if (!gm || !gm->am || !targets) return -1;
    
    // 添加监视点
    if (global_monitor_add_watch(gm, device_type, device_id, addr) != 0) {
        return -1;
    }
    
    // 创建规则
    action_rule_t rule;
    memset(&rule, 0, sizeof(action_rule_t));
    
    // 生成规则ID
    static int next_rule_id = 1000; // 使用不同的ID范围，避免与预定义规则冲突
    rule.rule_id = next_rule_id++;
    
    // 设置规则名称
    char rule_name_buf[64];
    snprintf(rule_name_buf, sizeof(rule_name_buf), "Watch_Rule_%d_%d_%08X", device_type, device_id, addr);
    char* rule_name = strdup(rule_name_buf);
    if (!rule_name) {
        rule.name = "Watch_Rule";
    } else {
        rule.name = rule_name;
    }
    
    // 设置触发条件
    rule.trigger = rule_trigger_create(addr, expected_value, expected_mask);
    
    // 设置目标处理动作
    memcpy(&rule.targets, targets, sizeof(action_target_array_t)); // 直接复制整个数组
    
    // 设置优先级
    rule.priority = 100; // 默认优先级
    
    // 添加规则
    int result = action_manager_add_rule(gm->am, &rule);
    
    // 清理临时分配的内存
    if (rule_name && strcmp(rule_name, "Watch_Rule") != 0) {
        free(rule_name);
    }
    
    return result;
}



// 记录内存访问
void memory_access_record(global_monitor_t* monitor, uint32_t addr, uint32_t value, 
                         int is_write, uint32_t device_type, uint32_t device_id) {
    if (!monitor) return;
    
    // 创建一个临时缓冲区来存储当前值
    uint8_t buffer[4];
    *(uint32_t*)buffer = value;
    
    // 将内存访问记录转发到地址范围变化处理函数
    global_monitor_handle_address_range_changes(monitor, device_type, device_id, 
                                              addr, addr + 4, buffer, 4);
}