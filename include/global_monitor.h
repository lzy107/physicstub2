// include/global_monitor.h
#ifndef GLOBAL_MONITOR_H
#define GLOBAL_MONITOR_H

#include <stdint.h>
#include "uthash.h"
#include "action_manager.h"

// 监视点结构
typedef struct {
    uint32_t addr;           // 监视地址
    uint32_t expect_value;   // 期望值
    action_rule_t* rule;     // 关联动作规则
    UT_hash_handle hh;       // uthash句柄
} watch_point_t;

// 全局监视器结构
typedef struct {
    watch_point_t* watches;   // 监视点哈希表
    action_manager_t* am;     // 动作管理器
    pthread_mutex_t mutex;    // 互斥锁
} global_monitor_t;

// API函数声明
global_monitor_t* global_monitor_create(action_manager_t* am);
void global_monitor_destroy(global_monitor_t* gm);
int global_monitor_add_watch(global_monitor_t* gm, uint32_t addr, uint32_t value, action_rule_t* rule);
void global_monitor_remove_watch(global_monitor_t* gm, uint32_t addr);
void global_monitor_on_address_change(global_monitor_t* gm, uint32_t addr, uint32_t value);

#endif