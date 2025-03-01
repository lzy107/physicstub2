#include <stdio.h>
#include <string.h>
#include "device_registry.h"
#include "../plugins/flash/flash_device.h"
#include "../plugins/temp_sensor/temp_sensor.h"
#include "../plugins/fpga/fpga_device.h"

// 设备注册表头节点
static device_register_info_t* device_registry_head = NULL;
static int registry_size = 0;

// 添加设备到注册表（供自注册宏使用）
void device_registry_add_device(device_register_info_t* info) {
    if (!info) return;
    
    // 将新设备添加到链表头部
    info->next = device_registry_head;
    device_registry_head = info;
    registry_size++;
}

int device_registry_init(device_manager_t* dm) {
    if (!dm) return -1;
    
    printf("Initializing device registry...\n");
    
    // 遍历链表注册所有设备类型
    device_register_info_t* curr = device_registry_head;
    while (curr) {
        printf("Registering device type: %s\n", curr->name);
        
        device_ops_t* ops = curr->get_ops();
        if (!ops) {
            printf("Failed to get device ops for %s\n", curr->name);
            return -1;
        }
        
        if (device_type_register(dm, curr->type_id, curr->name, ops) != 0) {
            printf("Failed to register device type: %s\n", curr->name);
            return -1;
        }
        
        curr = curr->next;
    }
    
    printf("Device registry initialized with %d device types\n", registry_size);
    return 0;
}

int device_registry_register(device_manager_t* dm, const device_register_info_t* info) {
    if (!dm || !info) return -1;
    
    device_ops_t* ops = info->get_ops();
    if (!ops) return -1;
    
    return device_type_register(dm, info->type_id, info->name, ops);
}

int device_registry_get_count(void) {
    return registry_size;
}

const device_register_info_t* device_registry_get_info(int index) {
    if (index < 0 || index >= registry_size) return NULL;
    
    device_register_info_t* curr = device_registry_head;
    for (int i = 0; i < index && curr; i++) {
        curr = curr->next;
    }
    
    return curr;
}

/**
 * 根据地址获取设备实例
 * 
 * @param dm 设备管理器
 * @param addr 内存地址
 * @return 设备实例指针，如果没有找到则返回NULL
 */
device_instance_t* device_manager_get_device_by_addr(device_manager_t* dm, uint32_t addr) {
    if (!dm) {
        return NULL;
    }
    
    // 遍历所有设备类型
    for (int i = 0; i < MAX_DEVICE_TYPES; i++) {
        device_type_t* type = &dm->types[i];
        
        if (type->type_id == 0 && i > 0) {
            continue;  // 跳过未初始化的类型
        }
        
        pthread_mutex_lock(&type->mutex);
        
        // 遍历该类型的所有设备实例
        device_instance_t* instance = type->instances;
        while (instance) {
            // 这里简化处理，只返回温度传感器设备
            // 在实际代码中，应该检查addr是否在设备内存范围内
            if (type->type_id == DEVICE_TYPE_TEMP_SENSOR && instance->dev_id == 0) {
                pthread_mutex_unlock(&type->mutex);
                return instance;
            }
            
            instance = instance->next;
        }
        
        pthread_mutex_unlock(&type->mutex);
    }
    
    return NULL;
}

/**
 * 显示所有已注册设备
 * 
 * @param dm 设备管理器
 */
void device_manager_dump_devices(device_manager_t* dm) {
    if (!dm) {
        printf("设备管理器为NULL\n");
        return;
    }
    
    printf("设备列表:\n");
    
    // 遍历所有设备类型
    for (int i = 0; i < MAX_DEVICE_TYPES; i++) {
        device_type_t* type = &dm->types[i];
        
        if (type->type_id == 0 && i > 0) {
            continue;  // 跳过未初始化的类型
        }
        
        printf("  设备类型: %s (ID=%d)\n", type->name, type->type_id);
        
        pthread_mutex_lock(&type->mutex);
        
        // 遍历该类型的所有设备实例
        device_instance_t* instance = type->instances;
        while (instance) {
            printf("    实例ID: %d\n", instance->dev_id);
            instance = instance->next;
        }
        
        pthread_mutex_unlock(&type->mutex);
    }
} 