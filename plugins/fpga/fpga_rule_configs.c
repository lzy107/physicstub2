#include "../../include/device_rule_configs.h"
#include "fpga_device.h"

// FPGA 设备规则配置
const device_rule_config_t fpga_rule_configs[] = {
    // 示例规则：当中断寄存器被写入时触发操作
    {
        .addr = 0x1,
        .expected_value = 0x00000001,  
        .expected_mask = 0x00000001,   
        .action_type = ACTION_TYPE_WRITE,
        .target_device_type = DEVICE_TYPE_FPGA,
        .target_device_id = 0,  // 同一设备
        .target_addr = 0x3,       // 不适用于回调
        .target_value = 0x4,      // 不适用于回调
        .callback = NULL
    }
};
const int fpga_rule_config_count = sizeof(fpga_rule_configs) / sizeof(fpga_rule_configs[0]); 