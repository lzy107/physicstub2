// src/plugin_system.c
#include <stdlib.h>
#include <string.h>
#include "plugin_system.h"
#include "device_registry.h"

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
    
    // 清理所有插件
    plugin_t* current, *tmp;
    HASH_ITER(hh, pl->plugins, current, tmp) {
        HASH_DEL(pl->plugins, current);
        free(current);
    }
    
    pthread_mutex_unlock(&pl->mutex);
    pthread_mutex_destroy(&pl->mutex);
    free(pl);
}

int plugin_load(plugin_loader_t* pl, const char* name) {
    if (!pl || !name) return -1;
    
    pthread_mutex_lock(&pl->mutex);
    
    // 检查插件是否已经加载
    plugin_t* existing;
    HASH_FIND_STR(pl->plugins, name, existing);
    if (existing) {
        pthread_mutex_unlock(&pl->mutex);
        return 0;  // 插件已加载
    }
    
    // 遍历设备注册表查找对应的设备
    for (int i = 0; i < device_registry_get_count(); i++) {
        const device_register_info_t* info = device_registry_get_info(i);
        if (info && strcmp(info->name, name) == 0) {
            // 创建新的插件实例
            plugin_t* plugin = (plugin_t*)calloc(1, sizeof(plugin_t));
            if (!plugin) {
                pthread_mutex_unlock(&pl->mutex);
                return -1;
            }
            
            strncpy(plugin->name, name, sizeof(plugin->name) - 1);
            plugin->dev_ops = info->get_ops();
            
            // 添加到哈希表
            HASH_ADD_STR(pl->plugins, name, plugin);
            
            pthread_mutex_unlock(&pl->mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&pl->mutex);
    return -1;  // 未找到对应的设备
}

void plugin_unload(plugin_loader_t* pl, const char* name) {
    if (!pl || !name) return;
    
    pthread_mutex_lock(&pl->mutex);
    plugin_t* plugin;
    HASH_FIND_STR(pl->plugins, name, plugin);
    
    if (plugin) {
        HASH_DEL(pl->plugins, plugin);
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