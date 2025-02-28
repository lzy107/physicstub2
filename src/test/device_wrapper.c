/**
 * @file device_wrapper.c
 * @brief 设备读写包装函数实现
 */

#include <stdio.h>
#include "../../include/device_test.h"
#include "../../include/device_types.h"

/**
 * @brief 设备读取函数
 * 
 * @param instance 设备实例
 * @param addr 地址
 * @param value 读取的值
 * @return int 成功返回0，失败返回负数
 */
int device_read(device_instance_t* instance, uint32_t addr, uint32_t* value) {
    if (!instance) return -1;
    
    // 调试输出
    printf("DEBUG: device_read - 设备ID: %d, 地址: 0x%08X\n", 
           instance->dev_id, addr);
    
    // 直接使用设备实例的地址空间读取
    if (instance->addr_space) {
        int ret = address_space_read(instance->addr_space, addr, value);
        printf("DEBUG: device_read - 通过地址空间读取, 返回值: %d, 读取值: 0x%08X\n", ret, *value);
        return ret;
    }
    
    printf("DEBUG: device_read - 设备实例没有地址空间\n");
    return -1;
}

/**
 * @brief 设备写入函数
 * 
 * @param instance 设备实例
 * @param addr 地址
 * @param value 写入的值
 * @return int 成功返回0，失败返回负数
 */
int device_write(device_instance_t* instance, uint32_t addr, uint32_t value) {
    if (!instance) return -1;
    
    // 调试输出
    printf("DEBUG: device_write - 设备ID: %d, 地址: 0x%08X, 值: 0x%08X\n", 
           instance->dev_id, addr, value);
    
    // 直接使用设备实例的地址空间写入
    if (instance->addr_space) {
        int ret = address_space_write(instance->addr_space, addr, value);
        printf("DEBUG: device_write - 通过地址空间写入, 返回值: %d\n", ret);
        return ret;
    }
    
    printf("DEBUG: device_write - 设备实例没有地址空间\n");
    return -1;
}
