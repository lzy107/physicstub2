// include/plugin_system.h
#ifndef PLUGIN_SYSTEM_H
#define PLUGIN_SYSTEM_H

#include <dlfcn.h>
#include "device_types.h"

// 插件操作结构
typedef struct {
    void* handle;              // 动态库句柄
    device_ops_t* dev_ops;     // 设备操作接口
    char name[32];             // 插件名称
    UT_hash_handle hh;         // uthash句柄
} plugin_t;

// 插件加载器结构
typedef struct {
    plugin_t* plugins;         // 插件哈希表
    pthread_mutex_t mutex;     // 互斥锁
} plugin_loader_t;

// API函数声明
plugin_loader_t* plugin_loader_create(void);
void plugin_loader_destroy(plugin_loader_t* pl);
int plugin_load(plugin_loader_t* pl, const char* path);
void plugin_unload(plugin_loader_t* pl, const char* name);
device_ops_t* plugin_get_ops(plugin_loader_t* pl, const char* name);

#endif