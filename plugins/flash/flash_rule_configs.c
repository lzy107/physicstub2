#include "../../include/device_rule_configs.h"
#include "flash_device.h"

// Flash 设备规则配置
const device_rule_config_t flash_rule_configs[] = {
    // 示例规则：当写入控制寄存器时触发操作
    {
        .addr = FLASH_CONTROL_REG,
        .expected_value = CTRL_ERASE,
        .expected_mask = CTRL_ERASE,
        .action_type = ACTION_TYPE_CALLBACK,
        .target_device_type = DEVICE_TYPE_FLASH,
        .target_device_id = 0,  // 同一设备
        .target_addr = 0,       // 不适用于回调
        .target_value = 0,      // 不适用于回调
        .callback = flash_erase_callback
    },
    // 写入控制寄存器时触发读操作
    {
        .addr = FLASH_CONTROL_REG,
        .expected_value = CTRL_READ,
        .expected_mask = CTRL_READ,
        .action_type = ACTION_TYPE_CALLBACK,
        .target_device_type = DEVICE_TYPE_FLASH,
        .target_device_id = 0,  // 同一设备
        .target_addr = 0,       // 不适用于回调
        .target_value = 0,      // 不适用于回调
        .callback = flash_read_callback
    },
    // 写入控制寄存器时触发写操作
    {
        .addr = FLASH_CONTROL_REG,
        .expected_value = CTRL_WRITE,
        .expected_mask = CTRL_WRITE,
        .action_type = ACTION_TYPE_CALLBACK,
        .target_device_type = DEVICE_TYPE_FLASH,
        .target_device_id = 0,  // 同一设备
        .target_addr = 0,       // 不适用于回调
        .target_value = 0,      // 不适用于回调
        .callback = flash_write_callback
    }
};
const int flash_rule_count = sizeof(flash_rule_configs) / sizeof(flash_rule_configs[0]); 