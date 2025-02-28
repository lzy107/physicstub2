// include/action_manager.h
#ifndef ACTION_MANAGER_H
#define ACTION_MANAGER_H

#include <pthread.h>
#include <stdint.h>
#include "device_types.h"

// 定义最大目标动作数量
#define MAX_ACTION_TARGETS 32

// 动作类型
typedef enum {
    ACTION_TYPE_NONE = 0,
    ACTION_TYPE_WRITE,
    ACTION_TYPE_SIGNAL,
    ACTION_TYPE_CALLBACK
} action_type_t;

// 动作回调函数类型
typedef void (*action_callback_t)(void* data);

// 目标处理动作结构
typedef struct action_target {
    action_type_t type;           // 动作类型
    device_type_id_t device_type; // 目标设备类型
    int device_id;                // 目标设备ID
    uint32_t target_addr;         // 目标地址
    uint32_t target_value;        // 目标值
    uint32_t target_mask;         // 目标掩码
    action_callback_t callback;   // 回调函数
    void* callback_data;          // 回调数据
} action_target_t;

// 目标动作数组结构
typedef struct {
    action_target_t targets[MAX_ACTION_TARGETS]; // 固定大小的目标动作数组
    int count;                                  // 数组中的目标数量
} action_target_array_t;

// 规则触发条件
typedef struct {
    uint32_t trigger_addr;        // 触发地址
    uint32_t expected_value;      // 期望值
    uint32_t expected_mask;       // 期望掩码
} rule_trigger_t;

// 规则表项结构
typedef struct rule_table_entry {
    const char* name;             // 规则名称
    rule_trigger_t trigger;       // 触发条件
    action_target_array_t targets; // 目标处理动作数组（直接包含，不是指针）
    int priority;                 // 优先级
} rule_table_entry_t;

// 动作规则结构
typedef struct action_rule {
    int rule_id;                  // 规则ID
    const char* name;             // 规则名称
    rule_trigger_t trigger;       // 触发条件
    action_target_array_t targets; // 目标处理动作数组（直接包含，不是指针）
    int priority;                 // 优先级
} action_rule_t;

// 规则提供者接口
typedef struct {
    const char* provider_name;
    const rule_table_entry_t* (*get_rules)(void);
    int (*get_rule_count)(void);
} rule_provider_t;

// 动作管理器
typedef struct {
    pthread_mutex_t mutex;        // 互斥锁
    action_rule_t* rules;         // 规则数组
    int rule_count;               // 规则数量
} action_manager_t;

// 创建目标处理动作
action_target_t* action_target_create(action_type_t type, device_type_id_t device_type, int device_id,
                                     uint32_t addr, uint32_t value, uint32_t mask, 
                                     action_callback_t callback, void* callback_data);

// 添加目标处理动作到链表
void action_target_add(action_target_t** head, action_target_t* target);

// 销毁目标处理动作链表
void action_target_destroy(action_target_t* targets);

// 创建规则触发条件
rule_trigger_t rule_trigger_create(uint32_t addr, uint32_t value, uint32_t mask);

// 创建规则表项
rule_table_entry_t* rule_table_entry_create(const char* name, rule_trigger_t trigger, 
                                          action_target_t* targets, int priority);

// 销毁规则表项
void rule_table_entry_destroy(rule_table_entry_t* entry);

// 创建动作管理器
action_manager_t* action_manager_create(void);

// 销毁动作管理器
void action_manager_destroy(action_manager_t* am);

// 注册规则提供者
void action_manager_register_provider(const rule_provider_t* provider);

// 加载所有规则
int action_manager_load_all_rules(action_manager_t* am);

// 从规则表批量添加规则
int action_manager_add_rules_from_table(action_manager_t* am, const rule_table_entry_t* table, int count);

// 添加单个规则
int action_manager_add_rule(action_manager_t* am, action_rule_t* rule);

// 移除规则
void action_manager_remove_rule(action_manager_t* am, int rule_id);

// 执行规则
void action_manager_execute_rule(action_manager_t* am, action_rule_t* rule, device_manager_t* dm);

#endif /* ACTION_MANAGER_H */