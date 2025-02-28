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

// 温度传感器设备内存配置函数
int temp_sensor_configure_memory(device_instance_t* instance, memory_region_config_t* configs, int config_count) {
    if (!instance || !configs || config_count <= 0) {
        return -1;
    }
    
    temp_sensor_data_t* dev_data = (temp_sensor_data_t*)instance->private_data;
    if (!dev_data) {
        return -1;
    }
    
    // 销毁现有的内存
    if (dev_data->memory) {
        device_memory_destroy(dev_data->memory);
        dev_data->memory = NULL;
    }
    
    // 创建新的内存
    dev_data->memory = device_memory_create_from_config(configs, config_count, NULL, DEVICE_TYPE_TEMP_SENSOR, instance->dev_id);
    if (!dev_data->memory) {
        return -1;
    }
    
    // 设置初始值
    uint32_t temp = 2500;    // 25°C
    uint32_t tlow = 1800;    // 18°C
    uint32_t thigh = 3000;   // 30°C
    uint32_t config = 0;     // 正常模式
    
    device_memory_write(dev_data->memory, TEMP_REG, temp);
    device_memory_write(dev_data->memory, TLOW_REG, tlow);
    device_memory_write(dev_data->memory, THIGH_REG, thigh);
    device_memory_write(dev_data->memory, CONFIG_REG, config);
    
    return 0;
} 