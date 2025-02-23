// flash_device.c
#include <stdlib.h>
#include <string.h>
#include "flash_device.h"
#include "device_registry.h"

// 注册FLASH设备
REGISTER_DEVICE(DEVICE_TYPE_FLASH, "FLASH", get_flash_device_ops);

// FLASH设备内存区域配置
static device_mem_region_t flash_regions[FLASH_REGION_COUNT] = {
    // 寄存器区域
    {
        .start_addr = FLASH_STATUS_REG,
        .unit_size = sizeof(uint8_t),
        .unit_count = 2,  // 2个8位寄存器
        .mem_ptr = NULL
    },
    // 数据区域
    {
        .start_addr = FLASH_DATA_START,
        .unit_size = sizeof(uint8_t),
        .unit_count = FLASH_TOTAL_SIZE,  // 64KB数据区
        .mem_ptr = NULL
    }
};

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

// 获取FLASH设备操作接口
device_ops_t* get_flash_device_ops(void) {
    return &flash_ops;
}

// 初始化FLASH设备
static int flash_init(device_instance_t* instance) {
    if (!instance) return -1;
    
    // 分配设备私有数据
    flash_dev_data_t* dev_data = (flash_dev_data_t*)calloc(1, sizeof(flash_dev_data_t));
    if (!dev_data) return -1;
    
    // 初始化内存配置
    dev_data->mem_config.regions = flash_regions;
    dev_data->mem_config.region_count = FLASH_REGION_COUNT;
    
    // 初始化设备内存
    if (device_mem_init(&dev_data->mem_config) != 0) {
        free(dev_data);
        return -1;
    }
    
    // 初始化互斥锁
    pthread_mutex_init(&dev_data->mutex, NULL);
    
    // 初始化所有数据为0xFF（FLASH擦除状态）
    uint8_t* data_region = (uint8_t*)dev_data->mem_config.regions[FLASH_DATA_REGION].mem_ptr;
    memset(data_region, 0xFF, FLASH_TOTAL_SIZE);
    
    instance->private_data = dev_data;
    return 0;
}

// 读取FLASH数据或寄存器
static int flash_read(device_instance_t* instance, uint32_t addr, uint32_t* value) {
    if (!instance || !value) return -1;
    
    flash_dev_data_t* dev_data = (flash_dev_data_t*)instance->private_data;
    if (!dev_data) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    uint8_t reg_value;
    int ret = device_mem_read(&dev_data->mem_config, addr, &reg_value, sizeof(reg_value));
    if (ret == 0) {
        *value = reg_value;
    }
    pthread_mutex_unlock(&dev_data->mutex);
    
    return ret;
}

// 写入FLASH数据或寄存器
static int flash_write(device_instance_t* instance, uint32_t addr, uint32_t value) {
    if (!instance) return -1;
    
    flash_dev_data_t* dev_data = (flash_dev_data_t*)instance->private_data;
    if (!dev_data) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    int ret = -1;
    uint8_t status;
    
    // 读取状态寄存器
    if (device_mem_read(&dev_data->mem_config, FLASH_STATUS_REG, &status, sizeof(status)) != 0) {
        pthread_mutex_unlock(&dev_data->mutex);
        return -1;
    }
    
    // 检查设备是否忙
    if (status & STATUS_BUSY) {
        pthread_mutex_unlock(&dev_data->mutex);
        return -1;
    }
    
    uint8_t reg_value = (uint8_t)value;
    
    // 写入寄存器
    if (addr == FLASH_STATUS_REG) {
        // 状态寄存器写保护检查
        if (!(status & STATUS_SRWD)) {
            ret = device_mem_write(&dev_data->mem_config, addr, &reg_value, sizeof(reg_value));
        }
    }
    else if (addr == FLASH_CTRL_REG) {
        ret = device_mem_write(&dev_data->mem_config, addr, &reg_value, sizeof(reg_value));
    }
    // 写入数据区
    else if (addr >= FLASH_DATA_START && addr < (FLASH_DATA_START + FLASH_TOTAL_SIZE)) {
        // 检查写使能
        if (status & STATUS_WEL) {
            // 模拟FLASH只能将1改为0的特性
            uint8_t old_value;
            if (device_mem_read(&dev_data->mem_config, addr, &old_value, sizeof(old_value)) == 0) {
                reg_value &= old_value;  // 只能将1改为0
                ret = device_mem_write(&dev_data->mem_config, addr, &reg_value, sizeof(reg_value));
                
                // 清除写使能
                status &= ~STATUS_WEL;
                device_mem_write(&dev_data->mem_config, FLASH_STATUS_REG, &status, sizeof(status));
            }
        }
    }
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 复位FLASH设备
static void flash_reset(device_instance_t* instance) {
    if (!instance) return;
    
    flash_dev_data_t* dev_data = (flash_dev_data_t*)instance->private_data;
    if (!dev_data) return;
    
    // 复位时不需要获取互斥锁，因为调用者已经持有锁
    
    // 清除所有寄存器
    uint8_t zero = 0;
    device_mem_write(&dev_data->mem_config, FLASH_STATUS_REG, &zero, sizeof(zero));
    device_mem_write(&dev_data->mem_config, FLASH_CTRL_REG, &zero, sizeof(zero));
    
    // 将所有数据恢复为0xFF（FLASH擦除状态）
    uint8_t* data_region = (uint8_t*)dev_data->mem_config.regions[FLASH_DATA_REGION].mem_ptr;
    memset(data_region, 0xFF, FLASH_TOTAL_SIZE);
}

// 销毁FLASH设备
static void flash_destroy(device_instance_t* instance) {
    if (!instance) return;
    
    flash_dev_data_t* dev_data = (flash_dev_data_t*)instance->private_data;
    if (!dev_data) return;
    
    // 销毁互斥锁
    pthread_mutex_destroy(&dev_data->mutex);
    
    // 释放设备内存
    device_mem_destroy(&dev_data->mem_config);
    
    // 释放设备数据
    free(dev_data);
    instance->private_data = NULL;
}