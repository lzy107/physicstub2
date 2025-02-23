// flash_device.c
#include <stdlib.h>
#include <string.h>
#include "flash_device.h"

// 私有函数声明
static int flash_init(device_instance_t* instance);
static int flash_read(device_instance_t* instance, uint32_t addr, uint32_t* value);
static int flash_write(device_instance_t* instance, uint32_t addr, uint32_t value);
static void flash_reset(device_instance_t* instance);
static void flash_destroy(device_instance_t* instance);

// FLASH设备操作接口实现
static device_ops_t flash_ops = {
    .init = flash_init,
    .read = flash_read,
    .write = flash_write,
    .reset = flash_reset,
    .destroy = flash_destroy
};

// 获取插件名称
const char* get_plugin_name(void) {
    return "flash_device";
}

// 获取设备操作接口
device_ops_t* get_device_ops(void) {
    return &flash_ops;
}

// 初始化FLASH设备
static int flash_init(device_instance_t* instance) {
    if (!instance) return -1;
    
    // 分配设备私有数据
    flash_dev_data_t* dev_data = (flash_dev_data_t*)calloc(1, sizeof(flash_dev_data_t));
    if (!dev_data) return -1;
    
    // 分配FLASH存储空间
    dev_data->memory = (uint8_t*)calloc(FLASH_TOTAL_SIZE, sizeof(uint8_t));
    if (!dev_data->memory) {
        free(dev_data);
        return -1;
    }
    
    // 初始化互斥锁和寄存器
    pthread_mutex_init(&dev_data->mutex, NULL);
    dev_data->status_reg = 0;
    dev_data->ctrl_reg = 0;
    
    instance->private_data = dev_data;
    return 0;
}

// 读取FLASH数据或寄存器
static int flash_read(device_instance_t* instance, uint32_t addr, uint32_t* value) {
    if (!instance || !value) return -1;
    
    flash_dev_data_t* dev_data = (flash_dev_data_t*)instance->private_data;
    if (!dev_data) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    // 读取寄存器
    if (addr == FLASH_STATUS_REG) {
        *value = dev_data->status_reg;
    }
    else if (addr == FLASH_CTRL_REG) {
        *value = dev_data->ctrl_reg;
    }
    // 读取数据区
    else if (addr >= FLASH_DATA_START && addr < FLASH_TOTAL_SIZE) {
        uint32_t offset = addr - FLASH_DATA_START;
        *value = dev_data->memory[offset];
    }
    else {
        pthread_mutex_unlock(&dev_data->mutex);
        return -1;
    }
    
    pthread_mutex_unlock(&dev_data->mutex);
    return 0;
}

// 写入FLASH数据或寄存器
static int flash_write(device_instance_t* instance, uint32_t addr, uint32_t value) {
    if (!instance) return -1;
    
    flash_dev_data_t* dev_data = (flash_dev_data_t*)instance->private_data;
    if (!dev_data) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    // 检查设备是否忙
    if (dev_data->status_reg & STATUS_BUSY) {
        pthread_mutex_unlock(&dev_data->mutex);
        return -1;
    }
    
    // 写入寄存器
    if (addr == FLASH_STATUS_REG) {
        // 状态寄存器写保护检查
        if (!(dev_data->status_reg & STATUS_SRWD)) {
            dev_data->status_reg = value & 0xFF;
        }
    }
    else if (addr == FLASH_CTRL_REG) {
        dev_data->ctrl_reg = value & 0xFF;
    }
    // 写入数据区
    else if (addr >= FLASH_DATA_START && addr < FLASH_TOTAL_SIZE) {
        // 检查写使能
        if (!(dev_data->status_reg & STATUS_WEL)) {
            pthread_mutex_unlock(&dev_data->mutex);
            return -1;
        }
        
        uint32_t offset = addr - FLASH_DATA_START;
        // 模拟FLASH只能将1改为0的特性
        dev_data->memory[offset] &= (uint8_t)value;
        
        // 清除写使能
        dev_data->status_reg &= ~STATUS_WEL;
    }
    else {
        pthread_mutex_unlock(&dev_data->mutex);
        return -1;
    }
    
    pthread_mutex_unlock(&dev_data->mutex);
    return 0;
}

// 复位FLASH设备
static void flash_reset(device_instance_t* instance) {
    if (!instance) return;
    
    flash_dev_data_t* dev_data = (flash_dev_data_t*)instance->private_data;
    if (!dev_data) return;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    // 清除所有寄存器
    dev_data->status_reg = 0;
    dev_data->ctrl_reg = 0;
    
    // 清除所有数据(设为0xFF)
    memset(dev_data->memory, 0xFF, FLASH_TOTAL_SIZE);
    
    pthread_mutex_unlock(&dev_data->mutex);
}

// 销毁FLASH设备
static void flash_destroy(device_instance_t* instance) {
    if (!instance) return;
    
    flash_dev_data_t* dev_data = (flash_dev_data_t*)instance->private_data;
    if (!dev_data) return;
    
    // 先获取锁，确保没有其他线程在使用设备
    pthread_mutex_lock(&dev_data->mutex);
    
    // 清理内存
    if (dev_data->memory) {
        free(dev_data->memory);
        dev_data->memory = NULL;
    }
    
    // 释放锁并销毁它
    pthread_mutex_unlock(&dev_data->mutex);
    pthread_mutex_destroy(&dev_data->mutex);
    
    // 最后释放设备数据
    free(dev_data);
    instance->private_data = NULL;
}