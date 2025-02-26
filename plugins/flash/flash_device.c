// flash_device.c
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "flash_device.h"
#include "../include/device_registry.h"
#include "../include/device_memory.h"

// 注册FLASH设备
REGISTER_DEVICE(DEVICE_TYPE_FLASH, "FLASH", get_flash_device_ops);

// 私有函数声明
static int flash_init(device_instance_t* instance);
static int flash_read(device_instance_t* instance, uint32_t addr, uint32_t* value);
static int flash_write(device_instance_t* instance, uint32_t addr, uint32_t value);
static void flash_reset(device_instance_t* instance);
static void flash_destroy(device_instance_t* instance);
static pthread_mutex_t* flash_get_mutex(device_instance_t* instance);

// FLASH设备操作接口实现
static device_ops_t flash_ops = {
    .init = flash_init,
    .read = flash_read,
    .write = flash_write,
    .reset = flash_reset,
    .destroy = flash_destroy,
    .get_mutex = flash_get_mutex
};

// 获取FLASH设备操作接口
device_ops_t* get_flash_device_ops(void) {
    return &flash_ops;
}

// 初始化FLASH设备
static int flash_init(device_instance_t* instance) {
    if (!instance) return -1;
    
    // 分配设备私有数据
    flash_device_t* dev_data = (flash_device_t*)calloc(1, sizeof(flash_device_t));
    if (!dev_data) return -1;
    
    // 初始化互斥锁
    pthread_mutex_init(&dev_data->mutex, NULL);
    
    // 创建设备内存
    dev_data->memory = device_memory_create(FLASH_MEM_SIZE, NULL, DEVICE_TYPE_FLASH, instance->dev_id);
    if (!dev_data->memory) {
        pthread_mutex_destroy(&dev_data->mutex);
        free(dev_data);
        return -1;
    }
    
    // 初始化所有数据为0xFF（FLASH擦除状态）
    memset(dev_data->memory->data, 0xFF, FLASH_MEM_SIZE);
    
    // 初始化设备状态
    dev_data->status = FLASH_STATUS_READY;
    dev_data->control = 0;
    dev_data->config = 0;
    dev_data->address = 0;
    dev_data->size = FLASH_MEM_SIZE;
    
    instance->private_data = dev_data;
    return 0;
}

// 读取FLASH数据或寄存器
static int flash_read(device_instance_t* instance, uint32_t addr, uint32_t* value) {
    if (!instance || !value) return -1;
    
    flash_device_t* dev_data = (flash_device_t*)instance->private_data;
    if (!dev_data) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    int ret = -1;
    
    // 读取寄存器
    if (addr == FLASH_REG_STATUS) {
        *value = dev_data->status;
        ret = 0;
    }
    else if (addr == FLASH_REG_CONTROL) {
        *value = dev_data->control;
        ret = 0;
    }
    else if (addr == FLASH_REG_CONFIG) {
        *value = dev_data->config;
        ret = 0;
    }
    else if (addr == FLASH_REG_ADDRESS) {
        *value = dev_data->address;
        ret = 0;
    }
    else if (addr == FLASH_REG_SIZE) {
        *value = dev_data->size;
        ret = 0;
    }
    else if (addr == FLASH_REG_DATA) {
        // 读取当前地址的数据
        uint32_t data_addr = dev_data->address;
        if (data_addr < FLASH_MEM_SIZE) {
            ret = device_memory_read(dev_data->memory, data_addr, value);
        }
    }
    else {
        // 直接读取内存
        ret = device_memory_read(dev_data->memory, addr, value);
    }
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 写入FLASH数据或寄存器
static int flash_write(device_instance_t* instance, uint32_t addr, uint32_t value) {
    if (!instance) return -1;
    
    flash_device_t* dev_data = (flash_device_t*)instance->private_data;
    if (!dev_data) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    int ret = -1;
    
    // 写入寄存器
    if (addr == FLASH_REG_STATUS) {
        // 状态寄存器写保护检查
        if (!(dev_data->status & FLASH_STATUS_SRWD)) {
            dev_data->status = value & 0xFF;
            ret = 0;
        }
    }
    else if (addr == FLASH_REG_CONTROL) {
        dev_data->control = value & 0xFF;
        ret = 0;
        
        // 处理控制命令
        if (value == FLASH_CTRL_ERASE) {
            // 擦除操作 - 将数据区域填充为0xFF
            memset(dev_data->memory->data + FLASH_DATA_START, 0xFF, FLASH_MEM_SIZE - FLASH_DATA_START);
            dev_data->status |= FLASH_STATUS_WEL; // 设置写使能
        }
    }
    else if (addr == FLASH_REG_CONFIG) {
        dev_data->config = value & 0xFF;
        ret = 0;
    }
    else if (addr == FLASH_REG_ADDRESS) {
        dev_data->address = value;
        ret = 0;
    }
    else if (addr == FLASH_REG_DATA) {
        // 写入当前地址的数据
        uint32_t data_addr = dev_data->address;
        if (data_addr < FLASH_MEM_SIZE) {
            // 检查写使能
            if (dev_data->status & FLASH_STATUS_WEL) {
                // 模拟FLASH只能将1改为0的特性
                uint32_t old_value;
                if (device_memory_read(dev_data->memory, data_addr, &old_value) == 0) {
                    value &= old_value;  // 只能将1改为0
                    ret = device_memory_write(dev_data->memory, data_addr, value);
                    
                    // 自动增加地址
                    dev_data->address += 4;
                    
                    // 清除写使能
                    dev_data->status &= ~FLASH_STATUS_WEL;
                }
            }
        }
    }
    else {
        // 直接写入内存
        // 检查写使能
        if (dev_data->status & FLASH_STATUS_WEL) {
            ret = device_memory_write(dev_data->memory, addr, value);
            
            // 清除写使能
            dev_data->status &= ~FLASH_STATUS_WEL;
        }
    }
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 复位FLASH设备
static void flash_reset(device_instance_t* instance) {
    if (!instance) return;
    
    flash_device_t* dev_data = (flash_device_t*)instance->private_data;
    if (!dev_data) return;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    // 清除所有寄存器
    dev_data->status = FLASH_STATUS_READY;
    dev_data->control = 0;
    dev_data->config = 0;
    dev_data->address = 0;
    
    // 将所有数据恢复为0xFF（FLASH擦除状态）
    memset(dev_data->memory->data, 0xFF, FLASH_MEM_SIZE);
    
    pthread_mutex_unlock(&dev_data->mutex);
}

// 销毁FLASH设备
static void flash_destroy(device_instance_t* instance) {
    if (!instance) return;
    
    flash_device_t* dev_data = (flash_device_t*)instance->private_data;
    if (!dev_data) return;
    
    // 销毁互斥锁
    pthread_mutex_destroy(&dev_data->mutex);
    
    // 释放设备内存
    device_memory_destroy(dev_data->memory);
    
    // 释放设备数据
    free(dev_data);
    instance->private_data = NULL;
}

// 获取Flash设备互斥锁
static pthread_mutex_t* flash_get_mutex(device_instance_t* instance) {
    if (!instance || !instance->private_data) return NULL;
    
    flash_device_t* dev_data = (flash_device_t*)instance->private_data;
    return &dev_data->mutex;
}

// 注册FLASH设备类型
void register_flash_device_type(device_manager_t* dm) {
    if (!dm) return;
    
    device_ops_t* ops = get_flash_device_ops();
    device_type_register(dm, DEVICE_TYPE_FLASH, "FLASH", ops);
}