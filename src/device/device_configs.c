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
    if (!region_count) {
        return NULL;
    }
    
    switch (device_type) {
        case DEVICE_TYPE_FLASH:
            *region_count = flash_region_count;
            // 设置设备类型
            for (int i = 0; i < flash_region_count; i++) {
                ((memory_region_t*)flash_memory_regions)[i].device_type = device_type;
            }
            return flash_memory_regions;
            
        case DEVICE_TYPE_TEMP_SENSOR:
            *region_count = temp_sensor_region_count;
            // 设置设备类型
            for (int i = 0; i < temp_sensor_region_count; i++) {
                ((memory_region_t*)temp_sensor_memory_regions)[i].device_type = device_type;
            }
            return temp_sensor_memory_regions;
            
        case DEVICE_TYPE_FPGA:
            *region_count = fpga_region_count;
            // 设置设备类型
            for (int i = 0; i < fpga_region_count; i++) {
                ((memory_region_t*)fpga_memory_regions)[i].device_type = device_type;
            }
            return fpga_memory_regions;
            
        default:
            *region_count = 0;
            return NULL;
    }
} 