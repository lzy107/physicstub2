#include <stdio.h>
#include <string.h>
#include "device_registry.h"
#include "../plugins/flash_device.h"

// 设备注册表
static const device_register_info_t device_registry[] = {
    {
        .type_id = DEVICE_TYPE_FLASH,
        .name = "FLASH",
        .get_ops = get_flash_device_ops
    },
    // 在这里添加更多设备类型
};

// 获取注册表大小
static const int registry_size = sizeof(device_registry) / sizeof(device_registry[0]);

int device_registry_init(device_manager_t* dm) {
    if (!dm) return -1;
    
    printf("Initializing device registry...\n");
    
    // 注册所有设备类型
    for (int i = 0; i < registry_size; i++) {
        const device_register_info_t* info = &device_registry[i];
        printf("Registering device type: %s\n", info->name);
        
        device_ops_t* ops = info->get_ops();
        if (!ops) {
            printf("Failed to get device ops for %s\n", info->name);
            return -1;
        }
        
        if (device_type_register(dm, info->type_id, info->name, ops) != 0) {
            printf("Failed to register device type: %s\n", info->name);
            return -1;
        }
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
    return &device_registry[index];
} 