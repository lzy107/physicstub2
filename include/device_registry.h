#ifndef DEVICE_REGISTRY_H
#define DEVICE_REGISTRY_H

#include "device_types.h"

// 设备注册信息结构
typedef struct {
    device_type_id_t type_id;     // 设备类型ID
    const char* name;             // 设备名称
    device_ops_t* (*get_ops)(void); // 获取设备操作接口的函数
} device_register_info_t;

// 初始化设备注册表
int device_registry_init(device_manager_t* dm);

// 注册单个设备类型
int device_registry_register(device_manager_t* dm, const device_register_info_t* info);

// 获取已注册的设备类型数量
int device_registry_get_count(void);

// 获取设备注册信息
const device_register_info_t* device_registry_get_info(int index);

#endif // DEVICE_REGISTRY_H 