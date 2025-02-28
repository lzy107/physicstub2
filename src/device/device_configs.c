#include <stdio.h>
#include "device_configs.h"
#include "../plugins/flash/flash_device.h"
#include "../plugins/temp_sensor/temp_sensor.h"
#include "../plugins/fpga/fpga_device.h"

// 外部声明各设备的内存区域配置
extern const memory_region_t flash_memory_regions[];
extern const int flash_region_count;

extern const memory_region_t temp_sensor_memory_regions[];
extern const int temp_sensor_region_count;

extern const memory_region_t fpga_memory_regions[];
extern const int fpga_region_count;

// 获取设备内存配置
const memory_region_t* get_device_memory_regions(uint32_t device_type, int* region_count) {
    printf("获取设备类型 %d 的内存区域配置...\n", device_type);
    if (!region_count) {
        printf("region_count 为空，返回 NULL\n");
        return NULL;
    }
    
    switch (device_type) {
        case DEVICE_TYPE_FLASH:
            printf("获取Flash设备内存区域配置，区域数量: %d\n", flash_region_count);
            *region_count = flash_region_count;
            printf("返回Flash内存区域配置\n");
            return flash_memory_regions;
            
        case DEVICE_TYPE_TEMP_SENSOR:
            printf("获取温度传感器设备内存区域配置，区域数量: %d\n", temp_sensor_region_count);
            *region_count = temp_sensor_region_count;
            printf("返回温度传感器内存区域配置\n");
            return temp_sensor_memory_regions;
            
        case DEVICE_TYPE_FPGA:
            printf("获取FPGA设备内存区域配置，区域数量: %d\n", fpga_region_count);
            *region_count = fpga_region_count;
            printf("返回FPGA内存区域配置\n");
            return fpga_memory_regions;
            
        default:
            printf("未知设备类型 %d，返回 NULL\n", device_type);
            *region_count = 0;
            return NULL;
    }
} 