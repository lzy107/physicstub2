#ifndef DEVICE_REGISTRY_H
#define DEVICE_REGISTRY_H

#include "device_types.h"

// 设备注册信息结构
typedef struct device_register_info {
    device_type_id_t type_id;     // 设备类型ID
    const char* name;             // 设备名称
    device_ops_t* (*get_ops)(void); // 获取设备操作接口的函数
    struct device_register_info* next; // 链表下一个节点
} device_register_info_t;

// 初始化设备注册表
int device_registry_init(device_manager_t* dm);

// 注册单个设备类型
int device_registry_register(device_manager_t* dm, const device_register_info_t* info);

// 获取已注册的设备类型数量
int device_registry_get_count(void);

// 获取设备注册信息
const device_register_info_t* device_registry_get_info(int index);

// 添加设备到注册表（供自注册宏使用）
void device_registry_add_device(device_register_info_t* info);

// 根据地址获取设备实例
device_instance_t* device_manager_get_device_by_addr(device_manager_t* dm, uint32_t addr);

// 根据类型和ID获取设备实例
device_instance_t* device_manager_get_device_by_type_id(device_manager_t* dm, int type_id, int device_id);

// 显示所有已注册设备
void device_manager_dump_devices(device_manager_t* dm);

// 设备自注册宏
#define REGISTER_DEVICE(type_id, name, get_ops_func) \
    static device_register_info_t __device_info_##type_id = { \
        type_id, \
        name, \
        get_ops_func, \
        NULL \
    }; \
    __attribute__((constructor)) \
    static void __register_device_##type_id(void) { \
        device_registry_add_device(&__device_info_##type_id); \
    }

// 设备管理函数声明
void device_manager_list_devices(device_manager_t* dm);
void device_manager_cleanup(device_manager_t* dm);

#endif // DEVICE_REGISTRY_H 