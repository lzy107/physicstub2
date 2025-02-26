// src/global_monitor.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global_monitor.h"
#include "device_registry.h"

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
    
    pthread_mutex_lock(&gm->mutex);
    
    // 更新监视点的值
    for (int i = 0; i < gm->watch_count; i++) {
        watch_point_t* wp = &gm->watch_points[i];
        if (wp->device_type == device_type && wp->device_id == device_id && wp->addr == addr) {
            wp->last_value = value;
            break;
        }
    }
    
    // 检查所有规则
    pthread_mutex_lock(&gm->am->mutex);
    for (int i = 0; i < gm->am->rule_count; i++) {
        action_rule_t* rule = &gm->am->rules[i];
        
        // 检查触发条件
        if (rule->trigger.trigger_addr == addr) {
            // 应用掩码并检查值是否匹配
            if ((value & rule->trigger.expected_mask) == 
                (rule->trigger.expected_value & rule->trigger.expected_mask)) {
                // 执行规则
                action_manager_execute_rule(gm->am, rule, gm->dm);
            }
        }
    }
    pthread_mutex_unlock(&gm->am->mutex);
    
    pthread_mutex_unlock(&gm->mutex);
}

// 处理地址范围内的监视点变化
void global_monitor_handle_address_range_changes(global_monitor_t* gm, device_type_id_t device_type,
                                               int device_id, uint32_t start_addr, uint32_t end_addr,
                                               uint8_t* memory_data, size_t memory_size) {
    if (!gm || !gm->am || !memory_data) return;
    
    pthread_mutex_lock(&gm->mutex);
    
    // 首先检查是否有任何监视点与当前设备类型和ID匹配
    int has_watchpoints = 0;
    for (int i = 0; i < gm->watch_count; i++) {
        watch_point_t* wp = &gm->watch_points[i];
        if (wp->device_type == device_type && wp->device_id == device_id) {
            has_watchpoints = 1;
            break;
        }
    }
    
    // 只有在有匹配的监视点时才继续处理
    if (has_watchpoints) {
        // 遍历所有监视点，只处理特定设备和地址范围内的监视点
        for (int i = 0; i < gm->watch_count; i++) {
            watch_point_t* wp = &gm->watch_points[i];
            
            // 只处理匹配当前设备的监视点，且地址在修改范围内的监视点
            if (wp->device_type == device_type && 
                wp->device_id == device_id && 
                wp->addr >= start_addr && wp->addr < end_addr) {
                
                // 确认地址在内存范围内
                if (wp->addr < memory_size) {
                    // 读取最新值
                    uint32_t value;
                    
                    // 检查地址对齐
                    if (wp->addr % 4 == 0 && wp->addr + 4 <= memory_size) {
                        value = *(uint32_t*)(memory_data + wp->addr);
                        
                        // 更新监视点的值
                        wp->last_value = value;
                        
                        // 检查规则触发
                        pthread_mutex_lock(&gm->am->mutex);
                        for (int j = 0; j < gm->am->rule_count; j++) {
                            action_rule_t* rule = &gm->am->rules[j];
                            
                            if (rule->trigger.trigger_addr == wp->addr) {
                                if ((value & rule->trigger.expected_mask) == 
                                    (rule->trigger.expected_value & rule->trigger.expected_mask)) {
                                    // 执行规则
                                    action_manager_execute_rule(gm->am, rule, gm->dm);
                                }
                            }
                        }
                        pthread_mutex_unlock(&gm->am->mutex);
                    }
                }
            }
        }
    }
    
    pthread_mutex_unlock(&gm->mutex);
}

// 添加监视点并设置规则
int global_monitor_setup_watch_rule(global_monitor_t* gm, device_type_id_t device_type, 
                                   int device_id, uint32_t addr, uint32_t expected_value, 
                                   uint32_t expected_mask, action_target_t* targets) {
    if (!gm || !gm->am) return -1;
    
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
    rule.targets = targets;
    
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