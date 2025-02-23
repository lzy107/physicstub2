// include/action_manager.h
#ifndef ACTION_MANAGER_H
#define ACTION_MANAGER_H

#include <stdint.h>
#include <pthread.h>

// 动作类型
typedef enum {
    ACTION_TYPE_WRITE,    // 写入操作
    ACTION_TYPE_SIGNAL,   // 信号操作
    ACTION_TYPE_CALLBACK  // 回调函数
} action_type_t;

// 动作规则回调函数类型
typedef void (*action_callback_t)(void* data);

// 动作规则结构
typedef struct {
    int rule_id;                 // 规则ID
    uint32_t trigger_addr;       // 触发地址
    uint32_t expect_value;       // 期望值
    action_type_t type;          // 动作类型
    uint32_t target_addr;        // 目标地址
    uint32_t action_value;       // 动作值
    int priority;                // 优先级
    action_callback_t callback;  // 回调函数
} action_rule_t;

// 规则表项结构
typedef struct {
    const char* name;           // 规则名称
    uint32_t trigger_addr;      // 触发地址
    uint32_t expect_value;      // 期望值
    action_type_t type;         // 动作类型
    uint32_t target_addr;       // 目标地址
    uint32_t action_value;      // 动作值
    int priority;               // 优先级
    action_callback_t callback; // 回调函数
} rule_table_entry_t;

// 规则提供者接口
typedef struct {
    const char* provider_name;                    // 提供者名称
    const rule_table_entry_t* (*get_rules)(void); // 获取规则表
    int (*get_rule_count)(void);                  // 获取规则数量
} rule_provider_t;

// 动作管理器结构
typedef struct {
    action_rule_t* rules;      // 规则数组
    int rule_count;            // 规则数量
    pthread_mutex_t mutex;     // 互斥锁
} action_manager_t;

// API函数声明
action_manager_t* action_manager_create(void);
void action_manager_destroy(action_manager_t* am);
int action_manager_add_rules_from_table(action_manager_t* am, const rule_table_entry_t* table, int count);
int action_manager_add_rule(action_manager_t* am, action_rule_t* rule);
void action_manager_remove_rule(action_manager_t* am, int rule_id);
void action_manager_execute_rule(action_manager_t* am, action_rule_t* rule);

// 规则注册相关函数
void action_manager_register_provider(const rule_provider_t* provider);
int action_manager_load_all_rules(action_manager_t* am);

// 规则提供者注册宏
#define REGISTER_RULE_PROVIDER(name, get_rules_func, get_count_func) \
    static const rule_provider_t __rule_provider = { \
        .provider_name = #name, \
        .get_rules = get_rules_func, \
        .get_rule_count = get_count_func \
    }; \
    __attribute__((constructor)) \
    static void __register_rules_##name(void) { \
        action_manager_register_provider(&__rule_provider); \
    }

#endif