// src/plugin_system.c
#include <stdlib.h>
#include <string.h>
#include "plugin_system.h"

plugin_loader_t* plugin_loader_create(void) {
    plugin_loader_t* pl = (plugin_loader_t*)calloc(1, sizeof(plugin_loader_t));
    if (!pl) return NULL;
    
    pthread_mutex_init(&pl->mutex, NULL);
    pl->plugins = NULL;
    
    return pl;
}

void plugin_loader_destroy(plugin_loader_t* pl) {
    if (!pl) return;
    
    pthread_mutex_lock(&pl->mutex);
    
    // 先将所有插件从哈希表中移除，但不立即释放资源
    plugin_t* plugins_to_free = NULL;
    plugin_t* current, *tmp;
    HASH_ITER(hh, pl->plugins, current, tmp) {
        HASH_DEL(pl->plugins, current);
        // 将插件添加到待释放列表
        current->hh.next = (void*)plugins_to_free;
        plugins_to_free = current;
    }
    
    // 释放互斥锁，因为我们已经安全地移除了所有插件
    pthread_mutex_unlock(&pl->mutex);
    pthread_mutex_destroy(&pl->mutex);
    
    // 现在安全地释放每个插件的资源
    while (plugins_to_free) {
        current = plugins_to_free;
        plugins_to_free = (plugin_t*)current->hh.next;
        
        if (current->handle) {
            dlclose(current->handle);
        }
        free(current);
    }
    
    // 最后释放插件加载器
    free(pl);
}

int plugin_load(plugin_loader_t* pl, const char* path) {
    if (!pl || !path) return -1;
    
    pthread_mutex_lock(&pl->mutex);
    
    // 打开动态库
    void* handle = dlopen(path, RTLD_NOW);
    if (!handle) {
        pthread_mutex_unlock(&pl->mutex);
        return -1;
    }
    
    // 获取设备操作接口
    device_ops_t* (*get_ops)() = dlsym(handle, "get_device_ops");
    if (!get_ops) {
        dlclose(handle);
        pthread_mutex_unlock(&pl->mutex);
        return -1;
    }
    
    // 获取插件名称
    const char* (*get_name)() = dlsym(handle, "get_plugin_name");
    if (!get_name) {
        dlclose(handle);
        pthread_mutex_unlock(&pl->mutex);
        return -1;
    }
    
    // 创建插件实例
    plugin_t* plugin = (plugin_t*)calloc(1, sizeof(plugin_t));
    if (!plugin) {
        dlclose(handle);
        pthread_mutex_unlock(&pl->mutex);
        return -1;
    }
    
    strncpy(plugin->name, get_name(), sizeof(plugin->name) - 1);
    plugin->handle = handle;
    plugin->dev_ops = get_ops();
    
    // 添加到哈希表
    HASH_ADD_STR(pl->plugins, name, plugin);
    
    pthread_mutex_unlock(&pl->mutex);
    return 0;
}

void plugin_unload(plugin_loader_t* pl, const char* name) {
    if (!pl || !name) return;
    
    pthread_mutex_lock(&pl->mutex);
    plugin_t* plugin;
    HASH_FIND_STR(pl->plugins, name, plugin);
    
    if (plugin) {
        HASH_DEL(pl->plugins, plugin);
        if (plugin->handle) {
            dlclose(plugin->handle);
        }
        free(plugin);
    }
    
    pthread_mutex_unlock(&pl->mutex);
}

device_ops_t* plugin_get_ops(plugin_loader_t* pl, const char* name) {
    if (!pl || !name) return NULL;
    
    device_ops_t* ops = NULL;
    pthread_mutex_lock(&pl->mutex);
    
    plugin_t* plugin;
    HASH_FIND_STR(pl->plugins, name, plugin);
    if (plugin) {
        ops = plugin->dev_ops;
    }
    
    pthread_mutex_unlock(&pl->mutex);
    return ops;
}