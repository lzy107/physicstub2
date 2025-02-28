#include "../../include/device_rule_configs.h"
#include "fpga_device.h"

// FPGA 设备规则配置
const device_rule_config_t fpga_rule_configs[] = {
    // 示例规则：当中断寄存器被写入时触发操作
    {
        .addr = FPGA_IRQ_REG,
        .expected_value = 0x00000001,  // 中断标志位
        .expected_mask = 0x00000001,   // 只检查第一位
        .action_type = ACTION_TYPE_CALLBACK,
        .target_device_type = DEVICE_TYPE_FPGA,
        .target_device_id = 0,  // 同一设备
        .target_addr = 0,       // 不适用于回调
        .target_value = 0,      // 不适用于回调
        .callback = fpga_irq_callback
    },
    // 控制寄存器写入时触发操作
    {
        .addr = FPGA_CONTROL_REG,
        .expected_value = 0x00000001,  // 启动位
        .expected_mask = 0x00000001,   // 只检查第一位
        .action_type = ACTION_TYPE_CALLBACK,
        .target_device_type = DEVICE_TYPE_FPGA,
        .target_device_id = 0,  // 同一设备
        .target_addr = 0,       // 不适用于回调
        .target_value = 0,      // 不适用于回调
        .callback = fpga_control_callback
    },
    // 配置寄存器写入时触发操作
    {
        .addr = FPGA_CONFIG_REG,
        .expected_value = 0x00000000,  // 任意值
        .expected_mask = 0x00000000,   // 不检查具体值
        .action_type = ACTION_TYPE_CALLBACK,
        .target_device_type = DEVICE_TYPE_FPGA,
        .target_device_id = 0,  // 同一设备
        .target_addr = 0,       // 不适用于回调
        .target_value = 0,      // 不适用于回调
        .callback = fpga_config_callback
    }
};
const int fpga_rule_config_count = sizeof(fpga_rule_configs) / sizeof(fpga_rule_configs[0]); 