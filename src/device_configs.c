#include "device_configs.h"
#include "../plugins/flash/flash_device.h"
#include "../plugins/temp_sensor/temp_sensor.h"
#include "../plugins/fpga/fpga_device.h"

// Flash 设备内存区域配置
const memory_region_t flash_memory_regions[] = {
    // 寄存器区域
    {
        .base_addr = 0x00,
        .unit_size = 4,  // 4字节单位
        .length = 8,     // 8个寄存器
        .data = NULL     // 初始化时分配
    },
    // 数据区域
    {
        .base_addr = FLASH_DATA_START,
        .unit_size = 4,  // 4字节单位
        .length = (FLASH_MEM_SIZE - FLASH_DATA_START) / 4,  // 剩余空间
        .data = NULL     // 初始化时分配
    }
};
const int flash_region_count = sizeof(flash_memory_regions) / sizeof(flash_memory_regions[0]);

// 温度传感器设备内存区域配置
const memory_region_t temp_sensor_memory_regions[] = {
    // 寄存器区域
    {
        .base_addr = 0x00,
        .unit_size = 4,  // 4字节单位
        .length = 8,     // 8个寄存器
        .data = NULL     // 初始化时分配
    },
    // 数据区域
    {
        .base_addr = 0x100,
        .unit_size = 4,  // 4字节单位
        .length = 64,    // 64个单位
        .data = NULL     // 初始化时分配
    }
};
const int temp_sensor_region_count = sizeof(temp_sensor_memory_regions) / sizeof(temp_sensor_memory_regions[0]);

// FPGA 设备内存区域配置
const memory_region_t fpga_memory_regions[] = {
    // 寄存器区域
    {
        .base_addr = 0x00,
        .unit_size = 4,  // 4字节单位
        .length = 16,    // 16个寄存器
        .data = NULL     // 初始化时分配
    },
    // 配置区域
    {
        .base_addr = FPGA_CONFIG_START,
        .unit_size = 4,  // 4字节单位
        .length = (FPGA_DATA_START - FPGA_CONFIG_START) / 4,
        .data = NULL     // 初始化时分配
    },
    // 数据区域
    {
        .base_addr = FPGA_DATA_START,
        .unit_size = 4,  // 4字节单位
        .length = (FPGA_MEM_SIZE - FPGA_DATA_START) / 4,
        .data = NULL     // 初始化时分配
    }
};
const int fpga_region_count = sizeof(fpga_memory_regions) / sizeof(fpga_memory_regions[0]);

// 获取设备内存配置
const memory_region_t* get_device_memory_regions(device_type_id_t device_type, int* count) {
    if (!count) return NULL;
    
    switch (device_type) {
        case DEVICE_TYPE_FLASH:
            *count = flash_region_count;
            return flash_memory_regions;
            
        case DEVICE_TYPE_TEMP_SENSOR:
            *count = temp_sensor_region_count;
            return temp_sensor_memory_regions;
            
        case DEVICE_TYPE_FPGA:
            *count = fpga_region_count;
            return fpga_memory_regions;
            
        default:
            *count = 0;
            return NULL;
    }
} 