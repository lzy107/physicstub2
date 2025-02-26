// include/device_types.h
#ifndef DEVICE_TYPES_H
#define DEVICE_TYPES_H

#include <stdint.h>
#include <pthread.h>
#include "address_space.h"

// 设备类型ID定义
typedef enum {
    DEVICE_TYPE_FLASH = 0,
    DEVICE_TYPE_TEMP_SENSOR,
    DEVICE_TYPE_I2C_BUS,
    DEVICE_TYPE_OPTICAL_MODULE,
    MAX_DEVICE_TYPES
} device_type_id_t;

// 设备实例结构
typedef struct device_instance {
    int dev_id;                           // 设备ID
    address_space_t* addr_space;          // 私有地址空间
    struct device_instance* next;         // 链表指针
    void* private_data;                   // 设备私有数据
} device_instance_t;

// 设备类型操作接口
typedef struct {
    int (*init)(device_instance_t* instance);
    int (*read)(device_instance_t* instance, uint32_t addr, uint32_t* value);
    int (*write)(device_instance_t* instance, uint32_t addr, uint32_t value);
    void (*reset)(device_instance_t* instance);
    void (*destroy)(device_instance_t* instance);
    pthread_mutex_t* (*get_mutex)(device_instance_t* instance);  // 获取设备互斥锁
} device_ops_t;

// 设备类型结构
typedef struct {
    device_type_id_t type_id;            // 类型ID
    char name[32];                        // 类型名称
    device_ops_t ops;                    // 操作接口
    device_instance_t* instances;         // 实例链表
    pthread_mutex_t mutex;                // 实例链表互斥锁
} device_type_t;

// 设备管理器结构
typedef struct {
    device_type_t types[MAX_DEVICE_TYPES];  // 设备类型数组
    pthread_mutex_t mutex;                   // 类型数组互斥锁
} device_manager_t;

// API函数声明
device_manager_t* device_manager_init(void);
void device_manager_destroy(device_manager_t* dm);
int device_type_register(device_manager_t* dm, device_type_id_t type_id, const char* name, device_ops_t* ops);
device_instance_t* device_create(device_manager_t* dm, device_type_id_t type_id, int dev_id);
void device_destroy(device_manager_t* dm, device_type_id_t type_id, int dev_id);
device_instance_t* device_get(device_manager_t* dm, device_type_id_t type_id, int dev_id);

#endif