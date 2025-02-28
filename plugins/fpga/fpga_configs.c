#include "../../include/device_configs.h"
#include "fpga_device.h"

// FPGA 设备内存区域配置
const memory_region_t fpga_memory_regions[] = {
    // 寄存器区域
    {
        .base_addr = 0x00,
        .unit_size = 4,  // 4字节单位
        .length = 16,    // 16个寄存器
        .data = NULL,    // 初始化时分配
        .device_type = DEVICE_TYPE_FPGA,
        .device_id = 0    // 默认ID为0，实际使用时会被覆盖
    },
    // 配置区域
    {
        .base_addr = FPGA_CONFIG_START,
        .unit_size = 4,  // 4字节单位
        .length = (FPGA_DATA_START - FPGA_CONFIG_START) / 4,
        .data = NULL,    // 初始化时分配
        .device_type = DEVICE_TYPE_FPGA,
        .device_id = 0    // 默认ID为0，实际使用时会被覆盖
    },
    // 数据区域
    {
        .base_addr = FPGA_DATA_START,
        .unit_size = 4,  // 4字节单位
        .length = (FPGA_MEM_SIZE - FPGA_DATA_START) / 4,
        .data = NULL,    // 初始化时分配
        .device_type = DEVICE_TYPE_FPGA,
        .device_id = 0    // 默认ID为0，实际使用时会被覆盖
    }
};
const int fpga_region_count = sizeof(fpga_memory_regions) / sizeof(fpga_memory_regions[0]);

// FPGA 设备内存配置函数
int fpga_configure_memory(device_instance_t* instance, memory_region_config_t* configs, int config_count) {
    if (!instance || !configs || config_count <= 0) {
        return -1;
    }
    
    fpga_dev_data_t* dev_data = (fpga_dev_data_t*)instance->private_data;
    if (!dev_data) {
        return -1;
    }
    
    // 销毁现有的内存
    if (dev_data->memory) {
        device_memory_destroy(dev_data->memory);
        dev_data->memory = NULL;
    }
    
    // 创建新的内存
    dev_data->memory = device_memory_create_from_config(configs, config_count, NULL, DEVICE_TYPE_FPGA, instance->dev_id);
    if (!dev_data->memory) {
        return -1;
    }
    
    // 初始化寄存器区域
    uint32_t status = STATUS_READY;
    device_memory_write(dev_data->memory, FPGA_STATUS_REG, status);
    device_memory_write(dev_data->memory, FPGA_CONFIG_REG, 0);
    device_memory_write(dev_data->memory, FPGA_CONTROL_REG, 0);
    device_memory_write(dev_data->memory, FPGA_IRQ_REG, 0);
    
    // 初始化数据区域为0
    for (int i = 0; i < dev_data->memory->region_count; i++) {
        memory_region_t* region = &dev_data->memory->regions[i];
        // 检查是否是数据区域（基地址大于等于FPGA_DATA_START）
        if (region->base_addr >= FPGA_DATA_START) {
            // 只初始化数据区域
            memset(region->data, 0, region->unit_size * region->length);
        }
    }
    
    return 0;
} 