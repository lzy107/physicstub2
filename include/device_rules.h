#ifndef DEVICE_RULES_H
#define DEVICE_RULES_H

#include <stdint.h>
#include <pthread.h>

// 前向声明
struct action_target;
typedef struct action_target action_target_t;
struct action_target_array;
typedef struct action_target_array action_target_array_t;

// 设备规则结构
typedef struct {
    uint32_t addr;                  // 监控地址
    uint32_t expected_value;        // 期望值
    uint32_t expected_mask;         // 掩码
    action_target_array_t targets;  // 目标动作数组（直接包含，不是指针）
    int active;                     // 规则是否激活
} device_rule_t;

// 设备规则管理器
typedef struct {
    pthread_mutex_t* mutex;         // 互斥锁
    device_rule_t* rules;           // 规则数组
    int rule_count;                 // 规则数量
    int rule_capacity;              // 规则容量
} device_rule_manager_t;

// 初始化设备规则管理器
void device_rule_manager_init(device_rule_manager_t* manager, pthread_mutex_t* mutex);

// 创建设备规则管理器
device_rule_manager_t* device_rule_manager_create(pthread_mutex_t* mutex);

// 添加设备规则
int device_rule_add(device_rule_manager_t* manager, uint32_t addr, 
                   uint32_t expected_value, uint32_t expected_mask, 
                   const action_target_array_t* targets);

// 检查值是否匹配规则条件
static inline int device_rule_check_match(uint32_t value, uint32_t expected_value, uint32_t expected_mask) {
    return (value & expected_mask) == (expected_value & expected_mask);
}

#endif /* DEVICE_RULES_H */ 