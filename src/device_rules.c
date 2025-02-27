#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "device_rules.h"
#include "action_manager.h"

// 添加设备规则
int device_rule_add(device_rule_manager_t* manager, uint32_t addr, 
                   uint32_t expected_value, uint32_t expected_mask, 
                   action_target_t* targets) {
    if (!manager || !manager->rules || !manager->mutex) return -1;
    
    pthread_mutex_lock(manager->mutex);
    
    // 检查是否已达到最大规则数
    if (manager->rule_count >= manager->max_rules) {
        pthread_mutex_unlock(manager->mutex);
        return -1;  // 规则已满
    }
    
    // 检查是否已存在相同地址的规则，如果存在则更新
    int found = -1;
    for (int i = 0; i < manager->rule_count; i++) {
        if (manager->rules[i].addr == addr) {
            found = i;
            break;
        }
    }
    
    if (found >= 0) {
        // 更新现有规则
        if (manager->rules[found].targets) {
            // 销毁旧的目标链表
            action_target_destroy(manager->rules[found].targets);
        }
        
        manager->rules[found].expected_value = expected_value;
        manager->rules[found].expected_mask = expected_mask;
        manager->rules[found].targets = targets;
        manager->rules[found].active = 1;
    } else {
        // 添加新规则
        int idx = manager->rule_count;
        manager->rules[idx].addr = addr;
        manager->rules[idx].expected_value = expected_value;
        manager->rules[idx].expected_mask = expected_mask;
        manager->rules[idx].targets = targets;
        manager->rules[idx].active = 1;
        manager->rule_count++;
    }
    
    pthread_mutex_unlock(manager->mutex);
    return 0;
} 