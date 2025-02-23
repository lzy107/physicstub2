#include <stdio.h>
#include <string.h>
#include "device_registry.h"
#include "../plugins/flash_device.h"
#include "../plugins/temp_sensor.h"
#include "../plugins/fpga_device.h"

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