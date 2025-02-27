#ifndef DEVICE_RULES_H
#define DEVICE_RULES_H

#include <stdint.h>
#include <pthread.h>

// 前向声明
struct action_target;
typedef struct action_target action_target_t;

// 设备规则结构
typedef struct {
    uint32_t addr;            // 监视地址
    uint32_t expected_value;  // 期望值
    uint32_t expected_mask;   // 掩码
    action_target_t* targets; // 目标处理动作
    int active;               // 是否激活
} device_rule_t;

// 设备规则管理接口
typedef struct device_rule_manager {
    device_rule_t* rules;     // 规则数组
    int rule_count;           // 当前规则数量
    int max_rules;            // 最大规则数量
    pthread_mutex_t* mutex;   // 互斥锁指针（设备自己的互斥锁）
} device_rule_manager_t;

// 初始化设备规则管理器
static inline void device_rule_manager_init(device_rule_manager_t* manager, pthread_mutex_t* mutex, device_rule_t* rules, int max_rules) {
    if (!manager) return;
    manager->rules = rules;
    manager->rule_count = 0;
    manager->max_rules = max_rules;
    manager->mutex = mutex;
}

// 添加设备规则
// 注意：这个函数需要在包含了action_manager.h的源文件中使用
int device_rule_add(device_rule_manager_t* manager, uint32_t addr, 
                   uint32_t expected_value, uint32_t expected_mask, 
                   action_target_t* targets);

// 检查值是否匹配规则条件
static inline int device_rule_check_match(uint32_t value, uint32_t expected_value, uint32_t expected_mask) {
    return (value & expected_mask) == (expected_value & expected_mask);
}

#endif /* DEVICE_RULES_H */ 