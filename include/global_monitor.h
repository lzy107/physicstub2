// include/global_monitor.h
#ifndef GLOBAL_MONITOR_H
#define GLOBAL_MONITOR_H

#include <pthread.h>
#include <stdint.h>
#include "action_manager.h"
#include "device_registry.h"

// 监视点结构
typedef struct {
    device_type_id_t device_type;  // 设备类型
    int device_id;                 // 设备ID
    uint32_t addr;                 // 监视的地址
    uint32_t last_value;           // 最后一次读取的值
} watch_point_t;

// 全局监视器
typedef struct {
    pthread_mutex_t mutex;         // 互斥锁
    action_manager_t* am;          // 动作管理器
    device_manager_t* dm;          // 设备管理器
    watch_point_t* watch_points;   // 监视点数组
    int watch_count;               // 监视点数量
} global_monitor_t;

// 创建全局监视器
global_monitor_t* global_monitor_create(action_manager_t* am, device_manager_t* dm);

// 销毁全局监视器
void global_monitor_destroy(global_monitor_t* gm);

// 添加监视点
int global_monitor_add_watch(global_monitor_t* gm, device_type_id_t device_type, 
                            int device_id, uint32_t addr);

// 移除监视点
void global_monitor_remove_watch(global_monitor_t* gm, device_type_id_t device_type, 
                                int device_id, uint32_t addr);

// 处理地址变化
void global_monitor_handle_address_change(global_monitor_t* gm, device_type_id_t device_type, 
                                         int device_id, uint32_t addr, uint32_t value);

// 添加监视点并设置规则
int global_monitor_setup_watch_rule(global_monitor_t* gm, device_type_id_t device_type, 
                                   int device_id, uint32_t addr, uint32_t expected_value, 
                                   uint32_t expected_mask, action_target_t* targets);

#endif /* GLOBAL_MONITOR_H */