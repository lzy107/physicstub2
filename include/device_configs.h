#ifndef DEVICE_CONFIGS_H
#define DEVICE_CONFIGS_H

#include "device_memory.h"
#include "device_types.h"

// Flash 设备配置
extern const memory_region_t flash_memory_regions[];
extern const int flash_region_count;

// 温度传感器设备配置
extern const memory_region_t temp_sensor_memory_regions[];
extern const int temp_sensor_region_count;

// FPGA 设备配置
extern const memory_region_t fpga_memory_regions[];
extern const int fpga_region_count;

// 获取设备内存配置
const memory_region_t* get_device_memory_regions(uint32_t device_type, int* region_count);

#endif /* DEVICE_CONFIGS_H */ 