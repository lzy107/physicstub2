#include "../../include/device_configs.h"
#include "flash_device.h"

// Flash 设备内存区域配置
const memory_region_t flash_memory_regions[] = {
    // 寄存器区域
    {
        .base_addr = 0x00,
        .unit_size = 4,  // 4字节单位
        .length = 8,     // 8个寄存器
        .data = NULL,    // 初始化时分配
        .device_type = DEVICE_TYPE_FLASH,
        .device_id = 0    // 默认ID为0，实际使用时会被覆盖
    },
    // 数据区域
    {
        .base_addr = FLASH_DATA_START,
        .unit_size = 4,  // 4字节单位
        .length = (FLASH_MEM_SIZE - FLASH_DATA_START) / 4,  // 剩余空间
        .data = NULL,    // 初始化时分配
        .device_type = DEVICE_TYPE_FLASH,
        .device_id = 0    // 默认ID为0，实际使用时会被覆盖
    }
};
const int flash_region_count = sizeof(flash_memory_regions) / sizeof(flash_memory_regions[0]);

// Flash 设备内存配置函数
int flash_configure_memory(device_instance_t* instance, memory_region_config_t* configs, int config_count) {
    if (!instance || !configs || config_count <= 0) {
        return -1;
    }
    
    flash_device_t* dev_data = (flash_device_t*)instance->priv_data;
    if (!dev_data) {
        return -1;
    }
    
    // 销毁现有的内存
    if (dev_data->memory) {
        device_memory_destroy(dev_data->memory);
        dev_data->memory = NULL;
    }
    
    // 创建新的内存
    dev_data->memory = device_memory_create_from_config(configs, config_count, NULL, DEVICE_TYPE_FLASH, instance->dev_id);
    if (!dev_data->memory) {
        return -1;
    }
    
    // 初始化寄存器
    device_memory_write(dev_data->memory, FLASH_REG_STATUS, FLASH_STATUS_READY);
    device_memory_write(dev_data->memory, FLASH_REG_CONFIG, 0);
    device_memory_write(dev_data->memory, FLASH_REG_ADDRESS, 0);
    device_memory_write(dev_data->memory, FLASH_REG_DATA, 0);
    device_memory_write(dev_data->memory, FLASH_REG_SIZE, FLASH_MEM_SIZE);
    
    return 0;
} 