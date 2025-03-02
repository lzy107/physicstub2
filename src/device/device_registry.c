#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>  // 添加这个头文件以支持gettimeofday
#include "device_registry.h"
#include "device_types.h"
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
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    printf("[%ld.%06ld] device_manager_get_device_by_addr - 查找地址 0x%08X 对应的设备\n", 
           tv.tv_sec, (long)tv.tv_usec, addr);
    fflush(stdout);
    
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
            // 获取设备的内存对象
            device_memory_t* memory = NULL;
            
            // 根据设备类型获取其内存对象
            if (type->ops.get_memory) {
                memory = type->ops.get_memory(instance);
            } else if (type->type_id == DEVICE_TYPE_TEMP_SENSOR) {
                // 对于温度传感器，尝试获取其内存区域
                temp_sensor_device_t* ts_dev = (temp_sensor_device_t*)instance->priv_data;
                if (ts_dev) {
                    memory = ts_dev->memory;
                }
            }
            
            // 检查地址是否在设备内存范围内
            if (memory) {
                for (int j = 0; j < memory->region_count; j++) {
                    memory_region_t* region = &memory->regions[j];
                    uint32_t end_addr = region->base_addr + region->length * region->unit_size;
                    
                    printf("[%ld.%06ld] device_manager_get_device_by_addr - 检查地址区域: 类型=%d, ID=%d, 区域=%d, 基址=0x%08X, 结束地址=0x%08X\n", 
                           tv.tv_sec, (long)tv.tv_usec, type->type_id, instance->dev_id, j, region->base_addr, end_addr - 1);
                    fflush(stdout);
                    
                    if (addr >= region->base_addr && addr < end_addr) {
                        printf("[%ld.%06ld] device_manager_get_device_by_addr - 找到设备: 类型=%d, ID=%d\n", 
                               tv.tv_sec, (long)tv.tv_usec, type->type_id, instance->dev_id);
                        fflush(stdout);
                        pthread_mutex_unlock(&type->mutex);
                        return instance;
                    }
                }
            } else {
                printf("[%ld.%06ld] device_manager_get_device_by_addr - 设备没有有效的内存: 类型=%d, ID=%d\n", 
                       tv.tv_sec, (long)tv.tv_usec, type->type_id, instance->dev_id);
                fflush(stdout);
            }
            
            instance = instance->next;
        }
        
        pthread_mutex_unlock(&type->mutex);
    }
    
    printf("[%ld.%06ld] device_manager_get_device_by_addr - 未找到匹配地址 0x%08X 的设备，尝试列出所有设备\n", 
           tv.tv_sec, (long)tv.tv_usec, addr);
    fflush(stdout);
    
    // 列出所有设备以帮助调试
    device_manager_dump_devices(dm);
    
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

/**
 * 根据设备类型和ID获取设备实例
 * @param dm 设备管理器
 * @param type_id 设备类型ID
 * @param device_id 设备ID
 * @return 设备实例指针，如果未找到则返回NULL
 */
device_instance_t* device_manager_get_device_by_type_id(device_manager_t* dm, int type_id, int device_id) {
    if (!dm || type_id < 0 || type_id >= MAX_DEVICE_TYPES || device_id <= 0) {
        printf("ERROR: device_manager_get_device_by_type_id - 无效参数: dm=%p, type_id=%d, device_id=%d\n", 
               dm, type_id, device_id);
        return NULL;
    }

    device_type_t* type = &dm->types[type_id];
    device_instance_t* instance = type->instances;
    
    printf("DEBUG: 在类型%d中查找设备ID=%d, 第一个实例=%p\n", type_id, device_id, instance);
    
    while (instance) {
        printf("DEBUG: 检查实例: type_id=%d, dev_id=%d\n", instance->type_id, instance->dev_id);
        if (instance->dev_id == device_id) {
            printf("DEBUG: 找到匹配的设备: type=%d, id=%d, 地址=%p\n", type_id, device_id, instance);
            return instance;
        }
        instance = instance->next;
    }
    
    printf("DEBUG: 未找到匹配的设备: type=%d, id=%d\n", type_id, device_id);
    return NULL;
}

/**
 * 列出所有已注册设备
 * @param dm 设备管理器
 */
void device_manager_list_devices(device_manager_t* dm) {
    if (!dm) {
        printf("ERROR: device_manager_list_devices - 无效参数: dm=NULL\n");
        return;
    }
    
    printf("已注册设备列表:\n");
    for (int i = 0; i < MAX_DEVICE_TYPES; i++) {
        device_type_t* type = &dm->types[i];
        if (type->type_id > 0) {
            printf("设备类型 %d (%s):\n", type->type_id, type->name);
            
            device_instance_t* instance = type->instances;
            int count = 0;
            
            while (instance) {
                printf("  设备ID=%d, 地址=%p\n", instance->dev_id, instance);
                count++;
                instance = instance->next;
            }
            
            if (count == 0) {
                printf("  没有实例\n");
            }
        }
    }
} 