#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "device_rules.h"
#include "action_manager.h"

// 添加设备规则
int device_rule_add(device_rule_manager_t* manager, uint32_t addr, 
                   uint32_t expected_value, uint32_t expected_mask,
                   const action_target_array_t* targets) {
    if (!manager || !targets) return -1;
    
    pthread_mutex_lock(manager->mutex);
    
    // 检查是否需要扩展规则数组
    if (manager->rule_count >= manager->rule_capacity) {
        int new_capacity = manager->rule_capacity * 2;
        if (new_capacity == 0) new_capacity = 4;
        
        device_rule_t* new_rules = (device_rule_t*)realloc(manager->rules, 
                                                         new_capacity * sizeof(device_rule_t));
        if (!new_rules) {
            pthread_mutex_unlock(manager->mutex);
            return -1;
        }
        
        manager->rules = new_rules;
        manager->rule_capacity = new_capacity;
    }
    
    // 添加新规则
    device_rule_t* rule = &manager->rules[manager->rule_count];
    rule->addr = addr;
    rule->expected_value = expected_value;
    rule->expected_mask = expected_mask;
    
    // 为targets分配内存并复制内容
    rule->targets = (action_target_array_t*)malloc(sizeof(action_target_array_t));
    if (!rule->targets) {
        pthread_mutex_unlock(manager->mutex);
        return -1;
    }
    memcpy(rule->targets, targets, sizeof(action_target_array_t));
    
    rule->active = 1;
    
    manager->rule_count++;
    
    pthread_mutex_unlock(manager->mutex);
    return 0;
}

// 初始化设备规则管理器
void device_rule_manager_init(device_rule_manager_t* manager, pthread_mutex_t* mutex) {
    if (!manager) return;
    
    manager->mutex = mutex;
    manager->rules = NULL;
    manager->rule_count = 0;
    manager->rule_capacity = 0;
}

// 创建设备规则管理器
device_rule_manager_t* device_rule_manager_create(pthread_mutex_t* mutex) {
    device_rule_manager_t* manager = (device_rule_manager_t*)calloc(1, sizeof(device_rule_manager_t));
    if (!manager) return NULL;
    
    device_rule_manager_init(manager, mutex);
    return manager;
} 