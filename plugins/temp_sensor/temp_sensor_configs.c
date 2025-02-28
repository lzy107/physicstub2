#include "../../include/device_configs.h"
#include "temp_sensor.h"

// 温度传感器设备内存区域配置
const memory_region_t temp_sensor_memory_regions[] = {
    // 寄存器区域
    {
        .base_addr = 0x00,
        .unit_size = 4,  // 4字节单位
        .length = 8,     // 8个寄存器
        .data = NULL,    // 初始化时分配
        .device_type = DEVICE_TYPE_TEMP_SENSOR,
        .device_id = 0    // 默认ID为0，实际使用时会被覆盖
    },
    // 数据区域
    {
        .base_addr = 0x100,
        .unit_size = 4,  // 4字节单位
        .length = 64,    // 64个单位
        .data = NULL,    // 初始化时分配
        .device_type = DEVICE_TYPE_TEMP_SENSOR,
        .device_id = 0    // 默认ID为0，实际使用时会被覆盖
    }
};
const int temp_sensor_region_count = sizeof(temp_sensor_memory_regions) / sizeof(temp_sensor_memory_regions[0]);

