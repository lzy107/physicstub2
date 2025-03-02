// src/device_types.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "device_types.h"
#include "device_rules.h"

// 全局设备管理器单例
static device_manager_t* g_device_manager = NULL;

device_manager_t* device_manager_get_instance(void) {
    if (!g_device_manager) {
        g_device_manager = device_manager_init();
    }
    return g_device_manager;
}

device_manager_t* device_manager_init(void) {
    device_manager_t* dm = (device_manager_t*)calloc(1, sizeof(device_manager_t));
    if (!dm) return NULL;
    
    pthread_mutex_init(&dm->mutex, NULL);
    for (int i = 0; i < MAX_DEVICE_TYPES; i++) {
        pthread_mutex_init(&dm->types[i].mutex, NULL);
        dm->types[i].instances = NULL;
    }
    
    return dm;
}

void device_manager_destroy(device_manager_t* dm) {
    if (!dm) return;
    
    printf("Starting device manager cleanup...\n");
    
    // 清理所有设备实例和类型
    for (int i = 0; i < MAX_DEVICE_TYPES; i++) {
        printf("Cleaning up device type %d...\n", i);
        
        pthread_mutex_lock(&dm->types[i].mutex);
        device_instance_t* curr = dm->types[i].instances;
        
        while (curr) {
            device_instance_t* next = curr->next;
            printf("  Destroying device instance %d...\n", curr->dev_id);
            
            // 先调用设备特定的清理函数
            if (dm->types[i].ops.destroy) {
                printf("  Calling device specific destroy function...\n");
                dm->types[i].ops.destroy(curr);
                printf("  Device specific destroy completed.\n");
            }
            
            // 最后释放设备实例
            printf("  Freeing device instance...\n");
            free(curr);
            printf("  Device instance freed.\n");
            
            curr = next;
        }
        
        pthread_mutex_unlock(&dm->types[i].mutex);
        printf("  Destroying device type mutex...\n");
        pthread_mutex_destroy(&dm->types[i].mutex);
        printf("Device type %d cleanup completed.\n", i);
    }
    
    printf("Destroying device manager mutex...\n");
    pthread_mutex_destroy(&dm->mutex);
    printf("Freeing device manager...\n");
    free(dm);
    printf("Device manager cleanup completed.\n");
}

int device_type_register(device_manager_t* dm, device_type_id_t type_id, const char* name, device_ops_t* ops) {
    if (!dm || !name || !ops || type_id >= MAX_DEVICE_TYPES) {
        return -1;
    }
    
    pthread_mutex_lock(&dm->mutex);
    
    device_type_t* type = &dm->types[type_id];
    snprintf(type->name, sizeof(type->name), "%s", name);
    type->type_id = type_id;
    type->ops = *ops;
    
    pthread_mutex_unlock(&dm->mutex);
    return 0;
}

device_instance_t* device_create(device_manager_t* dm, device_type_id_t type_id, int dev_id) {
    if (!dm || type_id >= MAX_DEVICE_TYPES) {
        return NULL;
    }
    
    device_type_t* type = &dm->types[type_id];
    pthread_mutex_lock(&type->mutex);
    
    // 检查是否已存在相同ID的实例
    device_instance_t* curr = type->instances;
    while (curr) {
        if (curr->dev_id == dev_id) {
            pthread_mutex_unlock(&type->mutex);
            return NULL;
        }
        curr = curr->next;
    }
    
    // 创建新实例
    device_instance_t* instance = (device_instance_t*)calloc(1, sizeof(device_instance_t));
    if (!instance) {
        pthread_mutex_unlock(&type->mutex);
        return NULL;
    }
    
    instance->dev_id = dev_id;
    instance->type_id = type_id;
    
    // 调用设备特定的初始化
    if (type->ops.init && type->ops.init(instance) != 0) {
        free(instance);
        pthread_mutex_unlock(&type->mutex);
        return NULL;
    }
    
    // 添加到链表头部
    instance->next = type->instances;
    type->instances = instance;
    
    pthread_mutex_unlock(&type->mutex);
    return instance;
}

void device_destroy(device_manager_t* dm, device_type_id_t type_id, int dev_id) {
    if (!dm || type_id >= MAX_DEVICE_TYPES) {
        return;
    }
    
    device_type_t* type = &dm->types[type_id];
    pthread_mutex_lock(&type->mutex);
    
    device_instance_t* prev = NULL;
    device_instance_t* curr = type->instances;
    
    while (curr) {
        if (curr->dev_id == dev_id) {
            if (prev) {
                prev->next = curr->next;
            } else {
                type->instances = curr->next;
            }
            
            if (type->ops.destroy) {
                type->ops.destroy(curr);
            }
            
            free(curr);
            break;
        }
        prev = curr;
        curr = curr->next;
    }
    
    pthread_mutex_unlock(&type->mutex);
}

device_instance_t* device_get(device_manager_t* dm, device_type_id_t type_id, int dev_id) {
    if (!dm || type_id >= MAX_DEVICE_TYPES) {
        return NULL;
    }
    
    device_type_t* type = &dm->types[type_id];
    device_instance_t* instance = NULL;
    
    pthread_mutex_lock(&type->mutex);
    
    device_instance_t* curr = type->instances;
    while (curr) {
        if (curr->dev_id == dev_id) {
            instance = curr;
            break;
        }
        curr = curr->next;
    }
    
    pthread_mutex_unlock(&type->mutex);
    return instance;
}

// 创建设备实例（带配置版本）
device_instance_t* device_create_with_config(device_manager_t* dm, device_type_id_t type_id, 
                                           int dev_id, device_config_t* config) {
    if (!dm || type_id >= MAX_DEVICE_TYPES || !config) {
        return NULL;
    }
    
    device_type_t* type = &dm->types[type_id];
    pthread_mutex_lock(&type->mutex);
    
    // 检查是否已存在相同ID的实例
    device_instance_t* curr = type->instances;
    while (curr) {
        if (curr->dev_id == dev_id) {
            pthread_mutex_unlock(&type->mutex);
            return NULL;
        }
        curr = curr->next;
    }
    
    // 创建新实例
    device_instance_t* instance = (device_instance_t*)calloc(1, sizeof(device_instance_t));
    if (!instance) {
        pthread_mutex_unlock(&type->mutex);
        return NULL;
    }
    
    instance->dev_id = dev_id;
    
    // 调用设备特定的初始化
    if (type->ops.init && type->ops.init(instance) != 0) {
        free(instance);
        pthread_mutex_unlock(&type->mutex);
        return NULL;
    }
    
    // 如果配置中包含内存区域配置，应用它们
    if (config->mem_regions && config->region_count > 0) {
        if (type->ops.configure_memory) {
            if (type->ops.configure_memory(instance, config->mem_regions, config->region_count) != 0) {
                if (type->ops.destroy) {
                    type->ops.destroy(instance);
                }
                free(instance);
                pthread_mutex_unlock(&type->mutex);
                return NULL;
            }
        }
    }
    
    // 如果配置中包含规则，设置它们
    if (config->rules && config->rule_count > 0) {
        struct device_rule_manager* rule_manager = NULL;
        if (type->ops.get_rule_manager) {
            rule_manager = type->ops.get_rule_manager(instance);
        }
        
        if (rule_manager) {
            for (int i = 0; i < config->rule_count; i++) {
                device_rule_t* rule = &config->rules[i];
                device_rule_add(rule_manager, rule->addr, rule->expected_value, 
                               rule->expected_mask, rule->targets);
            }
        }
    }
    
    // 添加到链表头部
    instance->next = type->instances;
    type->instances = instance;
    
    pthread_mutex_unlock(&type->mutex);
    return instance;
}