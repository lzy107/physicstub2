#include "../../include/device_configs.h"
#include "fpga_device.h"

// 定义FPGA内存区域常量
#define FPGA_CONFIG_START 0x100
#define FPGA_MEM_SIZE     0x10000

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
const int fpga_region_count = 3;

